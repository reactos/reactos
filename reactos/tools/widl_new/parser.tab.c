/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

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

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

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
     tFLOAT = 309,
     tHANDLE = 310,
     tHANDLET = 311,
     tHELPCONTEXT = 312,
     tHELPFILE = 313,
     tHELPSTRING = 314,
     tHELPSTRINGCONTEXT = 315,
     tHELPSTRINGDLL = 316,
     tHIDDEN = 317,
     tHYPER = 318,
     tID = 319,
     tIDEMPOTENT = 320,
     tIIDIS = 321,
     tIMMEDIATEBIND = 322,
     tIMPLICITHANDLE = 323,
     tIMPORT = 324,
     tIMPORTLIB = 325,
     tIN = 326,
     tINLINE = 327,
     tINPUTSYNC = 328,
     tINT = 329,
     tINT64 = 330,
     tINTERFACE = 331,
     tLCID = 332,
     tLENGTHIS = 333,
     tLIBRARY = 334,
     tLOCAL = 335,
     tLONG = 336,
     tMETHODS = 337,
     tMODULE = 338,
     tNONBROWSABLE = 339,
     tNONCREATABLE = 340,
     tNONEXTENSIBLE = 341,
     tOBJECT = 342,
     tODL = 343,
     tOLEAUTOMATION = 344,
     tOPTIONAL = 345,
     tOUT = 346,
     tPOINTERDEFAULT = 347,
     tPROPERTIES = 348,
     tPROPGET = 349,
     tPROPPUT = 350,
     tPROPPUTREF = 351,
     tPTR = 352,
     tPUBLIC = 353,
     tRANGE = 354,
     tREADONLY = 355,
     tREF = 356,
     tREQUESTEDIT = 357,
     tRESTRICTED = 358,
     tRETVAL = 359,
     tSAFEARRAY = 360,
     tSHORT = 361,
     tSIGNED = 362,
     tSINGLE = 363,
     tSIZEIS = 364,
     tSIZEOF = 365,
     tSMALL = 366,
     tSOURCE = 367,
     tSTDCALL = 368,
     tSTRING = 369,
     tSTRUCT = 370,
     tSWITCH = 371,
     tSWITCHIS = 372,
     tSWITCHTYPE = 373,
     tTRANSMITAS = 374,
     tTRUE = 375,
     tTYPEDEF = 376,
     tUNION = 377,
     tUNIQUE = 378,
     tUNSIGNED = 379,
     tUUID = 380,
     tV1ENUM = 381,
     tVARARG = 382,
     tVERSION = 383,
     tVOID = 384,
     tWCHAR = 385,
     tWIREMARSHAL = 386,
     CAST = 387,
     PPTR = 388,
     NEG = 389
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
#define tFLOAT 309
#define tHANDLE 310
#define tHANDLET 311
#define tHELPCONTEXT 312
#define tHELPFILE 313
#define tHELPSTRING 314
#define tHELPSTRINGCONTEXT 315
#define tHELPSTRINGDLL 316
#define tHIDDEN 317
#define tHYPER 318
#define tID 319
#define tIDEMPOTENT 320
#define tIIDIS 321
#define tIMMEDIATEBIND 322
#define tIMPLICITHANDLE 323
#define tIMPORT 324
#define tIMPORTLIB 325
#define tIN 326
#define tINLINE 327
#define tINPUTSYNC 328
#define tINT 329
#define tINT64 330
#define tINTERFACE 331
#define tLCID 332
#define tLENGTHIS 333
#define tLIBRARY 334
#define tLOCAL 335
#define tLONG 336
#define tMETHODS 337
#define tMODULE 338
#define tNONBROWSABLE 339
#define tNONCREATABLE 340
#define tNONEXTENSIBLE 341
#define tOBJECT 342
#define tODL 343
#define tOLEAUTOMATION 344
#define tOPTIONAL 345
#define tOUT 346
#define tPOINTERDEFAULT 347
#define tPROPERTIES 348
#define tPROPGET 349
#define tPROPPUT 350
#define tPROPPUTREF 351
#define tPTR 352
#define tPUBLIC 353
#define tRANGE 354
#define tREADONLY 355
#define tREF 356
#define tREQUESTEDIT 357
#define tRESTRICTED 358
#define tRETVAL 359
#define tSAFEARRAY 360
#define tSHORT 361
#define tSIGNED 362
#define tSINGLE 363
#define tSIZEIS 364
#define tSIZEOF 365
#define tSMALL 366
#define tSOURCE 367
#define tSTDCALL 368
#define tSTRING 369
#define tSTRUCT 370
#define tSWITCH 371
#define tSWITCHIS 372
#define tSWITCHTYPE 373
#define tTRANSMITAS 374
#define tTRUE 375
#define tTYPEDEF 376
#define tUNION 377
#define tUNIQUE 378
#define tUNSIGNED 379
#define tUUID 380
#define tV1ENUM 381
#define tVARARG 382
#define tVERSION 383
#define tVOID 384
#define tWCHAR 385
#define tWIREMARSHAL 386
#define CAST 387
#define PPTR 388
#define NEG 389




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

#include "windef.h"

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
static void set_type(var_t *v, type_t *type, int ptr_level, array_dims_t *arr);
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

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 136 "parser.y"
{
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
}
/* Line 187 of yacc.c.  */
#line 530 "parser.tab.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 543 "parser.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

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

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
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
      while (YYID (0))
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
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1091

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  154
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  75
/* YYNRULES -- Number of rules.  */
#define YYNRULES  268
/* YYNRULES -- Number of states.  */
#define YYNSTATES  495

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   389

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   136,     2,
     146,   147,   139,   138,   132,   137,   153,   140,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   134,   145,
       2,   152,     2,   133,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   150,     2,   151,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   148,   135,   149,   141,     2,     2,     2,
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
     125,   126,   127,   128,   129,   130,   131,   142,   143,   144
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     6,     9,    12,    16,    19,    22,
      25,    28,    29,    32,    35,    39,    42,    45,    48,    51,
      52,    56,    59,    61,    64,    66,    69,    72,    74,    77,
      80,    83,    88,    92,    96,   101,   104,   108,   112,   113,
     115,   117,   119,   123,   125,   130,   134,   141,   147,   148,
     152,   156,   158,   162,   167,   168,   170,   174,   176,   180,
     185,   187,   191,   192,   194,   196,   198,   200,   202,   207,
     212,   214,   216,   218,   220,   222,   224,   229,   234,   236,
     238,   243,   245,   250,   255,   260,   262,   264,   269,   274,
     279,   284,   289,   291,   296,   298,   303,   305,   311,   313,
     315,   320,   322,   324,   326,   328,   330,   332,   334,   336,
     338,   343,   345,   347,   349,   351,   358,   360,   362,   364,
     366,   371,   373,   375,   380,   385,   390,   395,   397,   399,
     404,   409,   411,   412,   414,   415,   418,   423,   427,   433,
     434,   437,   439,   441,   445,   449,   451,   457,   459,   463,
     464,   466,   468,   470,   472,   474,   476,   478,   484,   488,
     492,   496,   500,   504,   508,   512,   516,   519,   522,   525,
     530,   535,   539,   541,   545,   547,   552,   553,   556,   559,
     563,   566,   568,   573,   581,   582,   584,   585,   587,   589,
     591,   593,   595,   597,   599,   602,   605,   607,   609,   611,
     613,   615,   617,   619,   620,   622,   624,   627,   629,   632,
     635,   637,   639,   642,   645,   648,   653,   654,   657,   660,
     663,   666,   669,   672,   676,   679,   683,   689,   695,   696,
     699,   702,   705,   708,   714,   722,   724,   727,   730,   733,
     736,   739,   744,   747,   750,   752,   754,   758,   760,   764,
     766,   768,   770,   776,   778,   780,   782,   785,   787,   790,
     792,   795,   797,   800,   805,   810,   816,   827,   829
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     155,     0,    -1,   156,    -1,    -1,   156,   216,    -1,   156,
     215,    -1,   156,   202,   145,    -1,   156,   204,    -1,   156,
     219,    -1,   156,   166,    -1,   156,   159,    -1,    -1,   157,
     216,    -1,   157,   215,    -1,   157,   202,   145,    -1,   157,
     204,    -1,   157,   219,    -1,   157,   159,    -1,   157,   163,
      -1,    -1,   158,   195,   145,    -1,   158,   159,    -1,   145,
      -1,   181,   145,    -1,   160,    -1,   185,   145,    -1,   191,
     145,    -1,   162,    -1,   224,   145,    -1,   226,   145,    -1,
     227,   145,    -1,    37,   146,     8,   147,    -1,    69,     8,
     145,    -1,   161,   157,    10,    -1,    70,   146,     8,   147,
      -1,    79,     3,    -1,   174,   164,   148,    -1,   165,   157,
     149,    -1,    -1,   169,    -1,   129,    -1,   170,    -1,   169,
     132,   170,    -1,   168,    -1,   174,   225,   221,   171,    -1,
     225,   221,   171,    -1,   174,   225,   221,   146,   167,   147,
      -1,   225,   221,   146,   167,   147,    -1,    -1,   150,   172,
     151,    -1,   150,   139,   151,    -1,   187,    -1,   172,   132,
     188,    -1,   172,   151,   150,   188,    -1,    -1,   174,    -1,
     150,   175,   151,    -1,   177,    -1,   175,   132,   177,    -1,
     175,   151,   150,   177,    -1,     8,    -1,   176,   132,     8,
      -1,    -1,    13,    -1,    15,    -1,    16,    -1,    18,    -1,
      19,    -1,    24,   146,   198,   147,    -1,    26,   146,   189,
     147,    -1,    33,    -1,    34,    -1,    35,    -1,    36,    -1,
      38,    -1,    39,    -1,    40,   146,   190,   147,    -1,    40,
     146,     8,   147,    -1,    41,    -1,    42,    -1,    44,   146,
       8,   147,    -1,    46,    -1,    47,   146,   176,   147,    -1,
      48,   146,     8,   147,    -1,    48,   146,   190,   147,    -1,
      51,    -1,    55,    -1,    57,   146,   190,   147,    -1,    58,
     146,     8,   147,    -1,    59,   146,     8,   147,    -1,    60,
     146,   190,   147,    -1,    61,   146,     8,   147,    -1,    62,
      -1,    64,   146,   190,   147,    -1,    65,    -1,    66,   146,
     198,   147,    -1,    67,    -1,    68,   146,    56,     3,   147,
      -1,    71,    -1,    73,    -1,    78,   146,   186,   147,    -1,
      80,    -1,    84,    -1,    85,    -1,    86,    -1,    87,    -1,
      88,    -1,    89,    -1,    90,    -1,    91,    -1,    92,   146,
     223,   147,    -1,    94,    -1,    95,    -1,    96,    -1,    98,
      -1,    99,   146,   190,   132,   190,   147,    -1,   100,    -1,
     102,    -1,   103,    -1,   104,    -1,   109,   146,   186,   147,
      -1,   112,    -1,   114,    -1,   117,   146,   188,   147,    -1,
     118,   146,   225,   147,    -1,   119,   146,   225,   147,    -1,
     125,   146,     9,   147,    -1,   126,    -1,   127,    -1,   128,
     146,   228,   147,    -1,   131,   146,   225,   147,    -1,   223,
      -1,    -1,   113,    -1,    -1,   179,   180,    -1,    26,   188,
     134,   193,    -1,    38,   134,   193,    -1,    32,   225,   198,
     152,   190,    -1,    -1,   183,   132,    -1,   183,    -1,   184,
      -1,   183,   132,   184,    -1,   198,   152,   190,    -1,   198,
      -1,    49,   197,   148,   182,   149,    -1,   187,    -1,   186,
     132,   187,    -1,    -1,   188,    -1,     5,    -1,     6,    -1,
       7,    -1,    53,    -1,   120,    -1,     3,    -1,   188,   133,
     188,   134,   188,    -1,   188,   135,   188,    -1,   188,   136,
     188,    -1,   188,   138,   188,    -1,   188,   137,   188,    -1,
     188,   139,   188,    -1,   188,   140,   188,    -1,   188,    11,
     188,    -1,   188,    12,   188,    -1,   141,   188,    -1,   137,
     188,    -1,   139,   188,    -1,   146,   225,   147,   188,    -1,
     110,   146,   225,   147,    -1,   146,   188,   147,    -1,   190,
      -1,   189,   132,   190,    -1,   188,    -1,    52,    32,   225,
     198,    -1,    -1,   192,   193,    -1,   194,   145,    -1,   173,
     227,   145,    -1,   174,   145,    -1,   145,    -1,   173,   225,
     221,   171,    -1,   173,   225,   178,   221,   146,   167,   147,
      -1,    -1,   198,    -1,    -1,     3,    -1,     4,    -1,     3,
      -1,     4,    -1,    22,    -1,   130,    -1,   201,    -1,   107,
     201,    -1,   124,   201,    -1,   124,    -1,    54,    -1,   108,
      -1,    45,    -1,    20,    -1,    50,    -1,    56,    -1,    -1,
      74,    -1,    74,    -1,   106,   200,    -1,   111,    -1,    81,
     200,    -1,    63,   200,    -1,    75,    -1,    28,    -1,    29,
       3,    -1,    29,     4,    -1,   174,   202,    -1,   203,   148,
     205,   149,    -1,    -1,   205,   206,    -1,   173,   216,    -1,
      43,     3,    -1,    43,     4,    -1,   174,   207,    -1,    93,
     134,    -1,   209,   194,   145,    -1,    82,   134,    -1,   210,
     195,   145,    -1,   208,   148,   209,   210,   149,    -1,   208,
     148,   213,   145,   149,    -1,    -1,   134,     4,    -1,    76,
       3,    -1,    76,     4,    -1,   174,   213,    -1,   214,   212,
     148,   158,   149,    -1,   214,   134,     3,   148,   162,   158,
     149,    -1,   211,    -1,   213,   145,    -1,   207,   145,    -1,
      83,     3,    -1,    83,     4,    -1,   174,   217,    -1,   218,
     148,   158,   149,    -1,   139,   221,    -1,    32,   220,    -1,
     198,    -1,   220,    -1,   146,   221,   147,    -1,   221,    -1,
     222,   132,   221,    -1,   101,    -1,   123,    -1,    97,    -1,
     115,   197,   148,   192,   149,    -1,   129,    -1,     4,    -1,
     199,    -1,    32,   225,    -1,   185,    -1,    49,     3,    -1,
     224,    -1,   115,     3,    -1,   227,    -1,   122,     3,    -1,
     105,   146,   225,   147,    -1,   121,   173,   225,   222,    -1,
     122,   197,   148,   192,   149,    -1,   122,   197,   116,   146,
     194,   147,   196,   148,   179,   149,    -1,     5,    -1,     5,
     153,     5,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   281,   281,   289,   290,   291,   292,   296,   301,   302,
     303,   306,   307,   308,   309,   310,   314,   315,   316,   319,
     320,   321,   324,   325,   326,   327,   332,   333,   334,   339,
     340,   347,   349,   352,   355,   358,   360,   365,   368,   369,
     372,   375,   376,   377,   381,   386,   390,   396,   403,   404,
     405,   408,   409,   410,   413,   414,   418,   424,   425,   426,
     429,   430,   433,   434,   435,   436,   437,   438,   439,   440,
     441,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     451,   452,   453,   454,   455,   456,   457,   458,   459,   460,
     461,   462,   463,   464,   465,   466,   467,   468,   469,   470,
     471,   472,   473,   474,   475,   476,   477,   478,   479,   480,
     481,   482,   483,   484,   485,   486,   489,   490,   491,   492,
     493,   494,   495,   496,   497,   498,   499,   500,   501,   502,
     503,   504,   507,   508,   511,   512,   515,   519,   525,   531,
     532,   533,   536,   540,   549,   553,   558,   567,   568,   581,
     582,   585,   586,   587,   588,   589,   590,   591,   592,   593,
     594,   595,   596,   597,   598,   599,   600,   601,   602,   603,
     604,   605,   608,   609,   612,   618,   623,   624,   627,   628,
     629,   630,   633,   641,   653,   654,   657,   658,   659,   662,
     664,   667,   668,   669,   670,   671,   687,   688,   689,   690,
     691,   692,   693,   696,   697,   700,   701,   702,   703,   704,
     705,   706,   709,   710,   716,   725,   731,   732,   736,   739,
     740,   743,   755,   756,   759,   760,   763,   772,   781,   782,
     785,   786,   789,   797,   807,   816,   820,   821,   824,   825,
     828,   833,   839,   840,   843,   844,   845,   849,   850,   854,
     855,   856,   859,   870,   871,   872,   873,   874,   875,   876,
     877,   878,   879,   880,   883,   888,   893,   910,   911
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
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
  "tEXPLICITHANDLE", "tEXTERN", "tFALSE", "tFLOAT", "tHANDLE", "tHANDLET",
  "tHELPCONTEXT", "tHELPFILE", "tHELPSTRING", "tHELPSTRINGCONTEXT",
  "tHELPSTRINGDLL", "tHIDDEN", "tHYPER", "tID", "tIDEMPOTENT", "tIIDIS",
  "tIMMEDIATEBIND", "tIMPLICITHANDLE", "tIMPORT", "tIMPORTLIB", "tIN",
  "tINLINE", "tINPUTSYNC", "tINT", "tINT64", "tINTERFACE", "tLCID",
  "tLENGTHIS", "tLIBRARY", "tLOCAL", "tLONG", "tMETHODS", "tMODULE",
  "tNONBROWSABLE", "tNONCREATABLE", "tNONEXTENSIBLE", "tOBJECT", "tODL",
  "tOLEAUTOMATION", "tOPTIONAL", "tOUT", "tPOINTERDEFAULT", "tPROPERTIES",
  "tPROPGET", "tPROPPUT", "tPROPPUTREF", "tPTR", "tPUBLIC", "tRANGE",
  "tREADONLY", "tREF", "tREQUESTEDIT", "tRESTRICTED", "tRETVAL",
  "tSAFEARRAY", "tSHORT", "tSIGNED", "tSINGLE", "tSIZEIS", "tSIZEOF",
  "tSMALL", "tSOURCE", "tSTDCALL", "tSTRING", "tSTRUCT", "tSWITCH",
  "tSWITCHIS", "tSWITCHTYPE", "tTRANSMITAS", "tTRUE", "tTYPEDEF", "tUNION",
  "tUNIQUE", "tUNSIGNED", "tUUID", "tV1ENUM", "tVARARG", "tVERSION",
  "tVOID", "tWCHAR", "tWIREMARSHAL", "','", "'?'", "':'", "'|'", "'&'",
  "'-'", "'+'", "'*'", "'/'", "'~'", "CAST", "PPTR", "NEG", "';'", "'('",
  "')'", "'{'", "'}'", "'['", "']'", "'='", "'.'", "$accept", "input",
  "gbl_statements", "imp_statements", "int_statements", "statement",
  "cppquote", "import_start", "import", "importlib", "libraryhdr",
  "library_start", "librarydef", "m_args", "no_args", "args", "arg",
  "array", "array_list", "m_attributes", "attributes", "attrib_list",
  "str_list", "attribute", "callconv", "cases", "case", "constdef",
  "enums", "enum_list", "enum", "enumdef", "m_exprs", "m_expr", "expr",
  "expr_list_const", "expr_const", "externdef", "fields", "field",
  "s_field", "funcdef", "m_ident", "t_ident", "ident", "base_type",
  "m_int", "int_std", "coclass", "coclasshdr", "coclassdef",
  "coclass_ints", "coclass_int", "dispinterface", "dispinterfacehdr",
  "dispint_props", "dispint_meths", "dispinterfacedef", "inherit",
  "interface", "interfacehdr", "interfacedef", "interfacedec", "module",
  "modulehdr", "moduledef", "p_ident", "pident", "pident_list",
  "pointer_type", "structdef", "type", "typedef", "uniondef", "version", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
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
     385,   386,    44,    63,    58,   124,    38,    45,    43,    42,
      47,   126,   387,   388,   389,    59,    40,    41,   123,   125,
      91,    93,    61,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   154,   155,   156,   156,   156,   156,   156,   156,   156,
     156,   157,   157,   157,   157,   157,   157,   157,   157,   158,
     158,   158,   159,   159,   159,   159,   159,   159,   159,   159,
     159,   160,   161,   162,   163,   164,   165,   166,   167,   167,
     168,   169,   169,   169,   170,   170,   170,   170,   171,   171,
     171,   172,   172,   172,   173,   173,   174,   175,   175,   175,
     176,   176,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   177,   178,   178,   179,   179,   180,   180,   181,   182,
     182,   182,   183,   183,   184,   184,   185,   186,   186,   187,
     187,   188,   188,   188,   188,   188,   188,   188,   188,   188,
     188,   188,   188,   188,   188,   188,   188,   188,   188,   188,
     188,   188,   189,   189,   190,   191,   192,   192,   193,   193,
     193,   193,   194,   195,   196,   196,   197,   197,   197,   198,
     198,   199,   199,   199,   199,   199,   199,   199,   199,   199,
     199,   199,   199,   200,   200,   201,   201,   201,   201,   201,
     201,   201,   202,   202,   203,   204,   205,   205,   206,   207,
     207,   208,   209,   209,   210,   210,   211,   211,   212,   212,
     213,   213,   214,   215,   215,   215,   216,   216,   217,   217,
     218,   219,   220,   220,   221,   221,   221,   222,   222,   223,
     223,   223,   224,   225,   225,   225,   225,   225,   225,   225,
     225,   225,   225,   225,   226,   227,   227,   228,   228
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     0,     2,     2,     3,     2,     2,     2,
       2,     0,     2,     2,     3,     2,     2,     2,     2,     0,
       3,     2,     1,     2,     1,     2,     2,     1,     2,     2,
       2,     4,     3,     3,     4,     2,     3,     3,     0,     1,
       1,     1,     3,     1,     4,     3,     6,     5,     0,     3,
       3,     1,     3,     4,     0,     1,     3,     1,     3,     4,
       1,     3,     0,     1,     1,     1,     1,     1,     4,     4,
       1,     1,     1,     1,     1,     1,     4,     4,     1,     1,
       4,     1,     4,     4,     4,     1,     1,     4,     4,     4,
       4,     4,     1,     4,     1,     4,     1,     5,     1,     1,
       4,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       4,     1,     1,     1,     1,     6,     1,     1,     1,     1,
       4,     1,     1,     4,     4,     4,     4,     1,     1,     4,
       4,     1,     0,     1,     0,     2,     4,     3,     5,     0,
       2,     1,     1,     3,     3,     1,     5,     1,     3,     0,
       1,     1,     1,     1,     1,     1,     1,     5,     3,     3,
       3,     3,     3,     3,     3,     3,     2,     2,     2,     4,
       4,     3,     1,     3,     1,     4,     0,     2,     2,     3,
       2,     1,     4,     7,     0,     1,     0,     1,     1,     1,
       1,     1,     1,     1,     2,     2,     1,     1,     1,     1,
       1,     1,     1,     0,     1,     1,     2,     1,     2,     2,
       1,     1,     2,     2,     2,     4,     0,     2,     2,     2,
       2,     2,     2,     3,     2,     3,     5,     5,     0,     2,
       2,     2,     2,     5,     7,     1,     2,     2,     2,     2,
       2,     4,     2,     2,     1,     1,     3,     1,     3,     1,
       1,     1,     5,     1,     1,     1,     2,     1,     2,     1,
       2,     1,     2,     4,     4,     5,    10,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       3,     0,     2,     1,     0,     0,     0,     0,   186,     0,
       0,     0,   186,    54,   186,    22,    62,    10,    24,    11,
      27,    11,     9,     0,     0,     0,     0,     0,     0,     7,
       0,     0,   235,     0,   228,     5,     4,     0,     8,     0,
       0,     0,   212,   213,   254,   200,   191,   211,     0,   199,
     186,   201,   197,   202,   203,   205,   210,   203,     0,   203,
       0,   198,   207,   186,   186,   196,   253,   192,   257,   255,
     193,   259,     0,   261,     0,   219,   220,   187,   188,     0,
       0,     0,   230,   231,     0,     0,    55,     0,    63,    64,
      65,    66,    67,     0,     0,    70,    71,    72,    73,    74,
      75,     0,    78,    79,     0,    81,     0,     0,    85,    86,
       0,     0,     0,     0,     0,    92,     0,    94,     0,    96,
       0,    98,    99,     0,   101,   102,   103,   104,   105,   106,
     107,   108,   109,     0,   111,   112,   113,   251,   114,     0,
     116,   249,   117,   118,   119,     0,   121,   122,     0,     0,
       0,   250,     0,   127,   128,     0,     0,     0,    57,   131,
       0,     0,     0,     0,     0,   214,   221,   232,   240,    23,
      25,    26,     6,   216,   237,     0,   236,     0,     0,    19,
      28,    29,    30,   256,   258,   204,   209,   208,     0,   206,
     194,   260,   262,   195,   189,   190,     0,     0,   139,     0,
      32,   176,     0,     0,   176,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   149,
       0,     0,   149,     0,     0,     0,     0,     0,     0,    62,
      56,    33,     0,    17,    18,     0,     0,    15,    13,    12,
      16,    37,    35,   238,   239,    36,    54,     0,    54,     0,
       0,   229,    19,    54,     0,     0,    31,     0,   141,   142,
     145,   175,    54,     0,     0,     0,   244,   245,   247,   264,
      54,    54,     0,   156,   151,   152,   153,   154,     0,   155,
       0,     0,     0,     0,   174,     0,   172,     0,     0,     0,
      60,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   147,   150,     0,     0,     0,     0,     0,
       0,     0,   267,     0,     0,    58,    62,     0,    14,   215,
       0,   217,   222,     0,     0,     0,    54,     0,     0,    54,
     241,    21,     0,     0,   263,   138,   146,   140,     0,   181,
     252,     0,    55,   177,     0,   243,   242,     0,     0,     0,
     265,    68,     0,   167,   168,   166,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    69,    77,
      76,    80,     0,    82,    83,    84,    87,    88,    89,    90,
      91,    93,    95,     0,   149,   100,   110,     0,   120,   123,
     124,   125,   126,     0,   129,   130,    59,     0,   218,   224,
       0,   223,   226,     0,   227,    19,   233,   132,    20,   143,
     144,   261,   180,   178,   246,   248,   184,     0,   171,     0,
     164,   165,     0,   158,   159,   161,   160,   162,   163,   173,
      61,    97,   148,     0,   268,    34,    48,   225,    54,   133,
       0,   179,     0,   185,   170,   169,     0,   115,   149,   182,
     234,     0,   134,   157,     0,     0,    51,    38,     0,    50,
       0,    49,   253,     0,    43,    39,    41,     0,     0,     0,
       0,   266,   135,    52,     0,   183,     0,     0,    48,     0,
      54,    53,    42,    48,    38,    45,    54,   137,    38,    44,
       0,   136,     0,    47,    46
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     2,   160,   253,   331,    18,    19,    20,   234,
     164,    21,    22,   463,   464,   465,   466,   449,   455,   332,
      86,   157,   291,   158,   440,   458,   472,    24,   257,   258,
     259,    68,   302,   303,   284,   285,   286,    26,   262,   343,
     344,   333,   442,    79,   266,    69,   186,    70,   165,    28,
     237,   246,   321,    30,    31,   248,   326,    32,   178,    33,
      34,   238,   239,   168,    37,   240,   267,   268,   269,   159,
      71,   468,    40,    73,   313
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -454
static const yytype_int16 yypact[] =
{
    -454,    32,   736,  -454,   176,   695,  -103,   190,   207,    51,
      77,   209,   207,   -37,   207,  -454,   849,  -454,  -454,  -454,
    -454,  -454,  -454,    19,   -29,   -22,     0,    38,   -15,  -454,
      43,     9,  -454,    45,     7,  -454,  -454,    29,  -454,    72,
      74,   104,  -454,  -454,  -454,  -454,  -454,  -454,   695,  -454,
     212,  -454,  -454,  -454,   126,  -454,  -454,   126,    76,   126,
      26,  -454,  -454,   222,   231,    26,  -454,  -454,  -454,  -454,
    -454,  -454,   237,  -454,   243,  -454,  -454,  -454,  -454,   107,
     695,   112,  -454,  -454,   115,   695,  -454,   -88,  -454,  -454,
    -454,  -454,  -454,   127,   128,  -454,  -454,  -454,  -454,  -454,
    -454,   129,  -454,  -454,   131,  -454,   132,   135,  -454,  -454,
     136,   137,   138,   140,   145,  -454,   146,  -454,   148,  -454,
     149,  -454,  -454,   153,  -454,  -454,  -454,  -454,  -454,  -454,
    -454,  -454,  -454,   154,  -454,  -454,  -454,  -454,  -454,   166,
    -454,  -454,  -454,  -454,  -454,   168,  -454,  -454,   169,   172,
     173,  -454,   175,  -454,  -454,   179,   180,   -93,  -454,  -454,
     611,   705,   324,   239,   183,  -454,  -454,  -454,  -454,  -454,
    -454,  -454,  -454,  -454,  -454,     6,  -454,   242,   188,  -454,
    -454,  -454,  -454,  -454,   191,  -454,  -454,  -454,   695,  -454,
    -454,   191,   -80,  -454,  -454,  -454,   186,   193,   237,   237,
    -454,  -454,    46,   195,  -454,   237,   446,    58,   335,   336,
     282,   446,   337,   339,   446,   341,   446,   237,   294,   446,
      13,   446,   446,   446,   695,   695,   342,   347,   695,   849,
     208,  -454,   211,  -454,  -454,     8,   215,  -454,  -454,  -454,
    -454,  -454,  -454,  -454,  -454,  -454,    98,   227,   -60,   217,
     216,  -454,  -454,   264,   219,   446,  -454,   218,   236,  -454,
     221,  -454,  -115,   -13,    46,    46,  -454,  -454,  -454,   244,
     -37,   -57,   224,  -454,  -454,  -454,  -454,  -454,   223,  -454,
     446,   446,   446,   490,   556,  -107,  -454,   228,   233,   235,
    -454,   -35,   240,   241,   247,   249,   250,   252,   253,   254,
     258,   371,   -26,  -454,   556,   261,   259,   -23,   170,   265,
     269,   271,   267,   275,   278,  -454,   849,   376,  -454,  -454,
     -17,  -454,  -454,   256,   695,   262,   116,   246,   357,   932,
    -454,  -454,   695,   284,  -454,  -454,  -454,   237,   446,  -454,
    -454,   695,   285,  -454,   286,  -454,  -454,   280,    46,   290,
    -454,  -454,   695,  -454,  -454,  -454,   422,   291,   446,   446,
     446,   446,   446,   446,   446,   446,   446,   446,  -454,  -454,
    -454,  -454,   424,  -454,  -454,  -454,  -454,  -454,  -454,  -454,
    -454,  -454,  -454,   293,   446,  -454,  -454,   446,  -454,  -454,
    -454,  -454,  -454,   439,  -454,  -454,  -454,   298,  -454,  -454,
      46,  -454,  -454,   301,  -454,  -454,  -454,   334,  -454,  -454,
    -454,   303,  -454,  -454,  -454,  -454,   237,   307,  -454,   446,
    -454,  -454,   392,    93,    68,    63,    63,   260,   260,  -454,
    -454,  -454,  -454,   309,  -454,  -454,   300,  -454,   941,  -454,
      46,  -454,   310,  -454,  -454,  -454,   446,  -454,   498,  -454,
    -454,   311,  -454,   556,    50,   -55,  -454,   248,   -18,  -454,
     446,   312,    -5,   313,  -454,   327,  -454,   695,    46,   446,
     330,  -454,  -454,   556,   446,  -454,   361,    46,   -12,   536,
     -30,   556,  -454,    -6,   248,  -454,   -30,  -454,   248,  -454,
     314,  -454,   318,  -454,  -454
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -454,  -454,  -454,   449,  -234,    12,  -454,  -454,   147,  -454,
    -454,  -454,  -454,  -345,  -454,  -454,    -3,  -353,  -454,    -9,
      -2,  -454,  -454,  -208,  -454,  -454,  -454,  -454,  -454,  -454,
     142,     3,   266,  -361,  -210,  -454,  -169,  -454,   276,  -453,
    -224,   155,  -454,    55,   -70,  -454,   -28,    57,    15,  -454,
     485,  -454,  -454,    -7,  -454,  -454,  -454,  -454,  -454,    -8,
    -454,   487,     4,  -454,  -454,   496,   229,  -254,  -454,   287,
       5,    -4,  -454,     1,  -454
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -188
static const yytype_int16 yytable[] =
{
      23,    72,   196,    41,    85,    25,    36,    39,   469,   304,
     346,   347,   304,   308,    17,   167,   166,    27,   329,   263,
     470,   315,   323,   432,   325,   367,     7,   487,   203,   187,
     339,   189,     3,   491,   340,    16,  -187,     4,   288,   229,
     368,   293,   294,    74,   183,   297,   349,   299,     4,   194,
     195,     7,   306,   273,    47,   274,   275,   276,   230,    11,
     204,   273,     7,   274,   275,   276,   287,    84,  -187,    87,
     353,   354,   355,   356,   358,   359,   199,   460,   263,   358,
     359,   202,    11,    80,    11,    81,   335,   456,   339,    54,
      16,   163,   350,    16,   415,    11,   461,   372,   162,   247,
      55,    56,   163,   277,   358,   359,   384,    57,   396,   384,
     137,   277,   373,    16,   141,   339,   169,   190,    84,    87,
      16,   385,   193,   170,   388,   485,   264,   -40,   260,   261,
     489,   471,    59,   173,   484,   272,   151,    62,   448,   490,
     488,   177,   -40,   492,   448,   171,   436,   300,   420,   421,
     422,   423,   424,   425,   426,   427,   428,   175,   235,   235,
     278,    41,    41,    25,    25,    39,    39,   249,   278,   410,
     279,   438,   233,   233,   304,   236,   236,   179,   279,    42,
      43,   358,   359,   172,   254,   264,   451,   280,   174,   281,
     176,   282,   265,    75,    76,   280,   283,   281,   429,   282,
     185,   459,   365,   366,   283,   363,   364,   365,   366,   445,
      77,    78,    82,    83,   478,   184,    78,   180,   433,   181,
     309,   310,   188,   483,   314,   191,    78,   167,   166,   362,
     363,   364,   365,   366,   192,    78,   453,   320,   304,   324,
     194,   195,   243,   244,   354,   250,   251,   319,    16,   182,
     473,   197,    44,   341,    41,   198,    25,   200,    39,   479,
     342,   324,   341,   201,   481,   402,    16,   260,    45,   342,
      46,   358,   359,   205,   206,   207,    47,   208,   209,   357,
      48,   210,   211,   212,   213,   273,   214,   274,   275,   276,
     292,   215,   216,    49,   217,   218,     5,    50,    51,   219,
     220,     6,    52,   360,    53,   361,   362,   363,   364,   365,
     366,    54,   221,     8,   222,   223,     9,   389,   224,   225,
     400,   226,    55,    56,   398,   227,   228,   242,   407,    57,
      41,   245,    25,    10,    39,   277,   252,   400,   255,  -187,
     256,   270,   411,   289,   290,   295,   443,   296,   417,   298,
     301,   311,   312,    58,    59,    60,    61,   317,   316,    62,
     318,   322,   327,    63,   328,    44,   334,   336,   337,   352,
      64,   351,    65,   338,   383,   369,   348,   462,    67,    12,
     370,    45,   371,    46,   397,    13,    14,   374,   375,    47,
     399,   387,   278,    48,   376,   404,   377,   378,    16,   379,
     380,   381,   279,   358,   359,   382,    49,   401,   386,    15,
      50,    51,   390,   330,    16,    52,   391,    53,   392,   280,
     393,   281,   394,   282,    54,   395,    10,   414,   283,   408,
     412,   413,   430,   358,   359,    55,    56,   416,   419,    41,
     431,    25,    57,    39,   434,   435,   437,   439,   441,   273,
     448,   274,   275,   276,   444,   467,   447,   457,   452,   476,
     475,   493,   474,   477,   480,   494,    58,    59,    60,    61,
     161,   341,    62,   482,   467,   405,    63,   341,   342,   409,
     271,   403,   467,    64,   342,    65,   467,    29,   307,    35,
      66,    67,   345,   273,    44,   274,   275,   276,    38,   277,
       0,   273,     0,   274,   275,   276,     0,   305,     0,     0,
      45,    16,    46,     0,     0,     0,     0,     0,    47,     0,
       0,     0,    48,     0,     0,   360,   446,   361,   362,   363,
     364,   365,   366,     0,     0,    49,     0,     0,     0,    50,
      51,     0,     0,   277,    52,     0,    53,   358,   359,     0,
       0,   277,     0,    54,     0,   360,   278,   361,   362,   363,
     364,   365,   366,     0,    55,    56,   279,   358,   359,   418,
       0,    57,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   280,     0,   281,     0,   282,     0,     0,
       0,     0,   283,     0,     0,    58,    59,    60,    61,     0,
     278,    62,     0,     0,     0,    63,     0,     0,   278,     0,
     279,     0,    64,     0,    65,     0,     0,     0,   279,    66,
      67,   231,     0,     0,     0,     0,     0,   280,     0,   281,
       0,   282,     0,     0,     0,   280,   283,   454,     0,   282,
       4,     0,     0,     5,   283,     0,     0,     0,     6,     0,
       0,     0,     0,     0,     7,     0,     0,     0,     0,     0,
       8,     0,     0,     9,     0,     0,     0,     0,     0,   360,
     486,   361,   362,   363,   364,   365,   366,     0,     0,     0,
      10,   232,     0,     0,     0,     0,     0,    11,     0,   360,
       0,   361,   362,   363,   364,   365,   366,     0,     0,    44,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    45,     0,    46,     0,     0,
       0,     0,     0,    47,     0,     0,    12,    48,     0,     0,
       0,     0,    13,    14,     4,     0,     0,     5,     0,     0,
      49,     0,     6,     0,    50,    51,     0,     0,     7,    52,
       0,    53,     0,     0,     8,     0,    15,     9,    54,     0,
       0,    16,     0,     0,     0,     4,     0,     0,     5,    55,
      56,     0,     0,     6,    10,   232,    57,     0,     0,     7,
       0,    11,     0,     0,     0,     8,     0,     0,     9,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      58,    59,    60,    61,     0,    10,    62,     0,     0,     0,
      63,     0,    11,     0,     0,     0,     0,    64,     0,    65,
      12,     0,     0,     0,    66,    67,    13,    14,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      15,    12,     0,     0,   241,    16,     0,    13,    14,     0,
       0,     0,    88,     0,    89,    90,     0,    91,    92,     0,
       0,     0,     0,    93,     0,    94,     0,     0,     0,     0,
       0,    15,    95,    96,    97,    98,    16,    99,   100,   101,
     102,   103,     0,   104,     0,   105,   106,   107,     0,     0,
     108,     0,     0,     0,   109,     0,   110,   111,   112,   113,
     114,   115,     0,   116,   117,   118,   119,   120,     0,     0,
     121,     0,   122,     0,     0,     0,     0,   123,     0,   124,
       0,     0,     0,   125,   126,   127,   128,   129,   130,   131,
     132,   133,     0,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,     0,     0,     0,     0,   145,     0,
       0,   146,     0,   147,     5,     0,   148,   149,   150,     6,
       0,     0,   151,     5,   152,   153,   154,   155,     6,     0,
     156,     8,     0,     0,     9,     0,     0,     0,     0,     0,
       8,     0,     0,     9,     0,     0,     0,     0,     0,     0,
       0,    10,     0,     0,     0,     0,     0,     0,     0,     0,
      10,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    12,     0,     0,
       0,     0,     0,    13,    14,     0,    12,     0,     0,     0,
       0,     0,    13,    14,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    15,     0,     0,
       0,   406,    16,     0,     0,     0,    15,     0,     0,     0,
     450,    16
};

static const yytype_int16 yycheck[] =
{
       2,     5,    72,     2,    13,     2,     2,     2,    26,   219,
     264,   265,   222,   223,     2,    23,    23,     2,   252,    32,
      38,   229,    82,   384,   248,   132,    43,   480,   116,    57,
     145,    59,     0,   486,   149,   150,   116,    29,   207,   132,
     147,   210,   211,   146,    48,   214,   270,   216,    29,     3,
       4,    43,   221,     3,    28,     5,     6,     7,   151,    76,
     148,     3,    43,     5,     6,     7,     8,    12,   148,    14,
     280,   281,   282,   283,    11,    12,    80,   132,    32,    11,
      12,    85,    76,    32,    76,     8,   255,   448,   145,    63,
     150,    83,   149,   150,   348,    76,   151,   132,    79,    93,
      74,    75,    83,    53,    11,    12,   132,    81,   316,   132,
      97,    53,   147,   150,   101,   145,   145,    60,    63,    64,
     150,   147,    65,   145,   147,   478,   139,   132,   198,   199,
     483,   149,   106,   148,   146,   205,   123,   111,   150,   484,
     146,   134,   147,   488,   150,   145,   400,   217,   358,   359,
     360,   361,   362,   363,   364,   365,   366,   148,   160,   161,
     110,   160,   161,   160,   161,   160,   161,   175,   110,   338,
     120,   405,   160,   161,   384,   160,   161,   148,   120,     3,
       4,    11,    12,   145,   188,   139,   440,   137,   145,   139,
     145,   141,   146,     3,     4,   137,   146,   139,   367,   141,
      74,   151,   139,   140,   146,   137,   138,   139,   140,   419,
       3,     4,     3,     4,   468,     3,     4,   145,   387,   145,
     224,   225,   146,   477,   228,     3,     4,   235,   235,   136,
     137,   138,   139,   140,     3,     4,   446,   246,   448,   248,
       3,     4,     3,     4,   454,     3,     4,   149,   150,   145,
     460,     8,     4,   262,   253,   148,   253,   145,   253,   469,
     262,   270,   271,   148,   474,   149,   150,   337,    20,   271,
      22,    11,    12,   146,   146,   146,    28,   146,   146,   283,
      32,   146,   146,   146,   146,     3,   146,     5,     6,     7,
       8,   146,   146,    45,   146,   146,    32,    49,    50,   146,
     146,    37,    54,   133,    56,   135,   136,   137,   138,   139,
     140,    63,   146,    49,   146,   146,    52,   147,   146,   146,
     324,   146,    74,    75,   320,   146,   146,     3,   332,    81,
     329,   148,   329,    69,   329,    53,   148,   341,   152,   148,
     147,   146,   341,     8,     8,     8,   416,     8,   352,     8,
      56,     9,     5,   105,   106,   107,   108,   146,   150,   111,
     145,   134,   145,   115,   148,     4,   147,   149,   132,   146,
     122,   147,   124,   152,     3,   147,   132,   129,   130,   115,
     147,    20,   147,    22,     8,   121,   122,   147,   147,    28,
     134,   132,   110,    32,   147,   149,   147,   147,   150,   147,
     147,   147,   120,    11,    12,   147,    45,   145,   147,   145,
      49,    50,   147,   149,   150,    54,   147,    56,   147,   137,
     153,   139,   147,   141,    63,   147,    69,   147,   146,   145,
     145,   145,     8,    11,    12,    74,    75,   147,   147,   438,
     147,   438,    81,   438,     5,   147,   145,   113,   145,     3,
     150,     5,     6,     7,   147,   457,   147,   146,   148,   132,
     147,   147,   150,   467,   134,   147,   105,   106,   107,   108,
      21,   480,   111,   476,   476,   328,   115,   486,   480,   337,
     204,   326,   484,   122,   486,   124,   488,     2,   222,     2,
     129,   130,   263,     3,     4,     5,     6,     7,     2,    53,
      -1,     3,    -1,     5,     6,     7,    -1,   220,    -1,    -1,
      20,   150,    22,    -1,    -1,    -1,    -1,    -1,    28,    -1,
      -1,    -1,    32,    -1,    -1,   133,   134,   135,   136,   137,
     138,   139,   140,    -1,    -1,    45,    -1,    -1,    -1,    49,
      50,    -1,    -1,    53,    54,    -1,    56,    11,    12,    -1,
      -1,    53,    -1,    63,    -1,   133,   110,   135,   136,   137,
     138,   139,   140,    -1,    74,    75,   120,    11,    12,   147,
      -1,    81,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   137,    -1,   139,    -1,   141,    -1,    -1,
      -1,    -1,   146,    -1,    -1,   105,   106,   107,   108,    -1,
     110,   111,    -1,    -1,    -1,   115,    -1,    -1,   110,    -1,
     120,    -1,   122,    -1,   124,    -1,    -1,    -1,   120,   129,
     130,    10,    -1,    -1,    -1,    -1,    -1,   137,    -1,   139,
      -1,   141,    -1,    -1,    -1,   137,   146,   139,    -1,   141,
      29,    -1,    -1,    32,   146,    -1,    -1,    -1,    37,    -1,
      -1,    -1,    -1,    -1,    43,    -1,    -1,    -1,    -1,    -1,
      49,    -1,    -1,    52,    -1,    -1,    -1,    -1,    -1,   133,
     134,   135,   136,   137,   138,   139,   140,    -1,    -1,    -1,
      69,    70,    -1,    -1,    -1,    -1,    -1,    76,    -1,   133,
      -1,   135,   136,   137,   138,   139,   140,    -1,    -1,     4,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    20,    -1,    22,    -1,    -1,
      -1,    -1,    -1,    28,    -1,    -1,   115,    32,    -1,    -1,
      -1,    -1,   121,   122,    29,    -1,    -1,    32,    -1,    -1,
      45,    -1,    37,    -1,    49,    50,    -1,    -1,    43,    54,
      -1,    56,    -1,    -1,    49,    -1,   145,    52,    63,    -1,
      -1,   150,    -1,    -1,    -1,    29,    -1,    -1,    32,    74,
      75,    -1,    -1,    37,    69,    70,    81,    -1,    -1,    43,
      -1,    76,    -1,    -1,    -1,    49,    -1,    -1,    52,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     105,   106,   107,   108,    -1,    69,   111,    -1,    -1,    -1,
     115,    -1,    76,    -1,    -1,    -1,    -1,   122,    -1,   124,
     115,    -1,    -1,    -1,   129,   130,   121,   122,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     145,   115,    -1,    -1,   149,   150,    -1,   121,   122,    -1,
      -1,    -1,    13,    -1,    15,    16,    -1,    18,    19,    -1,
      -1,    -1,    -1,    24,    -1,    26,    -1,    -1,    -1,    -1,
      -1,   145,    33,    34,    35,    36,   150,    38,    39,    40,
      41,    42,    -1,    44,    -1,    46,    47,    48,    -1,    -1,
      51,    -1,    -1,    -1,    55,    -1,    57,    58,    59,    60,
      61,    62,    -1,    64,    65,    66,    67,    68,    -1,    -1,
      71,    -1,    73,    -1,    -1,    -1,    -1,    78,    -1,    80,
      -1,    -1,    -1,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    -1,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,    -1,    -1,    -1,    -1,   109,    -1,
      -1,   112,    -1,   114,    32,    -1,   117,   118,   119,    37,
      -1,    -1,   123,    32,   125,   126,   127,   128,    37,    -1,
     131,    49,    -1,    -1,    52,    -1,    -1,    -1,    -1,    -1,
      49,    -1,    -1,    52,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    69,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      69,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   115,    -1,    -1,
      -1,    -1,    -1,   121,   122,    -1,   115,    -1,    -1,    -1,
      -1,    -1,   121,   122,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   145,    -1,    -1,
      -1,   149,   150,    -1,    -1,    -1,   145,    -1,    -1,    -1,
     149,   150
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   155,   156,     0,    29,    32,    37,    43,    49,    52,
      69,    76,   115,   121,   122,   145,   150,   159,   160,   161,
     162,   165,   166,   174,   181,   185,   191,   202,   203,   204,
     207,   208,   211,   213,   214,   215,   216,   218,   219,   224,
     226,   227,     3,     4,     4,    20,    22,    28,    32,    45,
      49,    50,    54,    56,    63,    74,    75,    81,   105,   106,
     107,   108,   111,   115,   122,   124,   129,   130,   185,   199,
     201,   224,   225,   227,   146,     3,     4,     3,     4,   197,
      32,     8,     3,     4,   197,   173,   174,   197,    13,    15,
      16,    18,    19,    24,    26,    33,    34,    35,    36,    38,
      39,    40,    41,    42,    44,    46,    47,    48,    51,    55,
      57,    58,    59,    60,    61,    62,    64,    65,    66,    67,
      68,    71,    73,    78,    80,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   109,   112,   114,   117,   118,
     119,   123,   125,   126,   127,   128,   131,   175,   177,   223,
     157,   157,    79,    83,   164,   202,   207,   213,   217,   145,
     145,   145,   145,   148,   145,   148,   145,   134,   212,   148,
     145,   145,   145,   225,     3,    74,   200,   200,   146,   200,
     201,     3,     3,   201,     3,     4,   198,     8,   148,   225,
     145,   148,   225,   116,   148,   146,   146,   146,   146,   146,
     146,   146,   146,   146,   146,   146,   146,   146,   146,   146,
     146,   146,   146,   146,   146,   146,   146,   146,   146,   132,
     151,    10,    70,   159,   163,   174,   202,   204,   215,   216,
     219,   149,     3,     3,     4,   148,   205,    93,   209,   213,
       3,     4,   148,   158,   225,   152,   147,   182,   183,   184,
     198,   198,   192,    32,   139,   146,   198,   220,   221,   222,
     146,   192,   198,     3,     5,     6,     7,    53,   110,   120,
     137,   139,   141,   146,   188,   189,   190,     8,   190,     8,
       8,   176,     8,   190,   190,     8,     8,   190,     8,   190,
     198,    56,   186,   187,   188,   223,   190,   186,   188,   225,
     225,     9,     5,   228,   225,   177,   150,   146,   145,   149,
     173,   206,   134,    82,   173,   194,   210,   145,   148,   158,
     149,   159,   173,   195,   147,   190,   149,   132,   152,   145,
     149,   173,   174,   193,   194,   220,   221,   221,   132,   194,
     149,   147,   146,   188,   188,   188,   188,   225,    11,    12,
     133,   135,   136,   137,   138,   139,   140,   132,   147,   147,
     147,   147,   132,   147,   147,   147,   147,   147,   147,   147,
     147,   147,   147,     3,   132,   147,   147,   132,   147,   147,
     147,   147,   147,   153,   147,   147,   177,     8,   216,   134,
     225,   145,   149,   195,   149,   162,   149,   225,   145,   184,
     190,   227,   145,   145,   147,   221,   147,   225,   147,   147,
     188,   188,   188,   188,   188,   188,   188,   188,   188,   190,
       8,   147,   187,   190,     5,   147,   221,   145,   158,   113,
     178,   145,   196,   198,   147,   188,   134,   147,   150,   171,
     149,   221,   148,   188,   139,   172,   187,   146,   179,   151,
     132,   151,   129,   167,   168,   169,   170,   174,   225,    26,
      38,   149,   180,   188,   150,   147,   132,   225,   221,   188,
     134,   188,   170,   221,   146,   171,   134,   193,   146,   171,
     167,   193,   167,   147,   147
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
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
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
    while (YYID (0))
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
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

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
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
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
      YYSIZE_T yyn = 0;
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

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
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
      int yychecklim = YYLAST - yyn + 1;
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
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
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
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
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
  YYUSE (yyvaluep);

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
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
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
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

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
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

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
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;


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
	yytype_int16 *yyss1 = yyss;
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

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

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

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

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
#line 281 "parser.y"
    { fix_incomplete();
						  check_all_user_types((yyvsp[(1) - (1)].ifref_list));
						  write_proxies((yyvsp[(1) - (1)].ifref_list));
						  write_client((yyvsp[(1) - (1)].ifref_list));
						  write_server((yyvsp[(1) - (1)].ifref_list));
						;}
    break;

  case 3:
#line 289 "parser.y"
    { (yyval.ifref_list) = NULL; ;}
    break;

  case 4:
#line 290 "parser.y"
    { (yyval.ifref_list) = (yyvsp[(1) - (2)].ifref_list); ;}
    break;

  case 5:
#line 291 "parser.y"
    { (yyval.ifref_list) = append_ifref( (yyvsp[(1) - (2)].ifref_list), make_ifref((yyvsp[(2) - (2)].type)) ); ;}
    break;

  case 6:
#line 292 "parser.y"
    { (yyval.ifref_list) = (yyvsp[(1) - (3)].ifref_list);
						  reg_type((yyvsp[(2) - (3)].type), (yyvsp[(2) - (3)].type)->name, 0);
						  if (!parse_only && do_header) write_coclass_forward((yyvsp[(2) - (3)].type));
						;}
    break;

  case 7:
#line 296 "parser.y"
    { (yyval.ifref_list) = (yyvsp[(1) - (2)].ifref_list);
						  add_typelib_entry((yyvsp[(2) - (2)].type));
						  reg_type((yyvsp[(2) - (2)].type), (yyvsp[(2) - (2)].type)->name, 0);
						  if (!parse_only && do_header) write_coclass_forward((yyvsp[(2) - (2)].type));
						;}
    break;

  case 8:
#line 301 "parser.y"
    { (yyval.ifref_list) = (yyvsp[(1) - (2)].ifref_list); add_typelib_entry((yyvsp[(2) - (2)].type)); ;}
    break;

  case 9:
#line 302 "parser.y"
    { (yyval.ifref_list) = (yyvsp[(1) - (2)].ifref_list); ;}
    break;

  case 10:
#line 303 "parser.y"
    { (yyval.ifref_list) = (yyvsp[(1) - (2)].ifref_list); ;}
    break;

  case 11:
#line 306 "parser.y"
    {;}
    break;

  case 12:
#line 307 "parser.y"
    { if (!parse_only) add_typelib_entry((yyvsp[(2) - (2)].type)); ;}
    break;

  case 13:
#line 308 "parser.y"
    { if (!parse_only) add_typelib_entry((yyvsp[(2) - (2)].type)); ;}
    break;

  case 14:
#line 309 "parser.y"
    { reg_type((yyvsp[(2) - (3)].type), (yyvsp[(2) - (3)].type)->name, 0); if (!parse_only && do_header) write_coclass_forward((yyvsp[(2) - (3)].type)); ;}
    break;

  case 15:
#line 310 "parser.y"
    { if (!parse_only) add_typelib_entry((yyvsp[(2) - (2)].type));
						  reg_type((yyvsp[(2) - (2)].type), (yyvsp[(2) - (2)].type)->name, 0);
						  if (!parse_only && do_header) write_coclass_forward((yyvsp[(2) - (2)].type));
						;}
    break;

  case 16:
#line 314 "parser.y"
    { if (!parse_only) add_typelib_entry((yyvsp[(2) - (2)].type)); ;}
    break;

  case 17:
#line 315 "parser.y"
    {;}
    break;

  case 18:
#line 316 "parser.y"
    {;}
    break;

  case 19:
#line 319 "parser.y"
    { (yyval.func_list) = NULL; ;}
    break;

  case 20:
#line 320 "parser.y"
    { (yyval.func_list) = append_func( (yyvsp[(1) - (3)].func_list), (yyvsp[(2) - (3)].func) ); ;}
    break;

  case 21:
#line 321 "parser.y"
    { (yyval.func_list) = (yyvsp[(1) - (2)].func_list); ;}
    break;

  case 22:
#line 324 "parser.y"
    {;}
    break;

  case 23:
#line 325 "parser.y"
    { if (!parse_only && do_header) { write_constdef((yyvsp[(1) - (2)].var)); } ;}
    break;

  case 24:
#line 326 "parser.y"
    {;}
    break;

  case 25:
#line 327 "parser.y"
    { if (!parse_only && do_header) {
						    write_type(header, (yyvsp[(1) - (2)].type), FALSE, NULL);
						    fprintf(header, ";\n\n");
						  }
						;}
    break;

  case 26:
#line 332 "parser.y"
    { if (!parse_only && do_header) { write_externdef((yyvsp[(1) - (2)].var)); } ;}
    break;

  case 27:
#line 333 "parser.y"
    {;}
    break;

  case 28:
#line 334 "parser.y"
    { if (!parse_only && do_header) {
						    write_type(header, (yyvsp[(1) - (2)].type), FALSE, NULL);
						    fprintf(header, ";\n\n");
						  }
						;}
    break;

  case 29:
#line 339 "parser.y"
    {;}
    break;

  case 30:
#line 340 "parser.y"
    { if (!parse_only && do_header) {
						    write_type(header, (yyvsp[(1) - (2)].type), FALSE, NULL);
						    fprintf(header, ";\n\n");
						  }
						;}
    break;

  case 31:
#line 347 "parser.y"
    { if (!parse_only && do_header) fprintf(header, "%s\n", (yyvsp[(3) - (4)].str)); ;}
    break;

  case 32:
#line 349 "parser.y"
    { assert(yychar == YYEMPTY);
						  if (!do_import((yyvsp[(2) - (3)].str))) yychar = aEOF; ;}
    break;

  case 33:
#line 352 "parser.y"
    {;}
    break;

  case 34:
#line 355 "parser.y"
    { if(!parse_only) add_importlib((yyvsp[(3) - (4)].str)); ;}
    break;

  case 35:
#line 358 "parser.y"
    { (yyval.str) = (yyvsp[(2) - (2)].str); ;}
    break;

  case 36:
#line 360 "parser.y"
    { start_typelib((yyvsp[(2) - (3)].str), (yyvsp[(1) - (3)].attr_list));
						  if (!parse_only && do_header) write_library((yyvsp[(2) - (3)].str), (yyvsp[(1) - (3)].attr_list));
						  if (!parse_only && do_idfile) write_libid((yyvsp[(2) - (3)].str), (yyvsp[(1) - (3)].attr_list));
						;}
    break;

  case 37:
#line 365 "parser.y"
    { end_typelib(); ;}
    break;

  case 38:
#line 368 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 40:
#line 372 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 41:
#line 375 "parser.y"
    { check_arg((yyvsp[(1) - (1)].var)); (yyval.var_list) = append_var( NULL, (yyvsp[(1) - (1)].var) ); ;}
    break;

  case 42:
#line 376 "parser.y"
    { check_arg((yyvsp[(3) - (3)].var)); (yyval.var_list) = append_var( (yyvsp[(1) - (3)].var_list), (yyvsp[(3) - (3)].var)); ;}
    break;

  case 44:
#line 381 "parser.y"
    { (yyval.var) = (yyvsp[(3) - (4)].pident)->var;
						  (yyval.var)->attrs = (yyvsp[(1) - (4)].attr_list);
						  set_type((yyval.var), (yyvsp[(2) - (4)].type), (yyvsp[(3) - (4)].pident)->ptr_level, (yyvsp[(4) - (4)].array_dims));
						  free((yyvsp[(3) - (4)].pident));
						;}
    break;

  case 45:
#line 386 "parser.y"
    { (yyval.var) = (yyvsp[(2) - (3)].pident)->var;
						  set_type((yyval.var), (yyvsp[(1) - (3)].type), (yyvsp[(2) - (3)].pident)->ptr_level, (yyvsp[(3) - (3)].array_dims));
						  free((yyvsp[(2) - (3)].pident));
						;}
    break;

  case 46:
#line 390 "parser.y"
    { (yyval.var) = (yyvsp[(3) - (6)].pident)->var;
						  (yyval.var)->attrs = (yyvsp[(1) - (6)].attr_list);
						  set_type((yyval.var), (yyvsp[(2) - (6)].type), (yyvsp[(3) - (6)].pident)->ptr_level - 1, NULL);
						  free((yyvsp[(3) - (6)].pident));
						  (yyval.var)->args = (yyvsp[(5) - (6)].var_list);
						;}
    break;

  case 47:
#line 396 "parser.y"
    { (yyval.var) = (yyvsp[(2) - (5)].pident)->var;
						  set_type((yyval.var), (yyvsp[(1) - (5)].type), (yyvsp[(2) - (5)].pident)->ptr_level - 1, NULL);
						  free((yyvsp[(2) - (5)].pident));
						  (yyval.var)->args = (yyvsp[(4) - (5)].var_list);
						;}
    break;

  case 48:
#line 403 "parser.y"
    { (yyval.array_dims) = NULL; ;}
    break;

  case 49:
#line 404 "parser.y"
    { (yyval.array_dims) = (yyvsp[(2) - (3)].array_dims); ;}
    break;

  case 50:
#line 405 "parser.y"
    { (yyval.array_dims) = append_array( NULL, make_expr(EXPR_VOID) ); ;}
    break;

  case 51:
#line 408 "parser.y"
    { (yyval.array_dims) = append_array( NULL, (yyvsp[(1) - (1)].expr) ); ;}
    break;

  case 52:
#line 409 "parser.y"
    { (yyval.array_dims) = append_array( (yyvsp[(1) - (3)].array_dims), (yyvsp[(3) - (3)].expr) ); ;}
    break;

  case 53:
#line 410 "parser.y"
    { (yyval.array_dims) = append_array( (yyvsp[(1) - (4)].array_dims), (yyvsp[(4) - (4)].expr) ); ;}
    break;

  case 54:
#line 413 "parser.y"
    { (yyval.attr_list) = NULL; ;}
    break;

  case 56:
#line 418 "parser.y"
    { (yyval.attr_list) = (yyvsp[(2) - (3)].attr_list);
						  if (!(yyval.attr_list))
						    yyerror("empty attribute lists unsupported");
						;}
    break;

  case 57:
#line 424 "parser.y"
    { (yyval.attr_list) = append_attr( NULL, (yyvsp[(1) - (1)].attr) ); ;}
    break;

  case 58:
#line 425 "parser.y"
    { (yyval.attr_list) = append_attr( (yyvsp[(1) - (3)].attr_list), (yyvsp[(3) - (3)].attr) ); ;}
    break;

  case 59:
#line 426 "parser.y"
    { (yyval.attr_list) = append_attr( (yyvsp[(1) - (4)].attr_list), (yyvsp[(4) - (4)].attr) ); ;}
    break;

  case 60:
#line 429 "parser.y"
    { (yyval.str_list) = append_str( NULL, (yyvsp[(1) - (1)].str) ); ;}
    break;

  case 61:
#line 430 "parser.y"
    { (yyval.str_list) = append_str( (yyvsp[(1) - (3)].str_list), (yyvsp[(3) - (3)].str) ); ;}
    break;

  case 62:
#line 433 "parser.y"
    { (yyval.attr) = NULL; ;}
    break;

  case 63:
#line 434 "parser.y"
    { (yyval.attr) = make_attr(ATTR_AGGREGATABLE); ;}
    break;

  case 64:
#line 435 "parser.y"
    { (yyval.attr) = make_attr(ATTR_APPOBJECT); ;}
    break;

  case 65:
#line 436 "parser.y"
    { (yyval.attr) = make_attr(ATTR_ASYNC); ;}
    break;

  case 66:
#line 437 "parser.y"
    { (yyval.attr) = make_attr(ATTR_AUTO_HANDLE); ;}
    break;

  case 67:
#line 438 "parser.y"
    { (yyval.attr) = make_attr(ATTR_BINDABLE); ;}
    break;

  case 68:
#line 439 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_CALLAS, (yyvsp[(3) - (4)].var)); ;}
    break;

  case 69:
#line 440 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_CASE, (yyvsp[(3) - (4)].expr_list)); ;}
    break;

  case 70:
#line 441 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); ;}
    break;

  case 71:
#line 442 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_DONT_SERIALIZE */ ;}
    break;

  case 72:
#line 443 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_SERIALIZE */ ;}
    break;

  case 73:
#line 444 "parser.y"
    { (yyval.attr) = make_attr(ATTR_CONTROL); ;}
    break;

  case 74:
#line 445 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULT); ;}
    break;

  case 75:
#line 446 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULTCOLLELEM); ;}
    break;

  case 76:
#line 447 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_DEFAULTVALUE_EXPR, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 77:
#line 448 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_DEFAULTVALUE_STRING, (yyvsp[(3) - (4)].str)); ;}
    break;

  case 78:
#line 449 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULTVTABLE); ;}
    break;

  case 79:
#line 450 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DISPLAYBIND); ;}
    break;

  case 80:
#line 451 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_DLLNAME, (yyvsp[(3) - (4)].str)); ;}
    break;

  case 81:
#line 452 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DUAL); ;}
    break;

  case 82:
#line 453 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ENDPOINT, (yyvsp[(3) - (4)].str_list)); ;}
    break;

  case 83:
#line 454 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ENTRY_STRING, (yyvsp[(3) - (4)].str)); ;}
    break;

  case 84:
#line 455 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ENTRY_ORDINAL, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 85:
#line 456 "parser.y"
    { (yyval.attr) = make_attr(ATTR_EXPLICIT_HANDLE); ;}
    break;

  case 86:
#line 457 "parser.y"
    { (yyval.attr) = make_attr(ATTR_HANDLE); ;}
    break;

  case 87:
#line 458 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPCONTEXT, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 88:
#line 459 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPFILE, (yyvsp[(3) - (4)].str)); ;}
    break;

  case 89:
#line 460 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRING, (yyvsp[(3) - (4)].str)); ;}
    break;

  case 90:
#line 461 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRINGCONTEXT, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 91:
#line 462 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRINGDLL, (yyvsp[(3) - (4)].str)); ;}
    break;

  case 92:
#line 463 "parser.y"
    { (yyval.attr) = make_attr(ATTR_HIDDEN); ;}
    break;

  case 93:
#line 464 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ID, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 94:
#line 465 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IDEMPOTENT); ;}
    break;

  case 95:
#line 466 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_IIDIS, (yyvsp[(3) - (4)].var)); ;}
    break;

  case 96:
#line 467 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IMMEDIATEBIND); ;}
    break;

  case 97:
#line 468 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_IMPLICIT_HANDLE, (yyvsp[(4) - (5)].str)); ;}
    break;

  case 98:
#line 469 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IN); ;}
    break;

  case 99:
#line 470 "parser.y"
    { (yyval.attr) = make_attr(ATTR_INPUTSYNC); ;}
    break;

  case 100:
#line 471 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_LENGTHIS, (yyvsp[(3) - (4)].expr_list)); ;}
    break;

  case 101:
#line 472 "parser.y"
    { (yyval.attr) = make_attr(ATTR_LOCAL); ;}
    break;

  case 102:
#line 473 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONBROWSABLE); ;}
    break;

  case 103:
#line 474 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONCREATABLE); ;}
    break;

  case 104:
#line 475 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONEXTENSIBLE); ;}
    break;

  case 105:
#line 476 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OBJECT); ;}
    break;

  case 106:
#line 477 "parser.y"
    { (yyval.attr) = make_attr(ATTR_ODL); ;}
    break;

  case 107:
#line 478 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OLEAUTOMATION); ;}
    break;

  case 108:
#line 479 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OPTIONAL); ;}
    break;

  case 109:
#line 480 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OUT); ;}
    break;

  case 110:
#line 481 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_POINTERDEFAULT, (yyvsp[(3) - (4)].num)); ;}
    break;

  case 111:
#line 482 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPGET); ;}
    break;

  case 112:
#line 483 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPPUT); ;}
    break;

  case 113:
#line 484 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPPUTREF); ;}
    break;

  case 114:
#line 485 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PUBLIC); ;}
    break;

  case 115:
#line 486 "parser.y"
    { expr_list_t *list = append_expr( NULL, (yyvsp[(3) - (6)].expr) );
                                                     list = append_expr( list, (yyvsp[(5) - (6)].expr) );
                                                     (yyval.attr) = make_attrp(ATTR_RANGE, list); ;}
    break;

  case 116:
#line 489 "parser.y"
    { (yyval.attr) = make_attr(ATTR_READONLY); ;}
    break;

  case 117:
#line 490 "parser.y"
    { (yyval.attr) = make_attr(ATTR_REQUESTEDIT); ;}
    break;

  case 118:
#line 491 "parser.y"
    { (yyval.attr) = make_attr(ATTR_RESTRICTED); ;}
    break;

  case 119:
#line 492 "parser.y"
    { (yyval.attr) = make_attr(ATTR_RETVAL); ;}
    break;

  case 120:
#line 493 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SIZEIS, (yyvsp[(3) - (4)].expr_list)); ;}
    break;

  case 121:
#line 494 "parser.y"
    { (yyval.attr) = make_attr(ATTR_SOURCE); ;}
    break;

  case 122:
#line 495 "parser.y"
    { (yyval.attr) = make_attr(ATTR_STRING); ;}
    break;

  case 123:
#line 496 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SWITCHIS, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 124:
#line 497 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SWITCHTYPE, (yyvsp[(3) - (4)].type)); ;}
    break;

  case 125:
#line 498 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_TRANSMITAS, (yyvsp[(3) - (4)].type)); ;}
    break;

  case 126:
#line 499 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_UUID, (yyvsp[(3) - (4)].uuid)); ;}
    break;

  case 127:
#line 500 "parser.y"
    { (yyval.attr) = make_attr(ATTR_V1ENUM); ;}
    break;

  case 128:
#line 501 "parser.y"
    { (yyval.attr) = make_attr(ATTR_VARARG); ;}
    break;

  case 129:
#line 502 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_VERSION, (yyvsp[(3) - (4)].num)); ;}
    break;

  case 130:
#line 503 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_WIREMARSHAL, (yyvsp[(3) - (4)].type)); ;}
    break;

  case 131:
#line 504 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_POINTERTYPE, (yyvsp[(1) - (1)].num)); ;}
    break;

  case 134:
#line 511 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 135:
#line 512 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[(1) - (2)].var_list), (yyvsp[(2) - (2)].var) ); ;}
    break;

  case 136:
#line 515 "parser.y"
    { attr_t *a = make_attrp(ATTR_CASE, append_expr( NULL, (yyvsp[(2) - (4)].expr) ));
						  (yyval.var) = (yyvsp[(4) - (4)].var); if (!(yyval.var)) (yyval.var) = make_var(NULL);
						  (yyval.var)->attrs = append_attr( (yyval.var)->attrs, a );
						;}
    break;

  case 137:
#line 519 "parser.y"
    { attr_t *a = make_attr(ATTR_DEFAULT);
						  (yyval.var) = (yyvsp[(3) - (3)].var); if (!(yyval.var)) (yyval.var) = make_var(NULL);
						  (yyval.var)->attrs = append_attr( (yyval.var)->attrs, a );
						;}
    break;

  case 138:
#line 525 "parser.y"
    { (yyval.var) = reg_const((yyvsp[(3) - (5)].var));
						  set_type((yyval.var), (yyvsp[(2) - (5)].type), 0, NULL);
						  (yyval.var)->eval = (yyvsp[(5) - (5)].expr);
						;}
    break;

  case 139:
#line 531 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 140:
#line 532 "parser.y"
    { (yyval.var_list) = (yyvsp[(1) - (2)].var_list); ;}
    break;

  case 142:
#line 536 "parser.y"
    { if (!(yyvsp[(1) - (1)].var)->eval)
						    (yyvsp[(1) - (1)].var)->eval = make_exprl(EXPR_NUM, 0 /* default for first enum entry */);
                                                  (yyval.var_list) = append_var( NULL, (yyvsp[(1) - (1)].var) );
						;}
    break;

  case 143:
#line 540 "parser.y"
    { if (!(yyvsp[(3) - (3)].var)->eval)
                                                  {
                                                    var_t *last = LIST_ENTRY( list_tail((yyval.var_list)), var_t, entry );
                                                    (yyvsp[(3) - (3)].var)->eval = make_exprl(EXPR_NUM, last->eval->cval + 1);
                                                  }
                                                  (yyval.var_list) = append_var( (yyvsp[(1) - (3)].var_list), (yyvsp[(3) - (3)].var) );
						;}
    break;

  case 144:
#line 549 "parser.y"
    { (yyval.var) = reg_const((yyvsp[(1) - (3)].var));
						  (yyval.var)->eval = (yyvsp[(3) - (3)].expr);
                                                  (yyval.var)->type = make_int(0);
						;}
    break;

  case 145:
#line 553 "parser.y"
    { (yyval.var) = reg_const((yyvsp[(1) - (1)].var));
                                                  (yyval.var)->type = make_int(0);
						;}
    break;

  case 146:
#line 558 "parser.y"
    { (yyval.type) = get_typev(RPC_FC_ENUM16, (yyvsp[(2) - (5)].var), tsENUM);
						  (yyval.type)->kind = TKIND_ENUM;
						  (yyval.type)->fields = (yyvsp[(4) - (5)].var_list);
						  (yyval.type)->defined = TRUE;
                                                  if(in_typelib)
                                                      add_typelib_entry((yyval.type));
						;}
    break;

  case 147:
#line 567 "parser.y"
    { (yyval.expr_list) = append_expr( NULL, (yyvsp[(1) - (1)].expr) ); ;}
    break;

  case 148:
#line 568 "parser.y"
    { (yyval.expr_list) = append_expr( (yyvsp[(1) - (3)].expr_list), (yyvsp[(3) - (3)].expr) ); ;}
    break;

  case 149:
#line 581 "parser.y"
    { (yyval.expr) = make_expr(EXPR_VOID); ;}
    break;

  case 151:
#line 585 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_NUM, (yyvsp[(1) - (1)].num)); ;}
    break;

  case 152:
#line 586 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_HEXNUM, (yyvsp[(1) - (1)].num)); ;}
    break;

  case 153:
#line 587 "parser.y"
    { (yyval.expr) = make_exprd(EXPR_DOUBLE, (yyvsp[(1) - (1)].dbl)); ;}
    break;

  case 154:
#line 588 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_TRUEFALSE, 0); ;}
    break;

  case 155:
#line 589 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_TRUEFALSE, 1); ;}
    break;

  case 156:
#line 590 "parser.y"
    { (yyval.expr) = make_exprs(EXPR_IDENTIFIER, (yyvsp[(1) - (1)].str)); ;}
    break;

  case 157:
#line 591 "parser.y"
    { (yyval.expr) = make_expr3(EXPR_COND, (yyvsp[(1) - (5)].expr), (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].expr)); ;}
    break;

  case 158:
#line 592 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_OR , (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 159:
#line 593 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_AND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 160:
#line 594 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_ADD, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 161:
#line 595 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SUB, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 162:
#line 596 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MUL, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 163:
#line 597 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_DIV, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 164:
#line 598 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SHL, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 165:
#line 599 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SHR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 166:
#line 600 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_NOT, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 167:
#line 601 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_NEG, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 168:
#line 602 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_PPTR, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 169:
#line 603 "parser.y"
    { (yyval.expr) = make_exprt(EXPR_CAST, (yyvsp[(2) - (4)].type), (yyvsp[(4) - (4)].expr)); ;}
    break;

  case 170:
#line 604 "parser.y"
    { (yyval.expr) = make_exprt(EXPR_SIZEOF, (yyvsp[(3) - (4)].type), NULL); ;}
    break;

  case 171:
#line 605 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (3)].expr); ;}
    break;

  case 172:
#line 608 "parser.y"
    { (yyval.expr_list) = append_expr( NULL, (yyvsp[(1) - (1)].expr) ); ;}
    break;

  case 173:
#line 609 "parser.y"
    { (yyval.expr_list) = append_expr( (yyvsp[(1) - (3)].expr_list), (yyvsp[(3) - (3)].expr) ); ;}
    break;

  case 174:
#line 612 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr);
						  if (!(yyval.expr)->is_const)
						      yyerror("expression is not constant");
						;}
    break;

  case 175:
#line 618 "parser.y"
    { (yyval.var) = (yyvsp[(4) - (4)].var);
						  set_type((yyval.var), (yyvsp[(3) - (4)].type), 0, NULL);
						;}
    break;

  case 176:
#line 623 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 177:
#line 624 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[(1) - (2)].var_list), (yyvsp[(2) - (2)].var) ); ;}
    break;

  case 178:
#line 627 "parser.y"
    { (yyval.var) = (yyvsp[(1) - (2)].var); ;}
    break;

  case 179:
#line 628 "parser.y"
    { (yyval.var) = make_var(NULL); (yyval.var)->type = (yyvsp[(2) - (3)].type); (yyval.var)->attrs = (yyvsp[(1) - (3)].attr_list); ;}
    break;

  case 180:
#line 629 "parser.y"
    { (yyval.var) = make_var(NULL); (yyval.var)->attrs = (yyvsp[(1) - (2)].attr_list); ;}
    break;

  case 181:
#line 630 "parser.y"
    { (yyval.var) = NULL; ;}
    break;

  case 182:
#line 633 "parser.y"
    { (yyval.var) = (yyvsp[(3) - (4)].pident)->var;
						  (yyval.var)->attrs = (yyvsp[(1) - (4)].attr_list);
						  set_type((yyval.var), (yyvsp[(2) - (4)].type), (yyvsp[(3) - (4)].pident)->ptr_level, (yyvsp[(4) - (4)].array_dims));
						  free((yyvsp[(3) - (4)].pident));
						;}
    break;

  case 183:
#line 642 "parser.y"
    { var_t *v = (yyvsp[(4) - (7)].pident)->var;
						  v->attrs = (yyvsp[(1) - (7)].attr_list);
						  set_type(v, (yyvsp[(2) - (7)].type), (yyvsp[(4) - (7)].pident)->ptr_level, NULL);
						  free((yyvsp[(4) - (7)].pident));
						  (yyval.func) = make_func(v, (yyvsp[(6) - (7)].var_list));
						  if (is_attr(v->attrs, ATTR_IN)) {
						    yyerror("inapplicable attribute [in] for function '%s'",(yyval.func)->def->name);
						  }
						;}
    break;

  case 184:
#line 653 "parser.y"
    { (yyval.var) = NULL; ;}
    break;

  case 186:
#line 657 "parser.y"
    { (yyval.var) = NULL; ;}
    break;

  case 187:
#line 658 "parser.y"
    { (yyval.var) = make_var((yyvsp[(1) - (1)].str)); ;}
    break;

  case 188:
#line 659 "parser.y"
    { (yyval.var) = make_var((yyvsp[(1) - (1)].str)); ;}
    break;

  case 189:
#line 662 "parser.y"
    { (yyval.var) = make_var((yyvsp[(1) - (1)].str)); ;}
    break;

  case 190:
#line 664 "parser.y"
    { (yyval.var) = make_var((yyvsp[(1) - (1)].str)); ;}
    break;

  case 191:
#line 667 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[(1) - (1)].str)); ;}
    break;

  case 192:
#line 668 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[(1) - (1)].str)); ;}
    break;

  case 194:
#line 670 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type); (yyval.type)->sign = 1; ;}
    break;

  case 195:
#line 671 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type); (yyval.type)->sign = -1;
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

  case 196:
#line 687 "parser.y"
    { (yyval.type) = make_int(-1); ;}
    break;

  case 197:
#line 688 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[(1) - (1)].str)); ;}
    break;

  case 198:
#line 689 "parser.y"
    { (yyval.type) = duptype(find_type("float", 0), 1); ;}
    break;

  case 199:
#line 690 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[(1) - (1)].str)); ;}
    break;

  case 200:
#line 691 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[(1) - (1)].str)); ;}
    break;

  case 201:
#line 692 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[(1) - (1)].str)); ;}
    break;

  case 202:
#line 693 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[(1) - (1)].str)); ;}
    break;

  case 205:
#line 700 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[(1) - (1)].str)); ;}
    break;

  case 206:
#line 701 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[(1) - (2)].str)); ;}
    break;

  case 207:
#line 702 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[(1) - (1)].str)); ;}
    break;

  case 208:
#line 703 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[(1) - (2)].str)); ;}
    break;

  case 209:
#line 704 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[(1) - (2)].str)); ;}
    break;

  case 210:
#line 705 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[(1) - (1)].str)); ;}
    break;

  case 211:
#line 706 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[(1) - (1)].str)); ;}
    break;

  case 212:
#line 709 "parser.y"
    { (yyval.type) = make_class((yyvsp[(2) - (2)].str)); ;}
    break;

  case 213:
#line 710 "parser.y"
    { (yyval.type) = find_type((yyvsp[(2) - (2)].str), 0);
						  if ((yyval.type)->defined) yyerror("multiple definition error");
						  if ((yyval.type)->kind != TKIND_COCLASS) yyerror("%s was not declared a coclass", (yyvsp[(2) - (2)].str));
						;}
    break;

  case 214:
#line 716 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type);
						  (yyval.type)->attrs = (yyvsp[(1) - (2)].attr_list);
						  if (!parse_only && do_header)
						    write_coclass((yyval.type));
						  if (!parse_only && do_idfile)
						    write_clsid((yyval.type));
						;}
    break;

  case 215:
#line 725 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (4)].type);
						  (yyval.type)->ifaces = (yyvsp[(3) - (4)].ifref_list);
						  (yyval.type)->defined = TRUE;
						;}
    break;

  case 216:
#line 731 "parser.y"
    { (yyval.ifref_list) = NULL; ;}
    break;

  case 217:
#line 732 "parser.y"
    { (yyval.ifref_list) = append_ifref( (yyvsp[(1) - (2)].ifref_list), (yyvsp[(2) - (2)].ifref) ); ;}
    break;

  case 218:
#line 736 "parser.y"
    { (yyval.ifref) = make_ifref((yyvsp[(2) - (2)].type)); (yyval.ifref)->attrs = (yyvsp[(1) - (2)].attr_list); ;}
    break;

  case 219:
#line 739 "parser.y"
    { (yyval.type) = get_type(0, (yyvsp[(2) - (2)].str), 0); (yyval.type)->kind = TKIND_DISPATCH; ;}
    break;

  case 220:
#line 740 "parser.y"
    { (yyval.type) = get_type(0, (yyvsp[(2) - (2)].str), 0); (yyval.type)->kind = TKIND_DISPATCH; ;}
    break;

  case 221:
#line 743 "parser.y"
    { attr_t *attrs;
						  (yyval.type) = (yyvsp[(2) - (2)].type);
						  if ((yyval.type)->defined) yyerror("multiple definition error");
						  attrs = make_attr(ATTR_DISPINTERFACE);
						  (yyval.type)->attrs = append_attr( (yyvsp[(1) - (2)].attr_list), attrs );
						  (yyval.type)->ref = find_type("IDispatch", 0);
						  if (!(yyval.type)->ref) yyerror("IDispatch is undefined");
						  (yyval.type)->defined = TRUE;
						  if (!parse_only && do_header) write_forward((yyval.type));
						;}
    break;

  case 222:
#line 755 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 223:
#line 756 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[(1) - (3)].var_list), (yyvsp[(2) - (3)].var) ); ;}
    break;

  case 224:
#line 759 "parser.y"
    { (yyval.func_list) = NULL; ;}
    break;

  case 225:
#line 760 "parser.y"
    { (yyval.func_list) = append_func( (yyvsp[(1) - (3)].func_list), (yyvsp[(2) - (3)].func) ); ;}
    break;

  case 226:
#line 766 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (5)].type);
						  (yyval.type)->fields = (yyvsp[(3) - (5)].var_list);
						  (yyval.type)->funcs = (yyvsp[(4) - (5)].func_list);
						  if (!parse_only && do_header) write_dispinterface((yyval.type));
						  if (!parse_only && do_idfile) write_diid((yyval.type));
						;}
    break;

  case 227:
#line 773 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (5)].type);
						  (yyval.type)->fields = (yyvsp[(3) - (5)].type)->fields;
						  (yyval.type)->funcs = (yyvsp[(3) - (5)].type)->funcs;
						  if (!parse_only && do_header) write_dispinterface((yyval.type));
						  if (!parse_only && do_idfile) write_diid((yyval.type));
						;}
    break;

  case 228:
#line 781 "parser.y"
    { (yyval.type) = NULL; ;}
    break;

  case 229:
#line 782 "parser.y"
    { (yyval.type) = find_type2((yyvsp[(2) - (2)].str), 0); ;}
    break;

  case 230:
#line 785 "parser.y"
    { (yyval.type) = get_type(RPC_FC_IP, (yyvsp[(2) - (2)].str), 0); (yyval.type)->kind = TKIND_INTERFACE; ;}
    break;

  case 231:
#line 786 "parser.y"
    { (yyval.type) = get_type(RPC_FC_IP, (yyvsp[(2) - (2)].str), 0); (yyval.type)->kind = TKIND_INTERFACE; ;}
    break;

  case 232:
#line 789 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type);
						  if ((yyval.type)->defined) yyerror("multiple definition error");
						  (yyval.type)->attrs = (yyvsp[(1) - (2)].attr_list);
						  (yyval.type)->defined = TRUE;
						  if (!parse_only && do_header) write_forward((yyval.type));
						;}
    break;

  case 233:
#line 798 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (5)].type);
						  (yyval.type)->ref = (yyvsp[(2) - (5)].type);
						  (yyval.type)->funcs = (yyvsp[(4) - (5)].func_list);
						  compute_method_indexes((yyval.type));
						  if (!parse_only && do_header) write_interface((yyval.type));
						  if (!parse_only && do_idfile) write_iid((yyval.type));
						;}
    break;

  case 234:
#line 808 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (7)].type);
						  (yyval.type)->ref = find_type2((yyvsp[(3) - (7)].str), 0);
						  if (!(yyval.type)->ref) yyerror("base class '%s' not found in import", (yyvsp[(3) - (7)].str));
						  (yyval.type)->funcs = (yyvsp[(6) - (7)].func_list);
						  compute_method_indexes((yyval.type));
						  if (!parse_only && do_header) write_interface((yyval.type));
						  if (!parse_only && do_idfile) write_iid((yyval.type));
						;}
    break;

  case 235:
#line 816 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (1)].type); ;}
    break;

  case 236:
#line 820 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (2)].type); if (!parse_only && do_header) write_forward((yyval.type)); ;}
    break;

  case 237:
#line 821 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (2)].type); if (!parse_only && do_header) write_forward((yyval.type)); ;}
    break;

  case 238:
#line 824 "parser.y"
    { (yyval.type) = make_type(0, NULL); (yyval.type)->name = (yyvsp[(2) - (2)].str); (yyval.type)->kind = TKIND_MODULE; ;}
    break;

  case 239:
#line 825 "parser.y"
    { (yyval.type) = make_type(0, NULL); (yyval.type)->name = (yyvsp[(2) - (2)].str); (yyval.type)->kind = TKIND_MODULE; ;}
    break;

  case 240:
#line 828 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type);
						  (yyval.type)->attrs = (yyvsp[(1) - (2)].attr_list);
						;}
    break;

  case 241:
#line 833 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (4)].type);
						  (yyval.type)->funcs = (yyvsp[(3) - (4)].func_list);
						  /* FIXME: if (!parse_only && do_header) write_module($$); */
						;}
    break;

  case 242:
#line 839 "parser.y"
    { (yyval.pident) = (yyvsp[(2) - (2)].pident); (yyval.pident)->ptr_level++; ;}
    break;

  case 243:
#line 840 "parser.y"
    { (yyval.pident) = (yyvsp[(2) - (2)].pident); /* FIXME */ ;}
    break;

  case 244:
#line 843 "parser.y"
    { (yyval.pident) = make_pident((yyvsp[(1) - (1)].var)); ;}
    break;

  case 246:
#line 845 "parser.y"
    { (yyval.pident) = (yyvsp[(2) - (3)].pident); ;}
    break;

  case 247:
#line 849 "parser.y"
    { (yyval.pident_list) = append_pident( NULL, (yyvsp[(1) - (1)].pident) ); ;}
    break;

  case 248:
#line 850 "parser.y"
    { (yyval.pident_list) = append_pident( (yyvsp[(1) - (3)].pident_list), (yyvsp[(3) - (3)].pident) ); ;}
    break;

  case 249:
#line 854 "parser.y"
    { (yyval.num) = RPC_FC_RP; ;}
    break;

  case 250:
#line 855 "parser.y"
    { (yyval.num) = RPC_FC_UP; ;}
    break;

  case 251:
#line 856 "parser.y"
    { (yyval.num) = RPC_FC_FP; ;}
    break;

  case 252:
#line 859 "parser.y"
    { (yyval.type) = get_typev(RPC_FC_STRUCT, (yyvsp[(2) - (5)].var), tsSTRUCT);
                                                  /* overwrite RPC_FC_STRUCT with a more exact type */
						  (yyval.type)->type = get_struct_type( (yyvsp[(4) - (5)].var_list) );
						  (yyval.type)->kind = TKIND_RECORD;
						  (yyval.type)->fields = (yyvsp[(4) - (5)].var_list);
						  (yyval.type)->defined = TRUE;
                                                  if(in_typelib)
                                                      add_typelib_entry((yyval.type));
                                                ;}
    break;

  case 253:
#line 870 "parser.y"
    { (yyval.type) = duptype(find_type("void", 0), 1); ;}
    break;

  case 254:
#line 871 "parser.y"
    { (yyval.type) = find_type((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 255:
#line 872 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (1)].type); ;}
    break;

  case 256:
#line 873 "parser.y"
    { (yyval.type) = duptype((yyvsp[(2) - (2)].type), 1); (yyval.type)->is_const = TRUE; ;}
    break;

  case 257:
#line 874 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (1)].type); ;}
    break;

  case 258:
#line 875 "parser.y"
    { (yyval.type) = find_type2((yyvsp[(2) - (2)].str), tsENUM); ;}
    break;

  case 259:
#line 876 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (1)].type); ;}
    break;

  case 260:
#line 877 "parser.y"
    { (yyval.type) = get_type(RPC_FC_STRUCT, (yyvsp[(2) - (2)].str), tsSTRUCT); ;}
    break;

  case 261:
#line 878 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (1)].type); ;}
    break;

  case 262:
#line 879 "parser.y"
    { (yyval.type) = find_type2((yyvsp[(2) - (2)].str), tsUNION); ;}
    break;

  case 263:
#line 880 "parser.y"
    { (yyval.type) = make_safearray((yyvsp[(3) - (4)].type)); ;}
    break;

  case 264:
#line 883 "parser.y"
    { reg_typedefs((yyvsp[(3) - (4)].type), (yyvsp[(4) - (4)].pident_list), (yyvsp[(2) - (4)].attr_list));
						  process_typedefs((yyvsp[(4) - (4)].pident_list));
						;}
    break;

  case 265:
#line 888 "parser.y"
    { (yyval.type) = get_typev(RPC_FC_NON_ENCAPSULATED_UNION, (yyvsp[(2) - (5)].var), tsUNION);
						  (yyval.type)->kind = TKIND_UNION;
						  (yyval.type)->fields = (yyvsp[(4) - (5)].var_list);
						  (yyval.type)->defined = TRUE;
						;}
    break;

  case 266:
#line 895 "parser.y"
    { var_t *u = (yyvsp[(7) - (10)].var);
						  (yyval.type) = get_typev(RPC_FC_ENCAPSULATED_UNION, (yyvsp[(2) - (10)].var), tsUNION);
						  (yyval.type)->kind = TKIND_UNION;
						  if (!u) u = make_var( xstrdup("tagged_union") );
						  u->type = make_type(RPC_FC_NON_ENCAPSULATED_UNION, NULL);
						  u->type->kind = TKIND_UNION;
						  u->type->fields = (yyvsp[(9) - (10)].var_list);
						  u->type->defined = TRUE;
						  (yyval.type)->fields = append_var( (yyval.type)->fields, (yyvsp[(5) - (10)].var) );
						  (yyval.type)->fields = append_var( (yyval.type)->fields, u );
						  (yyval.type)->defined = TRUE;
						;}
    break;

  case 267:
#line 910 "parser.y"
    { (yyval.num) = MAKELONG((yyvsp[(1) - (1)].num), 0); ;}
    break;

  case 268:
#line 911 "parser.y"
    { (yyval.num) = MAKELONG((yyvsp[(1) - (3)].num), (yyvsp[(3) - (3)].num)); ;}
    break;


/* Line 1267 of yacc.c.  */
#line 3822 "parser.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
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
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
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
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
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
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
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


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token.  */
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
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 914 "parser.y"


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

static type_t *make_type(unsigned char type, type_t *ref)
{
  type_t *t = xmalloc(sizeof(type_t));
  t->name = NULL;
  t->kind = TKIND_PRIMITIVE;
  t->type = type;
  t->ref = ref;
  t->attrs = NULL;
  t->orig = NULL;
  t->funcs = NULL;
  t->fields = NULL;
  t->ifaces = NULL;
  t->dim = 0;
  t->size_is = NULL;
  t->length_is = NULL;
  t->typestring_offset = 0;
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

static void set_type(var_t *v, type_t *type, int ptr_level, array_dims_t *arr)
{
  expr_list_t *sizes = get_attrp(v->attrs, ATTR_SIZEIS);
  expr_list_t *lengs = get_attrp(v->attrs, ATTR_LENGTHIS);
  int sizeless, has_varconf;
  expr_t *dim;
  type_t *atype, **ptype;

  v->type = type;

  for ( ; 0 < ptr_level; --ptr_level)
    v->type = make_type(RPC_FC_RP, v->type);

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
        error("%s: total array size is too large", v->name);
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
  v->args = NULL;
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
  p->ptr_level = 0;
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
  return make_type(RPC_FC_FP, sa);
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
    yyerror("registering named type without name");
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
    t->fields = ot->fields;
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
      yyerror("'%s': [string] attribute is only valid on 'char', 'byte', or 'wchar_t' pointers and arrays",
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
          cur = ptr = make_type(RPC_FC_RP, cur);
          ptrc++;
        }
      } else {
        while (cptr < ptrc) {
          cur = cur->ref;
          cptr++;
        }
      }
      cur = alias(cur, name->name);
      cur->attrs = attrs;
      if (ptr_type)
      {
        if (is_ptr(cur))
          cur->type = ptr_type;
        else
          yyerror("'%s': pointer attribute applied to non-pointer type",
                  cur->name);
      }
      else if (is_str && ! is_ptr(cur))
        yyerror("'%s': [string] attribute applied to non-pointer type",
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
    yyerror("type '%s' not found", name);
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

  if (fields) LIST_FOR_EACH_ENTRY( field, fields, var_t, entry )
  {
    type_t *t = field->type;

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
                yyerror("field '%s' deriving from a conformant array must be the last field in the structure",
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
    case RPC_FC_ENUM16:
    case RPC_FC_ENUM32:
      break;

    case RPC_FC_RP:
    case RPC_FC_UP:
    case RPC_FC_FP:
    case RPC_FC_OP:
    case RPC_FC_CARRAY:
    case RPC_FC_CVARRAY:
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
          yyerror("field '%s' deriving from a conformant array must be the last field in the structure",
                  field->name);
      has_pointer = 1;
      break;

    case RPC_FC_CSTRUCT:
      has_conformance = 1;
      if (list_next( fields, &field->entry ))
          yyerror("field '%s' deriving from a conformant array must be the last field in the structure",
                  field->name);
      break;

    case RPC_FC_PSTRUCT:
      has_pointer = 1;
      break;

    default:
      fprintf(stderr,"Unknown struct member %s with type (0x%02x)\n",
              field->name, t->type);
      /* fallthru - treat it as complex */

    /* as soon as we see one of these these members, it's bogus... */
    case RPC_FC_IP:
    case RPC_FC_ENCAPSULATED_UNION:
    case RPC_FC_NON_ENCAPSULATED_UNION:
    case RPC_FC_TRANSMIT_AS:
    case RPC_FC_REPRESENT_AS:
    case RPC_FC_PAD:
    case RPC_FC_EMBEDDED_COMPLEX:
    case RPC_FC_BOGUS_STRUCT:
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
    yyerror("registering constant without name");
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
    if (f) yyerror("constant '%s' not found", name);
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
    yyerror("argument '%s' has void type", arg->name);
}

static void check_all_user_types(ifref_list_t *ifrefs)
{
  const ifref_t *ifref;
  const func_t *f;

  if (ifrefs) LIST_FOR_EACH_ENTRY(ifref, ifrefs, const ifref_t, entry)
  {
    const func_list_t *fs = ifref->iface->funcs;
    if (fs) LIST_FOR_EACH_ENTRY(f, fs, const func_t, entry)
      check_for_user_types(f->args);
  }
}

