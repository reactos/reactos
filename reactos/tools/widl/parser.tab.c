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
     NEG = 389,
     ADDRESSOF = 390
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
#define ADDRESSOF 390




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

unsigned char pointer_default = RPC_FC_UP;

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
static void set_type(var_t *v, type_t *type, int ptr_level, array_dims_t *arr, int top);
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
#line 136 "parser.y"
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
#line 522 "parser.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */
#line 534 "parser.tab.c"

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
#define YYLAST   1065

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  155
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  76
/* YYNRULES -- Number of rules. */
#define YYNRULES  271
/* YYNRULES -- Number of states. */
#define YYNSTATES  499

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   390

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   136,     2,
     147,   148,   139,   138,   132,   137,   154,   140,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   134,   146,
       2,   153,     2,   133,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   151,     2,   152,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   149,   135,   150,   141,     2,     2,     2,
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
     125,   126,   127,   128,   129,   130,   131,   142,   143,   144,
     145
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
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
     404,   409,   411,   413,   415,   416,   418,   419,   422,   427,
     431,   437,   438,   441,   443,   445,   449,   453,   455,   461,
     463,   467,   468,   470,   472,   474,   476,   478,   480,   482,
     488,   492,   496,   500,   504,   508,   512,   516,   520,   523,
     526,   529,   532,   537,   542,   546,   548,   552,   554,   559,
     560,   563,   566,   570,   573,   575,   580,   588,   589,   591,
     592,   594,   596,   598,   600,   602,   604,   606,   609,   612,
     614,   616,   618,   620,   622,   624,   626,   627,   629,   631,
     634,   636,   639,   642,   644,   646,   649,   652,   655,   660,
     661,   664,   667,   670,   673,   676,   679,   683,   686,   690,
     696,   702,   703,   706,   709,   712,   715,   721,   729,   731,
     734,   737,   740,   743,   746,   751,   754,   757,   759,   761,
     765,   767,   771,   773,   775,   777,   783,   785,   787,   789,
     792,   794,   797,   799,   802,   804,   807,   812,   817,   823,
     834,   836
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short int yyrhs[] =
{
     156,     0,    -1,   157,    -1,    -1,   157,   218,    -1,   157,
     217,    -1,   157,   204,   146,    -1,   157,   206,    -1,   157,
     221,    -1,   157,   167,    -1,   157,   160,    -1,    -1,   158,
     218,    -1,   158,   217,    -1,   158,   204,   146,    -1,   158,
     206,    -1,   158,   221,    -1,   158,   160,    -1,   158,   164,
      -1,    -1,   159,   197,   146,    -1,   159,   160,    -1,   146,
      -1,   183,   146,    -1,   161,    -1,   187,   146,    -1,   193,
     146,    -1,   163,    -1,   226,   146,    -1,   228,   146,    -1,
     229,   146,    -1,    37,   147,     8,   148,    -1,    69,     8,
     146,    -1,   162,   158,    10,    -1,    70,   147,     8,   148,
      -1,    79,     3,    -1,   175,   165,   149,    -1,   166,   158,
     150,    -1,    -1,   170,    -1,   129,    -1,   171,    -1,   170,
     132,   171,    -1,   169,    -1,   175,   227,   223,   172,    -1,
     227,   223,   172,    -1,   175,   227,   223,   147,   168,   148,
      -1,   227,   223,   147,   168,   148,    -1,    -1,   151,   173,
     152,    -1,   151,   139,   152,    -1,   189,    -1,   173,   132,
     190,    -1,   173,   152,   151,   190,    -1,    -1,   175,    -1,
     151,   176,   152,    -1,   178,    -1,   176,   132,   178,    -1,
     176,   152,   151,   178,    -1,     8,    -1,   177,   132,     8,
      -1,    -1,    13,    -1,    15,    -1,    16,    -1,    18,    -1,
      19,    -1,    24,   147,   200,   148,    -1,    26,   147,   191,
     148,    -1,    33,    -1,    34,    -1,    35,    -1,    36,    -1,
      38,    -1,    39,    -1,    40,   147,   192,   148,    -1,    40,
     147,     8,   148,    -1,    41,    -1,    42,    -1,    44,   147,
       8,   148,    -1,    46,    -1,    47,   147,   177,   148,    -1,
      48,   147,     8,   148,    -1,    48,   147,   192,   148,    -1,
      51,    -1,    55,    -1,    57,   147,   192,   148,    -1,    58,
     147,     8,   148,    -1,    59,   147,     8,   148,    -1,    60,
     147,   192,   148,    -1,    61,   147,     8,   148,    -1,    62,
      -1,    64,   147,   192,   148,    -1,    65,    -1,    66,   147,
     190,   148,    -1,    67,    -1,    68,   147,    56,     3,   148,
      -1,    71,    -1,    73,    -1,    78,   147,   188,   148,    -1,
      80,    -1,    84,    -1,    85,    -1,    86,    -1,    87,    -1,
      88,    -1,    89,    -1,    90,    -1,    91,    -1,    92,   147,
     225,   148,    -1,    94,    -1,    95,    -1,    96,    -1,    98,
      -1,    99,   147,   192,   132,   192,   148,    -1,   100,    -1,
     102,    -1,   103,    -1,   104,    -1,   109,   147,   188,   148,
      -1,   112,    -1,   114,    -1,   117,   147,   190,   148,    -1,
     118,   147,   227,   148,    -1,   119,   147,   227,   148,    -1,
     125,   147,   179,   148,    -1,   126,    -1,   127,    -1,   128,
     147,   230,   148,    -1,   131,   147,   227,   148,    -1,   225,
      -1,     9,    -1,     8,    -1,    -1,   113,    -1,    -1,   181,
     182,    -1,    26,   190,   134,   195,    -1,    38,   134,   195,
      -1,    32,   227,   200,   153,   192,    -1,    -1,   185,   132,
      -1,   185,    -1,   186,    -1,   185,   132,   186,    -1,   200,
     153,   192,    -1,   200,    -1,    49,   199,   149,   184,   150,
      -1,   189,    -1,   188,   132,   189,    -1,    -1,   190,    -1,
       5,    -1,     6,    -1,     7,    -1,    53,    -1,   120,    -1,
       3,    -1,   190,   133,   190,   134,   190,    -1,   190,   135,
     190,    -1,   190,   136,   190,    -1,   190,   138,   190,    -1,
     190,   137,   190,    -1,   190,   139,   190,    -1,   190,   140,
     190,    -1,   190,    11,   190,    -1,   190,    12,   190,    -1,
     141,   190,    -1,   137,   190,    -1,   136,   190,    -1,   139,
     190,    -1,   147,   227,   148,   190,    -1,   110,   147,   227,
     148,    -1,   147,   190,   148,    -1,   192,    -1,   191,   132,
     192,    -1,   190,    -1,    52,    32,   227,   200,    -1,    -1,
     194,   195,    -1,   196,   146,    -1,   174,   229,   146,    -1,
     175,   146,    -1,   146,    -1,   174,   227,   223,   172,    -1,
     174,   227,   180,   223,   147,   168,   148,    -1,    -1,   200,
      -1,    -1,     3,    -1,     4,    -1,     3,    -1,     4,    -1,
      22,    -1,   130,    -1,   203,    -1,   107,   203,    -1,   124,
     203,    -1,   124,    -1,    54,    -1,   108,    -1,    45,    -1,
      20,    -1,    50,    -1,    56,    -1,    -1,    74,    -1,    74,
      -1,   106,   202,    -1,   111,    -1,    81,   202,    -1,    63,
     202,    -1,    75,    -1,    28,    -1,    29,     3,    -1,    29,
       4,    -1,   175,   204,    -1,   205,   149,   207,   150,    -1,
      -1,   207,   208,    -1,   174,   218,    -1,    43,     3,    -1,
      43,     4,    -1,   175,   209,    -1,    93,   134,    -1,   211,
     196,   146,    -1,    82,   134,    -1,   212,   197,   146,    -1,
     210,   149,   211,   212,   150,    -1,   210,   149,   215,   146,
     150,    -1,    -1,   134,     4,    -1,    76,     3,    -1,    76,
       4,    -1,   175,   215,    -1,   216,   214,   149,   159,   150,
      -1,   216,   134,     3,   149,   163,   159,   150,    -1,   213,
      -1,   215,   146,    -1,   209,   146,    -1,    83,     3,    -1,
      83,     4,    -1,   175,   219,    -1,   220,   149,   159,   150,
      -1,   139,   223,    -1,    32,   222,    -1,   200,    -1,   222,
      -1,   147,   223,   148,    -1,   223,    -1,   224,   132,   223,
      -1,   101,    -1,   123,    -1,    97,    -1,   115,   199,   149,
     194,   150,    -1,   129,    -1,     4,    -1,   201,    -1,    32,
     227,    -1,   187,    -1,    49,     3,    -1,   226,    -1,   115,
       3,    -1,   229,    -1,   122,     3,    -1,   105,   147,   227,
     148,    -1,   121,   174,   227,   224,    -1,   122,   199,   149,
     194,   150,    -1,   122,   199,   116,   147,   196,   148,   198,
     149,   181,   150,    -1,     5,    -1,     5,   154,     5,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   285,   285,   294,   295,   296,   297,   301,   306,   307,
     308,   311,   312,   313,   314,   315,   319,   320,   321,   324,
     325,   326,   329,   330,   331,   332,   337,   338,   339,   344,
     345,   352,   354,   357,   360,   363,   365,   370,   373,   374,
     377,   380,   381,   382,   386,   391,   395,   401,   408,   409,
     410,   413,   414,   415,   418,   419,   423,   429,   430,   431,
     434,   435,   438,   439,   440,   441,   442,   443,   444,   445,
     446,   447,   448,   449,   450,   451,   452,   453,   454,   455,
     456,   457,   458,   459,   460,   461,   462,   463,   464,   465,
     466,   467,   468,   469,   470,   471,   472,   473,   474,   475,
     476,   477,   478,   479,   480,   481,   482,   483,   484,   485,
     486,   487,   488,   489,   490,   491,   494,   495,   496,   497,
     498,   499,   500,   501,   502,   503,   504,   505,   506,   507,
     508,   509,   513,   514,   519,   520,   523,   524,   527,   531,
     537,   543,   544,   545,   548,   552,   561,   565,   570,   579,
     580,   593,   594,   597,   598,   599,   600,   601,   602,   603,
     604,   605,   606,   607,   608,   609,   610,   611,   612,   613,
     614,   615,   616,   617,   618,   621,   622,   625,   631,   636,
     637,   640,   641,   642,   643,   646,   654,   666,   667,   670,
     671,   672,   675,   677,   680,   681,   682,   683,   684,   700,
     701,   702,   703,   704,   705,   706,   709,   710,   713,   714,
     715,   716,   717,   718,   719,   722,   723,   729,   738,   744,
     745,   749,   752,   753,   756,   768,   769,   772,   773,   776,
     785,   794,   795,   798,   799,   802,   813,   825,   836,   840,
     841,   844,   845,   848,   853,   859,   860,   863,   864,   865,
     869,   870,   874,   875,   876,   879,   890,   891,   892,   893,
     894,   895,   896,   897,   898,   899,   900,   903,   908,   913,
     930,   931
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
  "'-'", "'+'", "'*'", "'/'", "'~'", "CAST", "PPTR", "NEG", "ADDRESSOF",
  "';'", "'('", "')'", "'{'", "'}'", "'['", "']'", "'='", "'.'", "$accept",
  "input", "gbl_statements", "imp_statements", "int_statements",
  "statement", "cppquote", "import_start", "import", "importlib",
  "libraryhdr", "library_start", "librarydef", "m_args", "no_args", "args",
  "arg", "array", "array_list", "m_attributes", "attributes",
  "attrib_list", "str_list", "attribute", "uuid_string", "callconv",
  "cases", "case", "constdef", "enums", "enum_list", "enum", "enumdef",
  "m_exprs", "m_expr", "expr", "expr_list_const", "expr_const",
  "externdef", "fields", "field", "s_field", "funcdef", "m_ident",
  "t_ident", "ident", "base_type", "m_int", "int_std", "coclass",
  "coclasshdr", "coclassdef", "coclass_ints", "coclass_int",
  "dispinterface", "dispinterfacehdr", "dispint_props", "dispint_meths",
  "dispinterfacedef", "inherit", "interface", "interfacehdr",
  "interfacedef", "interfacedec", "module", "modulehdr", "moduledef",
  "p_ident", "pident", "pident_list", "pointer_type", "structdef", "type",
  "typedef", "uniondef", "version", 0
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
     385,   386,    44,    63,    58,   124,    38,    45,    43,    42,
      47,   126,   387,   388,   389,   390,    59,    40,    41,   123,
     125,    91,    93,    61,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,   155,   156,   157,   157,   157,   157,   157,   157,   157,
     157,   158,   158,   158,   158,   158,   158,   158,   158,   159,
     159,   159,   160,   160,   160,   160,   160,   160,   160,   160,
     160,   161,   162,   163,   164,   165,   166,   167,   168,   168,
     169,   170,   170,   170,   171,   171,   171,   171,   172,   172,
     172,   173,   173,   173,   174,   174,   175,   176,   176,   176,
     177,   177,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   179,   179,   180,   180,   181,   181,   182,   182,
     183,   184,   184,   184,   185,   185,   186,   186,   187,   188,
     188,   189,   189,   190,   190,   190,   190,   190,   190,   190,
     190,   190,   190,   190,   190,   190,   190,   190,   190,   190,
     190,   190,   190,   190,   190,   191,   191,   192,   193,   194,
     194,   195,   195,   195,   195,   196,   197,   198,   198,   199,
     199,   199,   200,   200,   201,   201,   201,   201,   201,   201,
     201,   201,   201,   201,   201,   201,   202,   202,   203,   203,
     203,   203,   203,   203,   203,   204,   204,   205,   206,   207,
     207,   208,   209,   209,   210,   211,   211,   212,   212,   213,
     213,   214,   214,   215,   215,   216,   217,   217,   217,   218,
     218,   219,   219,   220,   221,   222,   222,   223,   223,   223,
     224,   224,   225,   225,   225,   226,   227,   227,   227,   227,
     227,   227,   227,   227,   227,   227,   227,   228,   229,   229,
     230,   230
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
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
       4,     1,     1,     1,     0,     1,     0,     2,     4,     3,
       5,     0,     2,     1,     1,     3,     3,     1,     5,     1,
       3,     0,     1,     1,     1,     1,     1,     1,     1,     5,
       3,     3,     3,     3,     3,     3,     3,     3,     2,     2,
       2,     2,     4,     4,     3,     1,     3,     1,     4,     0,
       2,     2,     3,     2,     1,     4,     7,     0,     1,     0,
       1,     1,     1,     1,     1,     1,     1,     2,     2,     1,
       1,     1,     1,     1,     1,     1,     0,     1,     1,     2,
       1,     2,     2,     1,     1,     2,     2,     2,     4,     0,
       2,     2,     2,     2,     2,     2,     3,     2,     3,     5,
       5,     0,     2,     2,     2,     2,     5,     7,     1,     2,
       2,     2,     2,     2,     4,     2,     2,     1,     1,     3,
       1,     3,     1,     1,     1,     5,     1,     1,     1,     2,
       1,     2,     1,     2,     1,     2,     4,     4,     5,    10,
       1,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned short int yydefact[] =
{
       3,     0,     2,     1,     0,     0,     0,     0,   189,     0,
       0,     0,   189,    54,   189,    22,    62,    10,    24,    11,
      27,    11,     9,     0,     0,     0,     0,     0,     0,     7,
       0,     0,   238,     0,   231,     5,     4,     0,     8,     0,
       0,     0,   215,   216,   257,   203,   194,   214,     0,   202,
     189,   204,   200,   205,   206,   208,   213,   206,     0,   206,
       0,   201,   210,   189,   189,   199,   256,   195,   260,   258,
     196,   262,     0,   264,     0,   222,   223,   190,   191,     0,
       0,     0,   233,   234,     0,     0,    55,     0,    63,    64,
      65,    66,    67,     0,     0,    70,    71,    72,    73,    74,
      75,     0,    78,    79,     0,    81,     0,     0,    85,    86,
       0,     0,     0,     0,     0,    92,     0,    94,     0,    96,
       0,    98,    99,     0,   101,   102,   103,   104,   105,   106,
     107,   108,   109,     0,   111,   112,   113,   254,   114,     0,
     116,   252,   117,   118,   119,     0,   121,   122,     0,     0,
       0,   253,     0,   127,   128,     0,     0,     0,    57,   131,
       0,     0,     0,     0,     0,   217,   224,   235,   243,    23,
      25,    26,     6,   219,   240,     0,   239,     0,     0,    19,
      28,    29,    30,   259,   261,   207,   212,   211,     0,   209,
     197,   263,   265,   198,   192,   193,     0,     0,   141,     0,
      32,   179,     0,     0,   179,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   151,
       0,     0,   151,     0,     0,     0,     0,     0,     0,    62,
      56,    33,     0,    17,    18,     0,     0,    15,    13,    12,
      16,    37,    35,   241,   242,    36,    54,     0,    54,     0,
       0,   232,    19,    54,     0,     0,    31,     0,   143,   144,
     147,   178,    54,     0,     0,     0,   247,   248,   250,   267,
      54,    54,     0,   158,   153,   154,   155,   156,     0,   157,
       0,     0,     0,     0,     0,   177,     0,   175,     0,     0,
       0,    60,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   149,   152,     0,     0,     0,     0,
       0,     0,   133,   132,     0,   270,     0,     0,    58,    62,
       0,    14,   218,     0,   220,   225,     0,     0,     0,    54,
       0,     0,    54,   244,    21,     0,     0,   266,   140,   148,
     142,     0,   184,   255,     0,    55,   180,     0,   246,   245,
       0,     0,     0,   268,    68,     0,   170,   169,   171,   168,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    69,    77,    76,    80,     0,    82,    83,    84,
      87,    88,    89,    90,    91,    93,    95,     0,   151,   100,
     110,     0,   120,   123,   124,   125,   126,     0,   129,   130,
      59,     0,   221,   227,     0,   226,   229,     0,   230,    19,
     236,   134,    20,   145,   146,   264,   183,   181,   249,   251,
     187,     0,   174,     0,   166,   167,     0,   160,   161,   163,
     162,   164,   165,   176,    61,    97,   150,     0,   271,    34,
      48,   228,    54,   135,     0,   182,     0,   188,   173,   172,
       0,   115,   151,   185,   237,     0,   136,   159,     0,     0,
      51,    38,     0,    50,     0,    49,   256,     0,    43,    39,
      41,     0,     0,     0,     0,   269,   137,    52,     0,   186,
       0,     0,    48,     0,    54,    53,    42,    48,    38,    45,
      54,   139,    38,    44,     0,   138,     0,    47,    46
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,     1,     2,   160,   253,   334,    18,    19,    20,   234,
     164,    21,    22,   467,   468,   469,   470,   453,   459,   335,
      86,   157,   292,   158,   314,   444,   462,   476,    24,   257,
     258,   259,    68,   303,   304,   285,   286,   287,    26,   262,
     346,   347,   336,   446,    79,   266,    69,   186,    70,   165,
      28,   237,   246,   324,    30,    31,   248,   329,    32,   178,
      33,    34,   238,   239,   168,    37,   240,   267,   268,   269,
     159,    71,   472,    40,    73,   316
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -376
static const short int yypact[] =
{
    -376,    24,   773,  -376,    86,   684,   -43,   190,   193,    79,
     109,   210,   193,   -28,   193,  -376,   934,  -376,  -376,  -376,
    -376,  -376,  -376,    27,   -20,   -14,    -7,    -5,    20,  -376,
      14,    40,  -376,    53,     0,  -376,  -376,    59,  -376,    77,
      89,    92,  -376,  -376,  -376,  -376,  -376,  -376,   684,  -376,
     213,  -376,  -376,  -376,   178,  -376,  -376,   178,   108,   178,
      -6,  -376,  -376,   226,   228,    -6,  -376,  -376,  -376,  -376,
    -376,  -376,   237,  -376,   251,  -376,  -376,  -376,  -376,   115,
     684,   122,  -376,  -376,   123,   684,  -376,   -91,  -376,  -376,
    -376,  -376,  -376,   128,   132,  -376,  -376,  -376,  -376,  -376,
    -376,   136,  -376,  -376,   138,  -376,   143,   145,  -376,  -376,
     146,   147,   148,   151,   152,  -376,   153,  -376,   154,  -376,
     155,  -376,  -376,   158,  -376,  -376,  -376,  -376,  -376,  -376,
    -376,  -376,  -376,   159,  -376,  -376,  -376,  -376,  -376,   160,
    -376,  -376,  -376,  -376,  -376,   161,  -376,  -376,   162,   163,
     164,  -376,   165,  -376,  -376,   167,   169,   -79,  -376,  -376,
     615,   731,   314,   239,   170,  -376,  -376,  -376,  -376,  -376,
    -376,  -376,  -376,  -376,  -376,   -16,  -376,   241,   171,  -376,
    -376,  -376,  -376,  -376,   173,  -376,  -376,  -376,   684,  -376,
    -376,   173,   -85,  -376,  -376,  -376,   175,   176,   237,   237,
    -376,  -376,    48,   179,  -376,   237,   382,   243,   310,   321,
     450,   382,   322,   324,   382,   326,   382,   382,   280,   382,
     -51,   382,   382,   382,   684,   684,   217,   334,   684,   934,
     195,  -376,   194,  -376,  -376,    16,   198,  -376,  -376,  -376,
    -376,  -376,  -376,  -376,  -376,  -376,   116,   214,   -63,   206,
     205,  -376,  -376,   605,   216,   382,  -376,   215,   234,  -376,
     218,  -376,   -32,   -15,    48,    48,  -376,  -376,  -376,   240,
     -28,   -30,   221,  -376,  -376,  -376,  -376,  -376,   229,  -376,
     382,   382,   382,   382,   502,    17,   -93,  -376,   225,   227,
     230,  -376,   -83,   233,   235,   238,   244,   245,   246,   247,
     248,    67,   374,   -47,  -376,    17,   249,   259,   -46,   222,
     254,   255,  -376,  -376,   257,   252,   261,   262,  -376,   934,
     399,  -376,  -376,   -10,  -376,  -376,   277,   684,   266,   131,
     263,   345,   786,  -376,  -376,   684,   270,  -376,  -376,  -376,
     237,   382,  -376,  -376,   684,   272,  -376,   273,  -376,  -376,
     285,    48,   286,  -376,  -376,   684,  -376,  -376,  -376,  -376,
     542,   288,   382,   382,   382,   382,   382,   382,   382,   382,
     382,   382,  -376,  -376,  -376,  -376,   412,  -376,  -376,  -376,
    -376,  -376,  -376,  -376,  -376,  -376,  -376,   289,   382,  -376,
    -376,   382,  -376,  -376,  -376,  -376,  -376,   416,  -376,  -376,
    -376,   290,  -376,  -376,    48,  -376,  -376,   295,  -376,  -376,
    -376,   333,  -376,  -376,  -376,   303,  -376,  -376,  -376,  -376,
     237,   291,  -376,   382,  -376,  -376,   292,     9,    72,    -3,
      -3,   276,   276,  -376,  -376,  -376,  -376,   302,  -376,  -376,
     300,  -376,   795,  -376,    48,  -376,   305,  -376,  -376,  -376,
     382,  -376,   509,  -376,  -376,   313,  -376,    17,   137,   -78,
    -376,   293,   -12,  -376,   382,   311,   -41,   315,  -376,   329,
    -376,   684,    48,   382,   330,  -376,  -376,    17,   382,  -376,
     420,    48,   -80,   561,  -104,    17,  -376,    23,   293,  -376,
    -104,  -376,   293,  -376,   318,  -376,   320,  -376,  -376
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -376,  -376,  -376,   451,  -236,    11,  -376,  -376,   140,  -376,
    -376,  -376,  -376,  -300,  -376,  -376,    -1,  -360,  -376,    -9,
      -2,  -376,  -376,  -211,  -376,  -376,  -376,  -376,  -376,  -376,
    -376,   133,     3,   258,  -365,  -187,  -376,  -173,  -376,   281,
    -375,  -208,   168,  -376,    49,   -70,  -376,    74,    65,    25,
    -376,   475,  -376,  -376,   -13,  -376,  -376,  -376,  -376,  -376,
      -8,  -376,   482,     4,  -376,  -376,   485,   236,  -253,  -376,
     269,     5,    -4,  -376,     1,  -376
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -191
static const short int yytable[] =
{
      23,    72,   196,    41,    85,    25,    36,    39,   362,   363,
     166,   349,   350,    17,   473,   167,   332,   263,   318,   326,
     362,   363,    47,   436,     3,   203,   474,    27,   362,   363,
     301,  -190,   305,     7,   289,   305,   309,   294,   295,   371,
     328,   298,   342,   300,   183,     4,   137,    16,   307,   376,
     141,   194,   195,   229,   464,   372,     4,    54,   204,     7,
      11,    84,   352,    87,  -190,   377,    11,   488,    55,    56,
       7,   452,   151,   230,   465,    57,   199,   247,   362,   363,
     263,   202,   338,   362,   363,   388,   388,   460,    16,    42,
      43,   -40,    11,   356,   357,   358,   359,   360,   419,   163,
      59,   389,   392,    11,    74,    62,   162,   -40,   400,   491,
     163,    80,    84,    87,   342,   495,   342,    81,   343,    16,
     353,    16,   489,    16,   264,   190,   169,   493,   260,   261,
     193,   187,   170,   189,   177,   272,   369,   370,   475,   171,
     273,   172,   274,   275,   276,   366,   367,   368,   369,   370,
     364,   440,   365,   366,   367,   368,   369,   370,   235,   235,
     174,    41,    41,    25,    25,    39,    39,   249,   414,   173,
     492,   233,   233,   442,   452,   424,   425,   426,   427,   428,
     429,   430,   431,   432,   254,   236,   236,   264,   494,   175,
     277,   455,   496,    75,    76,   265,    77,    78,   433,   176,
     364,   305,   365,   366,   367,   368,   369,   370,   179,   367,
     368,   369,   370,    82,    83,   386,   184,    78,   437,   482,
     310,   311,   166,   180,   317,   312,   313,   167,   487,   191,
      78,   192,    78,   362,   363,   181,   449,   323,   182,   327,
     194,   195,   243,   244,   250,   251,   273,   278,   274,   275,
     276,   288,   185,   344,    41,   188,    25,   279,    39,   197,
     345,   327,   344,   457,   198,   305,   322,    16,   200,   345,
     260,   358,   201,   280,   281,   205,   282,   477,   283,   206,
     361,   406,    16,   207,   284,   208,   483,   362,   363,   463,
     209,   485,   210,   211,   212,   213,   277,    44,   214,   215,
     216,   217,   218,   362,   363,   219,   220,   221,   222,   223,
     224,   225,   226,    45,   227,    46,   228,   242,   290,   245,
     252,    47,  -190,   404,   256,    48,   270,   402,   255,   291,
     296,   411,   297,    41,   299,    25,   302,    39,    49,   315,
     404,   320,    50,    51,   321,   415,   319,    52,   325,    53,
     447,   421,   330,   278,   331,   364,    54,   365,   366,   367,
     368,   369,   370,   279,   337,   339,   340,    55,    56,   354,
     393,   341,   351,   373,    57,   374,   355,   387,   375,   280,
     281,   378,   282,   379,   283,   273,   380,   274,   275,   276,
     284,   391,   381,   382,   383,   384,   385,   390,    58,    59,
      60,    61,   394,   395,    62,   396,   397,   401,    63,   398,
     399,   403,   405,   408,    10,    64,   412,    65,   416,   417,
     434,   438,   466,    67,    44,   364,   450,   365,   366,   367,
     368,   369,   370,   418,   420,   277,   423,   435,   439,   448,
      45,   441,    46,    41,    16,    25,   443,    39,    47,   445,
     451,   452,    48,   273,   456,   274,   275,   276,   293,   471,
     461,   480,   478,   479,   484,    49,   497,   481,   498,    50,
      51,   409,   161,   413,    52,   344,    53,    29,   471,   486,
     308,   344,   345,    54,    35,   271,   471,    38,   345,   306,
     471,     0,   278,     0,    55,    56,     0,   407,     0,   348,
       0,    57,   279,   277,     0,   273,    44,   274,   275,   276,
       0,     0,   273,     0,   274,   275,   276,     0,   280,   281,
       0,   282,    45,   283,    46,    58,    59,    60,    61,   284,
      47,    62,     0,     0,    48,    63,     0,     0,     0,     0,
       0,     0,    64,     0,    65,     0,     0,    49,     0,    66,
      67,    50,    51,   362,   363,   277,    52,     0,    53,     0,
     278,     0,   277,     0,     0,    54,     0,     0,     0,     0,
     279,    16,   362,   363,     0,     0,    55,    56,     0,     0,
       0,     0,     0,    57,     0,     0,   280,   281,     0,   282,
       0,   283,     0,     0,     0,     0,     0,   284,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    58,    59,    60,
      61,     0,   278,    62,     0,     0,     0,    63,     0,   278,
       0,     0,   279,     0,    64,   231,    65,     0,     0,   279,
       0,    66,    67,     0,     0,     0,     0,     5,   280,   281,
       0,   282,     6,   283,     4,   280,   281,     5,   458,   284,
     283,     0,     6,     0,     8,     0,   284,     9,     7,     0,
       0,     0,     0,     0,     8,     0,     0,     9,     0,     0,
       0,     0,     0,     0,    10,   364,     0,   365,   366,   367,
     368,   369,   370,     0,    10,   232,     0,     0,    44,     0,
     422,    11,     0,     0,   364,   490,   365,   366,   367,   368,
     369,   370,     0,     0,    45,     0,    46,     0,     0,     0,
       0,     0,    47,     0,     0,     0,    48,     0,     0,     0,
      12,     0,     0,     0,     0,     0,    13,    14,     0,    49,
      12,     0,     0,    50,    51,     0,    13,    14,    52,     0,
      53,     0,     0,     0,     0,     0,     0,    54,     0,     0,
       0,    15,     0,     0,     0,   333,    16,     0,    55,    56,
       4,    15,     0,     5,     0,    57,    16,     0,     6,     0,
       0,     0,     0,     0,     7,     0,     0,     0,     0,     0,
       8,     0,     0,     9,     0,     0,     0,     0,     0,    58,
      59,    60,    61,     0,     0,    62,     0,     0,     0,    63,
      10,   232,     4,     0,     0,     5,    64,    11,    65,     0,
       6,     0,     0,    66,    67,     0,     7,     0,     5,     0,
       0,     0,     8,     6,     0,     9,     0,     5,     0,     0,
       0,     0,     6,     0,     0,     8,     0,     0,     9,     0,
       0,     0,    10,     0,     8,     0,    12,     9,     0,    11,
       0,     0,    13,    14,     0,    10,     0,     0,     0,     0,
       0,     0,     0,     0,    10,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    15,     0,     0,
       0,   241,    16,     0,     0,     0,     0,     0,    12,     0,
       0,     0,     0,     0,    13,    14,     0,     0,     0,     0,
       0,    12,     0,     0,     0,     0,     0,    13,    14,     0,
      12,     0,     0,     0,     0,     0,    13,    14,     0,    15,
       0,     0,     0,     0,    16,     0,     0,     0,     0,     0,
       0,     0,    15,     0,     0,     0,   410,    16,     0,     0,
       0,    15,     0,     0,     0,   454,    16,    88,     0,    89,
      90,     0,    91,    92,     0,     0,     0,     0,    93,     0,
      94,     0,     0,     0,     0,     0,     0,    95,    96,    97,
      98,     0,    99,   100,   101,   102,   103,     0,   104,     0,
     105,   106,   107,     0,     0,   108,     0,     0,     0,   109,
       0,   110,   111,   112,   113,   114,   115,     0,   116,   117,
     118,   119,   120,     0,     0,   121,     0,   122,     0,     0,
       0,     0,   123,     0,   124,     0,     0,     0,   125,   126,
     127,   128,   129,   130,   131,   132,   133,     0,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,     0,
       0,     0,     0,   145,     0,     0,   146,     0,   147,     0,
       0,   148,   149,   150,     0,     0,     0,   151,     0,   152,
     153,   154,   155,     0,     0,   156
};

static const short int yycheck[] =
{
       2,     5,    72,     2,    13,     2,     2,     2,    11,    12,
      23,   264,   265,     2,    26,    23,   252,    32,   229,    82,
      11,    12,    28,   388,     0,   116,    38,     2,    11,    12,
     217,   116,   219,    43,   207,   222,   223,   210,   211,   132,
     248,   214,   146,   216,    48,    29,    97,   151,   221,   132,
     101,     3,     4,   132,   132,   148,    29,    63,   149,    43,
      76,    12,   270,    14,   149,   148,    76,   147,    74,    75,
      43,   151,   123,   152,   152,    81,    80,    93,    11,    12,
      32,    85,   255,    11,    12,   132,   132,   452,   151,     3,
       4,   132,    76,   280,   281,   282,   283,   284,   351,    83,
     106,   148,   148,    76,   147,   111,    79,   148,   319,   484,
      83,    32,    63,    64,   146,   490,   146,     8,   150,   151,
     150,   151,   482,   151,   139,    60,   146,   487,   198,   199,
      65,    57,   146,    59,   134,   205,   139,   140,   150,   146,
       3,   146,     5,     6,     7,   136,   137,   138,   139,   140,
     133,   404,   135,   136,   137,   138,   139,   140,   160,   161,
     146,   160,   161,   160,   161,   160,   161,   175,   341,   149,
     147,   160,   161,   409,   151,   362,   363,   364,   365,   366,
     367,   368,   369,   370,   188,   160,   161,   139,   488,   149,
      53,   444,   492,     3,     4,   147,     3,     4,   371,   146,
     133,   388,   135,   136,   137,   138,   139,   140,   149,   137,
     138,   139,   140,     3,     4,   148,     3,     4,   391,   472,
     224,   225,   235,   146,   228,     8,     9,   235,   481,     3,
       4,     3,     4,    11,    12,   146,   423,   246,   146,   248,
       3,     4,     3,     4,     3,     4,     3,   110,     5,     6,
       7,     8,    74,   262,   253,   147,   253,   120,   253,     8,
     262,   270,   271,   450,   149,   452,   150,   151,   146,   271,
     340,   458,   149,   136,   137,   147,   139,   464,   141,   147,
     284,   150,   151,   147,   147,   147,   473,    11,    12,   152,
     147,   478,   147,   147,   147,   147,    53,     4,   147,   147,
     147,   147,   147,    11,    12,   147,   147,   147,   147,   147,
     147,   147,   147,    20,   147,    22,   147,     3,     8,   149,
     149,    28,   149,   327,   148,    32,   147,   323,   153,     8,
       8,   335,     8,   332,     8,   332,    56,   332,    45,     5,
     344,   147,    49,    50,   146,   344,   151,    54,   134,    56,
     420,   355,   146,   110,   149,   133,    63,   135,   136,   137,
     138,   139,   140,   120,   148,   150,   132,    74,    75,   148,
     148,   153,   132,   148,    81,   148,   147,     3,   148,   136,
     137,   148,   139,   148,   141,     3,   148,     5,     6,     7,
     147,   132,   148,   148,   148,   148,   148,   148,   105,   106,
     107,   108,   148,   148,   111,   148,   154,     8,   115,   148,
     148,   134,   146,   150,    69,   122,   146,   124,   146,   146,
       8,     5,   129,   130,     4,   133,   134,   135,   136,   137,
     138,   139,   140,   148,   148,    53,   148,   148,   148,   148,
      20,   146,    22,   442,   151,   442,   113,   442,    28,   146,
     148,   151,    32,     3,   149,     5,     6,     7,     8,   461,
     147,   132,   151,   148,   134,    45,   148,   471,   148,    49,
      50,   331,    21,   340,    54,   484,    56,     2,   480,   480,
     222,   490,   484,    63,     2,   204,   488,     2,   490,   220,
     492,    -1,   110,    -1,    74,    75,    -1,   329,    -1,   263,
      -1,    81,   120,    53,    -1,     3,     4,     5,     6,     7,
      -1,    -1,     3,    -1,     5,     6,     7,    -1,   136,   137,
      -1,   139,    20,   141,    22,   105,   106,   107,   108,   147,
      28,   111,    -1,    -1,    32,   115,    -1,    -1,    -1,    -1,
      -1,    -1,   122,    -1,   124,    -1,    -1,    45,    -1,   129,
     130,    49,    50,    11,    12,    53,    54,    -1,    56,    -1,
     110,    -1,    53,    -1,    -1,    63,    -1,    -1,    -1,    -1,
     120,   151,    11,    12,    -1,    -1,    74,    75,    -1,    -1,
      -1,    -1,    -1,    81,    -1,    -1,   136,   137,    -1,   139,
      -1,   141,    -1,    -1,    -1,    -1,    -1,   147,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   105,   106,   107,
     108,    -1,   110,   111,    -1,    -1,    -1,   115,    -1,   110,
      -1,    -1,   120,    -1,   122,    10,   124,    -1,    -1,   120,
      -1,   129,   130,    -1,    -1,    -1,    -1,    32,   136,   137,
      -1,   139,    37,   141,    29,   136,   137,    32,   139,   147,
     141,    -1,    37,    -1,    49,    -1,   147,    52,    43,    -1,
      -1,    -1,    -1,    -1,    49,    -1,    -1,    52,    -1,    -1,
      -1,    -1,    -1,    -1,    69,   133,    -1,   135,   136,   137,
     138,   139,   140,    -1,    69,    70,    -1,    -1,     4,    -1,
     148,    76,    -1,    -1,   133,   134,   135,   136,   137,   138,
     139,   140,    -1,    -1,    20,    -1,    22,    -1,    -1,    -1,
      -1,    -1,    28,    -1,    -1,    -1,    32,    -1,    -1,    -1,
     115,    -1,    -1,    -1,    -1,    -1,   121,   122,    -1,    45,
     115,    -1,    -1,    49,    50,    -1,   121,   122,    54,    -1,
      56,    -1,    -1,    -1,    -1,    -1,    -1,    63,    -1,    -1,
      -1,   146,    -1,    -1,    -1,   150,   151,    -1,    74,    75,
      29,   146,    -1,    32,    -1,    81,   151,    -1,    37,    -1,
      -1,    -1,    -1,    -1,    43,    -1,    -1,    -1,    -1,    -1,
      49,    -1,    -1,    52,    -1,    -1,    -1,    -1,    -1,   105,
     106,   107,   108,    -1,    -1,   111,    -1,    -1,    -1,   115,
      69,    70,    29,    -1,    -1,    32,   122,    76,   124,    -1,
      37,    -1,    -1,   129,   130,    -1,    43,    -1,    32,    -1,
      -1,    -1,    49,    37,    -1,    52,    -1,    32,    -1,    -1,
      -1,    -1,    37,    -1,    -1,    49,    -1,    -1,    52,    -1,
      -1,    -1,    69,    -1,    49,    -1,   115,    52,    -1,    76,
      -1,    -1,   121,   122,    -1,    69,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    69,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   146,    -1,    -1,
      -1,   150,   151,    -1,    -1,    -1,    -1,    -1,   115,    -1,
      -1,    -1,    -1,    -1,   121,   122,    -1,    -1,    -1,    -1,
      -1,   115,    -1,    -1,    -1,    -1,    -1,   121,   122,    -1,
     115,    -1,    -1,    -1,    -1,    -1,   121,   122,    -1,   146,
      -1,    -1,    -1,    -1,   151,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   146,    -1,    -1,    -1,   150,   151,    -1,    -1,
      -1,   146,    -1,    -1,    -1,   150,   151,    13,    -1,    15,
      16,    -1,    18,    19,    -1,    -1,    -1,    -1,    24,    -1,
      26,    -1,    -1,    -1,    -1,    -1,    -1,    33,    34,    35,
      36,    -1,    38,    39,    40,    41,    42,    -1,    44,    -1,
      46,    47,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,
      -1,    57,    58,    59,    60,    61,    62,    -1,    64,    65,
      66,    67,    68,    -1,    -1,    71,    -1,    73,    -1,    -1,
      -1,    -1,    78,    -1,    80,    -1,    -1,    -1,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    -1,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,    -1,
      -1,    -1,    -1,   109,    -1,    -1,   112,    -1,   114,    -1,
      -1,   117,   118,   119,    -1,    -1,    -1,   123,    -1,   125,
     126,   127,   128,    -1,    -1,   131
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,   156,   157,     0,    29,    32,    37,    43,    49,    52,
      69,    76,   115,   121,   122,   146,   151,   160,   161,   162,
     163,   166,   167,   175,   183,   187,   193,   204,   205,   206,
     209,   210,   213,   215,   216,   217,   218,   220,   221,   226,
     228,   229,     3,     4,     4,    20,    22,    28,    32,    45,
      49,    50,    54,    56,    63,    74,    75,    81,   105,   106,
     107,   108,   111,   115,   122,   124,   129,   130,   187,   201,
     203,   226,   227,   229,   147,     3,     4,     3,     4,   199,
      32,     8,     3,     4,   199,   174,   175,   199,    13,    15,
      16,    18,    19,    24,    26,    33,    34,    35,    36,    38,
      39,    40,    41,    42,    44,    46,    47,    48,    51,    55,
      57,    58,    59,    60,    61,    62,    64,    65,    66,    67,
      68,    71,    73,    78,    80,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   109,   112,   114,   117,   118,
     119,   123,   125,   126,   127,   128,   131,   176,   178,   225,
     158,   158,    79,    83,   165,   204,   209,   215,   219,   146,
     146,   146,   146,   149,   146,   149,   146,   134,   214,   149,
     146,   146,   146,   227,     3,    74,   202,   202,   147,   202,
     203,     3,     3,   203,     3,     4,   200,     8,   149,   227,
     146,   149,   227,   116,   149,   147,   147,   147,   147,   147,
     147,   147,   147,   147,   147,   147,   147,   147,   147,   147,
     147,   147,   147,   147,   147,   147,   147,   147,   147,   132,
     152,    10,    70,   160,   164,   175,   204,   206,   217,   218,
     221,   150,     3,     3,     4,   149,   207,    93,   211,   215,
       3,     4,   149,   159,   227,   153,   148,   184,   185,   186,
     200,   200,   194,    32,   139,   147,   200,   222,   223,   224,
     147,   194,   200,     3,     5,     6,     7,    53,   110,   120,
     136,   137,   139,   141,   147,   190,   191,   192,     8,   192,
       8,     8,   177,     8,   192,   192,     8,     8,   192,     8,
     192,   190,    56,   188,   189,   190,   225,   192,   188,   190,
     227,   227,     8,     9,   179,     5,   230,   227,   178,   151,
     147,   146,   150,   174,   208,   134,    82,   174,   196,   212,
     146,   149,   159,   150,   160,   174,   197,   148,   192,   150,
     132,   153,   146,   150,   174,   175,   195,   196,   222,   223,
     223,   132,   196,   150,   148,   147,   190,   190,   190,   190,
     190,   227,    11,    12,   133,   135,   136,   137,   138,   139,
     140,   132,   148,   148,   148,   148,   132,   148,   148,   148,
     148,   148,   148,   148,   148,   148,   148,     3,   132,   148,
     148,   132,   148,   148,   148,   148,   148,   154,   148,   148,
     178,     8,   218,   134,   227,   146,   150,   197,   150,   163,
     150,   227,   146,   186,   192,   229,   146,   146,   148,   223,
     148,   227,   148,   148,   190,   190,   190,   190,   190,   190,
     190,   190,   190,   192,     8,   148,   189,   192,     5,   148,
     223,   146,   159,   113,   180,   146,   198,   200,   148,   190,
     134,   148,   151,   172,   150,   223,   149,   190,   139,   173,
     189,   147,   181,   152,   132,   152,   129,   168,   169,   170,
     171,   175,   227,    26,    38,   150,   182,   190,   151,   148,
     132,   227,   223,   190,   134,   190,   171,   223,   147,   172,
     134,   195,   147,   172,   168,   195,   168,   148,   148
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
#line 285 "parser.y"
    { fix_incomplete();
						  check_all_user_types((yyvsp[0].ifref_list));
						  write_proxies((yyvsp[0].ifref_list));
						  write_client((yyvsp[0].ifref_list));
						  write_server((yyvsp[0].ifref_list));
						  write_dlldata((yyvsp[0].ifref_list));
						;}
    break;

  case 3:
#line 294 "parser.y"
    { (yyval.ifref_list) = NULL; ;}
    break;

  case 4:
#line 295 "parser.y"
    { (yyval.ifref_list) = (yyvsp[-1].ifref_list); ;}
    break;

  case 5:
#line 296 "parser.y"
    { (yyval.ifref_list) = append_ifref( (yyvsp[-1].ifref_list), make_ifref((yyvsp[0].type)) ); ;}
    break;

  case 6:
#line 297 "parser.y"
    { (yyval.ifref_list) = (yyvsp[-2].ifref_list);
						  reg_type((yyvsp[-1].type), (yyvsp[-1].type)->name, 0);
						  if (!parse_only && do_header) write_coclass_forward((yyvsp[-1].type));
						;}
    break;

  case 7:
#line 301 "parser.y"
    { (yyval.ifref_list) = (yyvsp[-1].ifref_list);
						  add_typelib_entry((yyvsp[0].type));
						  reg_type((yyvsp[0].type), (yyvsp[0].type)->name, 0);
						  if (!parse_only && do_header) write_coclass_forward((yyvsp[0].type));
						;}
    break;

  case 8:
#line 306 "parser.y"
    { (yyval.ifref_list) = (yyvsp[-1].ifref_list); add_typelib_entry((yyvsp[0].type)); ;}
    break;

  case 9:
#line 307 "parser.y"
    { (yyval.ifref_list) = (yyvsp[-1].ifref_list); ;}
    break;

  case 10:
#line 308 "parser.y"
    { (yyval.ifref_list) = (yyvsp[-1].ifref_list); ;}
    break;

  case 11:
#line 311 "parser.y"
    {;}
    break;

  case 12:
#line 312 "parser.y"
    { if (!parse_only) add_typelib_entry((yyvsp[0].type)); ;}
    break;

  case 13:
#line 313 "parser.y"
    { if (!parse_only) add_typelib_entry((yyvsp[0].type)); ;}
    break;

  case 14:
#line 314 "parser.y"
    { reg_type((yyvsp[-1].type), (yyvsp[-1].type)->name, 0); if (!parse_only && do_header) write_coclass_forward((yyvsp[-1].type)); ;}
    break;

  case 15:
#line 315 "parser.y"
    { if (!parse_only) add_typelib_entry((yyvsp[0].type));
						  reg_type((yyvsp[0].type), (yyvsp[0].type)->name, 0);
						  if (!parse_only && do_header) write_coclass_forward((yyvsp[0].type));
						;}
    break;

  case 16:
#line 319 "parser.y"
    { if (!parse_only) add_typelib_entry((yyvsp[0].type)); ;}
    break;

  case 17:
#line 320 "parser.y"
    {;}
    break;

  case 18:
#line 321 "parser.y"
    {;}
    break;

  case 19:
#line 324 "parser.y"
    { (yyval.func_list) = NULL; ;}
    break;

  case 20:
#line 325 "parser.y"
    { (yyval.func_list) = append_func( (yyvsp[-2].func_list), (yyvsp[-1].func) ); ;}
    break;

  case 21:
#line 326 "parser.y"
    { (yyval.func_list) = (yyvsp[-1].func_list); ;}
    break;

  case 22:
#line 329 "parser.y"
    {;}
    break;

  case 23:
#line 330 "parser.y"
    { if (!parse_only && do_header) { write_constdef((yyvsp[-1].var)); } ;}
    break;

  case 24:
#line 331 "parser.y"
    {;}
    break;

  case 25:
#line 332 "parser.y"
    { if (!parse_only && do_header) {
						    write_type_def_or_decl(header, (yyvsp[-1].type), FALSE, NULL);
						    fprintf(header, ";\n\n");
						  }
						;}
    break;

  case 26:
#line 337 "parser.y"
    { if (!parse_only && do_header) { write_externdef((yyvsp[-1].var)); } ;}
    break;

  case 27:
#line 338 "parser.y"
    {;}
    break;

  case 28:
#line 339 "parser.y"
    { if (!parse_only && do_header) {
						    write_type_def_or_decl(header, (yyvsp[-1].type), FALSE, NULL);
						    fprintf(header, ";\n\n");
						  }
						;}
    break;

  case 29:
#line 344 "parser.y"
    {;}
    break;

  case 30:
#line 345 "parser.y"
    { if (!parse_only && do_header) {
						    write_type_def_or_decl(header, (yyvsp[-1].type), FALSE, NULL);
						    fprintf(header, ";\n\n");
						  }
						;}
    break;

  case 31:
#line 352 "parser.y"
    { if (!parse_only && do_header) fprintf(header, "%s\n", (yyvsp[-1].str)); ;}
    break;

  case 32:
#line 354 "parser.y"
    { assert(yychar == YYEMPTY);
						  if (!do_import((yyvsp[-1].str))) yychar = aEOF; ;}
    break;

  case 33:
#line 357 "parser.y"
    {;}
    break;

  case 34:
#line 360 "parser.y"
    { if(!parse_only) add_importlib((yyvsp[-1].str)); ;}
    break;

  case 35:
#line 363 "parser.y"
    { (yyval.str) = (yyvsp[0].str); ;}
    break;

  case 36:
#line 365 "parser.y"
    { start_typelib((yyvsp[-1].str), (yyvsp[-2].attr_list));
						  if (!parse_only && do_header) write_library((yyvsp[-1].str), (yyvsp[-2].attr_list));
						  if (!parse_only && do_idfile) write_libid((yyvsp[-1].str), (yyvsp[-2].attr_list));
						;}
    break;

  case 37:
#line 370 "parser.y"
    { end_typelib(); ;}
    break;

  case 38:
#line 373 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 40:
#line 377 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 41:
#line 380 "parser.y"
    { check_arg((yyvsp[0].var)); (yyval.var_list) = append_var( NULL, (yyvsp[0].var) ); ;}
    break;

  case 42:
#line 381 "parser.y"
    { check_arg((yyvsp[0].var)); (yyval.var_list) = append_var( (yyvsp[-2].var_list), (yyvsp[0].var)); ;}
    break;

  case 44:
#line 386 "parser.y"
    { (yyval.var) = (yyvsp[-1].pident)->var;
						  (yyval.var)->attrs = (yyvsp[-3].attr_list);
						  set_type((yyval.var), (yyvsp[-2].type), (yyvsp[-1].pident)->ptr_level, (yyvsp[0].array_dims), TRUE);
						  free((yyvsp[-1].pident));
						;}
    break;

  case 45:
#line 391 "parser.y"
    { (yyval.var) = (yyvsp[-1].pident)->var;
						  set_type((yyval.var), (yyvsp[-2].type), (yyvsp[-1].pident)->ptr_level, (yyvsp[0].array_dims), TRUE);
						  free((yyvsp[-1].pident));
						;}
    break;

  case 46:
#line 395 "parser.y"
    { (yyval.var) = (yyvsp[-3].pident)->var;
						  (yyval.var)->attrs = (yyvsp[-5].attr_list);
						  set_type((yyval.var), (yyvsp[-4].type), (yyvsp[-3].pident)->ptr_level - 1, NULL, TRUE);
						  free((yyvsp[-3].pident));
						  (yyval.var)->args = (yyvsp[-1].var_list);
						;}
    break;

  case 47:
#line 401 "parser.y"
    { (yyval.var) = (yyvsp[-3].pident)->var;
						  set_type((yyval.var), (yyvsp[-4].type), (yyvsp[-3].pident)->ptr_level - 1, NULL, TRUE);
						  free((yyvsp[-3].pident));
						  (yyval.var)->args = (yyvsp[-1].var_list);
						;}
    break;

  case 48:
#line 408 "parser.y"
    { (yyval.array_dims) = NULL; ;}
    break;

  case 49:
#line 409 "parser.y"
    { (yyval.array_dims) = (yyvsp[-1].array_dims); ;}
    break;

  case 50:
#line 410 "parser.y"
    { (yyval.array_dims) = append_array( NULL, make_expr(EXPR_VOID) ); ;}
    break;

  case 51:
#line 413 "parser.y"
    { (yyval.array_dims) = append_array( NULL, (yyvsp[0].expr) ); ;}
    break;

  case 52:
#line 414 "parser.y"
    { (yyval.array_dims) = append_array( (yyvsp[-2].array_dims), (yyvsp[0].expr) ); ;}
    break;

  case 53:
#line 415 "parser.y"
    { (yyval.array_dims) = append_array( (yyvsp[-3].array_dims), (yyvsp[0].expr) ); ;}
    break;

  case 54:
#line 418 "parser.y"
    { (yyval.attr_list) = NULL; ;}
    break;

  case 56:
#line 423 "parser.y"
    { (yyval.attr_list) = (yyvsp[-1].attr_list);
						  if (!(yyval.attr_list))
						    error_loc("empty attribute lists unsupported\n");
						;}
    break;

  case 57:
#line 429 "parser.y"
    { (yyval.attr_list) = append_attr( NULL, (yyvsp[0].attr) ); ;}
    break;

  case 58:
#line 430 "parser.y"
    { (yyval.attr_list) = append_attr( (yyvsp[-2].attr_list), (yyvsp[0].attr) ); ;}
    break;

  case 59:
#line 431 "parser.y"
    { (yyval.attr_list) = append_attr( (yyvsp[-3].attr_list), (yyvsp[0].attr) ); ;}
    break;

  case 60:
#line 434 "parser.y"
    { (yyval.str_list) = append_str( NULL, (yyvsp[0].str) ); ;}
    break;

  case 61:
#line 435 "parser.y"
    { (yyval.str_list) = append_str( (yyvsp[-2].str_list), (yyvsp[0].str) ); ;}
    break;

  case 62:
#line 438 "parser.y"
    { (yyval.attr) = NULL; ;}
    break;

  case 63:
#line 439 "parser.y"
    { (yyval.attr) = make_attr(ATTR_AGGREGATABLE); ;}
    break;

  case 64:
#line 440 "parser.y"
    { (yyval.attr) = make_attr(ATTR_APPOBJECT); ;}
    break;

  case 65:
#line 441 "parser.y"
    { (yyval.attr) = make_attr(ATTR_ASYNC); ;}
    break;

  case 66:
#line 442 "parser.y"
    { (yyval.attr) = make_attr(ATTR_AUTO_HANDLE); ;}
    break;

  case 67:
#line 443 "parser.y"
    { (yyval.attr) = make_attr(ATTR_BINDABLE); ;}
    break;

  case 68:
#line 444 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_CALLAS, (yyvsp[-1].var)); ;}
    break;

  case 69:
#line 445 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_CASE, (yyvsp[-1].expr_list)); ;}
    break;

  case 70:
#line 446 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); ;}
    break;

  case 71:
#line 447 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_DONT_SERIALIZE */ ;}
    break;

  case 72:
#line 448 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_SERIALIZE */ ;}
    break;

  case 73:
#line 449 "parser.y"
    { (yyval.attr) = make_attr(ATTR_CONTROL); ;}
    break;

  case 74:
#line 450 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULT); ;}
    break;

  case 75:
#line 451 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULTCOLLELEM); ;}
    break;

  case 76:
#line 452 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_DEFAULTVALUE_EXPR, (yyvsp[-1].expr)); ;}
    break;

  case 77:
#line 453 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_DEFAULTVALUE_STRING, (yyvsp[-1].str)); ;}
    break;

  case 78:
#line 454 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULTVTABLE); ;}
    break;

  case 79:
#line 455 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DISPLAYBIND); ;}
    break;

  case 80:
#line 456 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_DLLNAME, (yyvsp[-1].str)); ;}
    break;

  case 81:
#line 457 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DUAL); ;}
    break;

  case 82:
#line 458 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ENDPOINT, (yyvsp[-1].str_list)); ;}
    break;

  case 83:
#line 459 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ENTRY_STRING, (yyvsp[-1].str)); ;}
    break;

  case 84:
#line 460 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ENTRY_ORDINAL, (yyvsp[-1].expr)); ;}
    break;

  case 85:
#line 461 "parser.y"
    { (yyval.attr) = make_attr(ATTR_EXPLICIT_HANDLE); ;}
    break;

  case 86:
#line 462 "parser.y"
    { (yyval.attr) = make_attr(ATTR_HANDLE); ;}
    break;

  case 87:
#line 463 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPCONTEXT, (yyvsp[-1].expr)); ;}
    break;

  case 88:
#line 464 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPFILE, (yyvsp[-1].str)); ;}
    break;

  case 89:
#line 465 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRING, (yyvsp[-1].str)); ;}
    break;

  case 90:
#line 466 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRINGCONTEXT, (yyvsp[-1].expr)); ;}
    break;

  case 91:
#line 467 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRINGDLL, (yyvsp[-1].str)); ;}
    break;

  case 92:
#line 468 "parser.y"
    { (yyval.attr) = make_attr(ATTR_HIDDEN); ;}
    break;

  case 93:
#line 469 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ID, (yyvsp[-1].expr)); ;}
    break;

  case 94:
#line 470 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IDEMPOTENT); ;}
    break;

  case 95:
#line 471 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_IIDIS, (yyvsp[-1].expr)); ;}
    break;

  case 96:
#line 472 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IMMEDIATEBIND); ;}
    break;

  case 97:
#line 473 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_IMPLICIT_HANDLE, (yyvsp[-1].str)); ;}
    break;

  case 98:
#line 474 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IN); ;}
    break;

  case 99:
#line 475 "parser.y"
    { (yyval.attr) = make_attr(ATTR_INPUTSYNC); ;}
    break;

  case 100:
#line 476 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_LENGTHIS, (yyvsp[-1].expr_list)); ;}
    break;

  case 101:
#line 477 "parser.y"
    { (yyval.attr) = make_attr(ATTR_LOCAL); ;}
    break;

  case 102:
#line 478 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONBROWSABLE); ;}
    break;

  case 103:
#line 479 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONCREATABLE); ;}
    break;

  case 104:
#line 480 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONEXTENSIBLE); ;}
    break;

  case 105:
#line 481 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OBJECT); ;}
    break;

  case 106:
#line 482 "parser.y"
    { (yyval.attr) = make_attr(ATTR_ODL); ;}
    break;

  case 107:
#line 483 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OLEAUTOMATION); ;}
    break;

  case 108:
#line 484 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OPTIONAL); ;}
    break;

  case 109:
#line 485 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OUT); ;}
    break;

  case 110:
#line 486 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_POINTERDEFAULT, (yyvsp[-1].num)); ;}
    break;

  case 111:
#line 487 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPGET); ;}
    break;

  case 112:
#line 488 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPPUT); ;}
    break;

  case 113:
#line 489 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPPUTREF); ;}
    break;

  case 114:
#line 490 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PUBLIC); ;}
    break;

  case 115:
#line 491 "parser.y"
    { expr_list_t *list = append_expr( NULL, (yyvsp[-3].expr) );
                                                     list = append_expr( list, (yyvsp[-1].expr) );
                                                     (yyval.attr) = make_attrp(ATTR_RANGE, list); ;}
    break;

  case 116:
#line 494 "parser.y"
    { (yyval.attr) = make_attr(ATTR_READONLY); ;}
    break;

  case 117:
#line 495 "parser.y"
    { (yyval.attr) = make_attr(ATTR_REQUESTEDIT); ;}
    break;

  case 118:
#line 496 "parser.y"
    { (yyval.attr) = make_attr(ATTR_RESTRICTED); ;}
    break;

  case 119:
#line 497 "parser.y"
    { (yyval.attr) = make_attr(ATTR_RETVAL); ;}
    break;

  case 120:
#line 498 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SIZEIS, (yyvsp[-1].expr_list)); ;}
    break;

  case 121:
#line 499 "parser.y"
    { (yyval.attr) = make_attr(ATTR_SOURCE); ;}
    break;

  case 122:
#line 500 "parser.y"
    { (yyval.attr) = make_attr(ATTR_STRING); ;}
    break;

  case 123:
#line 501 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SWITCHIS, (yyvsp[-1].expr)); ;}
    break;

  case 124:
#line 502 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SWITCHTYPE, (yyvsp[-1].type)); ;}
    break;

  case 125:
#line 503 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_TRANSMITAS, (yyvsp[-1].type)); ;}
    break;

  case 126:
#line 504 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_UUID, (yyvsp[-1].uuid)); ;}
    break;

  case 127:
#line 505 "parser.y"
    { (yyval.attr) = make_attr(ATTR_V1ENUM); ;}
    break;

  case 128:
#line 506 "parser.y"
    { (yyval.attr) = make_attr(ATTR_VARARG); ;}
    break;

  case 129:
#line 507 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_VERSION, (yyvsp[-1].num)); ;}
    break;

  case 130:
#line 508 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_WIREMARSHAL, (yyvsp[-1].type)); ;}
    break;

  case 131:
#line 509 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_POINTERTYPE, (yyvsp[0].num)); ;}
    break;

  case 133:
#line 514 "parser.y"
    { if (!is_valid_uuid((yyvsp[0].str)))
						    error_loc("invalid UUID: %s\n", (yyvsp[0].str));
						  (yyval.uuid) = parse_uuid((yyvsp[0].str)); ;}
    break;

  case 136:
#line 523 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 137:
#line 524 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[-1].var_list), (yyvsp[0].var) ); ;}
    break;

  case 138:
#line 527 "parser.y"
    { attr_t *a = make_attrp(ATTR_CASE, append_expr( NULL, (yyvsp[-2].expr) ));
						  (yyval.var) = (yyvsp[0].var); if (!(yyval.var)) (yyval.var) = make_var(NULL);
						  (yyval.var)->attrs = append_attr( (yyval.var)->attrs, a );
						;}
    break;

  case 139:
#line 531 "parser.y"
    { attr_t *a = make_attr(ATTR_DEFAULT);
						  (yyval.var) = (yyvsp[0].var); if (!(yyval.var)) (yyval.var) = make_var(NULL);
						  (yyval.var)->attrs = append_attr( (yyval.var)->attrs, a );
						;}
    break;

  case 140:
#line 537 "parser.y"
    { (yyval.var) = reg_const((yyvsp[-2].var));
						  set_type((yyval.var), (yyvsp[-3].type), 0, NULL, FALSE);
						  (yyval.var)->eval = (yyvsp[0].expr);
						;}
    break;

  case 141:
#line 543 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 142:
#line 544 "parser.y"
    { (yyval.var_list) = (yyvsp[-1].var_list); ;}
    break;

  case 144:
#line 548 "parser.y"
    { if (!(yyvsp[0].var)->eval)
						    (yyvsp[0].var)->eval = make_exprl(EXPR_NUM, 0 /* default for first enum entry */);
                                                  (yyval.var_list) = append_var( NULL, (yyvsp[0].var) );
						;}
    break;

  case 145:
#line 552 "parser.y"
    { if (!(yyvsp[0].var)->eval)
                                                  {
                                                    var_t *last = LIST_ENTRY( list_tail((yyval.var_list)), var_t, entry );
                                                    (yyvsp[0].var)->eval = make_exprl(EXPR_NUM, last->eval->cval + 1);
                                                  }
                                                  (yyval.var_list) = append_var( (yyvsp[-2].var_list), (yyvsp[0].var) );
						;}
    break;

  case 146:
#line 561 "parser.y"
    { (yyval.var) = reg_const((yyvsp[-2].var));
						  (yyval.var)->eval = (yyvsp[0].expr);
                                                  (yyval.var)->type = make_int(0);
						;}
    break;

  case 147:
#line 565 "parser.y"
    { (yyval.var) = reg_const((yyvsp[0].var));
                                                  (yyval.var)->type = make_int(0);
						;}
    break;

  case 148:
#line 570 "parser.y"
    { (yyval.type) = get_typev(RPC_FC_ENUM16, (yyvsp[-3].var), tsENUM);
						  (yyval.type)->kind = TKIND_ENUM;
						  (yyval.type)->fields = (yyvsp[-1].var_list);
						  (yyval.type)->defined = TRUE;
                                                  if(in_typelib)
                                                      add_typelib_entry((yyval.type));
						;}
    break;

  case 149:
#line 579 "parser.y"
    { (yyval.expr_list) = append_expr( NULL, (yyvsp[0].expr) ); ;}
    break;

  case 150:
#line 580 "parser.y"
    { (yyval.expr_list) = append_expr( (yyvsp[-2].expr_list), (yyvsp[0].expr) ); ;}
    break;

  case 151:
#line 593 "parser.y"
    { (yyval.expr) = make_expr(EXPR_VOID); ;}
    break;

  case 153:
#line 597 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_NUM, (yyvsp[0].num)); ;}
    break;

  case 154:
#line 598 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_HEXNUM, (yyvsp[0].num)); ;}
    break;

  case 155:
#line 599 "parser.y"
    { (yyval.expr) = make_exprd(EXPR_DOUBLE, (yyvsp[0].dbl)); ;}
    break;

  case 156:
#line 600 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_TRUEFALSE, 0); ;}
    break;

  case 157:
#line 601 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_TRUEFALSE, 1); ;}
    break;

  case 158:
#line 602 "parser.y"
    { (yyval.expr) = make_exprs(EXPR_IDENTIFIER, (yyvsp[0].str)); ;}
    break;

  case 159:
#line 603 "parser.y"
    { (yyval.expr) = make_expr3(EXPR_COND, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 160:
#line 604 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_OR , (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 161:
#line 605 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_AND, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 162:
#line 606 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_ADD, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 163:
#line 607 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SUB, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 164:
#line 608 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MUL, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 165:
#line 609 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_DIV, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 166:
#line 610 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SHL, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 167:
#line 611 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SHR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 168:
#line 612 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_NOT, (yyvsp[0].expr)); ;}
    break;

  case 169:
#line 613 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_NEG, (yyvsp[0].expr)); ;}
    break;

  case 170:
#line 614 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_ADDRESSOF, (yyvsp[0].expr)); ;}
    break;

  case 171:
#line 615 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_PPTR, (yyvsp[0].expr)); ;}
    break;

  case 172:
#line 616 "parser.y"
    { (yyval.expr) = make_exprt(EXPR_CAST, (yyvsp[-2].type), (yyvsp[0].expr)); ;}
    break;

  case 173:
#line 617 "parser.y"
    { (yyval.expr) = make_exprt(EXPR_SIZEOF, (yyvsp[-1].type), NULL); ;}
    break;

  case 174:
#line 618 "parser.y"
    { (yyval.expr) = (yyvsp[-1].expr); ;}
    break;

  case 175:
#line 621 "parser.y"
    { (yyval.expr_list) = append_expr( NULL, (yyvsp[0].expr) ); ;}
    break;

  case 176:
#line 622 "parser.y"
    { (yyval.expr_list) = append_expr( (yyvsp[-2].expr_list), (yyvsp[0].expr) ); ;}
    break;

  case 177:
#line 625 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr);
						  if (!(yyval.expr)->is_const)
						      error_loc("expression is not constant\n");
						;}
    break;

  case 178:
#line 631 "parser.y"
    { (yyval.var) = (yyvsp[0].var);
						  set_type((yyval.var), (yyvsp[-1].type), 0, NULL, FALSE);
						;}
    break;

  case 179:
#line 636 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 180:
#line 637 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[-1].var_list), (yyvsp[0].var) ); ;}
    break;

  case 181:
#line 640 "parser.y"
    { (yyval.var) = (yyvsp[-1].var); ;}
    break;

  case 182:
#line 641 "parser.y"
    { (yyval.var) = make_var(NULL); (yyval.var)->type = (yyvsp[-1].type); (yyval.var)->attrs = (yyvsp[-2].attr_list); ;}
    break;

  case 183:
#line 642 "parser.y"
    { (yyval.var) = make_var(NULL); (yyval.var)->attrs = (yyvsp[-1].attr_list); ;}
    break;

  case 184:
#line 643 "parser.y"
    { (yyval.var) = NULL; ;}
    break;

  case 185:
#line 646 "parser.y"
    { (yyval.var) = (yyvsp[-1].pident)->var;
						  (yyval.var)->attrs = (yyvsp[-3].attr_list);
						  set_type((yyval.var), (yyvsp[-2].type), (yyvsp[-1].pident)->ptr_level, (yyvsp[0].array_dims), FALSE);
						  free((yyvsp[-1].pident));
						;}
    break;

  case 186:
#line 655 "parser.y"
    { var_t *v = (yyvsp[-3].pident)->var;
						  v->attrs = (yyvsp[-6].attr_list);
						  set_type(v, (yyvsp[-5].type), (yyvsp[-3].pident)->ptr_level, NULL, FALSE);
						  free((yyvsp[-3].pident));
						  (yyval.func) = make_func(v, (yyvsp[-1].var_list));
						  if (is_attr(v->attrs, ATTR_IN)) {
						    error_loc("inapplicable attribute [in] for function '%s'\n",(yyval.func)->def->name);
						  }
						;}
    break;

  case 187:
#line 666 "parser.y"
    { (yyval.var) = NULL; ;}
    break;

  case 189:
#line 670 "parser.y"
    { (yyval.var) = NULL; ;}
    break;

  case 190:
#line 671 "parser.y"
    { (yyval.var) = make_var((yyvsp[0].str)); ;}
    break;

  case 191:
#line 672 "parser.y"
    { (yyval.var) = make_var((yyvsp[0].str)); ;}
    break;

  case 192:
#line 675 "parser.y"
    { (yyval.var) = make_var((yyvsp[0].str)); ;}
    break;

  case 193:
#line 677 "parser.y"
    { (yyval.var) = make_var((yyvsp[0].str)); ;}
    break;

  case 194:
#line 680 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 195:
#line 681 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 197:
#line 683 "parser.y"
    { (yyval.type) = (yyvsp[0].type); (yyval.type)->sign = 1; ;}
    break;

  case 198:
#line 684 "parser.y"
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

  case 199:
#line 700 "parser.y"
    { (yyval.type) = make_int(-1); ;}
    break;

  case 200:
#line 701 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 201:
#line 702 "parser.y"
    { (yyval.type) = duptype(find_type("float", 0), 1); ;}
    break;

  case 202:
#line 703 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 203:
#line 704 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 204:
#line 705 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 205:
#line 706 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 208:
#line 713 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 209:
#line 714 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[-1].str)); ;}
    break;

  case 210:
#line 715 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 211:
#line 716 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[-1].str)); ;}
    break;

  case 212:
#line 717 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[-1].str)); ;}
    break;

  case 213:
#line 718 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 214:
#line 719 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 215:
#line 722 "parser.y"
    { (yyval.type) = make_class((yyvsp[0].str)); ;}
    break;

  case 216:
#line 723 "parser.y"
    { (yyval.type) = find_type((yyvsp[0].str), 0);
						  if ((yyval.type)->defined) error_loc("multiple definition error\n");
						  if ((yyval.type)->kind != TKIND_COCLASS) error_loc("%s was not declared a coclass\n", (yyvsp[0].str));
						;}
    break;

  case 217:
#line 729 "parser.y"
    { (yyval.type) = (yyvsp[0].type);
						  (yyval.type)->attrs = (yyvsp[-1].attr_list);
						  if (!parse_only && do_header)
						    write_coclass((yyval.type));
						  if (!parse_only && do_idfile)
						    write_clsid((yyval.type));
						;}
    break;

  case 218:
#line 738 "parser.y"
    { (yyval.type) = (yyvsp[-3].type);
						  (yyval.type)->ifaces = (yyvsp[-1].ifref_list);
						  (yyval.type)->defined = TRUE;
						;}
    break;

  case 219:
#line 744 "parser.y"
    { (yyval.ifref_list) = NULL; ;}
    break;

  case 220:
#line 745 "parser.y"
    { (yyval.ifref_list) = append_ifref( (yyvsp[-1].ifref_list), (yyvsp[0].ifref) ); ;}
    break;

  case 221:
#line 749 "parser.y"
    { (yyval.ifref) = make_ifref((yyvsp[0].type)); (yyval.ifref)->attrs = (yyvsp[-1].attr_list); ;}
    break;

  case 222:
#line 752 "parser.y"
    { (yyval.type) = get_type(0, (yyvsp[0].str), 0); (yyval.type)->kind = TKIND_DISPATCH; ;}
    break;

  case 223:
#line 753 "parser.y"
    { (yyval.type) = get_type(0, (yyvsp[0].str), 0); (yyval.type)->kind = TKIND_DISPATCH; ;}
    break;

  case 224:
#line 756 "parser.y"
    { attr_t *attrs;
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

  case 225:
#line 768 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 226:
#line 769 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[-2].var_list), (yyvsp[-1].var) ); ;}
    break;

  case 227:
#line 772 "parser.y"
    { (yyval.func_list) = NULL; ;}
    break;

  case 228:
#line 773 "parser.y"
    { (yyval.func_list) = append_func( (yyvsp[-2].func_list), (yyvsp[-1].func) ); ;}
    break;

  case 229:
#line 779 "parser.y"
    { (yyval.type) = (yyvsp[-4].type);
						  (yyval.type)->fields = (yyvsp[-2].var_list);
						  (yyval.type)->funcs = (yyvsp[-1].func_list);
						  if (!parse_only && do_header) write_dispinterface((yyval.type));
						  if (!parse_only && do_idfile) write_diid((yyval.type));
						;}
    break;

  case 230:
#line 786 "parser.y"
    { (yyval.type) = (yyvsp[-4].type);
						  (yyval.type)->fields = (yyvsp[-2].type)->fields;
						  (yyval.type)->funcs = (yyvsp[-2].type)->funcs;
						  if (!parse_only && do_header) write_dispinterface((yyval.type));
						  if (!parse_only && do_idfile) write_diid((yyval.type));
						;}
    break;

  case 231:
#line 794 "parser.y"
    { (yyval.type) = NULL; ;}
    break;

  case 232:
#line 795 "parser.y"
    { (yyval.type) = find_type2((yyvsp[0].str), 0); ;}
    break;

  case 233:
#line 798 "parser.y"
    { (yyval.type) = get_type(RPC_FC_IP, (yyvsp[0].str), 0); (yyval.type)->kind = TKIND_INTERFACE; ;}
    break;

  case 234:
#line 799 "parser.y"
    { (yyval.type) = get_type(RPC_FC_IP, (yyvsp[0].str), 0); (yyval.type)->kind = TKIND_INTERFACE; ;}
    break;

  case 235:
#line 802 "parser.y"
    { (yyval.ifinfo).interface = (yyvsp[0].type);
						  (yyval.ifinfo).old_pointer_default = pointer_default;
						  if (is_attr((yyvsp[-1].attr_list), ATTR_POINTERDEFAULT))
						    pointer_default = get_attrv((yyvsp[-1].attr_list), ATTR_POINTERDEFAULT);
						  if ((yyvsp[0].type)->defined) error_loc("multiple definition error\n");
						  (yyvsp[0].type)->attrs = (yyvsp[-1].attr_list);
						  (yyvsp[0].type)->defined = TRUE;
						  if (!parse_only && do_header) write_forward((yyvsp[0].type));
						;}
    break;

  case 236:
#line 814 "parser.y"
    { (yyval.type) = (yyvsp[-4].ifinfo).interface;
						  (yyval.type)->ref = (yyvsp[-3].type);
						  (yyval.type)->funcs = (yyvsp[-1].func_list);
						  compute_method_indexes((yyval.type));
						  if (!parse_only && do_header) write_interface((yyval.type));
						  if (!parse_only && local_stubs) write_locals(local_stubs, (yyval.type), TRUE);
						  if (!parse_only && do_idfile) write_iid((yyval.type));
						  pointer_default = (yyvsp[-4].ifinfo).old_pointer_default;
						;}
    break;

  case 237:
#line 826 "parser.y"
    { (yyval.type) = (yyvsp[-6].ifinfo).interface;
						  (yyval.type)->ref = find_type2((yyvsp[-4].str), 0);
						  if (!(yyval.type)->ref) error_loc("base class '%s' not found in import\n", (yyvsp[-4].str));
						  (yyval.type)->funcs = (yyvsp[-1].func_list);
						  compute_method_indexes((yyval.type));
						  if (!parse_only && do_header) write_interface((yyval.type));
						  if (!parse_only && local_stubs) write_locals(local_stubs, (yyval.type), TRUE);
						  if (!parse_only && do_idfile) write_iid((yyval.type));
						  pointer_default = (yyvsp[-6].ifinfo).old_pointer_default;
						;}
    break;

  case 238:
#line 836 "parser.y"
    { (yyval.type) = (yyvsp[0].type); ;}
    break;

  case 239:
#line 840 "parser.y"
    { (yyval.type) = (yyvsp[-1].type); if (!parse_only && do_header) write_forward((yyval.type)); ;}
    break;

  case 240:
#line 841 "parser.y"
    { (yyval.type) = (yyvsp[-1].type); if (!parse_only && do_header) write_forward((yyval.type)); ;}
    break;

  case 241:
#line 844 "parser.y"
    { (yyval.type) = make_type(0, NULL); (yyval.type)->name = (yyvsp[0].str); (yyval.type)->kind = TKIND_MODULE; ;}
    break;

  case 242:
#line 845 "parser.y"
    { (yyval.type) = make_type(0, NULL); (yyval.type)->name = (yyvsp[0].str); (yyval.type)->kind = TKIND_MODULE; ;}
    break;

  case 243:
#line 848 "parser.y"
    { (yyval.type) = (yyvsp[0].type);
						  (yyval.type)->attrs = (yyvsp[-1].attr_list);
						;}
    break;

  case 244:
#line 853 "parser.y"
    { (yyval.type) = (yyvsp[-3].type);
						  (yyval.type)->funcs = (yyvsp[-1].func_list);
						  /* FIXME: if (!parse_only && do_header) write_module($$); */
						;}
    break;

  case 245:
#line 859 "parser.y"
    { (yyval.pident) = (yyvsp[0].pident); (yyval.pident)->ptr_level++; ;}
    break;

  case 246:
#line 860 "parser.y"
    { (yyval.pident) = (yyvsp[0].pident); /* FIXME */ ;}
    break;

  case 247:
#line 863 "parser.y"
    { (yyval.pident) = make_pident((yyvsp[0].var)); ;}
    break;

  case 249:
#line 865 "parser.y"
    { (yyval.pident) = (yyvsp[-1].pident); ;}
    break;

  case 250:
#line 869 "parser.y"
    { (yyval.pident_list) = append_pident( NULL, (yyvsp[0].pident) ); ;}
    break;

  case 251:
#line 870 "parser.y"
    { (yyval.pident_list) = append_pident( (yyvsp[-2].pident_list), (yyvsp[0].pident) ); ;}
    break;

  case 252:
#line 874 "parser.y"
    { (yyval.num) = RPC_FC_RP; ;}
    break;

  case 253:
#line 875 "parser.y"
    { (yyval.num) = RPC_FC_UP; ;}
    break;

  case 254:
#line 876 "parser.y"
    { (yyval.num) = RPC_FC_FP; ;}
    break;

  case 255:
#line 879 "parser.y"
    { (yyval.type) = get_typev(RPC_FC_STRUCT, (yyvsp[-3].var), tsSTRUCT);
                                                  /* overwrite RPC_FC_STRUCT with a more exact type */
						  (yyval.type)->type = get_struct_type( (yyvsp[-1].var_list) );
						  (yyval.type)->kind = TKIND_RECORD;
						  (yyval.type)->fields = (yyvsp[-1].var_list);
						  (yyval.type)->defined = TRUE;
                                                  if(in_typelib)
                                                      add_typelib_entry((yyval.type));
                                                ;}
    break;

  case 256:
#line 890 "parser.y"
    { (yyval.type) = duptype(find_type("void", 0), 1); ;}
    break;

  case 257:
#line 891 "parser.y"
    { (yyval.type) = find_type((yyvsp[0].str), 0); ;}
    break;

  case 258:
#line 892 "parser.y"
    { (yyval.type) = (yyvsp[0].type); ;}
    break;

  case 259:
#line 893 "parser.y"
    { (yyval.type) = duptype((yyvsp[0].type), 1); (yyval.type)->is_const = TRUE; ;}
    break;

  case 260:
#line 894 "parser.y"
    { (yyval.type) = (yyvsp[0].type); ;}
    break;

  case 261:
#line 895 "parser.y"
    { (yyval.type) = find_type2((yyvsp[0].str), tsENUM); ;}
    break;

  case 262:
#line 896 "parser.y"
    { (yyval.type) = (yyvsp[0].type); ;}
    break;

  case 263:
#line 897 "parser.y"
    { (yyval.type) = get_type(RPC_FC_STRUCT, (yyvsp[0].str), tsSTRUCT); ;}
    break;

  case 264:
#line 898 "parser.y"
    { (yyval.type) = (yyvsp[0].type); ;}
    break;

  case 265:
#line 899 "parser.y"
    { (yyval.type) = find_type2((yyvsp[0].str), tsUNION); ;}
    break;

  case 266:
#line 900 "parser.y"
    { (yyval.type) = make_safearray((yyvsp[-1].type)); ;}
    break;

  case 267:
#line 903 "parser.y"
    { reg_typedefs((yyvsp[-1].type), (yyvsp[0].pident_list), (yyvsp[-2].attr_list));
						  process_typedefs((yyvsp[0].pident_list));
						;}
    break;

  case 268:
#line 908 "parser.y"
    { (yyval.type) = get_typev(RPC_FC_NON_ENCAPSULATED_UNION, (yyvsp[-3].var), tsUNION);
						  (yyval.type)->kind = TKIND_UNION;
						  (yyval.type)->fields = (yyvsp[-1].var_list);
						  (yyval.type)->defined = TRUE;
						;}
    break;

  case 269:
#line 915 "parser.y"
    { var_t *u = (yyvsp[-3].var);
						  (yyval.type) = get_typev(RPC_FC_ENCAPSULATED_UNION, (yyvsp[-8].var), tsUNION);
						  (yyval.type)->kind = TKIND_UNION;
						  if (!u) u = make_var( xstrdup("tagged_union") );
						  u->type = make_type(RPC_FC_NON_ENCAPSULATED_UNION, NULL);
						  u->type->kind = TKIND_UNION;
						  u->type->fields = (yyvsp[-1].var_list);
						  u->type->defined = TRUE;
						  (yyval.type)->fields = append_var( (yyval.type)->fields, (yyvsp[-5].var) );
						  (yyval.type)->fields = append_var( (yyval.type)->fields, u );
						  (yyval.type)->defined = TRUE;
						;}
    break;

  case 270:
#line 930 "parser.y"
    { (yyval.num) = MAKEVERSION((yyvsp[0].num), 0); ;}
    break;

  case 271:
#line 931 "parser.y"
    { (yyval.num) = MAKEVERSION((yyvsp[-2].num), (yyvsp[0].num)); ;}
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
#line 3627 "parser.tab.c"

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


#line 934 "parser.y"


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
  t->fields = NULL;
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

static void set_type(var_t *v, type_t *type, int ptr_level, array_dims_t *arr,
                     int top)
{
  expr_list_t *sizes = get_attrp(v->attrs, ATTR_SIZEIS);
  expr_list_t *lengs = get_attrp(v->attrs, ATTR_LENGTHIS);
  int ptr_attr = get_attrv(v->attrs, ATTR_POINTERTYPE);
  int ptr_type = ptr_attr;
  int sizeless, has_varconf;
  expr_t *dim;
  type_t *atype, **ptype;

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
    case RPC_FC_ENUM16:
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
      check_for_user_types_and_context_handles(f->args);
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

