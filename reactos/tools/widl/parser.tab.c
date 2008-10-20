/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse parser_parse
#define yylex   parser_lex
#define yyerror parser_error
#define yylval  parser_lval
#define yychar  parser_char
#define yydebug parser_debug
#define yynerrs parser_nerrs


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     aIDENTIFIER = 258,
     aKNOWNTYPE = 259,
     aNUM = 260,
     aHEXNUM = 261,
     aDOUBLE = 262,
     aSTRING = 263,
     aUUID = 264,
     aEOF = 265,
     SHL = 266,
     SHR = 267,
     tAGGREGATABLE = 268,
     tALLOCATE = 269,
     tAPPOBJECT = 270,
     tASYNC = 271,
     tASYNCUUID = 272,
     tAUTOHANDLE = 273,
     tBINDABLE = 274,
     tBOOLEAN = 275,
     tBROADCAST = 276,
     tBYTE = 277,
     tBYTECOUNT = 278,
     tCALLAS = 279,
     tCALLBACK = 280,
     tCASE = 281,
     tCDECL = 282,
     tCHAR = 283,
     tCOCLASS = 284,
     tCODE = 285,
     tCOMMSTATUS = 286,
     tCONST = 287,
     tCONTEXTHANDLE = 288,
     tCONTEXTHANDLENOSERIALIZE = 289,
     tCONTEXTHANDLESERIALIZE = 290,
     tCONTROL = 291,
     tCPPQUOTE = 292,
     tDEFAULT = 293,
     tDEFAULTCOLLELEM = 294,
     tDEFAULTVALUE = 295,
     tDEFAULTVTABLE = 296,
     tDISPLAYBIND = 297,
     tDISPINTERFACE = 298,
     tDLLNAME = 299,
     tDOUBLE = 300,
     tDUAL = 301,
     tENDPOINT = 302,
     tENTRY = 303,
     tENUM = 304,
     tERRORSTATUST = 305,
     tEXPLICITHANDLE = 306,
     tEXTERN = 307,
     tFALSE = 308,
     tFASTCALL = 309,
     tFLOAT = 310,
     tHANDLE = 311,
     tHANDLET = 312,
     tHELPCONTEXT = 313,
     tHELPFILE = 314,
     tHELPSTRING = 315,
     tHELPSTRINGCONTEXT = 316,
     tHELPSTRINGDLL = 317,
     tHIDDEN = 318,
     tHYPER = 319,
     tID = 320,
     tIDEMPOTENT = 321,
     tIIDIS = 322,
     tIMMEDIATEBIND = 323,
     tIMPLICITHANDLE = 324,
     tIMPORT = 325,
     tIMPORTLIB = 326,
     tIN = 327,
     tINLINE = 328,
     tINPUTSYNC = 329,
     tINT = 330,
     tINT64 = 331,
     tINTERFACE = 332,
     tLCID = 333,
     tLENGTHIS = 334,
     tLIBRARY = 335,
     tLOCAL = 336,
     tLONG = 337,
     tMETHODS = 338,
     tMODULE = 339,
     tNONBROWSABLE = 340,
     tNONCREATABLE = 341,
     tNONEXTENSIBLE = 342,
     tOBJECT = 343,
     tODL = 344,
     tOLEAUTOMATION = 345,
     tOPTIONAL = 346,
     tOUT = 347,
     tPASCAL = 348,
     tPOINTERDEFAULT = 349,
     tPROPERTIES = 350,
     tPROPGET = 351,
     tPROPPUT = 352,
     tPROPPUTREF = 353,
     tPTR = 354,
     tPUBLIC = 355,
     tRANGE = 356,
     tREADONLY = 357,
     tREF = 358,
     tREQUESTEDIT = 359,
     tRESTRICTED = 360,
     tRETVAL = 361,
     tSAFEARRAY = 362,
     tSHORT = 363,
     tSIGNED = 364,
     tSINGLE = 365,
     tSIZEIS = 366,
     tSIZEOF = 367,
     tSMALL = 368,
     tSOURCE = 369,
     tSTDCALL = 370,
     tSTRICTCONTEXTHANDLE = 371,
     tSTRING = 372,
     tSTRUCT = 373,
     tSWITCH = 374,
     tSWITCHIS = 375,
     tSWITCHTYPE = 376,
     tTRANSMITAS = 377,
     tTRUE = 378,
     tTYPEDEF = 379,
     tUNION = 380,
     tUNIQUE = 381,
     tUNSIGNED = 382,
     tUUID = 383,
     tV1ENUM = 384,
     tVARARG = 385,
     tVERSION = 386,
     tVOID = 387,
     tWCHAR = 388,
     tWIREMARSHAL = 389,
     CAST = 390,
     PPTR = 391,
     NEG = 392,
     ADDRESSOF = 393
   };
#endif
/* Tokens.  */
#define aIDENTIFIER 258
#define aKNOWNTYPE 259
#define aNUM 260
#define aHEXNUM 261
#define aDOUBLE 262
#define aSTRING 263
#define aUUID 264
#define aEOF 265
#define SHL 266
#define SHR 267
#define tAGGREGATABLE 268
#define tALLOCATE 269
#define tAPPOBJECT 270
#define tASYNC 271
#define tASYNCUUID 272
#define tAUTOHANDLE 273
#define tBINDABLE 274
#define tBOOLEAN 275
#define tBROADCAST 276
#define tBYTE 277
#define tBYTECOUNT 278
#define tCALLAS 279
#define tCALLBACK 280
#define tCASE 281
#define tCDECL 282
#define tCHAR 283
#define tCOCLASS 284
#define tCODE 285
#define tCOMMSTATUS 286
#define tCONST 287
#define tCONTEXTHANDLE 288
#define tCONTEXTHANDLENOSERIALIZE 289
#define tCONTEXTHANDLESERIALIZE 290
#define tCONTROL 291
#define tCPPQUOTE 292
#define tDEFAULT 293
#define tDEFAULTCOLLELEM 294
#define tDEFAULTVALUE 295
#define tDEFAULTVTABLE 296
#define tDISPLAYBIND 297
#define tDISPINTERFACE 298
#define tDLLNAME 299
#define tDOUBLE 300
#define tDUAL 301
#define tENDPOINT 302
#define tENTRY 303
#define tENUM 304
#define tERRORSTATUST 305
#define tEXPLICITHANDLE 306
#define tEXTERN 307
#define tFALSE 308
#define tFASTCALL 309
#define tFLOAT 310
#define tHANDLE 311
#define tHANDLET 312
#define tHELPCONTEXT 313
#define tHELPFILE 314
#define tHELPSTRING 315
#define tHELPSTRINGCONTEXT 316
#define tHELPSTRINGDLL 317
#define tHIDDEN 318
#define tHYPER 319
#define tID 320
#define tIDEMPOTENT 321
#define tIIDIS 322
#define tIMMEDIATEBIND 323
#define tIMPLICITHANDLE 324
#define tIMPORT 325
#define tIMPORTLIB 326
#define tIN 327
#define tINLINE 328
#define tINPUTSYNC 329
#define tINT 330
#define tINT64 331
#define tINTERFACE 332
#define tLCID 333
#define tLENGTHIS 334
#define tLIBRARY 335
#define tLOCAL 336
#define tLONG 337
#define tMETHODS 338
#define tMODULE 339
#define tNONBROWSABLE 340
#define tNONCREATABLE 341
#define tNONEXTENSIBLE 342
#define tOBJECT 343
#define tODL 344
#define tOLEAUTOMATION 345
#define tOPTIONAL 346
#define tOUT 347
#define tPASCAL 348
#define tPOINTERDEFAULT 349
#define tPROPERTIES 350
#define tPROPGET 351
#define tPROPPUT 352
#define tPROPPUTREF 353
#define tPTR 354
#define tPUBLIC 355
#define tRANGE 356
#define tREADONLY 357
#define tREF 358
#define tREQUESTEDIT 359
#define tRESTRICTED 360
#define tRETVAL 361
#define tSAFEARRAY 362
#define tSHORT 363
#define tSIGNED 364
#define tSINGLE 365
#define tSIZEIS 366
#define tSIZEOF 367
#define tSMALL 368
#define tSOURCE 369
#define tSTDCALL 370
#define tSTRICTCONTEXTHANDLE 371
#define tSTRING 372
#define tSTRUCT 373
#define tSWITCH 374
#define tSWITCHIS 375
#define tSWITCHTYPE 376
#define tTRANSMITAS 377
#define tTRUE 378
#define tTYPEDEF 379
#define tUNION 380
#define tUNIQUE 381
#define tUNSIGNED 382
#define tUUID 383
#define tV1ENUM 384
#define tVARARG 385
#define tVERSION 386
#define tVOID 387
#define tWCHAR 388
#define tWIREMARSHAL 389
#define CAST 390
#define PPTR 391
#define NEG 392
#define ADDRESSOF 393




/* Copy the first part of user declarations.  */
#line 1 "parser.y"

/*
 * IDL Compiler
 *
 * Copyright 2002 Ove Kaaven
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"
#include "typelib.h"
#include "typegen.h"

#if defined(YYBYACC)
	/* Berkeley yacc (byacc) doesn't seem to know about these */
	/* Some *BSD supplied versions do define these though */
# ifndef YYEMPTY
#  define YYEMPTY	(-1)	/* Empty lookahead value of yychar */
# endif
# ifndef YYLEX
#  define YYLEX		yylex()
# endif

#elif defined(YYBISON)
	/* Bison was used for original development */
	/* #define YYEMPTY -2 */
	/* #define YYLEX   yylex() */

#else
	/* No yacc we know yet */
# if !defined(YYEMPTY) || !defined(YYLEX)
#  error Yacc version/type unknown. This version needs to be verified for settings of YYEMPTY and YYLEX.
# elif defined(__GNUC__)	/* gcc defines the #warning directive */
#  warning Yacc version/type unknown. It defines YYEMPTY and YYLEX, but is not tested
  /* #else we just take a chance that it works... */
# endif
#endif

#define YYERROR_VERBOSE

unsigned char pointer_default = RPC_FC_UP;
static int is_object_interface = FALSE;

typedef struct list typelist_t;
struct typenode {
  type_t *type;
  struct list entry;
};

typelist_t incomplete_types = LIST_INIT(incomplete_types);

static void add_incomplete(type_t *t);
static void fix_incomplete(void);

static str_list_t *append_str(str_list_t *list, char *str);
static attr_list_t *append_attr(attr_list_t *list, attr_t *attr);
static attr_t *make_attr(enum attr_type type);
static attr_t *make_attrv(enum attr_type type, unsigned long val);
static attr_t *make_attrp(enum attr_type type, void *val);
static expr_t *make_expr(enum expr_type type);
static expr_t *make_exprl(enum expr_type type, long val);
static expr_t *make_exprd(enum expr_type type, double val);
static expr_t *make_exprs(enum expr_type type, char *val);
static expr_t *make_exprt(enum expr_type type, type_t *tref, expr_t *expr);
static expr_t *make_expr1(enum expr_type type, expr_t *expr);
static expr_t *make_expr2(enum expr_type type, expr_t *exp1, expr_t *exp2);
static expr_t *make_expr3(enum expr_type type, expr_t *expr1, expr_t *expr2, expr_t *expr3);
static type_t *make_type(unsigned char type, type_t *ref);
static expr_list_t *append_expr(expr_list_t *list, expr_t *expr);
static array_dims_t *append_array(array_dims_t *list, expr_t *expr);
static void set_type(var_t *v, type_t *type, const pident_t *pident, array_dims_t *arr, int top);
static ifref_list_t *append_ifref(ifref_list_t *list, ifref_t *iface);
static ifref_t *make_ifref(type_t *iface);
static var_list_t *append_var(var_list_t *list, var_t *var);
static var_t *make_var(char *name);
static pident_list_t *append_pident(pident_list_t *list, pident_t *p);
static pident_t *make_pident(var_t *var);
static func_list_t *append_func(func_list_t *list, func_t *func);
static func_t *make_func(var_t *def, var_list_t *args);
static type_t *make_class(char *name);
static type_t *make_safearray(type_t *type);
static type_t *make_builtin(char *name);
static type_t *make_int(int sign);

static type_t *reg_type(type_t *type, const char *name, int t);
static type_t *reg_typedefs(type_t *type, var_list_t *names, attr_list_t *attrs);
static type_t *find_type(const char *name, int t);
static type_t *find_type2(char *name, int t);
static type_t *get_type(unsigned char type, char *name, int t);
static type_t *get_typev(unsigned char type, var_t *name, int t);
static int get_struct_type(var_list_t *fields);

static var_t *reg_const(var_t *var);
static var_t *find_const(char *name, int f);

static void write_libid(const char *name, const attr_list_t *attr);
static void write_clsid(type_t *cls);
static void write_diid(type_t *iface);
static void write_iid(type_t *iface);

static int compute_method_indexes(type_t *iface);
static char *gen_name(void);
static void process_typedefs(var_list_t *names);
static void check_arg(var_t *arg);
static void check_all_user_types(ifref_list_t *ifaces);

#define tsENUM   1
#define tsSTRUCT 2
#define tsUNION  3



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 139 "parser.y"
typedef union YYSTYPE {
	attr_t *attr;
	attr_list_t *attr_list;
	str_list_t *str_list;
	expr_t *expr;
	expr_list_t *expr_list;
	array_dims_t *array_dims;
	type_t *type;
	var_t *var;
	var_list_t *var_list;
	pident_t *pident;
	pident_list_t *pident_list;
	func_t *func;
	func_list_t *func_list;
	ifref_t *ifref;
	ifref_list_t *ifref_list;
	char *str;
	UUID *uuid;
	unsigned int num;
	double dbl;
	interface_info_t ifinfo;
} YYSTYPE;
/* Line 196 of yacc.c.  */
#line 531 "parser.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */
#line 543 "parser.tab.c"

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T) && (defined (__STDC__) || defined (__cplusplus))
# include <stddef.h> /* INFRINGES ON USER NAME SPACE */
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if defined (__STDC__) || defined (__cplusplus)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     define YYINCLUDED_STDLIB_H
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2005 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM ((YYSIZE_T) -1)
#  endif
#  ifdef __cplusplus
extern "C" {
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if (! defined (malloc) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if (! defined (free) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifdef __cplusplus
}
#  endif
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short int yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (YYSTYPE))			\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short int yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1044

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  158
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  78
/* YYNRULES -- Number of rules. */
#define YYNRULES  277
/* YYNRULES -- Number of states. */
#define YYNSTATES  506

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   393

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   139,     2,
     150,   151,   142,   141,   135,   140,   157,   143,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   137,   149,
       2,   156,     2,   136,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   154,     2,   155,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   152,   138,   153,   144,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     145,   146,   147,   148
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,     5,     6,     9,    12,    16,    19,    22,
      25,    28,    29,    32,    35,    39,    42,    45,    48,    51,
      54,    55,    59,    62,    63,    65,    68,    70,    73,    76,
      78,    81,    84,    87,    92,    96,   100,   106,   109,   113,
     118,   119,   121,   123,   125,   129,   131,   136,   140,   141,
     145,   149,   151,   155,   160,   161,   163,   167,   169,   173,
     178,   180,   184,   185,   187,   189,   191,   193,   195,   200,
     205,   207,   209,   211,   213,   215,   217,   222,   227,   229,
     231,   236,   238,   243,   248,   253,   255,   257,   262,   267,
     272,   277,   282,   284,   289,   291,   296,   298,   304,   306,
     308,   313,   315,   317,   319,   321,   323,   325,   327,   329,
     331,   336,   338,   340,   342,   344,   351,   353,   355,   357,
     359,   364,   366,   368,   370,   375,   380,   385,   390,   392,
     394,   399,   404,   406,   408,   410,   412,   414,   416,   418,
     419,   422,   427,   431,   437,   438,   441,   443,   445,   449,
     453,   455,   461,   463,   467,   468,   470,   472,   474,   476,
     478,   480,   482,   488,   492,   496,   500,   504,   508,   512,
     516,   520,   523,   526,   529,   532,   537,   542,   546,   548,
     552,   554,   559,   560,   563,   566,   570,   573,   575,   580,
     584,   585,   587,   588,   590,   592,   594,   596,   598,   600,
     602,   605,   608,   610,   612,   614,   616,   618,   620,   622,
     623,   625,   627,   630,   632,   635,   638,   640,   642,   645,
     648,   651,   657,   658,   661,   664,   667,   670,   673,   676,
     680,   683,   687,   693,   699,   700,   703,   706,   709,   712,
     719,   728,   731,   734,   737,   740,   743,   746,   752,   755,
     758,   761,   763,   768,   770,   774,   776,   778,   782,   784,
     786,   788,   794,   796,   798,   800,   803,   805,   808,   810,
     813,   815,   818,   823,   828,   834,   845,   847
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short int yyrhs[] =
{
     159,     0,    -1,   160,    -1,    -1,   160,   222,    -1,   160,
     221,    -1,   160,   208,   149,    -1,   160,   210,    -1,   160,
     225,    -1,   160,   171,    -1,   160,   164,    -1,    -1,   161,
     222,    -1,   161,   221,    -1,   161,   208,   149,    -1,   161,
     210,    -1,   161,   225,    -1,   161,   164,    -1,   161,   168,
      -1,   161,   171,    -1,    -1,   162,   201,   149,    -1,   162,
     164,    -1,    -1,   149,    -1,   187,   149,    -1,   165,    -1,
     191,   149,    -1,   197,   149,    -1,   167,    -1,   231,   149,
      -1,   233,   149,    -1,   234,   149,    -1,    37,   150,     8,
     151,    -1,    70,     8,   149,    -1,   166,   161,    10,    -1,
      71,   150,     8,   151,   163,    -1,    80,     3,    -1,   179,
     169,   152,    -1,   170,   161,   153,   163,    -1,    -1,   174,
      -1,   132,    -1,   175,    -1,   174,   135,   175,    -1,   173,
      -1,   179,   232,   226,   176,    -1,   232,   226,   176,    -1,
      -1,   154,   177,   155,    -1,   154,   142,   155,    -1,   193,
      -1,   177,   135,   194,    -1,   177,   155,   154,   194,    -1,
      -1,   179,    -1,   154,   180,   155,    -1,   182,    -1,   180,
     135,   182,    -1,   180,   155,   154,   182,    -1,     8,    -1,
     181,   135,     8,    -1,    -1,    13,    -1,    15,    -1,    16,
      -1,    18,    -1,    19,    -1,    24,   150,   204,   151,    -1,
      26,   150,   195,   151,    -1,    33,    -1,    34,    -1,    35,
      -1,    36,    -1,    38,    -1,    39,    -1,    40,   150,   196,
     151,    -1,    40,   150,     8,   151,    -1,    41,    -1,    42,
      -1,    44,   150,     8,   151,    -1,    46,    -1,    47,   150,
     181,   151,    -1,    48,   150,     8,   151,    -1,    48,   150,
     196,   151,    -1,    51,    -1,    56,    -1,    58,   150,   196,
     151,    -1,    59,   150,     8,   151,    -1,    60,   150,     8,
     151,    -1,    61,   150,   196,   151,    -1,    62,   150,     8,
     151,    -1,    63,    -1,    65,   150,   196,   151,    -1,    66,
      -1,    67,   150,   194,   151,    -1,    68,    -1,    69,   150,
      57,     3,   151,    -1,    72,    -1,    74,    -1,    79,   150,
     192,   151,    -1,    81,    -1,    85,    -1,    86,    -1,    87,
      -1,    88,    -1,    89,    -1,    90,    -1,    91,    -1,    92,
      -1,    94,   150,   230,   151,    -1,    96,    -1,    97,    -1,
      98,    -1,   100,    -1,   101,   150,   196,   135,   196,   151,
      -1,   102,    -1,   104,    -1,   105,    -1,   106,    -1,   111,
     150,   192,   151,    -1,   114,    -1,   116,    -1,   117,    -1,
     120,   150,   194,   151,    -1,   121,   150,   232,   151,    -1,
     122,   150,   232,   151,    -1,   128,   150,   183,   151,    -1,
     129,    -1,   130,    -1,   131,   150,   235,   151,    -1,   134,
     150,   232,   151,    -1,   230,    -1,     9,    -1,     8,    -1,
      27,    -1,    54,    -1,    93,    -1,   115,    -1,    -1,   185,
     186,    -1,    26,   194,   137,   199,    -1,    38,   137,   199,
      -1,    32,   232,   204,   156,   196,    -1,    -1,   189,   135,
      -1,   189,    -1,   190,    -1,   189,   135,   190,    -1,   204,
     156,   196,    -1,   204,    -1,    49,   203,   152,   188,   153,
      -1,   193,    -1,   192,   135,   193,    -1,    -1,   194,    -1,
       5,    -1,     6,    -1,     7,    -1,    53,    -1,   123,    -1,
       3,    -1,   194,   136,   194,   137,   194,    -1,   194,   138,
     194,    -1,   194,   139,   194,    -1,   194,   141,   194,    -1,
     194,   140,   194,    -1,   194,   142,   194,    -1,   194,   143,
     194,    -1,   194,    11,   194,    -1,   194,    12,   194,    -1,
     144,   194,    -1,   140,   194,    -1,   139,   194,    -1,   142,
     194,    -1,   150,   232,   151,   194,    -1,   112,   150,   232,
     151,    -1,   150,   194,   151,    -1,   196,    -1,   195,   135,
     196,    -1,   194,    -1,    52,    32,   232,   204,    -1,    -1,
     198,   199,    -1,   200,   149,    -1,   178,   234,   149,    -1,
     179,   149,    -1,   149,    -1,   178,   232,   226,   176,    -1,
     178,   232,   226,    -1,    -1,   204,    -1,    -1,     3,    -1,
       4,    -1,     3,    -1,     4,    -1,    22,    -1,   133,    -1,
     207,    -1,   109,   207,    -1,   127,   207,    -1,   127,    -1,
      55,    -1,   110,    -1,    45,    -1,    20,    -1,    50,    -1,
      57,    -1,    -1,    75,    -1,    75,    -1,   108,   206,    -1,
     113,    -1,    82,   206,    -1,    64,   206,    -1,    76,    -1,
      28,    -1,    29,     3,    -1,    29,     4,    -1,   179,   208,
      -1,   209,   152,   211,   153,   163,    -1,    -1,   211,   212,
      -1,   178,   222,    -1,    43,     3,    -1,    43,     4,    -1,
     179,   213,    -1,    95,   137,    -1,   215,   200,   149,    -1,
      83,   137,    -1,   216,   201,   149,    -1,   214,   152,   215,
     216,   153,    -1,   214,   152,   219,   149,   153,    -1,    -1,
     137,     4,    -1,    77,     3,    -1,    77,     4,    -1,   179,
     219,    -1,   220,   218,   152,   162,   153,   163,    -1,   220,
     137,     3,   152,   167,   162,   153,   163,    -1,   217,   163,
      -1,   219,   149,    -1,   213,   149,    -1,    84,     3,    -1,
      84,     4,    -1,   179,   223,    -1,   224,   152,   162,   153,
     163,    -1,   142,   226,    -1,    32,   226,    -1,   184,   226,
      -1,   228,    -1,   228,   150,   172,   151,    -1,   204,    -1,
     150,   226,   151,    -1,   227,    -1,   226,    -1,   229,   135,
     226,    -1,   103,    -1,   126,    -1,    99,    -1,   118,   203,
     152,   198,   153,    -1,   132,    -1,     4,    -1,   205,    -1,
      32,   232,    -1,   191,    -1,    49,     3,    -1,   231,    -1,
     118,     3,    -1,   234,    -1,   125,     3,    -1,   107,   150,
     232,   151,    -1,   124,   178,   232,   229,    -1,   125,   203,
     152,   198,   153,    -1,   125,   203,   119,   150,   200,   151,
     202,   152,   185,   153,    -1,     5,    -1,     5,   157,     5,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   292,   292,   301,   302,   303,   304,   308,   313,   314,
     315,   318,   319,   320,   321,   322,   326,   327,   328,   329,
     332,   333,   334,   337,   338,   341,   342,   343,   348,   349,
     350,   355,   356,   363,   365,   371,   375,   379,   381,   386,
     390,   391,   394,   397,   398,   399,   403,   408,   414,   415,
     416,   419,   420,   421,   424,   425,   429,   435,   436,   437,
     440,   441,   444,   445,   446,   447,   448,   449,   450,   451,
     452,   453,   454,   455,   456,   457,   458,   459,   460,   461,
     462,   463,   464,   465,   466,   467,   468,   469,   470,   471,
     472,   473,   474,   475,   476,   477,   478,   479,   480,   481,
     482,   483,   484,   485,   486,   487,   488,   489,   490,   491,
     492,   493,   494,   495,   496,   497,   500,   501,   502,   503,
     504,   505,   506,   507,   508,   509,   510,   511,   512,   513,
     514,   515,   516,   520,   521,   526,   527,   528,   529,   532,
     533,   536,   540,   546,   552,   553,   554,   557,   561,   570,
     574,   579,   588,   589,   602,   603,   606,   607,   608,   609,
     610,   611,   612,   613,   614,   615,   616,   617,   618,   619,
     620,   621,   622,   623,   624,   625,   626,   627,   630,   631,
     634,   640,   645,   646,   649,   650,   651,   652,   655,   663,
     675,   676,   679,   680,   681,   684,   686,   689,   690,   691,
     692,   693,   709,   710,   711,   712,   713,   714,   715,   718,
     719,   722,   723,   724,   725,   726,   727,   728,   731,   732,
     738,   747,   754,   755,   759,   762,   763,   766,   779,   780,
     783,   784,   787,   796,   805,   806,   809,   810,   813,   825,
     837,   849,   853,   854,   857,   858,   861,   866,   873,   874,
     875,   879,   882,   889,   890,   891,   898,   899,   903,   904,
     905,   908,   919,   920,   921,   922,   923,   924,   925,   926,
     927,   928,   929,   932,   937,   942,   959,   960
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "aIDENTIFIER", "aKNOWNTYPE", "aNUM",
  "aHEXNUM", "aDOUBLE", "aSTRING", "aUUID", "aEOF", "SHL", "SHR",
  "tAGGREGATABLE", "tALLOCATE", "tAPPOBJECT", "tASYNC", "tASYNCUUID",
  "tAUTOHANDLE", "tBINDABLE", "tBOOLEAN", "tBROADCAST", "tBYTE",
  "tBYTECOUNT", "tCALLAS", "tCALLBACK", "tCASE", "tCDECL", "tCHAR",
  "tCOCLASS", "tCODE", "tCOMMSTATUS", "tCONST", "tCONTEXTHANDLE",
  "tCONTEXTHANDLENOSERIALIZE", "tCONTEXTHANDLESERIALIZE", "tCONTROL",
  "tCPPQUOTE", "tDEFAULT", "tDEFAULTCOLLELEM", "tDEFAULTVALUE",
  "tDEFAULTVTABLE", "tDISPLAYBIND", "tDISPINTERFACE", "tDLLNAME",
  "tDOUBLE", "tDUAL", "tENDPOINT", "tENTRY", "tENUM", "tERRORSTATUST",
  "tEXPLICITHANDLE", "tEXTERN", "tFALSE", "tFASTCALL", "tFLOAT", "tHANDLE",
  "tHANDLET", "tHELPCONTEXT", "tHELPFILE", "tHELPSTRING",
  "tHELPSTRINGCONTEXT", "tHELPSTRINGDLL", "tHIDDEN", "tHYPER", "tID",
  "tIDEMPOTENT", "tIIDIS", "tIMMEDIATEBIND", "tIMPLICITHANDLE", "tIMPORT",
  "tIMPORTLIB", "tIN", "tINLINE", "tINPUTSYNC", "tINT", "tINT64",
  "tINTERFACE", "tLCID", "tLENGTHIS", "tLIBRARY", "tLOCAL", "tLONG",
  "tMETHODS", "tMODULE", "tNONBROWSABLE", "tNONCREATABLE",
  "tNONEXTENSIBLE", "tOBJECT", "tODL", "tOLEAUTOMATION", "tOPTIONAL",
  "tOUT", "tPASCAL", "tPOINTERDEFAULT", "tPROPERTIES", "tPROPGET",
  "tPROPPUT", "tPROPPUTREF", "tPTR", "tPUBLIC", "tRANGE", "tREADONLY",
  "tREF", "tREQUESTEDIT", "tRESTRICTED", "tRETVAL", "tSAFEARRAY", "tSHORT",
  "tSIGNED", "tSINGLE", "tSIZEIS", "tSIZEOF", "tSMALL", "tSOURCE",
  "tSTDCALL", "tSTRICTCONTEXTHANDLE", "tSTRING", "tSTRUCT", "tSWITCH",
  "tSWITCHIS", "tSWITCHTYPE", "tTRANSMITAS", "tTRUE", "tTYPEDEF", "tUNION",
  "tUNIQUE", "tUNSIGNED", "tUUID", "tV1ENUM", "tVARARG", "tVERSION",
  "tVOID", "tWCHAR", "tWIREMARSHAL", "','", "'?'", "':'", "'|'", "'&'",
  "'-'", "'+'", "'*'", "'/'", "'~'", "CAST", "PPTR", "NEG", "ADDRESSOF",
  "';'", "'('", "')'", "'{'", "'}'", "'['", "']'", "'='", "'.'", "$accept",
  "input", "gbl_statements", "imp_statements", "int_statements",
  "semicolon_opt", "statement", "cppquote", "import_start", "import",
  "importlib", "libraryhdr", "library_start", "librarydef", "m_args",
  "no_args", "args", "arg", "array", "array_list", "m_attributes",
  "attributes", "attrib_list", "str_list", "attribute", "uuid_string",
  "callconv", "cases", "case", "constdef", "enums", "enum_list", "enum",
  "enumdef", "m_exprs", "m_expr", "expr", "expr_list_const", "expr_const",
  "externdef", "fields", "field", "s_field", "funcdef", "m_ident",
  "t_ident", "ident", "base_type", "m_int", "int_std", "coclass",
  "coclasshdr", "coclassdef", "coclass_ints", "coclass_int",
  "dispinterface", "dispinterfacehdr", "dispint_props", "dispint_meths",
  "dispinterfacedef", "inherit", "interface", "interfacehdr",
  "interfacedef", "interfacedec", "module", "modulehdr", "moduledef",
  "pident", "func_ident", "direct_ident", "pident_list", "pointer_type",
  "structdef", "type", "typedef", "uniondef", "version", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short int yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,    44,    63,    58,   124,    38,
      45,    43,    42,    47,   126,   390,   391,   392,   393,    59,
      40,    41,   123,   125,    91,    93,    61,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,   158,   159,   160,   160,   160,   160,   160,   160,   160,
     160,   161,   161,   161,   161,   161,   161,   161,   161,   161,
     162,   162,   162,   163,   163,   164,   164,   164,   164,   164,
     164,   164,   164,   165,   166,   167,   168,   169,   170,   171,
     172,   172,   173,   174,   174,   174,   175,   175,   176,   176,
     176,   177,   177,   177,   178,   178,   179,   180,   180,   180,
     181,   181,   182,   182,   182,   182,   182,   182,   182,   182,
     182,   182,   182,   182,   182,   182,   182,   182,   182,   182,
     182,   182,   182,   182,   182,   182,   182,   182,   182,   182,
     182,   182,   182,   182,   182,   182,   182,   182,   182,   182,
     182,   182,   182,   182,   182,   182,   182,   182,   182,   182,
     182,   182,   182,   182,   182,   182,   182,   182,   182,   182,
     182,   182,   182,   182,   182,   182,   182,   182,   182,   182,
     182,   182,   182,   183,   183,   184,   184,   184,   184,   185,
     185,   186,   186,   187,   188,   188,   188,   189,   189,   190,
     190,   191,   192,   192,   193,   193,   194,   194,   194,   194,
     194,   194,   194,   194,   194,   194,   194,   194,   194,   194,
     194,   194,   194,   194,   194,   194,   194,   194,   195,   195,
     196,   197,   198,   198,   199,   199,   199,   199,   200,   201,
     202,   202,   203,   203,   203,   204,   204,   205,   205,   205,
     205,   205,   205,   205,   205,   205,   205,   205,   205,   206,
     206,   207,   207,   207,   207,   207,   207,   207,   208,   208,
     209,   210,   211,   211,   212,   213,   213,   214,   215,   215,
     216,   216,   217,   217,   218,   218,   219,   219,   220,   221,
     221,   221,   222,   222,   223,   223,   224,   225,   226,   226,
     226,   226,   227,   228,   228,   228,   229,   229,   230,   230,
     230,   231,   232,   232,   232,   232,   232,   232,   232,   232,
     232,   232,   232,   233,   234,   234,   235,   235
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     0,     2,     2,     3,     2,     2,     2,
       2,     0,     2,     2,     3,     2,     2,     2,     2,     2,
       0,     3,     2,     0,     1,     2,     1,     2,     2,     1,
       2,     2,     2,     4,     3,     3,     5,     2,     3,     4,
       0,     1,     1,     1,     3,     1,     4,     3,     0,     3,
       3,     1,     3,     4,     0,     1,     3,     1,     3,     4,
       1,     3,     0,     1,     1,     1,     1,     1,     4,     4,
       1,     1,     1,     1,     1,     1,     4,     4,     1,     1,
       4,     1,     4,     4,     4,     1,     1,     4,     4,     4,
       4,     4,     1,     4,     1,     4,     1,     5,     1,     1,
       4,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       4,     1,     1,     1,     1,     6,     1,     1,     1,     1,
       4,     1,     1,     1,     4,     4,     4,     4,     1,     1,
       4,     4,     1,     1,     1,     1,     1,     1,     1,     0,
       2,     4,     3,     5,     0,     2,     1,     1,     3,     3,
       1,     5,     1,     3,     0,     1,     1,     1,     1,     1,
       1,     1,     5,     3,     3,     3,     3,     3,     3,     3,
       3,     2,     2,     2,     2,     4,     4,     3,     1,     3,
       1,     4,     0,     2,     2,     3,     2,     1,     4,     3,
       0,     1,     0,     1,     1,     1,     1,     1,     1,     1,
       2,     2,     1,     1,     1,     1,     1,     1,     1,     0,
       1,     1,     2,     1,     2,     2,     1,     1,     2,     2,
       2,     5,     0,     2,     2,     2,     2,     2,     2,     3,
       2,     3,     5,     5,     0,     2,     2,     2,     2,     6,
       8,     2,     2,     2,     2,     2,     2,     5,     2,     2,
       2,     1,     4,     1,     3,     1,     1,     3,     1,     1,
       1,     5,     1,     1,     1,     2,     1,     2,     1,     2,
       1,     2,     4,     4,     5,    10,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned short int yydefact[] =
{
       3,     0,     2,     1,     0,     0,     0,     0,   192,     0,
       0,     0,   192,    54,   192,    62,    10,    26,    11,    29,
      11,     9,     0,     0,     0,     0,     0,     0,     7,     0,
       0,    23,     0,   234,     5,     4,     0,     8,     0,     0,
       0,   218,   219,   263,   206,   197,   217,     0,   205,   192,
     207,   203,   208,   209,   211,   216,   209,     0,   209,     0,
     204,   213,   192,   192,   202,   262,   198,   266,   264,   199,
     268,     0,   270,     0,   225,   226,   193,   194,     0,     0,
       0,   236,   237,     0,     0,    55,     0,    63,    64,    65,
      66,    67,     0,     0,    70,    71,    72,    73,    74,    75,
       0,    78,    79,     0,    81,     0,     0,    85,    86,     0,
       0,     0,     0,     0,    92,     0,    94,     0,    96,     0,
      98,    99,     0,   101,   102,   103,   104,   105,   106,   107,
     108,   109,     0,   111,   112,   113,   260,   114,     0,   116,
     258,   117,   118,   119,     0,   121,   122,   123,     0,     0,
       0,   259,     0,   128,   129,     0,     0,     0,    57,   132,
       0,     0,     0,     0,     0,   220,   227,   238,   246,    25,
      27,    28,     6,   222,   243,     0,    24,   241,   242,     0,
       0,    20,    30,    31,    32,   265,   267,   210,   215,   214,
       0,   212,   200,   269,   271,   201,   195,   196,     0,     0,
     144,     0,    34,   182,     0,     0,   182,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   154,     0,     0,   154,     0,     0,     0,     0,     0,
       0,    62,    56,    35,     0,    17,    18,    19,     0,    15,
      13,    12,    16,    23,    37,   244,   245,    38,    54,     0,
      54,     0,     0,   235,    20,    54,     0,     0,    33,     0,
     146,   147,   150,   181,    54,   135,     0,   136,   137,   138,
       0,     0,     0,   253,   256,   255,   251,   273,    54,    54,
       0,   161,   156,   157,   158,   159,     0,   160,     0,     0,
       0,     0,     0,   180,     0,   178,     0,     0,     0,    60,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   152,   155,     0,     0,     0,     0,     0,     0,
     134,   133,     0,   276,     0,     0,    58,    62,     0,    14,
      39,    23,     0,   223,   228,     0,     0,     0,    54,     0,
       0,    54,    23,    22,     0,     0,   272,   143,   151,   145,
       0,   187,   261,     0,    55,   183,     0,   249,   248,     0,
     250,    40,     0,     0,   274,    68,     0,   173,   172,   174,
     171,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    69,    77,    76,    80,     0,    82,    83,
      84,    87,    88,    89,    90,    91,    93,    95,     0,   154,
     100,   110,     0,   120,   124,   125,   126,   127,     0,   130,
     131,    59,     0,   221,   224,   230,     0,   229,   232,     0,
     233,    20,    23,   247,     0,    21,   148,   149,   270,   186,
     184,   254,   262,     0,    45,    41,    43,     0,     0,   257,
     190,     0,   177,     0,   169,   170,     0,   163,   164,   166,
     165,   167,   168,   179,    61,    97,   153,     0,   277,    23,
      48,   231,    54,   239,   189,   185,   252,     0,     0,    48,
       0,   191,   176,   175,     0,   115,    36,   154,   188,    23,
      44,    48,    47,   139,   162,     0,     0,    51,   240,    46,
       0,    50,     0,    49,     0,     0,   275,   140,    52,     0,
       0,    54,    53,    54,   142,   141
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,     1,     2,   160,   255,   177,   343,    17,    18,    19,
     236,   164,    20,   237,   433,   434,   435,   436,   478,   486,
     344,    85,   157,   300,   158,   322,   272,   490,   497,    23,
     259,   260,   261,    67,   311,   312,   293,   294,   295,    25,
     264,   355,   356,   345,   470,    78,   273,    68,   188,    69,
     238,    27,   239,   248,   333,    29,    30,   250,   338,    31,
     180,    32,    33,   240,   241,   168,    36,   242,   274,   275,
     276,   277,   159,    70,   416,    39,    72,   324
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -463
static const short int yypact[] =
{
    -463,    30,   766,  -463,   128,   732,   -86,   132,   134,    36,
      71,   140,   134,   -61,   134,   910,  -463,  -463,  -463,  -463,
    -463,  -463,    23,   -47,   -43,   -36,   -33,   -13,  -463,   -30,
      16,   -20,     2,    48,  -463,  -463,    61,  -463,    73,    77,
      79,  -463,  -463,  -463,  -463,  -463,  -463,   732,  -463,   151,
    -463,  -463,  -463,   146,  -463,  -463,   146,    81,   146,   532,
    -463,  -463,   153,   188,   532,  -463,  -463,  -463,  -463,  -463,
    -463,   190,  -463,   221,  -463,  -463,  -463,  -463,    80,   732,
      85,  -463,  -463,    88,   732,  -463,   -93,  -463,  -463,  -463,
    -463,  -463,    87,    92,  -463,  -463,  -463,  -463,  -463,  -463,
      93,  -463,  -463,    94,  -463,    96,    97,  -463,  -463,    98,
      99,   103,   104,   105,  -463,   107,  -463,   108,  -463,   110,
    -463,  -463,   113,  -463,  -463,  -463,  -463,  -463,  -463,  -463,
    -463,  -463,   115,  -463,  -463,  -463,  -463,  -463,   118,  -463,
    -463,  -463,  -463,  -463,   119,  -463,  -463,  -463,   121,   122,
     124,  -463,   125,  -463,  -463,   126,   129,  -112,  -463,  -463,
      28,   742,   275,   193,   131,  -463,  -463,  -463,  -463,  -463,
    -463,  -463,  -463,  -463,  -463,   -23,  -463,  -463,  -463,   195,
     133,  -463,  -463,  -463,  -463,  -463,   135,  -463,  -463,  -463,
     732,  -463,  -463,   135,   -90,  -463,  -463,  -463,   130,   141,
     190,   190,  -463,  -463,    91,   143,  -463,   190,   536,   291,
     272,   276,   303,   536,   283,   292,   536,   293,   536,   536,
     245,   536,   -68,   536,   536,   536,   732,   732,   199,   298,
     732,   910,   150,  -463,   155,  -463,  -463,  -463,   158,  -463,
    -463,  -463,  -463,   -20,  -463,  -463,  -463,  -463,    56,   175,
     -58,   164,   163,  -463,  -463,   352,   171,   536,  -463,   172,
     189,  -463,   170,  -463,   -39,  -463,    91,  -463,  -463,  -463,
      91,    91,    91,  -463,  -463,  -463,   177,   196,   -61,   -28,
     179,  -463,  -463,  -463,  -463,  -463,   182,  -463,   536,   536,
     536,   536,   524,   599,  -111,  -463,   185,   186,   187,  -463,
     -98,   192,   197,   198,   201,   206,   209,   214,   216,   178,
     325,   -60,  -463,   599,   217,   204,   -34,   239,   218,   220,
    -463,  -463,   225,   176,   232,   234,  -463,   910,   332,  -463,
    -463,   -20,   -27,  -463,  -463,   235,   732,   237,    58,   240,
     318,   768,   -20,  -463,   732,   246,  -463,  -463,  -463,   190,
     536,  -463,  -463,   732,   247,  -463,   249,  -463,  -463,   243,
    -463,   342,    91,   251,  -463,  -463,   732,  -463,  -463,  -463,
    -463,   575,   254,   536,   536,   536,   536,   536,   536,   536,
     536,   536,   536,  -463,  -463,  -463,  -463,   392,  -463,  -463,
    -463,  -463,  -463,  -463,  -463,  -463,  -463,  -463,   256,   536,
    -463,  -463,   536,  -463,  -463,  -463,  -463,  -463,   403,  -463,
    -463,  -463,   258,  -463,  -463,  -463,    91,  -463,  -463,   261,
    -463,  -463,   -20,  -463,    91,  -463,  -463,  -463,   262,  -463,
    -463,  -463,   -31,   265,  -463,   277,  -463,   732,    91,  -463,
     190,   268,  -463,   536,  -463,  -463,   481,    62,     7,    -1,
      -1,   208,   208,  -463,  -463,  -463,  -463,   269,  -463,   -20,
     259,  -463,   779,  -463,  -463,  -463,  -463,   458,    91,   259,
     271,  -463,  -463,  -463,   536,  -463,  -463,   548,  -463,   -20,
    -463,   259,  -463,  -463,   599,   211,   -88,  -463,  -463,  -463,
      10,  -463,   536,   267,   536,   288,  -463,  -463,   599,   536,
     591,   -71,   599,   -71,  -463,  -463
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -463,  -463,  -463,   407,  -242,  -234,    20,  -463,  -463,    89,
    -463,  -463,  -463,   426,  -463,  -463,  -463,   -35,  -399,  -463,
     -12,    -2,  -463,  -463,  -218,  -463,  -463,  -463,  -463,  -463,
    -463,  -463,    90,     4,   212,  -385,  -204,  -463,  -167,  -463,
     231,  -462,  -223,   100,  -463,    49,   -67,  -463,    72,    63,
      67,  -463,   438,  -463,  -463,   422,  -463,  -463,  -463,  -463,
    -463,   -15,  -463,   444,     3,  -463,  -463,   446,  -238,  -463,
    -463,  -463,   236,     6,    -3,  -463,     1,  -463
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -194
static const short int yytable[] =
{
      22,    84,    71,    40,   198,    35,    24,   167,    38,   330,
     373,   374,   341,   326,   456,   309,     7,   313,   373,   374,
     313,   317,    16,   231,   382,   335,   205,   337,   357,  -193,
       3,   136,   358,   359,   360,   140,   494,   387,   233,   504,
     383,   505,   297,   232,   185,   302,   303,   492,   495,   306,
      11,   308,     4,   388,    11,   363,   315,     4,   151,   206,
       5,    83,  -193,    86,    73,     6,     7,   493,    79,    26,
     482,     7,   249,   373,   374,   399,   201,     8,   351,    80,
       9,   204,   489,    15,   367,   368,   369,   370,   371,   165,
     347,   400,   487,    15,   196,   197,    15,   413,    10,   234,
      11,   399,   169,   162,   -42,    11,   170,   163,   423,   411,
     351,    83,    86,   171,   352,    15,   172,   403,   265,   174,
     -42,   351,   192,   266,   439,   364,    15,   195,   189,   176,
     191,    41,    42,   262,   263,    74,    75,    76,    77,   173,
     280,   380,   381,    81,    82,   267,    12,   378,   379,   380,
     381,   178,    13,    14,   186,    77,   193,    77,    22,    22,
     251,    40,    40,   496,    24,    24,    38,    38,   175,   444,
     445,   446,   447,   448,   449,   450,   451,   452,   460,   462,
     235,   235,    15,   427,   268,   179,   464,   256,   463,   373,
     374,   194,    77,   196,   197,   313,   245,   246,   252,   253,
     469,   377,   378,   379,   380,   381,   269,   320,   321,   331,
      15,   418,    15,   181,   281,   453,   282,   283,   284,   373,
     374,   187,   182,   318,   319,   476,   183,   325,   184,   199,
     481,   190,   200,   270,   202,   457,   332,   207,   336,   473,
     203,   271,   208,   209,   210,   488,   211,   212,   213,   214,
     373,   374,   353,   215,   216,   217,    40,   218,   219,    24,
     220,    38,   354,   221,   285,   222,   336,   353,   223,   224,
     484,   225,   226,   313,   227,   228,   229,   354,   244,   230,
     298,   369,   262,   247,   299,   254,   257,  -193,   498,   372,
     500,   304,   258,   278,   281,   502,   282,   283,   284,   296,
     305,   307,   310,   323,   327,   328,   281,   329,   282,   283,
     284,   301,   334,   339,   375,   340,   376,   377,   378,   379,
     380,   381,   346,   286,   349,   348,   350,   361,   398,   397,
     365,   362,   366,   408,   287,   414,   384,   385,   386,   402,
     412,   424,    40,   389,   285,    24,    43,    38,   390,   391,
     288,   289,   392,   290,   428,   291,   285,   393,   438,   437,
     394,   292,    44,   441,    45,   395,   491,   396,   401,   405,
      46,   406,   415,   471,    47,   375,   407,   376,   377,   378,
     379,   380,   381,   409,     5,   410,   417,    48,    10,     6,
     404,    49,    50,   420,   431,   425,   429,    51,   430,    52,
     454,     8,   440,   286,     9,   443,    53,   455,   458,   459,
     461,   465,   467,   477,   287,   286,   466,    54,    55,   472,
     475,   499,    10,   483,    56,   501,   287,   161,    21,   421,
     288,   289,   480,   290,   468,   291,   316,   279,   419,   426,
      28,   292,   288,   289,   166,   290,    34,   291,    37,    57,
      58,    59,    60,   292,     0,    61,     0,     0,   314,     0,
      62,     0,    43,    40,   438,   437,    24,    63,    38,    64,
      12,     0,     0,     0,   432,    66,    13,    14,    44,     0,
      45,     0,     0,     0,     0,     0,    46,     0,     0,   353,
      47,   353,   373,   374,     0,     0,    15,     0,     0,   354,
       0,   354,     0,    48,     0,   342,    15,    49,    50,     0,
       0,     0,     0,    51,     0,    52,     0,     0,     0,     0,
       0,     0,    53,     0,     0,     0,     0,   281,    43,   282,
     283,   284,     0,    54,    55,     0,     0,     0,     0,   281,
      56,   282,   283,   284,    44,     0,    45,     0,     0,     0,
       0,   281,    46,   282,   283,   284,    47,     0,     0,     0,
      46,     0,     0,     0,     0,    57,    58,    59,    60,    48,
       0,    61,     0,    49,    50,     0,    62,   285,     0,    51,
       0,    52,     0,    63,     0,    64,   373,   374,    53,   285,
      65,    66,     0,     0,     0,     0,    53,     0,     0,    54,
      55,   285,   373,   374,     0,     0,    56,    54,    55,     0,
     373,   374,    15,     0,    56,     0,     0,   375,   474,   376,
     377,   378,   379,   380,   381,     0,     0,     0,     0,     0,
       0,    57,    58,    59,    60,     0,   286,    61,     0,     0,
      58,     0,    62,     0,     0,    61,     0,   287,   286,    63,
       0,    64,     0,     0,     0,     0,    65,    66,     0,   287,
     286,     0,     0,   288,   289,     0,   290,     0,   291,     0,
       0,   287,     0,     0,   292,   288,   289,     0,   290,     0,
     291,     0,     0,     0,     0,     0,   292,   288,   289,     0,
     485,     0,   291,     0,     0,     0,     0,     0,   292,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   375,     0,   376,   377,   378,   379,   380,   381,     0,
       0,     0,     0,     0,     0,     0,   442,   375,   503,   376,
     377,   378,   379,   380,   381,   375,    43,   376,   377,   378,
     379,   380,   381,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    44,     0,    45,     0,     0,     0,     0,     0,
      46,     0,     0,     0,    47,     0,     0,     0,     0,     0,
       0,     4,     0,     0,     5,     0,     0,    48,     0,     6,
       0,    49,    50,     0,     0,     7,     0,    51,     0,    52,
       0,     8,     0,     0,     9,     4,    53,     0,     5,     0,
       5,     0,     0,     6,     0,     6,     0,    54,    55,     7,
       0,     5,    10,   234,    56,     8,     6,     8,     9,    11,
       9,     0,     0,     0,     0,     0,     0,     0,     8,     0,
       0,     9,     0,     0,     0,     0,    10,     0,    10,    57,
      58,    59,    60,    11,     0,    61,     0,     0,     0,    10,
      62,     0,     0,     0,     0,     0,     0,    63,     0,    64,
      12,     0,     0,     0,    65,    66,    13,    14,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    12,     0,    12,     0,     0,     0,
      13,    14,    13,    14,     0,   243,    15,    12,     0,     0,
       0,     0,     0,    13,    14,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      15,   422,    15,    87,     0,    88,    89,     0,    90,    91,
       0,     0,   479,    15,    92,     0,    93,     0,     0,     0,
       0,     0,     0,    94,    95,    96,    97,     0,    98,    99,
     100,   101,   102,     0,   103,     0,   104,   105,   106,     0,
       0,   107,     0,     0,     0,     0,   108,     0,   109,   110,
     111,   112,   113,   114,     0,   115,   116,   117,   118,   119,
       0,     0,   120,     0,   121,     0,     0,     0,     0,   122,
       0,   123,     0,     0,     0,   124,   125,   126,   127,   128,
     129,   130,   131,     0,   132,     0,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,     0,     0,     0,
       0,   144,     0,     0,   145,     0,   146,   147,     0,     0,
     148,   149,   150,     0,     0,     0,   151,     0,   152,   153,
     154,   155,     0,     0,   156
};

static const short int yycheck[] =
{
       2,    13,     5,     2,    71,     2,     2,    22,     2,   243,
      11,    12,   254,   231,   399,   219,    43,   221,    11,    12,
     224,   225,     2,   135,   135,    83,   119,   250,   266,   119,
       0,    99,   270,   271,   272,   103,    26,   135,    10,   501,
     151,   503,   209,   155,    47,   212,   213,   135,    38,   216,
      77,   218,    29,   151,    77,   278,   223,    29,   126,   152,
      32,    12,   152,    14,   150,    37,    43,   155,    32,     2,
     469,    43,    95,    11,    12,   135,    79,    49,   149,     8,
      52,    84,   481,   154,   288,   289,   290,   291,   292,    22,
     257,   151,   477,   154,     3,     4,   154,   331,    70,    71,
      77,   135,   149,    80,   135,    77,   149,    84,   342,   327,
     149,    62,    63,   149,   153,   154,   149,   151,    27,   149,
     151,   149,    59,    32,   362,   153,   154,    64,    56,   149,
      58,     3,     4,   200,   201,     3,     4,     3,     4,   152,
     207,   142,   143,     3,     4,    54,   118,   140,   141,   142,
     143,   149,   124,   125,     3,     4,     3,     4,   160,   161,
     175,   160,   161,   153,   160,   161,   160,   161,   152,   373,
     374,   375,   376,   377,   378,   379,   380,   381,   416,   421,
     160,   161,   154,   350,    93,   137,   424,   190,   422,    11,
      12,     3,     4,     3,     4,   399,     3,     4,     3,     4,
     438,   139,   140,   141,   142,   143,   115,     8,     9,   153,
     154,   153,   154,   152,     3,   382,     5,     6,     7,    11,
      12,    75,   149,   226,   227,   459,   149,   230,   149,     8,
     468,   150,   152,   142,   149,   402,   248,   150,   250,   443,
     152,   150,   150,   150,   150,   479,   150,   150,   150,   150,
      11,    12,   264,   150,   150,   150,   255,   150,   150,   255,
     150,   255,   264,   150,    53,   150,   278,   279,   150,   150,
     474,   150,   150,   477,   150,   150,   150,   279,     3,   150,
       8,   485,   349,   152,     8,   152,   156,   152,   492,   292,
     494,     8,   151,   150,     3,   499,     5,     6,     7,     8,
       8,     8,    57,     5,   154,   150,     3,   149,     5,     6,
       7,     8,   137,   149,   136,   152,   138,   139,   140,   141,
     142,   143,   151,   112,   135,   153,   156,   150,     3,   151,
     151,   135,   150,   157,   123,   332,   151,   151,   151,   135,
       8,   344,   341,   151,    53,   341,     4,   341,   151,   151,
     139,   140,   151,   142,   353,   144,    53,   151,   361,   361,
     151,   150,    20,   366,    22,   151,   155,   151,   151,   151,
      28,   151,   137,   440,    32,   136,   151,   138,   139,   140,
     141,   142,   143,   151,    32,   151,   149,    45,    70,    37,
     151,    49,    50,   153,   151,   149,   149,    55,   149,    57,
       8,    49,   151,   112,    52,   151,    64,   151,     5,   151,
     149,   149,   135,   154,   123,   112,   151,    75,    76,   151,
     151,   154,    70,   152,    82,   137,   123,    20,     2,   340,
     139,   140,   467,   142,   437,   144,   224,   206,   338,   349,
       2,   150,   139,   140,    22,   142,     2,   144,     2,   107,
     108,   109,   110,   150,    -1,   113,    -1,    -1,   222,    -1,
     118,    -1,     4,   462,   467,   467,   462,   125,   462,   127,
     118,    -1,    -1,    -1,   132,   133,   124,   125,    20,    -1,
      22,    -1,    -1,    -1,    -1,    -1,    28,    -1,    -1,   501,
      32,   503,    11,    12,    -1,    -1,   154,    -1,    -1,   501,
      -1,   503,    -1,    45,    -1,   153,   154,    49,    50,    -1,
      -1,    -1,    -1,    55,    -1,    57,    -1,    -1,    -1,    -1,
      -1,    -1,    64,    -1,    -1,    -1,    -1,     3,     4,     5,
       6,     7,    -1,    75,    76,    -1,    -1,    -1,    -1,     3,
      82,     5,     6,     7,    20,    -1,    22,    -1,    -1,    -1,
      -1,     3,    28,     5,     6,     7,    32,    -1,    -1,    -1,
      28,    -1,    -1,    -1,    -1,   107,   108,   109,   110,    45,
      -1,   113,    -1,    49,    50,    -1,   118,    53,    -1,    55,
      -1,    57,    -1,   125,    -1,   127,    11,    12,    64,    53,
     132,   133,    -1,    -1,    -1,    -1,    64,    -1,    -1,    75,
      76,    53,    11,    12,    -1,    -1,    82,    75,    76,    -1,
      11,    12,   154,    -1,    82,    -1,    -1,   136,   137,   138,
     139,   140,   141,   142,   143,    -1,    -1,    -1,    -1,    -1,
      -1,   107,   108,   109,   110,    -1,   112,   113,    -1,    -1,
     108,    -1,   118,    -1,    -1,   113,    -1,   123,   112,   125,
      -1,   127,    -1,    -1,    -1,    -1,   132,   133,    -1,   123,
     112,    -1,    -1,   139,   140,    -1,   142,    -1,   144,    -1,
      -1,   123,    -1,    -1,   150,   139,   140,    -1,   142,    -1,
     144,    -1,    -1,    -1,    -1,    -1,   150,   139,   140,    -1,
     142,    -1,   144,    -1,    -1,    -1,    -1,    -1,   150,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   136,    -1,   138,   139,   140,   141,   142,   143,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   151,   136,   137,   138,
     139,   140,   141,   142,   143,   136,     4,   138,   139,   140,
     141,   142,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    20,    -1,    22,    -1,    -1,    -1,    -1,    -1,
      28,    -1,    -1,    -1,    32,    -1,    -1,    -1,    -1,    -1,
      -1,    29,    -1,    -1,    32,    -1,    -1,    45,    -1,    37,
      -1,    49,    50,    -1,    -1,    43,    -1,    55,    -1,    57,
      -1,    49,    -1,    -1,    52,    29,    64,    -1,    32,    -1,
      32,    -1,    -1,    37,    -1,    37,    -1,    75,    76,    43,
      -1,    32,    70,    71,    82,    49,    37,    49,    52,    77,
      52,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,    -1,
      -1,    52,    -1,    -1,    -1,    -1,    70,    -1,    70,   107,
     108,   109,   110,    77,    -1,   113,    -1,    -1,    -1,    70,
     118,    -1,    -1,    -1,    -1,    -1,    -1,   125,    -1,   127,
     118,    -1,    -1,    -1,   132,   133,   124,   125,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   118,    -1,   118,    -1,    -1,    -1,
     124,   125,   124,   125,    -1,   153,   154,   118,    -1,    -1,
      -1,    -1,    -1,   124,   125,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     154,   153,   154,    13,    -1,    15,    16,    -1,    18,    19,
      -1,    -1,   153,   154,    24,    -1,    26,    -1,    -1,    -1,
      -1,    -1,    -1,    33,    34,    35,    36,    -1,    38,    39,
      40,    41,    42,    -1,    44,    -1,    46,    47,    48,    -1,
      -1,    51,    -1,    -1,    -1,    -1,    56,    -1,    58,    59,
      60,    61,    62,    63,    -1,    65,    66,    67,    68,    69,
      -1,    -1,    72,    -1,    74,    -1,    -1,    -1,    -1,    79,
      -1,    81,    -1,    -1,    -1,    85,    86,    87,    88,    89,
      90,    91,    92,    -1,    94,    -1,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,    -1,    -1,    -1,
      -1,   111,    -1,    -1,   114,    -1,   116,   117,    -1,    -1,
     120,   121,   122,    -1,    -1,    -1,   126,    -1,   128,   129,
     130,   131,    -1,    -1,   134
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,   159,   160,     0,    29,    32,    37,    43,    49,    52,
      70,    77,   118,   124,   125,   154,   164,   165,   166,   167,
     170,   171,   179,   187,   191,   197,   208,   209,   210,   213,
     214,   217,   219,   220,   221,   222,   224,   225,   231,   233,
     234,     3,     4,     4,    20,    22,    28,    32,    45,    49,
      50,    55,    57,    64,    75,    76,    82,   107,   108,   109,
     110,   113,   118,   125,   127,   132,   133,   191,   205,   207,
     231,   232,   234,   150,     3,     4,     3,     4,   203,    32,
       8,     3,     4,   203,   178,   179,   203,    13,    15,    16,
      18,    19,    24,    26,    33,    34,    35,    36,    38,    39,
      40,    41,    42,    44,    46,    47,    48,    51,    56,    58,
      59,    60,    61,    62,    63,    65,    66,    67,    68,    69,
      72,    74,    79,    81,    85,    86,    87,    88,    89,    90,
      91,    92,    94,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   111,   114,   116,   117,   120,   121,
     122,   126,   128,   129,   130,   131,   134,   180,   182,   230,
     161,   161,    80,    84,   169,   208,   213,   219,   223,   149,
     149,   149,   149,   152,   149,   152,   149,   163,   149,   137,
     218,   152,   149,   149,   149,   232,     3,    75,   206,   206,
     150,   206,   207,     3,     3,   207,     3,     4,   204,     8,
     152,   232,   149,   152,   232,   119,   152,   150,   150,   150,
     150,   150,   150,   150,   150,   150,   150,   150,   150,   150,
     150,   150,   150,   150,   150,   150,   150,   150,   150,   150,
     150,   135,   155,    10,    71,   164,   168,   171,   208,   210,
     221,   222,   225,   153,     3,     3,     4,   152,   211,    95,
     215,   219,     3,     4,   152,   162,   232,   156,   151,   188,
     189,   190,   204,   204,   198,    27,    32,    54,    93,   115,
     142,   150,   184,   204,   226,   227,   228,   229,   150,   198,
     204,     3,     5,     6,     7,    53,   112,   123,   139,   140,
     142,   144,   150,   194,   195,   196,     8,   196,     8,     8,
     181,     8,   196,   196,     8,     8,   196,     8,   196,   194,
      57,   192,   193,   194,   230,   196,   192,   194,   232,   232,
       8,     9,   183,     5,   235,   232,   182,   154,   150,   149,
     163,   153,   178,   212,   137,    83,   178,   200,   216,   149,
     152,   162,   153,   164,   178,   201,   151,   196,   153,   135,
     156,   149,   153,   178,   179,   199,   200,   226,   226,   226,
     226,   150,   135,   200,   153,   151,   150,   194,   194,   194,
     194,   194,   232,    11,    12,   136,   138,   139,   140,   141,
     142,   143,   135,   151,   151,   151,   151,   135,   151,   151,
     151,   151,   151,   151,   151,   151,   151,   151,     3,   135,
     151,   151,   135,   151,   151,   151,   151,   151,   157,   151,
     151,   182,     8,   163,   222,   137,   232,   149,   153,   201,
     153,   167,   153,   163,   232,   149,   190,   196,   234,   149,
     149,   151,   132,   172,   173,   174,   175,   179,   232,   226,
     151,   232,   151,   151,   194,   194,   194,   194,   194,   194,
     194,   194,   194,   196,     8,   151,   193,   196,     5,   151,
     226,   149,   162,   163,   226,   149,   151,   135,   232,   226,
     202,   204,   151,   194,   137,   151,   163,   154,   176,   153,
     175,   226,   176,   152,   194,   142,   177,   193,   163,   176,
     185,   155,   135,   155,    26,    38,   153,   186,   194,   154,
     194,   137,   194,   137,   199,   199
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (0)


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (N)								\
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (0)
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
              (Loc).first_line, (Loc).first_column,	\
              (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr,					\
                  Type, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short int *bottom, short int *top)
#else
static void
yy_stack_print (bottom, top)
    short int *bottom;
    short int *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname[yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      size_t yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

#endif /* YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);


# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()
    ;
#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short int yyssa[YYINITDEPTH];
  short int *yyss = yyssa;
  short int *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short int *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short int *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a look-ahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to look-ahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 292 "parser.y"
    { fix_incomplete();
						  check_all_user_types((yyvsp[0].ifref_list));
						  write_proxies((yyvsp[0].ifref_list));
						  write_client((yyvsp[0].ifref_list));
						  write_server((yyvsp[0].ifref_list));
						  write_dlldata((yyvsp[0].ifref_list));
						;}
    break;

  case 3:
#line 301 "parser.y"
    { (yyval.ifref_list) = NULL; ;}
    break;

  case 4:
#line 302 "parser.y"
    { (yyval.ifref_list) = (yyvsp[-1].ifref_list); ;}
    break;

  case 5:
#line 303 "parser.y"
    { (yyval.ifref_list) = append_ifref( (yyvsp[-1].ifref_list), make_ifref((yyvsp[0].type)) ); ;}
    break;

  case 6:
#line 304 "parser.y"
    { (yyval.ifref_list) = (yyvsp[-2].ifref_list);
						  reg_type((yyvsp[-1].type), (yyvsp[-1].type)->name, 0);
						  if (!parse_only && do_header) write_coclass_forward((yyvsp[-1].type));
						;}
    break;

  case 7:
#line 308 "parser.y"
    { (yyval.ifref_list) = (yyvsp[-1].ifref_list);
						  add_typelib_entry((yyvsp[0].type));
						  reg_type((yyvsp[0].type), (yyvsp[0].type)->name, 0);
						  if (!parse_only && do_header) write_coclass_forward((yyvsp[0].type));
						;}
    break;

  case 8:
#line 313 "parser.y"
    { (yyval.ifref_list) = (yyvsp[-1].ifref_list); add_typelib_entry((yyvsp[0].type)); ;}
    break;

  case 9:
#line 314 "parser.y"
    { (yyval.ifref_list) = (yyvsp[-1].ifref_list); ;}
    break;

  case 10:
#line 315 "parser.y"
    { (yyval.ifref_list) = (yyvsp[-1].ifref_list); ;}
    break;

  case 11:
#line 318 "parser.y"
    {;}
    break;

  case 12:
#line 319 "parser.y"
    { if (!parse_only) add_typelib_entry((yyvsp[0].type)); ;}
    break;

  case 13:
#line 320 "parser.y"
    { if (!parse_only) add_typelib_entry((yyvsp[0].type)); ;}
    break;

  case 14:
#line 321 "parser.y"
    { reg_type((yyvsp[-1].type), (yyvsp[-1].type)->name, 0); if (!parse_only && do_header) write_coclass_forward((yyvsp[-1].type)); ;}
    break;

  case 15:
#line 322 "parser.y"
    { if (!parse_only) add_typelib_entry((yyvsp[0].type));
						  reg_type((yyvsp[0].type), (yyvsp[0].type)->name, 0);
						  if (!parse_only && do_header) write_coclass_forward((yyvsp[0].type));
						;}
    break;

  case 16:
#line 326 "parser.y"
    { if (!parse_only) add_typelib_entry((yyvsp[0].type)); ;}
    break;

  case 17:
#line 327 "parser.y"
    {;}
    break;

  case 18:
#line 328 "parser.y"
    {;}
    break;

  case 19:
#line 329 "parser.y"
    {;}
    break;

  case 20:
#line 332 "parser.y"
    { (yyval.func_list) = NULL; ;}
    break;

  case 21:
#line 333 "parser.y"
    { (yyval.func_list) = append_func( (yyvsp[-2].func_list), (yyvsp[-1].func) ); ;}
    break;

  case 22:
#line 334 "parser.y"
    { (yyval.func_list) = (yyvsp[-1].func_list); ;}
    break;

  case 25:
#line 341 "parser.y"
    { if (!parse_only && do_header) { write_constdef((yyvsp[-1].var)); } ;}
    break;

  case 26:
#line 342 "parser.y"
    {;}
    break;

  case 27:
#line 343 "parser.y"
    { if (!parse_only && do_header) {
						    write_type_def_or_decl(header, (yyvsp[-1].type), FALSE, NULL);
						    fprintf(header, ";\n\n");
						  }
						;}
    break;

  case 28:
#line 348 "parser.y"
    { if (!parse_only && do_header) { write_externdef((yyvsp[-1].var)); } ;}
    break;

  case 29:
#line 349 "parser.y"
    {;}
    break;

  case 30:
#line 350 "parser.y"
    { if (!parse_only && do_header) {
						    write_type_def_or_decl(header, (yyvsp[-1].type), FALSE, NULL);
						    fprintf(header, ";\n\n");
						  }
						;}
    break;

  case 31:
#line 355 "parser.y"
    {;}
    break;

  case 32:
#line 356 "parser.y"
    { if (!parse_only && do_header) {
						    write_type_def_or_decl(header, (yyvsp[-1].type), FALSE, NULL);
						    fprintf(header, ";\n\n");
						  }
						;}
    break;

  case 33:
#line 363 "parser.y"
    { if (!parse_only && do_header) fprintf(header, "%s\n", (yyvsp[-1].str)); ;}
    break;

  case 34:
#line 365 "parser.y"
    { assert(yychar == YYEMPTY);
						  (yyval.num) = do_import((yyvsp[-1].str));
						  if (!(yyval.num)) yychar = aEOF;
						;}
    break;

  case 35:
#line 372 "parser.y"
    { if ((yyvsp[-2].num)) pop_import(); ;}
    break;

  case 36:
#line 376 "parser.y"
    { if(!parse_only) add_importlib((yyvsp[-2].str)); ;}
    break;

  case 37:
#line 379 "parser.y"
    { (yyval.str) = (yyvsp[0].str); ;}
    break;

  case 38:
#line 381 "parser.y"
    { if (!parse_only) start_typelib((yyvsp[-1].str), (yyvsp[-2].attr_list));
						  if (!parse_only && do_header) write_library((yyvsp[-1].str), (yyvsp[-2].attr_list));
						  if (!parse_only && do_idfile) write_libid((yyvsp[-1].str), (yyvsp[-2].attr_list));
						;}
    break;

  case 39:
#line 387 "parser.y"
    { if (!parse_only) end_typelib(); ;}
    break;

  case 40:
#line 390 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 42:
#line 394 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 43:
#line 397 "parser.y"
    { check_arg((yyvsp[0].var)); (yyval.var_list) = append_var( NULL, (yyvsp[0].var) ); ;}
    break;

  case 44:
#line 398 "parser.y"
    { check_arg((yyvsp[0].var)); (yyval.var_list) = append_var( (yyvsp[-2].var_list), (yyvsp[0].var)); ;}
    break;

  case 46:
#line 403 "parser.y"
    { (yyval.var) = (yyvsp[-1].pident)->var;
						  (yyval.var)->attrs = (yyvsp[-3].attr_list);
						  set_type((yyval.var), (yyvsp[-2].type), (yyvsp[-1].pident), (yyvsp[0].array_dims), TRUE);
						  free((yyvsp[-1].pident));
						;}
    break;

  case 47:
#line 408 "parser.y"
    { (yyval.var) = (yyvsp[-1].pident)->var;
						  set_type((yyval.var), (yyvsp[-2].type), (yyvsp[-1].pident), (yyvsp[0].array_dims), TRUE);
						  free((yyvsp[-1].pident));
						;}
    break;

  case 48:
#line 414 "parser.y"
    { (yyval.array_dims) = NULL; ;}
    break;

  case 49:
#line 415 "parser.y"
    { (yyval.array_dims) = (yyvsp[-1].array_dims); ;}
    break;

  case 50:
#line 416 "parser.y"
    { (yyval.array_dims) = append_array( NULL, make_expr(EXPR_VOID) ); ;}
    break;

  case 51:
#line 419 "parser.y"
    { (yyval.array_dims) = append_array( NULL, (yyvsp[0].expr) ); ;}
    break;

  case 52:
#line 420 "parser.y"
    { (yyval.array_dims) = append_array( (yyvsp[-2].array_dims), (yyvsp[0].expr) ); ;}
    break;

  case 53:
#line 421 "parser.y"
    { (yyval.array_dims) = append_array( (yyvsp[-3].array_dims), (yyvsp[0].expr) ); ;}
    break;

  case 54:
#line 424 "parser.y"
    { (yyval.attr_list) = NULL; ;}
    break;

  case 56:
#line 429 "parser.y"
    { (yyval.attr_list) = (yyvsp[-1].attr_list);
						  if (!(yyval.attr_list))
						    error_loc("empty attribute lists unsupported\n");
						;}
    break;

  case 57:
#line 435 "parser.y"
    { (yyval.attr_list) = append_attr( NULL, (yyvsp[0].attr) ); ;}
    break;

  case 58:
#line 436 "parser.y"
    { (yyval.attr_list) = append_attr( (yyvsp[-2].attr_list), (yyvsp[0].attr) ); ;}
    break;

  case 59:
#line 437 "parser.y"
    { (yyval.attr_list) = append_attr( (yyvsp[-3].attr_list), (yyvsp[0].attr) ); ;}
    break;

  case 60:
#line 440 "parser.y"
    { (yyval.str_list) = append_str( NULL, (yyvsp[0].str) ); ;}
    break;

  case 61:
#line 441 "parser.y"
    { (yyval.str_list) = append_str( (yyvsp[-2].str_list), (yyvsp[0].str) ); ;}
    break;

  case 62:
#line 444 "parser.y"
    { (yyval.attr) = NULL; ;}
    break;

  case 63:
#line 445 "parser.y"
    { (yyval.attr) = make_attr(ATTR_AGGREGATABLE); ;}
    break;

  case 64:
#line 446 "parser.y"
    { (yyval.attr) = make_attr(ATTR_APPOBJECT); ;}
    break;

  case 65:
#line 447 "parser.y"
    { (yyval.attr) = make_attr(ATTR_ASYNC); ;}
    break;

  case 66:
#line 448 "parser.y"
    { (yyval.attr) = make_attr(ATTR_AUTO_HANDLE); ;}
    break;

  case 67:
#line 449 "parser.y"
    { (yyval.attr) = make_attr(ATTR_BINDABLE); ;}
    break;

  case 68:
#line 450 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_CALLAS, (yyvsp[-1].var)); ;}
    break;

  case 69:
#line 451 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_CASE, (yyvsp[-1].expr_list)); ;}
    break;

  case 70:
#line 452 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); ;}
    break;

  case 71:
#line 453 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_DONT_SERIALIZE */ ;}
    break;

  case 72:
#line 454 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_SERIALIZE */ ;}
    break;

  case 73:
#line 455 "parser.y"
    { (yyval.attr) = make_attr(ATTR_CONTROL); ;}
    break;

  case 74:
#line 456 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULT); ;}
    break;

  case 75:
#line 457 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULTCOLLELEM); ;}
    break;

  case 76:
#line 458 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_DEFAULTVALUE_EXPR, (yyvsp[-1].expr)); ;}
    break;

  case 77:
#line 459 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_DEFAULTVALUE_STRING, (yyvsp[-1].str)); ;}
    break;

  case 78:
#line 460 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULTVTABLE); ;}
    break;

  case 79:
#line 461 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DISPLAYBIND); ;}
    break;

  case 80:
#line 462 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_DLLNAME, (yyvsp[-1].str)); ;}
    break;

  case 81:
#line 463 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DUAL); ;}
    break;

  case 82:
#line 464 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ENDPOINT, (yyvsp[-1].str_list)); ;}
    break;

  case 83:
#line 465 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ENTRY_STRING, (yyvsp[-1].str)); ;}
    break;

  case 84:
#line 466 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ENTRY_ORDINAL, (yyvsp[-1].expr)); ;}
    break;

  case 85:
#line 467 "parser.y"
    { (yyval.attr) = make_attr(ATTR_EXPLICIT_HANDLE); ;}
    break;

  case 86:
#line 468 "parser.y"
    { (yyval.attr) = make_attr(ATTR_HANDLE); ;}
    break;

  case 87:
#line 469 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPCONTEXT, (yyvsp[-1].expr)); ;}
    break;

  case 88:
#line 470 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPFILE, (yyvsp[-1].str)); ;}
    break;

  case 89:
#line 471 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRING, (yyvsp[-1].str)); ;}
    break;

  case 90:
#line 472 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRINGCONTEXT, (yyvsp[-1].expr)); ;}
    break;

  case 91:
#line 473 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRINGDLL, (yyvsp[-1].str)); ;}
    break;

  case 92:
#line 474 "parser.y"
    { (yyval.attr) = make_attr(ATTR_HIDDEN); ;}
    break;

  case 93:
#line 475 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ID, (yyvsp[-1].expr)); ;}
    break;

  case 94:
#line 476 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IDEMPOTENT); ;}
    break;

  case 95:
#line 477 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_IIDIS, (yyvsp[-1].expr)); ;}
    break;

  case 96:
#line 478 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IMMEDIATEBIND); ;}
    break;

  case 97:
#line 479 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_IMPLICIT_HANDLE, (yyvsp[-1].str)); ;}
    break;

  case 98:
#line 480 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IN); ;}
    break;

  case 99:
#line 481 "parser.y"
    { (yyval.attr) = make_attr(ATTR_INPUTSYNC); ;}
    break;

  case 100:
#line 482 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_LENGTHIS, (yyvsp[-1].expr_list)); ;}
    break;

  case 101:
#line 483 "parser.y"
    { (yyval.attr) = make_attr(ATTR_LOCAL); ;}
    break;

  case 102:
#line 484 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONBROWSABLE); ;}
    break;

  case 103:
#line 485 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONCREATABLE); ;}
    break;

  case 104:
#line 486 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONEXTENSIBLE); ;}
    break;

  case 105:
#line 487 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OBJECT); ;}
    break;

  case 106:
#line 488 "parser.y"
    { (yyval.attr) = make_attr(ATTR_ODL); ;}
    break;

  case 107:
#line 489 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OLEAUTOMATION); ;}
    break;

  case 108:
#line 490 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OPTIONAL); ;}
    break;

  case 109:
#line 491 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OUT); ;}
    break;

  case 110:
#line 492 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_POINTERDEFAULT, (yyvsp[-1].num)); ;}
    break;

  case 111:
#line 493 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPGET); ;}
    break;

  case 112:
#line 494 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPPUT); ;}
    break;

  case 113:
#line 495 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPPUTREF); ;}
    break;

  case 114:
#line 496 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PUBLIC); ;}
    break;

  case 115:
#line 497 "parser.y"
    { expr_list_t *list = append_expr( NULL, (yyvsp[-3].expr) );
                                                     list = append_expr( list, (yyvsp[-1].expr) );
                                                     (yyval.attr) = make_attrp(ATTR_RANGE, list); ;}
    break;

  case 116:
#line 500 "parser.y"
    { (yyval.attr) = make_attr(ATTR_READONLY); ;}
    break;

  case 117:
#line 501 "parser.y"
    { (yyval.attr) = make_attr(ATTR_REQUESTEDIT); ;}
    break;

  case 118:
#line 502 "parser.y"
    { (yyval.attr) = make_attr(ATTR_RESTRICTED); ;}
    break;

  case 119:
#line 503 "parser.y"
    { (yyval.attr) = make_attr(ATTR_RETVAL); ;}
    break;

  case 120:
#line 504 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SIZEIS, (yyvsp[-1].expr_list)); ;}
    break;

  case 121:
#line 505 "parser.y"
    { (yyval.attr) = make_attr(ATTR_SOURCE); ;}
    break;

  case 122:
#line 506 "parser.y"
    { (yyval.attr) = make_attr(ATTR_STRICTCONTEXTHANDLE); ;}
    break;

  case 123:
#line 507 "parser.y"
    { (yyval.attr) = make_attr(ATTR_STRING); ;}
    break;

  case 124:
#line 508 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SWITCHIS, (yyvsp[-1].expr)); ;}
    break;

  case 125:
#line 509 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SWITCHTYPE, (yyvsp[-1].type)); ;}
    break;

  case 126:
#line 510 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_TRANSMITAS, (yyvsp[-1].type)); ;}
    break;

  case 127:
#line 511 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_UUID, (yyvsp[-1].uuid)); ;}
    break;

  case 128:
#line 512 "parser.y"
    { (yyval.attr) = make_attr(ATTR_V1ENUM); ;}
    break;

  case 129:
#line 513 "parser.y"
    { (yyval.attr) = make_attr(ATTR_VARARG); ;}
    break;

  case 130:
#line 514 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_VERSION, (yyvsp[-1].num)); ;}
    break;

  case 131:
#line 515 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_WIREMARSHAL, (yyvsp[-1].type)); ;}
    break;

  case 132:
#line 516 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_POINTERTYPE, (yyvsp[0].num)); ;}
    break;

  case 134:
#line 521 "parser.y"
    { if (!is_valid_uuid((yyvsp[0].str)))
						    error_loc("invalid UUID: %s\n", (yyvsp[0].str));
						  (yyval.uuid) = parse_uuid((yyvsp[0].str)); ;}
    break;

  case 135:
#line 526 "parser.y"
    { (yyval.str) = (yyvsp[0].str); ;}
    break;

  case 136:
#line 527 "parser.y"
    { (yyval.str) = (yyvsp[0].str); ;}
    break;

  case 137:
#line 528 "parser.y"
    { (yyval.str) = (yyvsp[0].str); ;}
    break;

  case 138:
#line 529 "parser.y"
    { (yyval.str) = (yyvsp[0].str); ;}
    break;

  case 139:
#line 532 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 140:
#line 533 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[-1].var_list), (yyvsp[0].var) ); ;}
    break;

  case 141:
#line 536 "parser.y"
    { attr_t *a = make_attrp(ATTR_CASE, append_expr( NULL, (yyvsp[-2].expr) ));
						  (yyval.var) = (yyvsp[0].var); if (!(yyval.var)) (yyval.var) = make_var(NULL);
						  (yyval.var)->attrs = append_attr( (yyval.var)->attrs, a );
						;}
    break;

  case 142:
#line 540 "parser.y"
    { attr_t *a = make_attr(ATTR_DEFAULT);
						  (yyval.var) = (yyvsp[0].var); if (!(yyval.var)) (yyval.var) = make_var(NULL);
						  (yyval.var)->attrs = append_attr( (yyval.var)->attrs, a );
						;}
    break;

  case 143:
#line 546 "parser.y"
    { (yyval.var) = reg_const((yyvsp[-2].var));
						  set_type((yyval.var), (yyvsp[-3].type), NULL, NULL, FALSE);
						  (yyval.var)->eval = (yyvsp[0].expr);
						;}
    break;

  case 144:
#line 552 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 145:
#line 553 "parser.y"
    { (yyval.var_list) = (yyvsp[-1].var_list); ;}
    break;

  case 147:
#line 557 "parser.y"
    { if (!(yyvsp[0].var)->eval)
						    (yyvsp[0].var)->eval = make_exprl(EXPR_NUM, 0 /* default for first enum entry */);
                                                  (yyval.var_list) = append_var( NULL, (yyvsp[0].var) );
						;}
    break;

  case 148:
#line 561 "parser.y"
    { if (!(yyvsp[0].var)->eval)
                                                  {
                                                    var_t *last = LIST_ENTRY( list_tail((yyval.var_list)), var_t, entry );
                                                    (yyvsp[0].var)->eval = make_exprl(EXPR_NUM, last->eval->cval + 1);
                                                  }
                                                  (yyval.var_list) = append_var( (yyvsp[-2].var_list), (yyvsp[0].var) );
						;}
    break;

  case 149:
#line 570 "parser.y"
    { (yyval.var) = reg_const((yyvsp[-2].var));
						  (yyval.var)->eval = (yyvsp[0].expr);
                                                  (yyval.var)->type = make_int(0);
						;}
    break;

  case 150:
#line 574 "parser.y"
    { (yyval.var) = reg_const((yyvsp[0].var));
                                                  (yyval.var)->type = make_int(0);
						;}
    break;

  case 151:
#line 579 "parser.y"
    { (yyval.type) = get_typev(RPC_FC_ENUM16, (yyvsp[-3].var), tsENUM);
						  (yyval.type)->kind = TKIND_ENUM;
						  (yyval.type)->fields_or_args = (yyvsp[-1].var_list);
						  (yyval.type)->defined = TRUE;
                                                  if(in_typelib)
                                                      add_typelib_entry((yyval.type));
						;}
    break;

  case 152:
#line 588 "parser.y"
    { (yyval.expr_list) = append_expr( NULL, (yyvsp[0].expr) ); ;}
    break;

  case 153:
#line 589 "parser.y"
    { (yyval.expr_list) = append_expr( (yyvsp[-2].expr_list), (yyvsp[0].expr) ); ;}
    break;

  case 154:
#line 602 "parser.y"
    { (yyval.expr) = make_expr(EXPR_VOID); ;}
    break;

  case 156:
#line 606 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_NUM, (yyvsp[0].num)); ;}
    break;

  case 157:
#line 607 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_HEXNUM, (yyvsp[0].num)); ;}
    break;

  case 158:
#line 608 "parser.y"
    { (yyval.expr) = make_exprd(EXPR_DOUBLE, (yyvsp[0].dbl)); ;}
    break;

  case 159:
#line 609 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_TRUEFALSE, 0); ;}
    break;

  case 160:
#line 610 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_TRUEFALSE, 1); ;}
    break;

  case 161:
#line 611 "parser.y"
    { (yyval.expr) = make_exprs(EXPR_IDENTIFIER, (yyvsp[0].str)); ;}
    break;

  case 162:
#line 612 "parser.y"
    { (yyval.expr) = make_expr3(EXPR_COND, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 163:
#line 613 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_OR , (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 164:
#line 614 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_AND, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 165:
#line 615 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_ADD, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 166:
#line 616 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SUB, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 167:
#line 617 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MUL, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 168:
#line 618 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_DIV, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 169:
#line 619 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SHL, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 170:
#line 620 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SHR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 171:
#line 621 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_NOT, (yyvsp[0].expr)); ;}
    break;

  case 172:
#line 622 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_NEG, (yyvsp[0].expr)); ;}
    break;

  case 173:
#line 623 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_ADDRESSOF, (yyvsp[0].expr)); ;}
    break;

  case 174:
#line 624 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_PPTR, (yyvsp[0].expr)); ;}
    break;

  case 175:
#line 625 "parser.y"
    { (yyval.expr) = make_exprt(EXPR_CAST, (yyvsp[-2].type), (yyvsp[0].expr)); ;}
    break;

  case 176:
#line 626 "parser.y"
    { (yyval.expr) = make_exprt(EXPR_SIZEOF, (yyvsp[-1].type), NULL); ;}
    break;

  case 177:
#line 627 "parser.y"
    { (yyval.expr) = (yyvsp[-1].expr); ;}
    break;

  case 178:
#line 630 "parser.y"
    { (yyval.expr_list) = append_expr( NULL, (yyvsp[0].expr) ); ;}
    break;

  case 179:
#line 631 "parser.y"
    { (yyval.expr_list) = append_expr( (yyvsp[-2].expr_list), (yyvsp[0].expr) ); ;}
    break;

  case 180:
#line 634 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr);
						  if (!(yyval.expr)->is_const)
						      error_loc("expression is not constant\n");
						;}
    break;

  case 181:
#line 640 "parser.y"
    { (yyval.var) = (yyvsp[0].var);
						  set_type((yyval.var), (yyvsp[-1].type), NULL, NULL, FALSE);
						;}
    break;

  case 182:
#line 645 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 183:
#line 646 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[-1].var_list), (yyvsp[0].var) ); ;}
    break;

  case 184:
#line 649 "parser.y"
    { (yyval.var) = (yyvsp[-1].var); ;}
    break;

  case 185:
#line 650 "parser.y"
    { (yyval.var) = make_var(NULL); (yyval.var)->type = (yyvsp[-1].type); (yyval.var)->attrs = (yyvsp[-2].attr_list); ;}
    break;

  case 186:
#line 651 "parser.y"
    { (yyval.var) = make_var(NULL); (yyval.var)->attrs = (yyvsp[-1].attr_list); ;}
    break;

  case 187:
#line 652 "parser.y"
    { (yyval.var) = NULL; ;}
    break;

  case 188:
#line 655 "parser.y"
    { (yyval.var) = (yyvsp[-1].pident)->var;
						  (yyval.var)->attrs = (yyvsp[-3].attr_list);
						  set_type((yyval.var), (yyvsp[-2].type), (yyvsp[-1].pident), (yyvsp[0].array_dims), FALSE);
						  free((yyvsp[-1].pident));
						;}
    break;

  case 189:
#line 663 "parser.y"
    { var_t *v = (yyvsp[0].pident)->var;
						  var_list_t *args = (yyvsp[0].pident)->args;
						  v->attrs = (yyvsp[-2].attr_list);
						  set_type(v, (yyvsp[-1].type), (yyvsp[0].pident), NULL, FALSE);
						  free((yyvsp[0].pident));
						  (yyval.func) = make_func(v, args);
						  if (is_attr(v->attrs, ATTR_IN)) {
						    error_loc("inapplicable attribute [in] for function '%s'\n",(yyval.func)->def->name);
						  }
						;}
    break;

  case 190:
#line 675 "parser.y"
    { (yyval.var) = NULL; ;}
    break;

  case 192:
#line 679 "parser.y"
    { (yyval.var) = NULL; ;}
    break;

  case 193:
#line 680 "parser.y"
    { (yyval.var) = make_var((yyvsp[0].str)); ;}
    break;

  case 194:
#line 681 "parser.y"
    { (yyval.var) = make_var((yyvsp[0].str)); ;}
    break;

  case 195:
#line 684 "parser.y"
    { (yyval.var) = make_var((yyvsp[0].str)); ;}
    break;

  case 196:
#line 686 "parser.y"
    { (yyval.var) = make_var((yyvsp[0].str)); ;}
    break;

  case 197:
#line 689 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 198:
#line 690 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 200:
#line 692 "parser.y"
    { (yyval.type) = (yyvsp[0].type); (yyval.type)->sign = 1; ;}
    break;

  case 201:
#line 693 "parser.y"
    { (yyval.type) = (yyvsp[0].type); (yyval.type)->sign = -1;
						  switch ((yyval.type)->type) {
						  case RPC_FC_CHAR:  break;
						  case RPC_FC_SMALL: (yyval.type)->type = RPC_FC_USMALL; break;
						  case RPC_FC_SHORT: (yyval.type)->type = RPC_FC_USHORT; break;
						  case RPC_FC_LONG:  (yyval.type)->type = RPC_FC_ULONG;  break;
						  case RPC_FC_HYPER:
						    if ((yyval.type)->name[0] == 'h') /* hyper, as opposed to __int64 */
                                                    {
                                                      (yyval.type) = alias((yyval.type), "MIDL_uhyper");
                                                      (yyval.type)->sign = 0;
                                                    }
						    break;
						  default: break;
						  }
						;}
    break;

  case 202:
#line 709 "parser.y"
    { (yyval.type) = make_int(-1); ;}
    break;

  case 203:
#line 710 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 204:
#line 711 "parser.y"
    { (yyval.type) = duptype(find_type("float", 0), 1); ;}
    break;

  case 205:
#line 712 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 206:
#line 713 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 207:
#line 714 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 208:
#line 715 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 211:
#line 722 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 212:
#line 723 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[-1].str)); ;}
    break;

  case 213:
#line 724 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 214:
#line 725 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[-1].str)); ;}
    break;

  case 215:
#line 726 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[-1].str)); ;}
    break;

  case 216:
#line 727 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 217:
#line 728 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 218:
#line 731 "parser.y"
    { (yyval.type) = make_class((yyvsp[0].str)); ;}
    break;

  case 219:
#line 732 "parser.y"
    { (yyval.type) = find_type((yyvsp[0].str), 0);
						  if ((yyval.type)->defined) error_loc("multiple definition error\n");
						  if ((yyval.type)->kind != TKIND_COCLASS) error_loc("%s was not declared a coclass\n", (yyvsp[0].str));
						;}
    break;

  case 220:
#line 738 "parser.y"
    { (yyval.type) = (yyvsp[0].type);
						  (yyval.type)->attrs = (yyvsp[-1].attr_list);
						  if (!parse_only && do_header)
						    write_coclass((yyval.type));
						  if (!parse_only && do_idfile)
						    write_clsid((yyval.type));
						;}
    break;

  case 221:
#line 748 "parser.y"
    { (yyval.type) = (yyvsp[-4].type);
						  (yyval.type)->ifaces = (yyvsp[-2].ifref_list);
						  (yyval.type)->defined = TRUE;
						;}
    break;

  case 222:
#line 754 "parser.y"
    { (yyval.ifref_list) = NULL; ;}
    break;

  case 223:
#line 755 "parser.y"
    { (yyval.ifref_list) = append_ifref( (yyvsp[-1].ifref_list), (yyvsp[0].ifref) ); ;}
    break;

  case 224:
#line 759 "parser.y"
    { (yyval.ifref) = make_ifref((yyvsp[0].type)); (yyval.ifref)->attrs = (yyvsp[-1].attr_list); ;}
    break;

  case 225:
#line 762 "parser.y"
    { (yyval.type) = get_type(0, (yyvsp[0].str), 0); (yyval.type)->kind = TKIND_DISPATCH; ;}
    break;

  case 226:
#line 763 "parser.y"
    { (yyval.type) = get_type(0, (yyvsp[0].str), 0); (yyval.type)->kind = TKIND_DISPATCH; ;}
    break;

  case 227:
#line 766 "parser.y"
    { attr_t *attrs;
						  is_object_interface = TRUE;
						  (yyval.type) = (yyvsp[0].type);
						  if ((yyval.type)->defined) error_loc("multiple definition error\n");
						  attrs = make_attr(ATTR_DISPINTERFACE);
						  (yyval.type)->attrs = append_attr( (yyvsp[-1].attr_list), attrs );
						  (yyval.type)->ref = find_type("IDispatch", 0);
						  if (!(yyval.type)->ref) error_loc("IDispatch is undefined\n");
						  (yyval.type)->defined = TRUE;
						  if (!parse_only && do_header) write_forward((yyval.type));
						;}
    break;

  case 228:
#line 779 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 229:
#line 780 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[-2].var_list), (yyvsp[-1].var) ); ;}
    break;

  case 230:
#line 783 "parser.y"
    { (yyval.func_list) = NULL; ;}
    break;

  case 231:
#line 784 "parser.y"
    { (yyval.func_list) = append_func( (yyvsp[-2].func_list), (yyvsp[-1].func) ); ;}
    break;

  case 232:
#line 790 "parser.y"
    { (yyval.type) = (yyvsp[-4].type);
						  (yyval.type)->fields_or_args = (yyvsp[-2].var_list);
						  (yyval.type)->funcs = (yyvsp[-1].func_list);
						  if (!parse_only && do_header) write_dispinterface((yyval.type));
						  if (!parse_only && do_idfile) write_diid((yyval.type));
						;}
    break;

  case 233:
#line 797 "parser.y"
    { (yyval.type) = (yyvsp[-4].type);
						  (yyval.type)->fields_or_args = (yyvsp[-2].type)->fields_or_args;
						  (yyval.type)->funcs = (yyvsp[-2].type)->funcs;
						  if (!parse_only && do_header) write_dispinterface((yyval.type));
						  if (!parse_only && do_idfile) write_diid((yyval.type));
						;}
    break;

  case 234:
#line 805 "parser.y"
    { (yyval.type) = NULL; ;}
    break;

  case 235:
#line 806 "parser.y"
    { (yyval.type) = find_type2((yyvsp[0].str), 0); ;}
    break;

  case 236:
#line 809 "parser.y"
    { (yyval.type) = get_type(RPC_FC_IP, (yyvsp[0].str), 0); (yyval.type)->kind = TKIND_INTERFACE; ;}
    break;

  case 237:
#line 810 "parser.y"
    { (yyval.type) = get_type(RPC_FC_IP, (yyvsp[0].str), 0); (yyval.type)->kind = TKIND_INTERFACE; ;}
    break;

  case 238:
#line 813 "parser.y"
    { (yyval.ifinfo).interface = (yyvsp[0].type);
						  (yyval.ifinfo).old_pointer_default = pointer_default;
						  if (is_attr((yyvsp[-1].attr_list), ATTR_POINTERDEFAULT))
						    pointer_default = get_attrv((yyvsp[-1].attr_list), ATTR_POINTERDEFAULT);
						  is_object_interface = is_object((yyvsp[-1].attr_list));
						  if ((yyvsp[0].type)->defined) error_loc("multiple definition error\n");
						  (yyvsp[0].type)->attrs = (yyvsp[-1].attr_list);
						  (yyvsp[0].type)->defined = TRUE;
						  if (!parse_only && do_header) write_forward((yyvsp[0].type));
						;}
    break;

  case 239:
#line 826 "parser.y"
    { (yyval.type) = (yyvsp[-5].ifinfo).interface;
						  (yyval.type)->ref = (yyvsp[-4].type);
						  (yyval.type)->funcs = (yyvsp[-2].func_list);
						  compute_method_indexes((yyval.type));
						  if (!parse_only && do_header) write_interface((yyval.type));
						  if (!parse_only && local_stubs) write_locals(local_stubs, (yyval.type), TRUE);
						  if (!parse_only && do_idfile) write_iid((yyval.type));
						  pointer_default = (yyvsp[-5].ifinfo).old_pointer_default;
						;}
    break;

  case 240:
#line 839 "parser.y"
    { (yyval.type) = (yyvsp[-7].ifinfo).interface;
						  (yyval.type)->ref = find_type2((yyvsp[-5].str), 0);
						  if (!(yyval.type)->ref) error_loc("base class '%s' not found in import\n", (yyvsp[-5].str));
						  (yyval.type)->funcs = (yyvsp[-2].func_list);
						  compute_method_indexes((yyval.type));
						  if (!parse_only && do_header) write_interface((yyval.type));
						  if (!parse_only && local_stubs) write_locals(local_stubs, (yyval.type), TRUE);
						  if (!parse_only && do_idfile) write_iid((yyval.type));
						  pointer_default = (yyvsp[-7].ifinfo).old_pointer_default;
						;}
    break;

  case 241:
#line 849 "parser.y"
    { (yyval.type) = (yyvsp[-1].type); ;}
    break;

  case 242:
#line 853 "parser.y"
    { (yyval.type) = (yyvsp[-1].type); if (!parse_only && do_header) write_forward((yyval.type)); ;}
    break;

  case 243:
#line 854 "parser.y"
    { (yyval.type) = (yyvsp[-1].type); if (!parse_only && do_header) write_forward((yyval.type)); ;}
    break;

  case 244:
#line 857 "parser.y"
    { (yyval.type) = make_type(0, NULL); (yyval.type)->name = (yyvsp[0].str); (yyval.type)->kind = TKIND_MODULE; ;}
    break;

  case 245:
#line 858 "parser.y"
    { (yyval.type) = make_type(0, NULL); (yyval.type)->name = (yyvsp[0].str); (yyval.type)->kind = TKIND_MODULE; ;}
    break;

  case 246:
#line 861 "parser.y"
    { (yyval.type) = (yyvsp[0].type);
						  (yyval.type)->attrs = (yyvsp[-1].attr_list);
						;}
    break;

  case 247:
#line 867 "parser.y"
    { (yyval.type) = (yyvsp[-4].type);
						  (yyval.type)->funcs = (yyvsp[-2].func_list);
						  /* FIXME: if (!parse_only && do_header) write_module($$); */
						;}
    break;

  case 248:
#line 873 "parser.y"
    { (yyval.pident) = (yyvsp[0].pident); (yyval.pident)->ptr_level++; ;}
    break;

  case 249:
#line 874 "parser.y"
    { (yyval.pident) = (yyvsp[0].pident); /* FIXME */ ;}
    break;

  case 250:
#line 875 "parser.y"
    { (yyval.pident) = (yyvsp[0].pident);
						  if ((yyval.pident)->callconv) parser_warning("multiple calling conventions %s, %s for function %s\n", (yyval.pident)->callconv, (yyvsp[-1].str), (yyval.pident)->var->name);
						  (yyval.pident)->callconv = (yyvsp[-1].str);
						;}
    break;

  case 252:
#line 883 "parser.y"
    { (yyval.pident) = (yyvsp[-3].pident);
						  (yyvsp[-3].pident)->args = (yyvsp[-1].var_list);
						  (yyvsp[-3].pident)->is_func = TRUE;
						;}
    break;

  case 253:
#line 889 "parser.y"
    { (yyval.pident) = make_pident((yyvsp[0].var)); ;}
    break;

  case 254:
#line 890 "parser.y"
    { (yyval.pident) = (yyvsp[-1].pident); ;}
    break;

  case 255:
#line 891 "parser.y"
    { (yyval.pident) = (yyvsp[0].pident);
						  (yyval.pident)->func_ptr_level = (yyval.pident)->ptr_level;
						  (yyval.pident)->ptr_level = 0;
						;}
    break;

  case 256:
#line 898 "parser.y"
    { (yyval.pident_list) = append_pident( NULL, (yyvsp[0].pident) ); ;}
    break;

  case 257:
#line 899 "parser.y"
    { (yyval.pident_list) = append_pident( (yyvsp[-2].pident_list), (yyvsp[0].pident) ); ;}
    break;

  case 258:
#line 903 "parser.y"
    { (yyval.num) = RPC_FC_RP; ;}
    break;

  case 259:
#line 904 "parser.y"
    { (yyval.num) = RPC_FC_UP; ;}
    break;

  case 260:
#line 905 "parser.y"
    { (yyval.num) = RPC_FC_FP; ;}
    break;

  case 261:
#line 908 "parser.y"
    { (yyval.type) = get_typev(RPC_FC_STRUCT, (yyvsp[-3].var), tsSTRUCT);
                                                  /* overwrite RPC_FC_STRUCT with a more exact type */
						  (yyval.type)->type = get_struct_type( (yyvsp[-1].var_list) );
						  (yyval.type)->kind = TKIND_RECORD;
						  (yyval.type)->fields_or_args = (yyvsp[-1].var_list);
						  (yyval.type)->defined = TRUE;
                                                  if(in_typelib)
                                                      add_typelib_entry((yyval.type));
                                                ;}
    break;

  case 262:
#line 919 "parser.y"
    { (yyval.type) = duptype(find_type("void", 0), 1); ;}
    break;

  case 263:
#line 920 "parser.y"
    { (yyval.type) = find_type((yyvsp[0].str), 0); ;}
    break;

  case 264:
#line 921 "parser.y"
    { (yyval.type) = (yyvsp[0].type); ;}
    break;

  case 265:
#line 922 "parser.y"
    { (yyval.type) = duptype((yyvsp[0].type), 1); (yyval.type)->is_const = TRUE; ;}
    break;

  case 266:
#line 923 "parser.y"
    { (yyval.type) = (yyvsp[0].type); ;}
    break;

  case 267:
#line 924 "parser.y"
    { (yyval.type) = find_type2((yyvsp[0].str), tsENUM); ;}
    break;

  case 268:
#line 925 "parser.y"
    { (yyval.type) = (yyvsp[0].type); ;}
    break;

  case 269:
#line 926 "parser.y"
    { (yyval.type) = get_type(RPC_FC_STRUCT, (yyvsp[0].str), tsSTRUCT); ;}
    break;

  case 270:
#line 927 "parser.y"
    { (yyval.type) = (yyvsp[0].type); ;}
    break;

  case 271:
#line 928 "parser.y"
    { (yyval.type) = find_type2((yyvsp[0].str), tsUNION); ;}
    break;

  case 272:
#line 929 "parser.y"
    { (yyval.type) = make_safearray((yyvsp[-1].type)); ;}
    break;

  case 273:
#line 932 "parser.y"
    { reg_typedefs((yyvsp[-1].type), (yyvsp[0].pident_list), (yyvsp[-2].attr_list));
						  process_typedefs((yyvsp[0].pident_list));
						;}
    break;

  case 274:
#line 937 "parser.y"
    { (yyval.type) = get_typev(RPC_FC_NON_ENCAPSULATED_UNION, (yyvsp[-3].var), tsUNION);
						  (yyval.type)->kind = TKIND_UNION;
						  (yyval.type)->fields_or_args = (yyvsp[-1].var_list);
						  (yyval.type)->defined = TRUE;
						;}
    break;

  case 275:
#line 944 "parser.y"
    { var_t *u = (yyvsp[-3].var);
						  (yyval.type) = get_typev(RPC_FC_ENCAPSULATED_UNION, (yyvsp[-8].var), tsUNION);
						  (yyval.type)->kind = TKIND_UNION;
						  if (!u) u = make_var( xstrdup("tagged_union") );
						  u->type = make_type(RPC_FC_NON_ENCAPSULATED_UNION, NULL);
						  u->type->kind = TKIND_UNION;
						  u->type->fields_or_args = (yyvsp[-1].var_list);
						  u->type->defined = TRUE;
						  (yyval.type)->fields_or_args = append_var( (yyval.type)->fields_or_args, (yyvsp[-5].var) );
						  (yyval.type)->fields_or_args = append_var( (yyval.type)->fields_or_args, u );
						  (yyval.type)->defined = TRUE;
						;}
    break;

  case 276:
#line 959 "parser.y"
    { (yyval.num) = MAKEVERSION((yyvsp[0].num), 0); ;}
    break;

  case 277:
#line 960 "parser.y"
    { (yyval.num) = MAKEVERSION((yyvsp[-2].num), (yyvsp[0].num)); ;}
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
#line 3673 "parser.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  int yytype = YYTRANSLATE (yychar);
	  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
	  YYSIZE_T yysize = yysize0;
	  YYSIZE_T yysize1;
	  int yysize_overflow = 0;
	  char *yymsg = 0;
#	  define YYERROR_VERBOSE_ARGS_MAXIMUM 5
	  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
	  int yyx;

#if 0
	  /* This is so xgettext sees the translatable formats that are
	     constructed on the fly.  */
	  YY_("syntax error, unexpected %s");
	  YY_("syntax error, unexpected %s, expecting %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
#endif
	  char *yyfmt;
	  char const *yyf;
	  static char const yyunexpected[] = "syntax error, unexpected %s";
	  static char const yyexpecting[] = ", expecting %s";
	  static char const yyor[] = " or %s";
	  char yyformat[sizeof yyunexpected
			+ sizeof yyexpecting - 1
			+ ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
			   * (sizeof yyor - 1))];
	  char const *yyprefix = yyexpecting;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 1;

	  yyarg[0] = yytname[yytype];
	  yyfmt = yystpcpy (yyformat, yyunexpected);

	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
		  {
		    yycount = 1;
		    yysize = yysize0;
		    yyformat[sizeof yyunexpected - 1] = '\0';
		    break;
		  }
		yyarg[yycount++] = yytname[yyx];
		yysize1 = yysize + yytnamerr (0, yytname[yyx]);
		yysize_overflow |= yysize1 < yysize;
		yysize = yysize1;
		yyfmt = yystpcpy (yyfmt, yyprefix);
		yyprefix = yyor;
	      }

	  yyf = YY_(yyformat);
	  yysize1 = yysize + yystrlen (yyf);
	  yysize_overflow |= yysize1 < yysize;
	  yysize = yysize1;

	  if (!yysize_overflow && yysize <= YYSTACK_ALLOC_MAXIMUM)
	    yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg)
	    {
	      /* Avoid sprintf, as that infringes on the user's name space.
		 Don't have undefined behavior even if the translation
		 produced a string with the wrong number of "%s"s.  */
	      char *yyp = yymsg;
	      int yyi = 0;
	      while ((*yyp = *yyf))
		{
		  if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		    {
		      yyp += yytnamerr (yyp, yyarg[yyi++]);
		      yyf += 2;
		    }
		  else
		    {
		      yyp++;
		      yyf++;
		    }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    {
	      yyerror (YY_("syntax error"));
	      goto yyexhaustedlab;
	    }
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror (YY_("syntax error"));
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
        {
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
        }
      else
	{
	  yydestruct ("Error: discarding", yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (0)
     goto yyerrorlab;

yyvsp -= yylen;
  yyssp -= yylen;
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping", yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token. */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK;
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 963 "parser.y"


static void decl_builtin(const char *name, unsigned char type)
{
  type_t *t = make_type(type, NULL);
  t->name = xstrdup(name);
  reg_type(t, name, 0);
}

static type_t *make_builtin(char *name)
{
  /* NAME is strdup'd in the lexer */
  type_t *t = duptype(find_type(name, 0), 0);
  t->name = name;
  return t;
}

static type_t *make_int(int sign)
{
  type_t *t = duptype(find_type("int", 0), 1);

  t->sign = sign;
  if (sign < 0)
    t->type = t->type == RPC_FC_LONG ? RPC_FC_ULONG : RPC_FC_USHORT;

  return t;
}

void init_types(void)
{
  decl_builtin("void", 0);
  decl_builtin("byte", RPC_FC_BYTE);
  decl_builtin("wchar_t", RPC_FC_WCHAR);
  decl_builtin("int", RPC_FC_LONG);     /* win32 */
  decl_builtin("short", RPC_FC_SHORT);
  decl_builtin("small", RPC_FC_SMALL);
  decl_builtin("long", RPC_FC_LONG);
  decl_builtin("hyper", RPC_FC_HYPER);
  decl_builtin("__int64", RPC_FC_HYPER);
  decl_builtin("char", RPC_FC_CHAR);
  decl_builtin("float", RPC_FC_FLOAT);
  decl_builtin("double", RPC_FC_DOUBLE);
  decl_builtin("boolean", RPC_FC_BYTE);
  decl_builtin("error_status_t", RPC_FC_ERROR_STATUS_T);
  decl_builtin("handle_t", RPC_FC_BIND_PRIMITIVE);
}

static str_list_t *append_str(str_list_t *list, char *str)
{
    struct str_list_entry_t *entry;

    if (!str) return list;
    if (!list)
    {
        list = xmalloc( sizeof(*list) );
        list_init( list );
    }
    entry = xmalloc( sizeof(*entry) );
    entry->str = str;
    list_add_tail( list, &entry->entry );
    return list;
}

static attr_list_t *append_attr(attr_list_t *list, attr_t *attr)
{
    if (!attr) return list;
    if (!list)
    {
        list = xmalloc( sizeof(*list) );
        list_init( list );
    }
    list_add_tail( list, &attr->entry );
    return list;
}

static attr_t *make_attr(enum attr_type type)
{
  attr_t *a = xmalloc(sizeof(attr_t));
  a->type = type;
  a->u.ival = 0;
  return a;
}

static attr_t *make_attrv(enum attr_type type, unsigned long val)
{
  attr_t *a = xmalloc(sizeof(attr_t));
  a->type = type;
  a->u.ival = val;
  return a;
}

static attr_t *make_attrp(enum attr_type type, void *val)
{
  attr_t *a = xmalloc(sizeof(attr_t));
  a->type = type;
  a->u.pval = val;
  return a;
}

static expr_t *make_expr(enum expr_type type)
{
  expr_t *e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = NULL;
  e->u.lval = 0;
  e->is_const = FALSE;
  e->cval = 0;
  return e;
}

static expr_t *make_exprl(enum expr_type type, long val)
{
  expr_t *e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = NULL;
  e->u.lval = val;
  e->is_const = FALSE;
  /* check for numeric constant */
  if (type == EXPR_NUM || type == EXPR_HEXNUM || type == EXPR_TRUEFALSE) {
    /* make sure true/false value is valid */
    assert(type != EXPR_TRUEFALSE || val == 0 || val == 1);
    e->is_const = TRUE;
    e->cval = val;
  }
  return e;
}

static expr_t *make_exprd(enum expr_type type, double val)
{
  expr_t *e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = NULL;
  e->u.dval = val;
  e->is_const = TRUE;
  e->cval = val;
  return e;
}

static expr_t *make_exprs(enum expr_type type, char *val)
{
  expr_t *e;
  e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = NULL;
  e->u.sval = val;
  e->is_const = FALSE;
  /* check for predefined constants */
  if (type == EXPR_IDENTIFIER) {
    var_t *c = find_const(val, 0);
    if (c) {
      e->u.sval = c->name;
      free(val);
      e->is_const = TRUE;
      e->cval = c->eval->cval;
    }
  }
  return e;
}

static expr_t *make_exprt(enum expr_type type, type_t *tref, expr_t *expr)
{
  expr_t *e;
  e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = expr;
  e->u.tref = tref;
  e->is_const = FALSE;
  /* check for cast of constant expression */
  if (type == EXPR_SIZEOF) {
    switch (tref->type) {
      case RPC_FC_BYTE:
      case RPC_FC_CHAR:
      case RPC_FC_SMALL:
      case RPC_FC_USMALL:
        e->is_const = TRUE;
        e->cval = 1;
        break;
      case RPC_FC_WCHAR:
      case RPC_FC_USHORT:
      case RPC_FC_SHORT:
        e->is_const = TRUE;
        e->cval = 2;
        break;
      case RPC_FC_LONG:
      case RPC_FC_ULONG:
      case RPC_FC_FLOAT:
      case RPC_FC_ERROR_STATUS_T:
        e->is_const = TRUE;
        e->cval = 4;
        break;
      case RPC_FC_HYPER:
      case RPC_FC_DOUBLE:
        e->is_const = TRUE;
        e->cval = 8;
        break;
    }
  }
  if (type == EXPR_CAST && expr->is_const) {
    e->is_const = TRUE;
    e->cval = expr->cval;
  }
  return e;
}

static expr_t *make_expr1(enum expr_type type, expr_t *expr)
{
  expr_t *e;
  if (type == EXPR_ADDRESSOF && expr->type != EXPR_IDENTIFIER)
    error("address-of operator applied to invalid expression\n");
  e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = expr;
  e->u.lval = 0;
  e->is_const = FALSE;
  /* check for compile-time optimization */
  if (expr->is_const) {
    e->is_const = TRUE;
    switch (type) {
    case EXPR_NEG:
      e->cval = -expr->cval;
      break;
    case EXPR_NOT:
      e->cval = ~expr->cval;
      break;
    default:
      e->is_const = FALSE;
      break;
    }
  }
  return e;
}

static expr_t *make_expr2(enum expr_type type, expr_t *expr1, expr_t *expr2)
{
  expr_t *e;
  e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = expr1;
  e->u.ext = expr2;
  e->is_const = FALSE;
  /* check for compile-time optimization */
  if (expr1->is_const && expr2->is_const) {
    e->is_const = TRUE;
    switch (type) {
    case EXPR_ADD:
      e->cval = expr1->cval + expr2->cval;
      break;
    case EXPR_SUB:
      e->cval = expr1->cval - expr2->cval;
      break;
    case EXPR_MUL:
      e->cval = expr1->cval * expr2->cval;
      break;
    case EXPR_DIV:
      e->cval = expr1->cval / expr2->cval;
      break;
    case EXPR_OR:
      e->cval = expr1->cval | expr2->cval;
      break;
    case EXPR_AND:
      e->cval = expr1->cval & expr2->cval;
      break;
    case EXPR_SHL:
      e->cval = expr1->cval << expr2->cval;
      break;
    case EXPR_SHR:
      e->cval = expr1->cval >> expr2->cval;
      break;
    default:
      e->is_const = FALSE;
      break;
    }
  }
  return e;
}

static expr_t *make_expr3(enum expr_type type, expr_t *expr1, expr_t *expr2, expr_t *expr3)
{
  expr_t *e;
  e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = expr1;
  e->u.ext = expr2;
  e->ext2 = expr3;
  e->is_const = FALSE;
  /* check for compile-time optimization */
  if (expr1->is_const && expr2->is_const && expr3->is_const) {
    e->is_const = TRUE;
    switch (type) {
    case EXPR_COND:
      e->cval = expr1->cval ? expr2->cval : expr3->cval;
      break;
    default:
      e->is_const = FALSE;
      break;
    }
  }
  return e;
}

static expr_list_t *append_expr(expr_list_t *list, expr_t *expr)
{
    if (!expr) return list;
    if (!list)
    {
        list = xmalloc( sizeof(*list) );
        list_init( list );
    }
    list_add_tail( list, &expr->entry );
    return list;
}

static array_dims_t *append_array(array_dims_t *list, expr_t *expr)
{
    if (!expr) return list;
    if (!list)
    {
        list = xmalloc( sizeof(*list) );
        list_init( list );
    }
    list_add_tail( list, &expr->entry );
    return list;
}

static struct list type_pool = LIST_INIT(type_pool);
typedef struct
{
  type_t data;
  struct list link;
} type_pool_node_t;

type_t *alloc_type(void)
{
  type_pool_node_t *node = xmalloc(sizeof *node);
  list_add_tail(&type_pool, &node->link);
  return &node->data;
}

void set_all_tfswrite(int val)
{
  type_pool_node_t *node;
  LIST_FOR_EACH_ENTRY(node, &type_pool, type_pool_node_t, link)
    node->data.tfswrite = val;
}

static type_t *make_type(unsigned char type, type_t *ref)
{
  type_t *t = alloc_type();
  t->name = NULL;
  t->kind = TKIND_PRIMITIVE;
  t->type = type;
  t->ref = ref;
  t->attrs = NULL;
  t->orig = NULL;
  t->funcs = NULL;
  t->fields_or_args = NULL;
  t->ifaces = NULL;
  t->dim = 0;
  t->size_is = NULL;
  t->length_is = NULL;
  t->typestring_offset = 0;
  t->ptrdesc = 0;
  t->declarray = FALSE;
  t->ignore = (parse_only != 0);
  t->is_const = FALSE;
  t->sign = 0;
  t->defined = FALSE;
  t->written = FALSE;
  t->user_types_registered = FALSE;
  t->tfswrite = FALSE;
  t->typelib_idx = -1;
  return t;
}

static void set_type(var_t *v, type_t *type, const pident_t *pident, array_dims_t *arr,
                      int top)
{
  expr_list_t *sizes = get_attrp(v->attrs, ATTR_SIZEIS);
  expr_list_t *lengs = get_attrp(v->attrs, ATTR_LENGTHIS);
  int ptr_attr = get_attrv(v->attrs, ATTR_POINTERTYPE);
  int ptr_type = ptr_attr;
  int sizeless, has_varconf;
  expr_t *dim;
  type_t *atype, **ptype;
  int ptr_level = (pident ? pident->ptr_level : 0);

  v->type = type;

  if (!ptr_type && top)
    ptr_type = RPC_FC_RP;

  for ( ; 0 < ptr_level; --ptr_level)
  {
    v->type = make_type(pointer_default, v->type);
    if (ptr_level == 1 && ptr_type && !arr)
    {
      v->type->type = ptr_type;
      ptr_type = 0;
    }
  }

  if (ptr_type && !arr)
  {
    if (is_ptr(v->type))
    {
      if (v->type->type != ptr_type)
      {
        v->type = duptype(v->type, 1);
        v->type->type = ptr_type;
      }
    }
    else if (!arr && ptr_attr)
      error("%s: pointer attribute applied to non-pointer type\n", v->name);
  }

  if (pident && pident->is_func) {
    int func_ptr_level = pident->func_ptr_level;
    v->type = make_type(RPC_FC_FUNCTION, v->type);
    v->type->fields_or_args = pident->args;
    if (pident->callconv)
      v->type->attrs = append_attr(NULL, make_attrp(ATTR_CALLCONV, pident->callconv));
    else if (is_object_interface) {
      static char *stdmethodcalltype;
      if (!stdmethodcalltype) stdmethodcalltype = strdup("STDMETHODCALLTYPE");
      v->type->attrs = append_attr(NULL, make_attrp(ATTR_CALLCONV, stdmethodcalltype));
    }
    for (; func_ptr_level > 0; func_ptr_level--)
      v->type = make_type(ptr_type, v->type);
  }

  sizeless = FALSE;
  if (arr) LIST_FOR_EACH_ENTRY_REV(dim, arr, expr_t, entry)
  {
    if (sizeless)
      error("%s: only the first array dimension can be unspecified\n", v->name);

    if (dim->is_const)
    {
      unsigned int align = 0;
      size_t size = type_memsize(v->type, &align);

      if (dim->cval <= 0)
        error("%s: array dimension must be positive\n", v->name);

      if (0xffffffffuL / size < (unsigned long) dim->cval)
        error("%s: total array size is too large\n", v->name);
      else if (0xffffuL < size * dim->cval)
        v->type = make_type(RPC_FC_LGFARRAY, v->type);
      else
        v->type = make_type(RPC_FC_SMFARRAY, v->type);
    }
    else
    {
      sizeless = TRUE;
      v->type = make_type(RPC_FC_CARRAY, v->type);
    }

    v->type->declarray = TRUE;
    v->type->dim = dim->cval;
  }

  ptype = &v->type;
  has_varconf = FALSE;
  if (sizes) LIST_FOR_EACH_ENTRY(dim, sizes, expr_t, entry)
  {
    if (dim->type != EXPR_VOID)
    {
      has_varconf = TRUE;
      atype = *ptype = duptype(*ptype, 0);

      if (atype->type == RPC_FC_SMFARRAY || atype->type == RPC_FC_LGFARRAY)
        error("%s: cannot specify size_is for a fixed sized array\n", v->name);

      if (atype->type != RPC_FC_CARRAY && !is_ptr(atype))
        error("%s: size_is attribute applied to illegal type\n", v->name);

      atype->type = RPC_FC_CARRAY;
      atype->size_is = dim;
    }

    ptype = &(*ptype)->ref;
    if (*ptype == NULL)
      error("%s: too many expressions in size_is attribute\n", v->name);
  }

  ptype = &v->type;
  if (lengs) LIST_FOR_EACH_ENTRY(dim, lengs, expr_t, entry)
  {
    if (dim->type != EXPR_VOID)
    {
      has_varconf = TRUE;
      atype = *ptype = duptype(*ptype, 0);

      if (atype->type == RPC_FC_SMFARRAY)
        atype->type = RPC_FC_SMVARRAY;
      else if (atype->type == RPC_FC_LGFARRAY)
        atype->type = RPC_FC_LGVARRAY;
      else if (atype->type == RPC_FC_CARRAY)
        atype->type = RPC_FC_CVARRAY;
      else
        error("%s: length_is attribute applied to illegal type\n", v->name);

      atype->length_is = dim;
    }

    ptype = &(*ptype)->ref;
    if (*ptype == NULL)
      error("%s: too many expressions in length_is attribute\n", v->name);
  }

  if (has_varconf && !last_array(v->type))
  {
    ptype = &v->type;
    for (ptype = &v->type; is_array(*ptype); ptype = &(*ptype)->ref)
    {
      *ptype = duptype(*ptype, 0);
      (*ptype)->type = RPC_FC_BOGUS_ARRAY;
    }
  }

  if (is_array(v->type))
  {
    const type_t *rt = v->type->ref;
    if (is_user_type(rt))
      v->type->type = RPC_FC_BOGUS_ARRAY;
    else
      switch (rt->type)
        {
        case RPC_FC_BOGUS_STRUCT:
        case RPC_FC_NON_ENCAPSULATED_UNION:
        case RPC_FC_ENCAPSULATED_UNION:
        case RPC_FC_ENUM16:
          v->type->type = RPC_FC_BOGUS_ARRAY;
          break;
          /* FC_RP should be above, but widl overuses these, and will break things.  */
        case RPC_FC_UP:
        case RPC_FC_RP:
          if (rt->ref->type == RPC_FC_IP)
            v->type->type = RPC_FC_BOGUS_ARRAY;
          break;
        }
  }
}

static ifref_list_t *append_ifref(ifref_list_t *list, ifref_t *iface)
{
    if (!iface) return list;
    if (!list)
    {
        list = xmalloc( sizeof(*list) );
        list_init( list );
    }
    list_add_tail( list, &iface->entry );
    return list;
}

static ifref_t *make_ifref(type_t *iface)
{
  ifref_t *l = xmalloc(sizeof(ifref_t));
  l->iface = iface;
  l->attrs = NULL;
  return l;
}

static var_list_t *append_var(var_list_t *list, var_t *var)
{
    if (!var) return list;
    if (!list)
    {
        list = xmalloc( sizeof(*list) );
        list_init( list );
    }
    list_add_tail( list, &var->entry );
    return list;
}

static var_t *make_var(char *name)
{
  var_t *v = xmalloc(sizeof(var_t));
  v->name = name;
  v->type = NULL;
  v->attrs = NULL;
  v->eval = NULL;
  return v;
}

static pident_list_t *append_pident(pident_list_t *list, pident_t *p)
{
  if (!p) return list;
  if (!list) {
    list = xmalloc(sizeof(*list));
    list_init(list);
  }
  list_add_tail(list, &p->entry);
  return list;
}

static pident_t *make_pident(var_t *var)
{
  pident_t *p = xmalloc(sizeof(*p));
  p->var = var;
  p->is_func = FALSE;
  p->ptr_level = 0;
  p->func_ptr_level = 0;
  p->args = NULL;
  p->callconv = NULL;
  return p;
}

static func_list_t *append_func(func_list_t *list, func_t *func)
{
    if (!func) return list;
    if (!list)
    {
        list = xmalloc( sizeof(*list) );
        list_init( list );
    }
    list_add_tail( list, &func->entry );
    return list;
}

static func_t *make_func(var_t *def, var_list_t *args)
{
  func_t *f = xmalloc(sizeof(func_t));
  f->def = def;
  f->args = args;
  f->ignore = parse_only;
  f->idx = -1;
  return f;
}

static type_t *make_class(char *name)
{
  type_t *c = make_type(0, NULL);
  c->name = name;
  c->kind = TKIND_COCLASS;
  return c;
}

static type_t *make_safearray(type_t *type)
{
  type_t *sa = duptype(find_type("SAFEARRAY", 0), 1);
  sa->ref = type;
  return make_type(pointer_default, sa);
}

#define HASHMAX 64

static int hash_ident(const char *name)
{
  const char *p = name;
  int sum = 0;
  /* a simple sum hash is probably good enough */
  while (*p) {
    sum += *p;
    p++;
  }
  return sum & (HASHMAX-1);
}

/***** type repository *****/

struct rtype {
  const char *name;
  type_t *type;
  int t;
  struct rtype *next;
};

struct rtype *type_hash[HASHMAX];

static type_t *reg_type(type_t *type, const char *name, int t)
{
  struct rtype *nt;
  int hash;
  if (!name) {
    error_loc("registering named type without name\n");
    return type;
  }
  hash = hash_ident(name);
  nt = xmalloc(sizeof(struct rtype));
  nt->name = name;
  nt->type = type;
  nt->t = t;
  nt->next = type_hash[hash];
  type_hash[hash] = nt;
  return type;
}

static int is_incomplete(const type_t *t)
{
  return !t->defined && (is_struct(t->type) || is_union(t->type));
}

static void add_incomplete(type_t *t)
{
  struct typenode *tn = xmalloc(sizeof *tn);
  tn->type = t;
  list_add_tail(&incomplete_types, &tn->entry);
}

static void fix_type(type_t *t)
{
  if (t->kind == TKIND_ALIAS && is_incomplete(t)) {
    type_t *ot = t->orig;
    fix_type(ot);
    t->fields_or_args = ot->fields_or_args;
    t->defined = ot->defined;
  }
}

static void fix_incomplete(void)
{
  struct typenode *tn, *next;

  LIST_FOR_EACH_ENTRY_SAFE(tn, next, &incomplete_types, struct typenode, entry) {
    fix_type(tn->type);
    free(tn);
  }
}

static type_t *reg_typedefs(type_t *type, pident_list_t *pidents, attr_list_t *attrs)
{
  type_t *ptr = type;
  const pident_t *pident;
  int ptrc = 0;
  int is_str = is_attr(attrs, ATTR_STRING);
  unsigned char ptr_type = get_attrv(attrs, ATTR_POINTERTYPE);

  if (is_str)
  {
    type_t *t = type;
    unsigned char c;

    while (is_ptr(t))
      t = t->ref;

    c = t->type;
    if (c != RPC_FC_CHAR && c != RPC_FC_BYTE && c != RPC_FC_WCHAR)
    {
      pident = LIST_ENTRY( list_head( pidents ), const pident_t, entry );
      error_loc("'%s': [string] attribute is only valid on 'char', 'byte', or 'wchar_t' pointers and arrays\n",
              pident->var->name);
    }
  }

  /* We must generate names for tagless enum, struct or union.
     Typedef-ing a tagless enum, struct or union means we want the typedef
     to be included in a library whether it has other attributes or not,
     hence the public attribute.  */
  if ((type->kind == TKIND_ENUM || type->kind == TKIND_RECORD
       || type->kind == TKIND_UNION) && ! type->name && ! parse_only)
  {
    if (! is_attr(attrs, ATTR_PUBLIC))
      attrs = append_attr( attrs, make_attr(ATTR_PUBLIC) );
    type->name = gen_name();
  }

  LIST_FOR_EACH_ENTRY( pident, pidents, const pident_t, entry )
  {
    var_t *name = pident->var;

    if (name->name) {
      type_t *cur = ptr;
      int cptr = pident->ptr_level;
      if (cptr > ptrc) {
        while (cptr > ptrc) {
          cur = ptr = make_type(pointer_default, cur);
          ptrc++;
        }
      } else {
        while (cptr < ptrc) {
          cur = cur->ref;
          cptr++;
        }
      }
      if (pident->is_func) {
        int func_ptr_level = pident->func_ptr_level;
        cur = make_type(RPC_FC_FUNCTION, cur);
        cur->fields_or_args = pident->args;
        if (pident->callconv)
          cur->attrs = append_attr(NULL, make_attrp(ATTR_CALLCONV, pident->callconv));
        else if (is_object_interface) {
          static char *stdmethodcalltype;
          if (!stdmethodcalltype) stdmethodcalltype = strdup("STDMETHODCALLTYPE");
          cur->attrs = append_attr(NULL, make_attrp(ATTR_CALLCONV, stdmethodcalltype));
        }
        for (; func_ptr_level > 0; func_ptr_level--)
          cur = make_type(pointer_default, cur);
      }
      cur = alias(cur, name->name);
      cur->attrs = attrs;
      if (ptr_type)
      {
        if (is_ptr(cur))
          cur->type = ptr_type;
        else
          error_loc("'%s': pointer attribute applied to non-pointer type\n",
                  cur->name);
      }
      else if (is_str && ! is_ptr(cur))
        error_loc("'%s': [string] attribute applied to non-pointer type\n",
                cur->name);

      if (is_incomplete(cur))
        add_incomplete(cur);
      reg_type(cur, cur->name, 0);
    }
  }
  return type;
}

static type_t *find_type(const char *name, int t)
{
  struct rtype *cur = type_hash[hash_ident(name)];
  while (cur && (cur->t != t || strcmp(cur->name, name)))
    cur = cur->next;
  if (!cur) {
    error_loc("type '%s' not found\n", name);
    return NULL;
  }
  return cur->type;
}

static type_t *find_type2(char *name, int t)
{
  type_t *tp = find_type(name, t);
  free(name);
  return tp;
}

int is_type(const char *name)
{
  struct rtype *cur = type_hash[hash_ident(name)];
  while (cur && (cur->t || strcmp(cur->name, name)))
    cur = cur->next;
  if (cur) return TRUE;
  return FALSE;
}

static type_t *get_type(unsigned char type, char *name, int t)
{
  struct rtype *cur = NULL;
  type_t *tp;
  if (name) {
    cur = type_hash[hash_ident(name)];
    while (cur && (cur->t != t || strcmp(cur->name, name)))
      cur = cur->next;
  }
  if (cur) {
    free(name);
    return cur->type;
  }
  tp = make_type(type, NULL);
  tp->name = name;
  if (!name) return tp;
  return reg_type(tp, name, t);
}

static type_t *get_typev(unsigned char type, var_t *name, int t)
{
  char *sname = NULL;
  if (name) {
    sname = name->name;
    free(name);
  }
  return get_type(type, sname, t);
}

static int get_struct_type(var_list_t *fields)
{
  int has_pointer = 0;
  int has_conformance = 0;
  int has_variance = 0;
  var_t *field;

  if (get_padding(fields))
    return RPC_FC_BOGUS_STRUCT;

  if (fields) LIST_FOR_EACH_ENTRY( field, fields, var_t, entry )
  {
    type_t *t = field->type;

    if (is_user_type(t))
      return RPC_FC_BOGUS_STRUCT;

    if (is_ptr(t))
    {
        do
            t = t->ref;
        while (is_ptr(t));

        switch (t->type)
        {
        case RPC_FC_IP:
        case RPC_FC_ENCAPSULATED_UNION:
        case RPC_FC_NON_ENCAPSULATED_UNION:
        case RPC_FC_BOGUS_STRUCT:
            return RPC_FC_BOGUS_STRUCT;
        }

        has_pointer = 1;
        continue;
    }

    if (field->type->declarray)
    {
        if (is_string_type(field->attrs, field->type))
        {
            if (is_conformant_array(field->type))
                has_conformance = 1;
            has_variance = 1;
            continue;
        }

        if (is_array(field->type->ref))
            return RPC_FC_BOGUS_STRUCT;

        if (is_conformant_array(field->type))
        {
            has_conformance = 1;
            if (field->type->declarray && list_next(fields, &field->entry))
                error_loc("field '%s' deriving from a conformant array must be the last field in the structure\n",
                        field->name);
        }
        if (field->type->length_is)
            has_variance = 1;

        t = field->type->ref;
    }

    switch (t->type)
    {
    /*
     * RPC_FC_BYTE, RPC_FC_STRUCT, etc
     *  Simple types don't effect the type of struct.
     *  A struct containing a simple struct is still a simple struct.
     *  So long as we can block copy the data, we return RPC_FC_STRUCT.
     */
    case 0: /* void pointer */
    case RPC_FC_BYTE:
    case RPC_FC_CHAR:
    case RPC_FC_SMALL:
    case RPC_FC_USMALL:
    case RPC_FC_WCHAR:
    case RPC_FC_SHORT:
    case RPC_FC_USHORT:
    case RPC_FC_LONG:
    case RPC_FC_ULONG:
    case RPC_FC_INT3264:
    case RPC_FC_UINT3264:
    case RPC_FC_HYPER:
    case RPC_FC_FLOAT:
    case RPC_FC_DOUBLE:
    case RPC_FC_STRUCT:
    case RPC_FC_ENUM32:
      break;

    case RPC_FC_RP:
    case RPC_FC_UP:
    case RPC_FC_FP:
    case RPC_FC_OP:
    case RPC_FC_CARRAY:
    case RPC_FC_CVARRAY:
    case RPC_FC_BOGUS_ARRAY:
      has_pointer = 1;
      break;

    /*
     * Propagate member attributes
     *  a struct should be at least as complex as its member
     */
    case RPC_FC_CVSTRUCT:
      has_conformance = 1;
      has_variance = 1;
      has_pointer = 1;
      break;

    case RPC_FC_CPSTRUCT:
      has_conformance = 1;
      if (list_next( fields, &field->entry ))
          error_loc("field '%s' deriving from a conformant array must be the last field in the structure\n",
                  field->name);
      has_pointer = 1;
      break;

    case RPC_FC_CSTRUCT:
      has_conformance = 1;
      if (list_next( fields, &field->entry ))
          error_loc("field '%s' deriving from a conformant array must be the last field in the structure\n",
                  field->name);
      break;

    case RPC_FC_PSTRUCT:
      has_pointer = 1;
      break;

    default:
      error_loc("Unknown struct member %s with type (0x%02x)\n", field->name, t->type);
      /* fallthru - treat it as complex */

    /* as soon as we see one of these these members, it's bogus... */
    case RPC_FC_ENCAPSULATED_UNION:
    case RPC_FC_NON_ENCAPSULATED_UNION:
    case RPC_FC_BOGUS_STRUCT:
    case RPC_FC_ENUM16:
      return RPC_FC_BOGUS_STRUCT;
    }
  }

  if( has_variance )
  {
    if ( has_conformance )
      return RPC_FC_CVSTRUCT;
    else
      return RPC_FC_BOGUS_STRUCT;
  }
  if( has_conformance && has_pointer )
    return RPC_FC_CPSTRUCT;
  if( has_conformance )
    return RPC_FC_CSTRUCT;
  if( has_pointer )
    return RPC_FC_PSTRUCT;
  return RPC_FC_STRUCT;
}

/***** constant repository *****/

struct rconst {
  char *name;
  var_t *var;
  struct rconst *next;
};

struct rconst *const_hash[HASHMAX];

static var_t *reg_const(var_t *var)
{
  struct rconst *nc;
  int hash;
  if (!var->name) {
    error_loc("registering constant without name\n");
    return var;
  }
  hash = hash_ident(var->name);
  nc = xmalloc(sizeof(struct rconst));
  nc->name = var->name;
  nc->var = var;
  nc->next = const_hash[hash];
  const_hash[hash] = nc;
  return var;
}

static var_t *find_const(char *name, int f)
{
  struct rconst *cur = const_hash[hash_ident(name)];
  while (cur && strcmp(cur->name, name))
    cur = cur->next;
  if (!cur) {
    if (f) error_loc("constant '%s' not found\n", name);
    return NULL;
  }
  return cur->var;
}

static void write_libid(const char *name, const attr_list_t *attr)
{
  const UUID *uuid = get_attrp(attr, ATTR_UUID);
  write_guid(idfile, "LIBID", name, uuid);
}

static void write_clsid(type_t *cls)
{
  const UUID *uuid = get_attrp(cls->attrs, ATTR_UUID);
  write_guid(idfile, "CLSID", cls->name, uuid);
}

static void write_diid(type_t *iface)
{
  const UUID *uuid = get_attrp(iface->attrs, ATTR_UUID);
  write_guid(idfile, "DIID", iface->name, uuid);
}

static void write_iid(type_t *iface)
{
  const UUID *uuid = get_attrp(iface->attrs, ATTR_UUID);
  write_guid(idfile, "IID", iface->name, uuid);
}

static int compute_method_indexes(type_t *iface)
{
  int idx;
  func_t *f;

  if (iface->ref)
    idx = compute_method_indexes(iface->ref);
  else
    idx = 0;

  if (!iface->funcs)
    return idx;

  LIST_FOR_EACH_ENTRY( f, iface->funcs, func_t, entry )
    if (! is_callas(f->def->attrs))
      f->idx = idx++;

  return idx;
}

static char *gen_name(void)
{
  static const char format[] = "__WIDL_%s_generated_name_%08lX";
  static unsigned long n = 0;
  static const char *file_id;
  static size_t size;
  char *name;

  if (! file_id)
  {
    char *dst = dup_basename(input_name, ".idl");
    file_id = dst;

    for (; *dst; ++dst)
      if (! isalnum((unsigned char) *dst))
        *dst = '_';

    size = sizeof format - 7 + strlen(file_id) + 8;
  }

  name = xmalloc(size);
  sprintf(name, format, file_id, n++);
  return name;
}

static void process_typedefs(pident_list_t *pidents)
{
  pident_t *pident, *next;

  if (!pidents) return;
  LIST_FOR_EACH_ENTRY_SAFE( pident, next, pidents, pident_t, entry )
  {
    var_t *var = pident->var;
    type_t *type = find_type(var->name, 0);

    if (! parse_only && do_header)
      write_typedef(type);
    if (in_typelib && type->attrs)
      add_typelib_entry(type);

    free(pident);
    free(var);
  }
}

static void check_arg(var_t *arg)
{
  type_t *t = arg->type;

  if (t->type == 0 && ! is_var_ptr(arg))
    error_loc("argument '%s' has void type\n", arg->name);
}

static void check_all_user_types(ifref_list_t *ifrefs)
{
  const ifref_t *ifref;
  const func_t *f;

  if (ifrefs) LIST_FOR_EACH_ENTRY(ifref, ifrefs, const ifref_t, entry)
  {
    const func_list_t *fs = ifref->iface->funcs;
    if (fs) LIST_FOR_EACH_ENTRY(f, fs, const func_t, entry)
      check_for_additional_prototype_types(f->args);
  }
}

int is_valid_uuid(const char *s)
{
  int i;

  for (i = 0; i < 36; ++i)
    if (i == 8 || i == 13 || i == 18 || i == 23)
    {
      if (s[i] != '-')
        return FALSE;
    }
    else
      if (!isxdigit(s[i]))
        return FALSE;

  return s[i] == '\0';
}

