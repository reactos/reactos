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
     aSTRING = 262,
     aUUID = 263,
     aEOF = 264,
     SHL = 265,
     SHR = 266,
     tAGGREGATABLE = 267,
     tALLOCATE = 268,
     tAPPOBJECT = 269,
     tASYNC = 270,
     tASYNCUUID = 271,
     tAUTOHANDLE = 272,
     tBINDABLE = 273,
     tBOOLEAN = 274,
     tBROADCAST = 275,
     tBYTE = 276,
     tBYTECOUNT = 277,
     tCALLAS = 278,
     tCALLBACK = 279,
     tCASE = 280,
     tCDECL = 281,
     tCHAR = 282,
     tCOCLASS = 283,
     tCODE = 284,
     tCOMMSTATUS = 285,
     tCONST = 286,
     tCONTEXTHANDLE = 287,
     tCONTEXTHANDLENOSERIALIZE = 288,
     tCONTEXTHANDLESERIALIZE = 289,
     tCONTROL = 290,
     tCPPQUOTE = 291,
     tDEFAULT = 292,
     tDEFAULTCOLLELEM = 293,
     tDEFAULTVALUE = 294,
     tDEFAULTVTABLE = 295,
     tDISPLAYBIND = 296,
     tDISPINTERFACE = 297,
     tDLLNAME = 298,
     tDOUBLE = 299,
     tDUAL = 300,
     tENDPOINT = 301,
     tENTRY = 302,
     tENUM = 303,
     tERRORSTATUST = 304,
     tEXPLICITHANDLE = 305,
     tEXTERN = 306,
     tFALSE = 307,
     tFLOAT = 308,
     tHANDLE = 309,
     tHANDLET = 310,
     tHELPCONTEXT = 311,
     tHELPFILE = 312,
     tHELPSTRING = 313,
     tHELPSTRINGCONTEXT = 314,
     tHELPSTRINGDLL = 315,
     tHIDDEN = 316,
     tHYPER = 317,
     tID = 318,
     tIDEMPOTENT = 319,
     tIIDIS = 320,
     tIMMEDIATEBIND = 321,
     tIMPLICITHANDLE = 322,
     tIMPORT = 323,
     tIMPORTLIB = 324,
     tIN = 325,
     tINLINE = 326,
     tINPUTSYNC = 327,
     tINT = 328,
     tINT64 = 329,
     tINTERFACE = 330,
     tLCID = 331,
     tLENGTHIS = 332,
     tLIBRARY = 333,
     tLOCAL = 334,
     tLONG = 335,
     tMETHODS = 336,
     tMODULE = 337,
     tNONBROWSABLE = 338,
     tNONCREATABLE = 339,
     tNONEXTENSIBLE = 340,
     tOBJECT = 341,
     tODL = 342,
     tOLEAUTOMATION = 343,
     tOPTIONAL = 344,
     tOUT = 345,
     tPOINTERDEFAULT = 346,
     tPROPERTIES = 347,
     tPROPGET = 348,
     tPROPPUT = 349,
     tPROPPUTREF = 350,
     tPTR = 351,
     tPUBLIC = 352,
     tRANGE = 353,
     tREADONLY = 354,
     tREF = 355,
     tREQUESTEDIT = 356,
     tRESTRICTED = 357,
     tRETVAL = 358,
     tSAFEARRAY = 359,
     tSHORT = 360,
     tSIGNED = 361,
     tSINGLE = 362,
     tSIZEIS = 363,
     tSIZEOF = 364,
     tSMALL = 365,
     tSOURCE = 366,
     tSTDCALL = 367,
     tSTRING = 368,
     tSTRUCT = 369,
     tSWITCH = 370,
     tSWITCHIS = 371,
     tSWITCHTYPE = 372,
     tTRANSMITAS = 373,
     tTRUE = 374,
     tTYPEDEF = 375,
     tUNION = 376,
     tUNIQUE = 377,
     tUNSIGNED = 378,
     tUUID = 379,
     tV1ENUM = 380,
     tVARARG = 381,
     tVERSION = 382,
     tVOID = 383,
     tWCHAR = 384,
     tWIREMARSHAL = 385,
     CAST = 386,
     PPTR = 387,
     NEG = 388
   };
#endif
/* Tokens.  */
#define aIDENTIFIER 258
#define aKNOWNTYPE 259
#define aNUM 260
#define aHEXNUM 261
#define aSTRING 262
#define aUUID 263
#define aEOF 264
#define SHL 265
#define SHR 266
#define tAGGREGATABLE 267
#define tALLOCATE 268
#define tAPPOBJECT 269
#define tASYNC 270
#define tASYNCUUID 271
#define tAUTOHANDLE 272
#define tBINDABLE 273
#define tBOOLEAN 274
#define tBROADCAST 275
#define tBYTE 276
#define tBYTECOUNT 277
#define tCALLAS 278
#define tCALLBACK 279
#define tCASE 280
#define tCDECL 281
#define tCHAR 282
#define tCOCLASS 283
#define tCODE 284
#define tCOMMSTATUS 285
#define tCONST 286
#define tCONTEXTHANDLE 287
#define tCONTEXTHANDLENOSERIALIZE 288
#define tCONTEXTHANDLESERIALIZE 289
#define tCONTROL 290
#define tCPPQUOTE 291
#define tDEFAULT 292
#define tDEFAULTCOLLELEM 293
#define tDEFAULTVALUE 294
#define tDEFAULTVTABLE 295
#define tDISPLAYBIND 296
#define tDISPINTERFACE 297
#define tDLLNAME 298
#define tDOUBLE 299
#define tDUAL 300
#define tENDPOINT 301
#define tENTRY 302
#define tENUM 303
#define tERRORSTATUST 304
#define tEXPLICITHANDLE 305
#define tEXTERN 306
#define tFALSE 307
#define tFLOAT 308
#define tHANDLE 309
#define tHANDLET 310
#define tHELPCONTEXT 311
#define tHELPFILE 312
#define tHELPSTRING 313
#define tHELPSTRINGCONTEXT 314
#define tHELPSTRINGDLL 315
#define tHIDDEN 316
#define tHYPER 317
#define tID 318
#define tIDEMPOTENT 319
#define tIIDIS 320
#define tIMMEDIATEBIND 321
#define tIMPLICITHANDLE 322
#define tIMPORT 323
#define tIMPORTLIB 324
#define tIN 325
#define tINLINE 326
#define tINPUTSYNC 327
#define tINT 328
#define tINT64 329
#define tINTERFACE 330
#define tLCID 331
#define tLENGTHIS 332
#define tLIBRARY 333
#define tLOCAL 334
#define tLONG 335
#define tMETHODS 336
#define tMODULE 337
#define tNONBROWSABLE 338
#define tNONCREATABLE 339
#define tNONEXTENSIBLE 340
#define tOBJECT 341
#define tODL 342
#define tOLEAUTOMATION 343
#define tOPTIONAL 344
#define tOUT 345
#define tPOINTERDEFAULT 346
#define tPROPERTIES 347
#define tPROPGET 348
#define tPROPPUT 349
#define tPROPPUTREF 350
#define tPTR 351
#define tPUBLIC 352
#define tRANGE 353
#define tREADONLY 354
#define tREF 355
#define tREQUESTEDIT 356
#define tRESTRICTED 357
#define tRETVAL 358
#define tSAFEARRAY 359
#define tSHORT 360
#define tSIGNED 361
#define tSINGLE 362
#define tSIZEIS 363
#define tSIZEOF 364
#define tSMALL 365
#define tSOURCE 366
#define tSTDCALL 367
#define tSTRING 368
#define tSTRUCT 369
#define tSWITCH 370
#define tSWITCHIS 371
#define tSWITCHTYPE 372
#define tTRANSMITAS 373
#define tTRUE 374
#define tTYPEDEF 375
#define tUNION 376
#define tUNIQUE 377
#define tUNSIGNED 378
#define tUUID 379
#define tV1ENUM 380
#define tVARARG 381
#define tVERSION 382
#define tVOID 383
#define tWCHAR 384
#define tWIREMARSHAL 385
#define CAST 386
#define PPTR 387
#define NEG 388




/* Copy the first part of user declarations.  */
#line 1 "tools\\widl_new\\parser.y"

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

static str_list_t *append_str(str_list_t *list, char *str);
static attr_list_t *append_attr(attr_list_t *list, attr_t *attr);
static attr_t *make_attr(enum attr_type type);
static attr_t *make_attrv(enum attr_type type, unsigned long val);
static attr_t *make_attrp(enum attr_type type, void *val);
static expr_t *make_expr(enum expr_type type);
static expr_t *make_exprl(enum expr_type type, long val);
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
#line 122 "tools\\widl_new\\parser.y"
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
} YYSTYPE;
/* Line 196 of yacc.c.  */
#line 502 "tools\\widl_new\\parser.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */
#line 514 "tools\\widl_new\\parser.tab.c"

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
#define YYLAST   1073

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  153
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  75
/* YYNRULES -- Number of rules. */
#define YYNRULES  267
/* YYNRULES -- Number of states. */
#define YYNSTATES  494

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   388

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   135,     2,
     145,   146,   138,   137,   131,   136,   152,   139,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   133,   144,
       2,   151,     2,   132,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   149,     2,   150,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   147,   134,   148,   140,     2,     2,     2,
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
     125,   126,   127,   128,   129,   130,   141,   142,   143
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
     404,   409,   411,   412,   414,   415,   418,   423,   427,   433,
     434,   437,   439,   441,   445,   449,   451,   457,   459,   463,
     464,   466,   468,   470,   472,   474,   476,   482,   486,   490,
     494,   498,   502,   506,   510,   514,   517,   520,   523,   528,
     533,   537,   539,   543,   545,   550,   551,   554,   557,   561,
     564,   566,   571,   579,   580,   582,   583,   585,   587,   589,
     591,   593,   595,   597,   600,   603,   605,   607,   609,   611,
     613,   615,   617,   618,   620,   622,   625,   627,   630,   633,
     635,   637,   640,   643,   646,   651,   652,   655,   658,   661,
     664,   667,   670,   674,   677,   681,   687,   693,   694,   697,
     700,   703,   706,   712,   720,   722,   725,   728,   731,   734,
     737,   742,   745,   748,   750,   752,   756,   758,   762,   764,
     766,   768,   774,   776,   778,   780,   783,   785,   788,   790,
     793,   795,   798,   803,   808,   814,   825,   827
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short int yyrhs[] =
{
     154,     0,    -1,   155,    -1,    -1,   155,   215,    -1,   155,
     214,    -1,   155,   201,   144,    -1,   155,   203,    -1,   155,
     218,    -1,   155,   165,    -1,   155,   158,    -1,    -1,   156,
     215,    -1,   156,   214,    -1,   156,   201,   144,    -1,   156,
     203,    -1,   156,   218,    -1,   156,   158,    -1,   156,   162,
      -1,    -1,   157,   194,   144,    -1,   157,   158,    -1,   144,
      -1,   180,   144,    -1,   159,    -1,   184,   144,    -1,   190,
     144,    -1,   161,    -1,   223,   144,    -1,   225,   144,    -1,
     226,   144,    -1,    36,   145,     7,   146,    -1,    68,     7,
     144,    -1,   160,   156,     9,    -1,    69,   145,     7,   146,
      -1,    78,     3,    -1,   173,   163,   147,    -1,   164,   156,
     148,    -1,    -1,   168,    -1,   128,    -1,   169,    -1,   168,
     131,   169,    -1,   167,    -1,   173,   224,   220,   170,    -1,
     224,   220,   170,    -1,   173,   224,   220,   145,   166,   146,
      -1,   224,   220,   145,   166,   146,    -1,    -1,   149,   171,
     150,    -1,   149,   138,   150,    -1,   186,    -1,   171,   131,
     187,    -1,   171,   150,   149,   187,    -1,    -1,   173,    -1,
     149,   174,   150,    -1,   176,    -1,   174,   131,   176,    -1,
     174,   150,   149,   176,    -1,     7,    -1,   175,   131,     7,
      -1,    -1,    12,    -1,    14,    -1,    15,    -1,    17,    -1,
      18,    -1,    23,   145,   197,   146,    -1,    25,   145,   188,
     146,    -1,    32,    -1,    33,    -1,    34,    -1,    35,    -1,
      37,    -1,    38,    -1,    39,   145,   189,   146,    -1,    39,
     145,     7,   146,    -1,    40,    -1,    41,    -1,    43,   145,
       7,   146,    -1,    45,    -1,    46,   145,   175,   146,    -1,
      47,   145,     7,   146,    -1,    47,   145,   189,   146,    -1,
      50,    -1,    54,    -1,    56,   145,   189,   146,    -1,    57,
     145,     7,   146,    -1,    58,   145,     7,   146,    -1,    59,
     145,   189,   146,    -1,    60,   145,     7,   146,    -1,    61,
      -1,    63,   145,   189,   146,    -1,    64,    -1,    65,   145,
     197,   146,    -1,    66,    -1,    67,   145,    55,     3,   146,
      -1,    70,    -1,    72,    -1,    77,   145,   185,   146,    -1,
      79,    -1,    83,    -1,    84,    -1,    85,    -1,    86,    -1,
      87,    -1,    88,    -1,    89,    -1,    90,    -1,    91,   145,
     222,   146,    -1,    93,    -1,    94,    -1,    95,    -1,    97,
      -1,    98,   145,   189,   131,   189,   146,    -1,    99,    -1,
     101,    -1,   102,    -1,   103,    -1,   108,   145,   185,   146,
      -1,   111,    -1,   113,    -1,   116,   145,   187,   146,    -1,
     117,   145,   224,   146,    -1,   118,   145,   224,   146,    -1,
     124,   145,     8,   146,    -1,   125,    -1,   126,    -1,   127,
     145,   227,   146,    -1,   130,   145,   224,   146,    -1,   222,
      -1,    -1,   112,    -1,    -1,   178,   179,    -1,    25,   187,
     133,   192,    -1,    37,   133,   192,    -1,    31,   224,   197,
     151,   189,    -1,    -1,   182,   131,    -1,   182,    -1,   183,
      -1,   182,   131,   183,    -1,   197,   151,   189,    -1,   197,
      -1,    48,   196,   147,   181,   148,    -1,   186,    -1,   185,
     131,   186,    -1,    -1,   187,    -1,     5,    -1,     6,    -1,
      52,    -1,   119,    -1,     3,    -1,   187,   132,   187,   133,
     187,    -1,   187,   134,   187,    -1,   187,   135,   187,    -1,
     187,   137,   187,    -1,   187,   136,   187,    -1,   187,   138,
     187,    -1,   187,   139,   187,    -1,   187,    10,   187,    -1,
     187,    11,   187,    -1,   140,   187,    -1,   136,   187,    -1,
     138,   187,    -1,   145,   224,   146,   187,    -1,   109,   145,
     224,   146,    -1,   145,   187,   146,    -1,   189,    -1,   188,
     131,   189,    -1,   187,    -1,    51,    31,   224,   197,    -1,
      -1,   191,   192,    -1,   193,   144,    -1,   172,   226,   144,
      -1,   173,   144,    -1,   144,    -1,   172,   224,   220,   170,
      -1,   172,   224,   177,   220,   145,   166,   146,    -1,    -1,
     197,    -1,    -1,     3,    -1,     4,    -1,     3,    -1,     4,
      -1,    21,    -1,   129,    -1,   200,    -1,   106,   200,    -1,
     123,   200,    -1,   123,    -1,    53,    -1,   107,    -1,    44,
      -1,    19,    -1,    49,    -1,    55,    -1,    -1,    73,    -1,
      73,    -1,   105,   199,    -1,   110,    -1,    80,   199,    -1,
      62,   199,    -1,    74,    -1,    27,    -1,    28,     3,    -1,
      28,     4,    -1,   173,   201,    -1,   202,   147,   204,   148,
      -1,    -1,   204,   205,    -1,   172,   215,    -1,    42,     3,
      -1,    42,     4,    -1,   173,   206,    -1,    92,   133,    -1,
     208,   193,   144,    -1,    81,   133,    -1,   209,   194,   144,
      -1,   207,   147,   208,   209,   148,    -1,   207,   147,   212,
     144,   148,    -1,    -1,   133,     4,    -1,    75,     3,    -1,
      75,     4,    -1,   173,   212,    -1,   213,   211,   147,   157,
     148,    -1,   213,   133,     3,   147,   161,   157,   148,    -1,
     210,    -1,   212,   144,    -1,   206,   144,    -1,    82,     3,
      -1,    82,     4,    -1,   173,   216,    -1,   217,   147,   157,
     148,    -1,   138,   220,    -1,    31,   219,    -1,   197,    -1,
     219,    -1,   145,   220,   146,    -1,   220,    -1,   221,   131,
     220,    -1,   100,    -1,   122,    -1,    96,    -1,   114,   196,
     147,   191,   148,    -1,   128,    -1,     4,    -1,   198,    -1,
      31,   224,    -1,   184,    -1,    48,     3,    -1,   223,    -1,
     114,     3,    -1,   226,    -1,   121,     3,    -1,   104,   145,
     224,   146,    -1,   120,   172,   224,   221,    -1,   121,   196,
     147,   191,   148,    -1,   121,   196,   115,   145,   193,   146,
     195,   147,   178,   148,    -1,     5,    -1,     5,   152,     5,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   265,   265,   268,   269,   270,   271,   275,   280,   281,
     282,   285,   286,   287,   288,   289,   293,   294,   295,   298,
     299,   300,   303,   304,   305,   306,   307,   308,   309,   310,
     311,   314,   316,   319,   322,   325,   327,   332,   335,   336,
     339,   342,   343,   344,   348,   353,   357,   363,   370,   371,
     372,   375,   376,   377,   380,   381,   385,   391,   392,   393,
     396,   397,   400,   401,   402,   403,   404,   405,   406,   407,
     408,   409,   410,   411,   412,   413,   414,   415,   416,   417,
     418,   419,   420,   421,   422,   423,   424,   425,   426,   427,
     428,   429,   430,   431,   432,   433,   434,   435,   436,   437,
     438,   439,   440,   441,   442,   443,   444,   445,   446,   447,
     448,   449,   450,   451,   452,   453,   456,   457,   458,   459,
     460,   461,   462,   463,   464,   465,   466,   467,   468,   469,
     470,   471,   474,   475,   478,   479,   482,   486,   492,   498,
     499,   500,   503,   507,   516,   520,   525,   534,   535,   548,
     549,   552,   553,   554,   555,   556,   557,   558,   559,   560,
     561,   562,   563,   564,   565,   566,   567,   568,   569,   570,
     571,   574,   575,   578,   584,   589,   590,   593,   594,   595,
     596,   599,   607,   619,   620,   623,   624,   625,   628,   630,
     633,   634,   635,   636,   637,   653,   654,   655,   656,   657,
     658,   659,   662,   663,   666,   667,   668,   669,   670,   671,
     672,   675,   676,   682,   691,   697,   698,   702,   705,   706,
     709,   721,   722,   725,   726,   729,   738,   747,   748,   751,
     752,   755,   763,   773,   782,   786,   787,   790,   791,   794,
     799,   805,   806,   809,   810,   811,   815,   816,   820,   821,
     822,   825,   836,   837,   838,   839,   840,   841,   842,   843,
     844,   845,   846,   849,   854,   859,   876,   877
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "aIDENTIFIER", "aKNOWNTYPE", "aNUM",
  "aHEXNUM", "aSTRING", "aUUID", "aEOF", "SHL", "SHR", "tAGGREGATABLE",
  "tALLOCATE", "tAPPOBJECT", "tASYNC", "tASYNCUUID", "tAUTOHANDLE",
  "tBINDABLE", "tBOOLEAN", "tBROADCAST", "tBYTE", "tBYTECOUNT", "tCALLAS",
  "tCALLBACK", "tCASE", "tCDECL", "tCHAR", "tCOCLASS", "tCODE",
  "tCOMMSTATUS", "tCONST", "tCONTEXTHANDLE", "tCONTEXTHANDLENOSERIALIZE",
  "tCONTEXTHANDLESERIALIZE", "tCONTROL", "tCPPQUOTE", "tDEFAULT",
  "tDEFAULTCOLLELEM", "tDEFAULTVALUE", "tDEFAULTVTABLE", "tDISPLAYBIND",
  "tDISPINTERFACE", "tDLLNAME", "tDOUBLE", "tDUAL", "tENDPOINT", "tENTRY",
  "tENUM", "tERRORSTATUST", "tEXPLICITHANDLE", "tEXTERN", "tFALSE",
  "tFLOAT", "tHANDLE", "tHANDLET", "tHELPCONTEXT", "tHELPFILE",
  "tHELPSTRING", "tHELPSTRINGCONTEXT", "tHELPSTRINGDLL", "tHIDDEN",
  "tHYPER", "tID", "tIDEMPOTENT", "tIIDIS", "tIMMEDIATEBIND",
  "tIMPLICITHANDLE", "tIMPORT", "tIMPORTLIB", "tIN", "tINLINE",
  "tINPUTSYNC", "tINT", "tINT64", "tINTERFACE", "tLCID", "tLENGTHIS",
  "tLIBRARY", "tLOCAL", "tLONG", "tMETHODS", "tMODULE", "tNONBROWSABLE",
  "tNONCREATABLE", "tNONEXTENSIBLE", "tOBJECT", "tODL", "tOLEAUTOMATION",
  "tOPTIONAL", "tOUT", "tPOINTERDEFAULT", "tPROPERTIES", "tPROPGET",
  "tPROPPUT", "tPROPPUTREF", "tPTR", "tPUBLIC", "tRANGE", "tREADONLY",
  "tREF", "tREQUESTEDIT", "tRESTRICTED", "tRETVAL", "tSAFEARRAY", "tSHORT",
  "tSIGNED", "tSINGLE", "tSIZEIS", "tSIZEOF", "tSMALL", "tSOURCE",
  "tSTDCALL", "tSTRING", "tSTRUCT", "tSWITCH", "tSWITCHIS", "tSWITCHTYPE",
  "tTRANSMITAS", "tTRUE", "tTYPEDEF", "tUNION", "tUNIQUE", "tUNSIGNED",
  "tUUID", "tV1ENUM", "tVARARG", "tVERSION", "tVOID", "tWCHAR",
  "tWIREMARSHAL", "','", "'?'", "':'", "'|'", "'&'", "'-'", "'+'", "'*'",
  "'/'", "'~'", "CAST", "PPTR", "NEG", "';'", "'('", "')'", "'{'", "'}'",
  "'['", "']'", "'='", "'.'", "$accept", "input", "gbl_statements",
  "imp_statements", "int_statements", "statement", "cppquote",
  "import_start", "import", "importlib", "libraryhdr", "library_start",
  "librarydef", "m_args", "no_args", "args", "arg", "array", "array_list",
  "m_attributes", "attributes", "attrib_list", "str_list", "attribute",
  "callconv", "cases", "case", "constdef", "enums", "enum_list", "enum",
  "enumdef", "m_exprs", "m_expr", "expr", "expr_list_const", "expr_const",
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
     385,    44,    63,    58,   124,    38,    45,    43,    42,    47,
     126,   386,   387,   388,    59,    40,    41,   123,   125,    91,
      93,    61,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,   153,   154,   155,   155,   155,   155,   155,   155,   155,
     155,   156,   156,   156,   156,   156,   156,   156,   156,   157,
     157,   157,   158,   158,   158,   158,   158,   158,   158,   158,
     158,   159,   160,   161,   162,   163,   164,   165,   166,   166,
     167,   168,   168,   168,   169,   169,   169,   169,   170,   170,
     170,   171,   171,   171,   172,   172,   173,   174,   174,   174,
     175,   175,   176,   176,   176,   176,   176,   176,   176,   176,
     176,   176,   176,   176,   176,   176,   176,   176,   176,   176,
     176,   176,   176,   176,   176,   176,   176,   176,   176,   176,
     176,   176,   176,   176,   176,   176,   176,   176,   176,   176,
     176,   176,   176,   176,   176,   176,   176,   176,   176,   176,
     176,   176,   176,   176,   176,   176,   176,   176,   176,   176,
     176,   176,   176,   176,   176,   176,   176,   176,   176,   176,
     176,   176,   177,   177,   178,   178,   179,   179,   180,   181,
     181,   181,   182,   182,   183,   183,   184,   185,   185,   186,
     186,   187,   187,   187,   187,   187,   187,   187,   187,   187,
     187,   187,   187,   187,   187,   187,   187,   187,   187,   187,
     187,   188,   188,   189,   190,   191,   191,   192,   192,   192,
     192,   193,   194,   195,   195,   196,   196,   196,   197,   197,
     198,   198,   198,   198,   198,   198,   198,   198,   198,   198,
     198,   198,   199,   199,   200,   200,   200,   200,   200,   200,
     200,   201,   201,   202,   203,   204,   204,   205,   206,   206,
     207,   208,   208,   209,   209,   210,   210,   211,   211,   212,
     212,   213,   214,   214,   214,   215,   215,   216,   216,   217,
     218,   219,   219,   220,   220,   220,   221,   221,   222,   222,
     222,   223,   224,   224,   224,   224,   224,   224,   224,   224,
     224,   224,   224,   225,   226,   226,   227,   227
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
       4,     1,     0,     1,     0,     2,     4,     3,     5,     0,
       2,     1,     1,     3,     3,     1,     5,     1,     3,     0,
       1,     1,     1,     1,     1,     1,     5,     3,     3,     3,
       3,     3,     3,     3,     3,     2,     2,     2,     4,     4,
       3,     1,     3,     1,     4,     0,     2,     2,     3,     2,
       1,     4,     7,     0,     1,     0,     1,     1,     1,     1,
       1,     1,     1,     2,     2,     1,     1,     1,     1,     1,
       1,     1,     0,     1,     1,     2,     1,     2,     2,     1,
       1,     2,     2,     2,     4,     0,     2,     2,     2,     2,
       2,     2,     3,     2,     3,     5,     5,     0,     2,     2,
       2,     2,     5,     7,     1,     2,     2,     2,     2,     2,
       4,     2,     2,     1,     1,     3,     1,     3,     1,     1,
       1,     5,     1,     1,     1,     2,     1,     2,     1,     2,
       1,     2,     4,     4,     5,    10,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned short int yydefact[] =
{
       3,     0,     2,     1,     0,     0,     0,     0,   185,     0,
       0,     0,   185,    54,   185,    22,    62,    10,    24,    11,
      27,    11,     9,     0,     0,     0,     0,     0,     0,     7,
       0,     0,   234,     0,   227,     5,     4,     0,     8,     0,
       0,     0,   211,   212,   253,   199,   190,   210,     0,   198,
     185,   200,   196,   201,   202,   204,   209,   202,     0,   202,
       0,   197,   206,   185,   185,   195,   252,   191,   256,   254,
     192,   258,     0,   260,     0,   218,   219,   186,   187,     0,
       0,     0,   229,   230,     0,     0,    55,     0,    63,    64,
      65,    66,    67,     0,     0,    70,    71,    72,    73,    74,
      75,     0,    78,    79,     0,    81,     0,     0,    85,    86,
       0,     0,     0,     0,     0,    92,     0,    94,     0,    96,
       0,    98,    99,     0,   101,   102,   103,   104,   105,   106,
     107,   108,   109,     0,   111,   112,   113,   250,   114,     0,
     116,   248,   117,   118,   119,     0,   121,   122,     0,     0,
       0,   249,     0,   127,   128,     0,     0,     0,    57,   131,
       0,     0,     0,     0,     0,   213,   220,   231,   239,    23,
      25,    26,     6,   215,   236,     0,   235,     0,     0,    19,
      28,    29,    30,   255,   257,   203,   208,   207,     0,   205,
     193,   259,   261,   194,   188,   189,     0,     0,   139,     0,
      32,   175,     0,     0,   175,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   149,
       0,     0,   149,     0,     0,     0,     0,     0,     0,    62,
      56,    33,     0,    17,    18,     0,     0,    15,    13,    12,
      16,    37,    35,   237,   238,    36,    54,     0,    54,     0,
       0,   228,    19,    54,     0,     0,    31,     0,   141,   142,
     145,   174,    54,     0,     0,     0,   243,   244,   246,   263,
      54,    54,     0,   155,   151,   152,   153,     0,   154,     0,
       0,     0,     0,   173,     0,   171,     0,     0,     0,    60,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   147,   150,     0,     0,     0,     0,     0,     0,
       0,   266,     0,     0,    58,    62,     0,    14,   214,     0,
     216,   221,     0,     0,     0,    54,     0,     0,    54,   240,
      21,     0,     0,   262,   138,   146,   140,     0,   180,   251,
       0,    55,   176,     0,   242,   241,     0,     0,     0,   264,
      68,     0,   166,   167,   165,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    69,    77,    76,
      80,     0,    82,    83,    84,    87,    88,    89,    90,    91,
      93,    95,     0,   149,   100,   110,     0,   120,   123,   124,
     125,   126,     0,   129,   130,    59,     0,   217,   223,     0,
     222,   225,     0,   226,    19,   232,   132,    20,   143,   144,
     260,   179,   177,   245,   247,   183,     0,   170,     0,   163,
     164,     0,   157,   158,   160,   159,   161,   162,   172,    61,
      97,   148,     0,   267,    34,    48,   224,    54,   133,     0,
     178,     0,   184,   169,   168,     0,   115,   149,   181,   233,
       0,   134,   156,     0,     0,    51,    38,     0,    50,     0,
      49,   252,     0,    43,    39,    41,     0,     0,     0,     0,
     265,   135,    52,     0,   182,     0,     0,    48,     0,    54,
      53,    42,    48,    38,    45,    54,   137,    38,    44,     0,
     136,     0,    47,    46
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,     1,     2,   160,   253,   330,    18,    19,    20,   234,
     164,    21,    22,   462,   463,   464,   465,   448,   454,   331,
      86,   157,   290,   158,   439,   457,   471,    24,   257,   258,
     259,    68,   301,   302,   283,   284,   285,    26,   262,   342,
     343,   332,   441,    79,   266,    69,   186,    70,   165,    28,
     237,   246,   320,    30,    31,   248,   325,    32,   178,    33,
      34,   238,   239,   168,    37,   240,   267,   268,   269,   159,
      71,   467,    40,    73,   312
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -365
static const short int yypact[] =
{
    -365,    34,   728,  -365,   167,   687,  -113,   169,   200,    12,
      57,   208,   200,   -82,   200,  -365,   841,  -365,  -365,  -365,
    -365,  -365,  -365,     7,   -67,   -60,   -49,   -21,    10,  -365,
     -10,    46,  -365,    64,    62,  -365,  -365,    66,  -365,    78,
     105,   113,  -365,  -365,  -365,  -365,  -365,  -365,   687,  -365,
     212,  -365,  -365,  -365,   197,  -365,  -365,   197,   128,   197,
      26,  -365,  -365,   215,   222,    26,  -365,  -365,  -365,  -365,
    -365,  -365,   231,  -365,   270,  -365,  -365,  -365,  -365,   132,
     687,   138,  -365,  -365,   133,   687,  -365,   -91,  -365,  -365,
    -365,  -365,  -365,   139,   144,  -365,  -365,  -365,  -365,  -365,
    -365,   145,  -365,  -365,   146,  -365,   149,   150,  -365,  -365,
     151,   153,   155,   156,   157,  -365,   158,  -365,   159,  -365,
     160,  -365,  -365,   162,  -365,  -365,  -365,  -365,  -365,  -365,
    -365,  -365,  -365,   164,  -365,  -365,  -365,  -365,  -365,   168,
    -365,  -365,  -365,  -365,  -365,   170,  -365,  -365,   171,   175,
     176,  -365,   177,  -365,  -365,   180,   181,  -102,  -365,  -365,
     529,   697,   309,   237,   183,  -365,  -365,  -365,  -365,  -365,
    -365,  -365,  -365,  -365,  -365,     0,  -365,   239,   187,  -365,
    -365,  -365,  -365,  -365,   188,  -365,  -365,  -365,   687,  -365,
    -365,   188,   -88,  -365,  -365,  -365,   163,   182,   231,   231,
    -365,  -365,    47,   192,  -365,   231,   305,   136,   331,   332,
     280,   305,   333,   335,   305,   336,   305,   231,   289,   305,
      -3,   305,   305,   305,   687,   687,   338,   347,   687,   841,
     204,  -365,   210,  -365,  -365,    32,   216,  -365,  -365,  -365,
    -365,  -365,  -365,  -365,  -365,  -365,    98,   223,   -58,   217,
     219,  -365,  -365,   -11,   213,   305,  -365,   214,   232,  -365,
     218,  -365,   -31,   -13,    47,    47,  -365,  -365,  -365,   233,
     -82,   -22,   224,  -365,  -365,  -365,  -365,   226,  -365,   305,
     305,   305,   496,   532,   -48,  -365,   228,   229,   230,  -365,
     -44,   234,   235,   236,   238,   240,   241,   245,   246,   248,
     365,   -35,  -365,   532,   249,   247,   -30,   449,   250,   251,
     252,   225,   254,   255,  -365,   841,   372,  -365,  -365,    -6,
    -365,  -365,   269,   687,   264,   103,   242,   315,   686,  -365,
    -365,   687,   265,  -365,  -365,  -365,   231,   305,  -365,  -365,
     687,   266,  -365,   268,  -365,  -365,   267,    47,   271,  -365,
    -365,   687,  -365,  -365,  -365,   519,   273,   305,   305,   305,
     305,   305,   305,   305,   305,   305,   305,  -365,  -365,  -365,
    -365,   378,  -365,  -365,  -365,  -365,  -365,  -365,  -365,  -365,
    -365,  -365,   275,   305,  -365,  -365,   305,  -365,  -365,  -365,
    -365,  -365,   417,  -365,  -365,  -365,   277,  -365,  -365,    47,
    -365,  -365,   282,  -365,  -365,  -365,   317,  -365,  -365,  -365,
     283,  -365,  -365,  -365,  -365,   231,   284,  -365,   305,  -365,
    -365,   551,    94,    44,    51,    51,   257,   257,  -365,  -365,
    -365,  -365,   285,  -365,  -365,   286,  -365,   924,  -365,    47,
    -365,   287,  -365,  -365,  -365,   305,  -365,   401,  -365,  -365,
     291,  -365,   532,    60,   -92,  -365,   244,   -16,  -365,   305,
     295,    -1,   301,  -365,   318,  -365,   687,    47,   305,   319,
    -365,  -365,   532,   305,  -365,   384,    47,   -25,   562,    50,
     532,  -365,    -5,   244,  -365,    50,  -365,   244,  -365,   302,
    -365,   310,  -365,  -365
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -365,  -365,  -365,   430,  -244,    15,  -365,  -365,   134,  -365,
    -365,  -365,  -365,  -296,  -365,  -365,   -20,  -276,  -365,    -9,
      -2,  -365,  -365,  -207,  -365,  -365,  -365,  -365,  -365,  -365,
     127,     3,   243,  -357,  -209,  -365,  -169,  -365,   262,  -364,
    -202,   143,  -365,    16,   -70,  -365,   -26,   142,    17,  -365,
     465,  -365,  -365,    -7,  -365,  -365,  -365,  -365,  -365,    -8,
    -365,   467,     4,  -365,  -365,   469,   209,  -253,  -365,   258,
       5,    -4,  -365,     1,  -365
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -187
static const short int yytable[] =
{
      23,    72,   196,    41,    85,    25,    36,    39,   328,   468,
     303,   345,   346,   303,   307,   167,   166,    17,   263,    27,
       5,   469,   314,   322,   203,     6,   431,  -186,    84,   229,
      87,   187,    74,   189,     3,     4,     7,     8,   287,   459,
       9,   292,   293,    80,   183,   296,   324,   298,   230,     7,
     194,   195,   305,    47,   357,   358,   204,    10,   460,  -186,
       4,   357,   358,   273,    81,   274,   275,    16,   348,    11,
     352,   353,   354,   355,     7,    11,   199,   169,   263,    84,
      87,   202,    11,   366,   170,   162,   334,   371,    54,   163,
     455,    16,   247,   137,   414,   171,   383,   141,   367,    55,
      56,   383,   372,    12,   357,   358,    57,    11,   395,    13,
      14,   384,   276,   338,   163,   486,   387,   339,    16,   151,
     483,   490,   338,   172,   447,   264,   349,    16,   260,   261,
     -40,    59,   470,    15,   174,   272,    62,   329,    16,   273,
     487,   274,   275,   286,   447,   -40,   435,   299,   419,   420,
     421,   422,   423,   424,   425,   426,   427,   173,   235,   235,
     437,    41,    41,    25,    25,    39,    39,   249,   409,   277,
      42,    43,    75,    76,   303,   233,   233,   236,   236,   278,
     362,   363,   364,   365,   254,   264,   450,   489,   276,   364,
     365,   491,   265,   175,   338,   177,   279,   428,   280,    16,
     281,   484,   190,    77,    78,   282,   488,   193,   176,   444,
     458,    82,    83,   179,   477,   184,    78,   432,   191,    78,
     308,   309,   180,   482,   313,   192,    78,   167,   166,   361,
     362,   363,   364,   365,   194,   195,   452,   319,   303,   323,
     243,   244,   250,   251,   353,   277,   318,    16,    44,   181,
     472,   401,    16,   340,    41,   278,    25,   182,    39,   478,
     341,   323,   340,    45,   480,    46,   260,   357,   358,   341,
     185,    47,   279,   188,   280,    48,   281,   197,   356,   198,
     201,   282,   200,   273,   205,   274,   275,   291,    49,   206,
     207,   208,    50,    51,   209,   210,   211,    52,   212,    53,
     213,   214,   215,   216,   217,   218,    54,   219,   273,   220,
     274,   275,   242,   221,   255,   222,   223,    55,    56,   399,
     224,   225,   226,   397,    57,   227,   228,   406,   256,    41,
     245,    25,   276,    39,   252,  -186,   399,   270,   288,   289,
     294,   410,   295,   297,   300,   442,   310,   416,    58,    59,
      60,    61,   311,   315,    62,   316,   321,   276,    63,   333,
     317,   326,   335,   336,   347,    64,   327,    65,   382,   337,
     350,   351,   461,    67,   368,   369,   370,   392,   386,   396,
     373,   374,   375,    10,   376,   429,   377,   378,    44,   277,
     403,   379,   380,    16,   381,   385,   389,   390,   391,   278,
     393,   394,   398,    45,   273,    46,   274,   275,   400,   407,
     411,    47,   412,   413,   277,    48,   279,   415,   280,   418,
     281,   430,   433,   434,   278,   282,   436,   440,    49,   438,
     443,   446,    50,    51,   451,   447,   456,    52,    41,    53,
      25,   279,    39,   280,   473,   281,    54,   474,   492,   475,
     282,   161,   479,   276,   466,   481,   493,    55,    56,   357,
     358,   404,   476,   408,    57,   306,   271,    29,   402,    35,
     340,    38,   344,   466,     0,     0,   340,   341,   304,     0,
       0,   466,     0,   341,     0,   466,     0,     0,    58,    59,
      60,    61,     0,     0,    62,     0,     0,     0,    63,   273,
      44,   274,   275,     0,     0,    64,     0,    65,     0,     0,
     277,     0,    66,    67,     0,    45,     0,    46,     0,     0,
     278,     0,     0,    47,     0,     0,     0,    48,     0,   357,
     358,     0,     0,    16,     0,     0,     0,   279,   231,   453,
      49,   281,   357,   358,    50,    51,   282,     0,   276,    52,
       0,    53,     0,     0,     0,     0,     0,     4,    54,     0,
       5,   357,   358,     0,     0,     6,     0,     0,     0,    55,
      56,     7,   357,   358,     0,     0,    57,     8,     0,     0,
       9,   359,     0,   360,   361,   362,   363,   364,   365,     0,
       0,     0,     0,     0,     0,   388,     0,    10,   232,     0,
      58,    59,    60,    61,    11,   277,    62,     0,     0,     0,
      63,     0,     0,     0,     0,   278,     0,    64,     0,    65,
       0,     0,     0,     0,    66,    67,     0,     0,     0,     0,
       0,     0,   279,     0,   280,     0,   281,     0,     0,     0,
       0,   282,     0,    12,     0,     0,     0,     0,     0,    13,
      14,   359,     0,   360,   361,   362,   363,   364,   365,     0,
       0,     0,     0,     0,   359,   417,   360,   361,   362,   363,
     364,   365,     0,    15,     0,     0,     0,     0,    16,     0,
       0,     0,     0,   359,   445,   360,   361,   362,   363,   364,
     365,    44,     0,     0,   359,   485,   360,   361,   362,   363,
     364,   365,     0,     0,     0,     0,    45,     0,    46,     0,
       0,     0,     0,     0,    47,     0,     0,     5,    48,     0,
       0,     0,     6,     0,     0,     4,     0,     0,     5,     0,
       0,    49,     0,     6,     8,    50,    51,     9,     0,     7,
      52,     0,    53,     0,     0,     8,     0,     0,     9,    54,
       0,     0,     0,     0,    10,     0,     4,     0,     0,     5,
      55,    56,     0,     0,     6,    10,   232,    57,     0,     0,
       7,     0,    11,     0,     0,     0,     8,     0,     0,     9,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    58,    59,    60,    61,     0,    10,    62,     0,     0,
      12,    63,     0,    11,     0,     0,    13,    14,    64,     0,
      65,    12,     0,     0,     0,    66,    67,    13,    14,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      15,     0,     0,     0,   405,    16,     0,     0,     0,     0,
       0,    15,    12,     0,     0,   241,    16,     0,    13,    14,
       0,     0,     0,    88,     0,    89,    90,     0,    91,    92,
       0,     0,     0,     0,    93,     0,    94,     0,     0,     0,
       0,     0,    15,    95,    96,    97,    98,    16,    99,   100,
     101,   102,   103,     0,   104,     0,   105,   106,   107,     0,
       0,   108,     0,     0,     0,   109,     0,   110,   111,   112,
     113,   114,   115,     0,   116,   117,   118,   119,   120,     0,
       0,   121,     0,   122,     0,     0,     0,     0,   123,     0,
     124,     0,     0,     0,   125,   126,   127,   128,   129,   130,
     131,   132,   133,     0,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,     0,     0,     0,     0,   145,
       0,     0,   146,     0,   147,     5,     0,   148,   149,   150,
       6,     0,     0,   151,     0,   152,   153,   154,   155,     0,
       0,   156,     8,     0,     0,     9,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    10,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    12,     0,
       0,     0,     0,     0,    13,    14,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    15,     0,
       0,     0,   449,    16
};

static const short int yycheck[] =
{
       2,     5,    72,     2,    13,     2,     2,     2,   252,    25,
     219,   264,   265,   222,   223,    23,    23,     2,    31,     2,
      31,    37,   229,    81,   115,    36,   383,   115,    12,   131,
      14,    57,   145,    59,     0,    28,    42,    48,   207,   131,
      51,   210,   211,    31,    48,   214,   248,   216,   150,    42,
       3,     4,   221,    27,    10,    11,   147,    68,   150,   147,
      28,    10,    11,     3,     7,     5,     6,   149,   270,    75,
     279,   280,   281,   282,    42,    75,    80,   144,    31,    63,
      64,    85,    75,   131,   144,    78,   255,   131,    62,    82,
     447,   149,    92,    96,   347,   144,   131,   100,   146,    73,
      74,   131,   146,   114,    10,    11,    80,    75,   315,   120,
     121,   146,    52,   144,    82,   479,   146,   148,   149,   122,
     145,   485,   144,   144,   149,   138,   148,   149,   198,   199,
     131,   105,   148,   144,   144,   205,   110,   148,   149,     3,
     145,     5,     6,     7,   149,   146,   399,   217,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   147,   160,   161,
     404,   160,   161,   160,   161,   160,   161,   175,   337,   109,
       3,     4,     3,     4,   383,   160,   161,   160,   161,   119,
     136,   137,   138,   139,   188,   138,   439,   483,    52,   138,
     139,   487,   145,   147,   144,   133,   136,   366,   138,   149,
     140,   477,    60,     3,     4,   145,   482,    65,   144,   418,
     150,     3,     4,   147,   467,     3,     4,   386,     3,     4,
     224,   225,   144,   476,   228,     3,     4,   235,   235,   135,
     136,   137,   138,   139,     3,     4,   445,   246,   447,   248,
       3,     4,     3,     4,   453,   109,   148,   149,     4,   144,
     459,   148,   149,   262,   253,   119,   253,   144,   253,   468,
     262,   270,   271,    19,   473,    21,   336,    10,    11,   271,
      73,    27,   136,   145,   138,    31,   140,     7,   282,   147,
     147,   145,   144,     3,   145,     5,     6,     7,    44,   145,
     145,   145,    48,    49,   145,   145,   145,    53,   145,    55,
     145,   145,   145,   145,   145,   145,    62,   145,     3,   145,
       5,     6,     3,   145,   151,   145,   145,    73,    74,   323,
     145,   145,   145,   319,    80,   145,   145,   331,   146,   328,
     147,   328,    52,   328,   147,   147,   340,   145,     7,     7,
       7,   340,     7,     7,    55,   415,     8,   351,   104,   105,
     106,   107,     5,   149,   110,   145,   133,    52,   114,   146,
     144,   144,   148,   131,   131,   121,   147,   123,     3,   151,
     146,   145,   128,   129,   146,   146,   146,   152,   131,     7,
     146,   146,   146,    68,   146,     7,   146,   146,     4,   109,
     148,   146,   146,   149,   146,   146,   146,   146,   146,   119,
     146,   146,   133,    19,     3,    21,     5,     6,   144,   144,
     144,    27,   144,   146,   109,    31,   136,   146,   138,   146,
     140,   146,     5,   146,   119,   145,   144,   144,    44,   112,
     146,   146,    48,    49,   147,   149,   145,    53,   437,    55,
     437,   136,   437,   138,   149,   140,    62,   146,   146,   131,
     145,    21,   133,    52,   456,   475,   146,    73,    74,    10,
      11,   327,   466,   336,    80,   222,   204,     2,   325,     2,
     479,     2,   263,   475,    -1,    -1,   485,   479,   220,    -1,
      -1,   483,    -1,   485,    -1,   487,    -1,    -1,   104,   105,
     106,   107,    -1,    -1,   110,    -1,    -1,    -1,   114,     3,
       4,     5,     6,    -1,    -1,   121,    -1,   123,    -1,    -1,
     109,    -1,   128,   129,    -1,    19,    -1,    21,    -1,    -1,
     119,    -1,    -1,    27,    -1,    -1,    -1,    31,    -1,    10,
      11,    -1,    -1,   149,    -1,    -1,    -1,   136,     9,   138,
      44,   140,    10,    11,    48,    49,   145,    -1,    52,    53,
      -1,    55,    -1,    -1,    -1,    -1,    -1,    28,    62,    -1,
      31,    10,    11,    -1,    -1,    36,    -1,    -1,    -1,    73,
      74,    42,    10,    11,    -1,    -1,    80,    48,    -1,    -1,
      51,   132,    -1,   134,   135,   136,   137,   138,   139,    -1,
      -1,    -1,    -1,    -1,    -1,   146,    -1,    68,    69,    -1,
     104,   105,   106,   107,    75,   109,   110,    -1,    -1,    -1,
     114,    -1,    -1,    -1,    -1,   119,    -1,   121,    -1,   123,
      -1,    -1,    -1,    -1,   128,   129,    -1,    -1,    -1,    -1,
      -1,    -1,   136,    -1,   138,    -1,   140,    -1,    -1,    -1,
      -1,   145,    -1,   114,    -1,    -1,    -1,    -1,    -1,   120,
     121,   132,    -1,   134,   135,   136,   137,   138,   139,    -1,
      -1,    -1,    -1,    -1,   132,   146,   134,   135,   136,   137,
     138,   139,    -1,   144,    -1,    -1,    -1,    -1,   149,    -1,
      -1,    -1,    -1,   132,   133,   134,   135,   136,   137,   138,
     139,     4,    -1,    -1,   132,   133,   134,   135,   136,   137,
     138,   139,    -1,    -1,    -1,    -1,    19,    -1,    21,    -1,
      -1,    -1,    -1,    -1,    27,    -1,    -1,    31,    31,    -1,
      -1,    -1,    36,    -1,    -1,    28,    -1,    -1,    31,    -1,
      -1,    44,    -1,    36,    48,    48,    49,    51,    -1,    42,
      53,    -1,    55,    -1,    -1,    48,    -1,    -1,    51,    62,
      -1,    -1,    -1,    -1,    68,    -1,    28,    -1,    -1,    31,
      73,    74,    -1,    -1,    36,    68,    69,    80,    -1,    -1,
      42,    -1,    75,    -1,    -1,    -1,    48,    -1,    -1,    51,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   104,   105,   106,   107,    -1,    68,   110,    -1,    -1,
     114,   114,    -1,    75,    -1,    -1,   120,   121,   121,    -1,
     123,   114,    -1,    -1,    -1,   128,   129,   120,   121,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     144,    -1,    -1,    -1,   148,   149,    -1,    -1,    -1,    -1,
      -1,   144,   114,    -1,    -1,   148,   149,    -1,   120,   121,
      -1,    -1,    -1,    12,    -1,    14,    15,    -1,    17,    18,
      -1,    -1,    -1,    -1,    23,    -1,    25,    -1,    -1,    -1,
      -1,    -1,   144,    32,    33,    34,    35,   149,    37,    38,
      39,    40,    41,    -1,    43,    -1,    45,    46,    47,    -1,
      -1,    50,    -1,    -1,    -1,    54,    -1,    56,    57,    58,
      59,    60,    61,    -1,    63,    64,    65,    66,    67,    -1,
      -1,    70,    -1,    72,    -1,    -1,    -1,    -1,    77,    -1,
      79,    -1,    -1,    -1,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,    -1,    -1,    -1,    -1,   108,
      -1,    -1,   111,    -1,   113,    31,    -1,   116,   117,   118,
      36,    -1,    -1,   122,    -1,   124,   125,   126,   127,    -1,
      -1,   130,    48,    -1,    -1,    51,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    68,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,    -1,
      -1,    -1,    -1,    -1,   120,   121,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   144,    -1,
      -1,    -1,   148,   149
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,   154,   155,     0,    28,    31,    36,    42,    48,    51,
      68,    75,   114,   120,   121,   144,   149,   158,   159,   160,
     161,   164,   165,   173,   180,   184,   190,   201,   202,   203,
     206,   207,   210,   212,   213,   214,   215,   217,   218,   223,
     225,   226,     3,     4,     4,    19,    21,    27,    31,    44,
      48,    49,    53,    55,    62,    73,    74,    80,   104,   105,
     106,   107,   110,   114,   121,   123,   128,   129,   184,   198,
     200,   223,   224,   226,   145,     3,     4,     3,     4,   196,
      31,     7,     3,     4,   196,   172,   173,   196,    12,    14,
      15,    17,    18,    23,    25,    32,    33,    34,    35,    37,
      38,    39,    40,    41,    43,    45,    46,    47,    50,    54,
      56,    57,    58,    59,    60,    61,    63,    64,    65,    66,
      67,    70,    72,    77,    79,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   108,   111,   113,   116,   117,
     118,   122,   124,   125,   126,   127,   130,   174,   176,   222,
     156,   156,    78,    82,   163,   201,   206,   212,   216,   144,
     144,   144,   144,   147,   144,   147,   144,   133,   211,   147,
     144,   144,   144,   224,     3,    73,   199,   199,   145,   199,
     200,     3,     3,   200,     3,     4,   197,     7,   147,   224,
     144,   147,   224,   115,   147,   145,   145,   145,   145,   145,
     145,   145,   145,   145,   145,   145,   145,   145,   145,   145,
     145,   145,   145,   145,   145,   145,   145,   145,   145,   131,
     150,     9,    69,   158,   162,   173,   201,   203,   214,   215,
     218,   148,     3,     3,     4,   147,   204,    92,   208,   212,
       3,     4,   147,   157,   224,   151,   146,   181,   182,   183,
     197,   197,   191,    31,   138,   145,   197,   219,   220,   221,
     145,   191,   197,     3,     5,     6,    52,   109,   119,   136,
     138,   140,   145,   187,   188,   189,     7,   189,     7,     7,
     175,     7,   189,   189,     7,     7,   189,     7,   189,   197,
      55,   185,   186,   187,   222,   189,   185,   187,   224,   224,
       8,     5,   227,   224,   176,   149,   145,   144,   148,   172,
     205,   133,    81,   172,   193,   209,   144,   147,   157,   148,
     158,   172,   194,   146,   189,   148,   131,   151,   144,   148,
     172,   173,   192,   193,   219,   220,   220,   131,   193,   148,
     146,   145,   187,   187,   187,   187,   224,    10,    11,   132,
     134,   135,   136,   137,   138,   139,   131,   146,   146,   146,
     146,   131,   146,   146,   146,   146,   146,   146,   146,   146,
     146,   146,     3,   131,   146,   146,   131,   146,   146,   146,
     146,   146,   152,   146,   146,   176,     7,   215,   133,   224,
     144,   148,   194,   148,   161,   148,   224,   144,   183,   189,
     226,   144,   144,   146,   220,   146,   224,   146,   146,   187,
     187,   187,   187,   187,   187,   187,   187,   187,   189,     7,
     146,   186,   189,     5,   146,   220,   144,   157,   112,   177,
     144,   195,   197,   146,   187,   133,   146,   149,   170,   148,
     220,   147,   187,   138,   171,   186,   145,   178,   150,   131,
     150,   128,   166,   167,   168,   169,   173,   224,    25,    37,
     148,   179,   187,   149,   146,   131,   224,   220,   187,   133,
     187,   169,   220,   145,   170,   133,   192,   145,   170,   166,
     192,   166,   146,   146
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
#line 265 "tools\\widl_new\\parser.y"
    { write_proxies((yyvsp[0].ifref_list)); write_client((yyvsp[0].ifref_list)); write_server((yyvsp[0].ifref_list)); ;}
    break;

  case 3:
#line 268 "tools\\widl_new\\parser.y"
    { (yyval.ifref_list) = NULL; ;}
    break;

  case 4:
#line 269 "tools\\widl_new\\parser.y"
    { (yyval.ifref_list) = (yyvsp[-1].ifref_list); ;}
    break;

  case 5:
#line 270 "tools\\widl_new\\parser.y"
    { (yyval.ifref_list) = append_ifref( (yyvsp[-1].ifref_list), make_ifref((yyvsp[0].type)) ); ;}
    break;

  case 6:
#line 271 "tools\\widl_new\\parser.y"
    { (yyval.ifref_list) = (yyvsp[-2].ifref_list);
						  reg_type((yyvsp[-1].type), (yyvsp[-1].type)->name, 0);
						  if (!parse_only && do_header) write_coclass_forward((yyvsp[-1].type));
						;}
    break;

  case 7:
#line 275 "tools\\widl_new\\parser.y"
    { (yyval.ifref_list) = (yyvsp[-1].ifref_list);
						  add_typelib_entry((yyvsp[0].type));
						  reg_type((yyvsp[0].type), (yyvsp[0].type)->name, 0);
						  if (!parse_only && do_header) write_coclass_forward((yyvsp[0].type));
						;}
    break;

  case 8:
#line 280 "tools\\widl_new\\parser.y"
    { (yyval.ifref_list) = (yyvsp[-1].ifref_list); add_typelib_entry((yyvsp[0].type)); ;}
    break;

  case 9:
#line 281 "tools\\widl_new\\parser.y"
    { (yyval.ifref_list) = (yyvsp[-1].ifref_list); ;}
    break;

  case 10:
#line 282 "tools\\widl_new\\parser.y"
    { (yyval.ifref_list) = (yyvsp[-1].ifref_list); ;}
    break;

  case 11:
#line 285 "tools\\widl_new\\parser.y"
    {;}
    break;

  case 12:
#line 286 "tools\\widl_new\\parser.y"
    { if (!parse_only) add_typelib_entry((yyvsp[0].type)); ;}
    break;

  case 13:
#line 287 "tools\\widl_new\\parser.y"
    { if (!parse_only) add_typelib_entry((yyvsp[0].type)); ;}
    break;

  case 14:
#line 288 "tools\\widl_new\\parser.y"
    { reg_type((yyvsp[-1].type), (yyvsp[-1].type)->name, 0); if (!parse_only && do_header) write_coclass_forward((yyvsp[-1].type)); ;}
    break;

  case 15:
#line 289 "tools\\widl_new\\parser.y"
    { if (!parse_only) add_typelib_entry((yyvsp[0].type));
						  reg_type((yyvsp[0].type), (yyvsp[0].type)->name, 0);
						  if (!parse_only && do_header) write_coclass_forward((yyvsp[0].type));
						;}
    break;

  case 16:
#line 293 "tools\\widl_new\\parser.y"
    { if (!parse_only) add_typelib_entry((yyvsp[0].type)); ;}
    break;

  case 17:
#line 294 "tools\\widl_new\\parser.y"
    {;}
    break;

  case 18:
#line 295 "tools\\widl_new\\parser.y"
    {;}
    break;

  case 19:
#line 298 "tools\\widl_new\\parser.y"
    { (yyval.func_list) = NULL; ;}
    break;

  case 20:
#line 299 "tools\\widl_new\\parser.y"
    { (yyval.func_list) = append_func( (yyvsp[-2].func_list), (yyvsp[-1].func) ); ;}
    break;

  case 21:
#line 300 "tools\\widl_new\\parser.y"
    { (yyval.func_list) = (yyvsp[-1].func_list); ;}
    break;

  case 22:
#line 303 "tools\\widl_new\\parser.y"
    {;}
    break;

  case 23:
#line 304 "tools\\widl_new\\parser.y"
    { if (!parse_only && do_header) { write_constdef((yyvsp[-1].var)); } ;}
    break;

  case 24:
#line 305 "tools\\widl_new\\parser.y"
    {;}
    break;

  case 25:
#line 306 "tools\\widl_new\\parser.y"
    { if (!parse_only && do_header) { write_type(header, (yyvsp[-1].type)); fprintf(header, ";\n\n"); } ;}
    break;

  case 26:
#line 307 "tools\\widl_new\\parser.y"
    { if (!parse_only && do_header) { write_externdef((yyvsp[-1].var)); } ;}
    break;

  case 27:
#line 308 "tools\\widl_new\\parser.y"
    {;}
    break;

  case 28:
#line 309 "tools\\widl_new\\parser.y"
    { if (!parse_only && do_header) { write_type(header, (yyvsp[-1].type)); fprintf(header, ";\n\n"); } ;}
    break;

  case 29:
#line 310 "tools\\widl_new\\parser.y"
    {;}
    break;

  case 30:
#line 311 "tools\\widl_new\\parser.y"
    { if (!parse_only && do_header) { write_type(header, (yyvsp[-1].type)); fprintf(header, ";\n\n"); } ;}
    break;

  case 31:
#line 314 "tools\\widl_new\\parser.y"
    { if (!parse_only && do_header) fprintf(header, "%s\n", (yyvsp[-1].str)); ;}
    break;

  case 32:
#line 316 "tools\\widl_new\\parser.y"
    { assert(yychar == YYEMPTY);
						  if (!do_import((yyvsp[-1].str))) yychar = aEOF; ;}
    break;

  case 33:
#line 319 "tools\\widl_new\\parser.y"
    {;}
    break;

  case 34:
#line 322 "tools\\widl_new\\parser.y"
    { if(!parse_only) add_importlib((yyvsp[-1].str)); ;}
    break;

  case 35:
#line 325 "tools\\widl_new\\parser.y"
    { (yyval.str) = (yyvsp[0].str); ;}
    break;

  case 36:
#line 327 "tools\\widl_new\\parser.y"
    { start_typelib((yyvsp[-1].str), (yyvsp[-2].attr_list));
						  if (!parse_only && do_header) write_library((yyvsp[-1].str), (yyvsp[-2].attr_list));
						  if (!parse_only && do_idfile) write_libid((yyvsp[-1].str), (yyvsp[-2].attr_list));
						;}
    break;

  case 37:
#line 332 "tools\\widl_new\\parser.y"
    { end_typelib(); ;}
    break;

  case 38:
#line 335 "tools\\widl_new\\parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 40:
#line 339 "tools\\widl_new\\parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 41:
#line 342 "tools\\widl_new\\parser.y"
    { check_arg((yyvsp[0].var)); (yyval.var_list) = append_var( NULL, (yyvsp[0].var) ); ;}
    break;

  case 42:
#line 343 "tools\\widl_new\\parser.y"
    { check_arg((yyvsp[0].var)); (yyval.var_list) = append_var( (yyvsp[-2].var_list), (yyvsp[0].var)); ;}
    break;

  case 44:
#line 348 "tools\\widl_new\\parser.y"
    { (yyval.var) = (yyvsp[-1].pident)->var;
						  set_type((yyval.var), (yyvsp[-2].type), (yyvsp[-1].pident)->ptr_level, (yyvsp[0].array_dims));
						  free((yyvsp[-1].pident));
						  (yyval.var)->attrs = (yyvsp[-3].attr_list);
						;}
    break;

  case 45:
#line 353 "tools\\widl_new\\parser.y"
    { (yyval.var) = (yyvsp[-1].pident)->var;
						  set_type((yyval.var), (yyvsp[-2].type), (yyvsp[-1].pident)->ptr_level, (yyvsp[0].array_dims));
						  free((yyvsp[-1].pident));
						;}
    break;

  case 46:
#line 357 "tools\\widl_new\\parser.y"
    { (yyval.var) = (yyvsp[-3].pident)->var;
						  set_type((yyval.var), (yyvsp[-4].type), (yyvsp[-3].pident)->ptr_level - 1, NULL);
						  free((yyvsp[-3].pident));
						  (yyval.var)->attrs = (yyvsp[-5].attr_list);
						  (yyval.var)->args = (yyvsp[-1].var_list);
						;}
    break;

  case 47:
#line 363 "tools\\widl_new\\parser.y"
    { (yyval.var) = (yyvsp[-3].pident)->var;
						  set_type((yyval.var), (yyvsp[-4].type), (yyvsp[-3].pident)->ptr_level - 1, NULL);
						  free((yyvsp[-3].pident));
						  (yyval.var)->args = (yyvsp[-1].var_list);
						;}
    break;

  case 48:
#line 370 "tools\\widl_new\\parser.y"
    { (yyval.array_dims) = NULL; ;}
    break;

  case 49:
#line 371 "tools\\widl_new\\parser.y"
    { (yyval.array_dims) = (yyvsp[-1].array_dims); ;}
    break;

  case 50:
#line 372 "tools\\widl_new\\parser.y"
    { (yyval.array_dims) = append_array( NULL, make_expr(EXPR_VOID) ); ;}
    break;

  case 51:
#line 375 "tools\\widl_new\\parser.y"
    { (yyval.array_dims) = append_array( NULL, (yyvsp[0].expr) ); ;}
    break;

  case 52:
#line 376 "tools\\widl_new\\parser.y"
    { (yyval.array_dims) = append_array( (yyvsp[-2].array_dims), (yyvsp[0].expr) ); ;}
    break;

  case 53:
#line 377 "tools\\widl_new\\parser.y"
    { (yyval.array_dims) = append_array( (yyvsp[-3].array_dims), (yyvsp[0].expr) ); ;}
    break;

  case 54:
#line 380 "tools\\widl_new\\parser.y"
    { (yyval.attr_list) = NULL; ;}
    break;

  case 56:
#line 385 "tools\\widl_new\\parser.y"
    { (yyval.attr_list) = (yyvsp[-1].attr_list);
						  if (!(yyval.attr_list))
						    yyerror("empty attribute lists unsupported");
						;}
    break;

  case 57:
#line 391 "tools\\widl_new\\parser.y"
    { (yyval.attr_list) = append_attr( NULL, (yyvsp[0].attr) ); ;}
    break;

  case 58:
#line 392 "tools\\widl_new\\parser.y"
    { (yyval.attr_list) = append_attr( (yyvsp[-2].attr_list), (yyvsp[0].attr) ); ;}
    break;

  case 59:
#line 393 "tools\\widl_new\\parser.y"
    { (yyval.attr_list) = append_attr( (yyvsp[-3].attr_list), (yyvsp[0].attr) ); ;}
    break;

  case 60:
#line 396 "tools\\widl_new\\parser.y"
    { (yyval.str_list) = append_str( NULL, (yyvsp[0].str) ); ;}
    break;

  case 61:
#line 397 "tools\\widl_new\\parser.y"
    { (yyval.str_list) = append_str( (yyvsp[-2].str_list), (yyvsp[0].str) ); ;}
    break;

  case 62:
#line 400 "tools\\widl_new\\parser.y"
    { (yyval.attr) = NULL; ;}
    break;

  case 63:
#line 401 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_AGGREGATABLE); ;}
    break;

  case 64:
#line 402 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_APPOBJECT); ;}
    break;

  case 65:
#line 403 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_ASYNC); ;}
    break;

  case 66:
#line 404 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_AUTO_HANDLE); ;}
    break;

  case 67:
#line 405 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_BINDABLE); ;}
    break;

  case 68:
#line 406 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrp(ATTR_CALLAS, (yyvsp[-1].var)); ;}
    break;

  case 69:
#line 407 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrp(ATTR_CASE, (yyvsp[-1].expr_list)); ;}
    break;

  case 70:
#line 408 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); ;}
    break;

  case 71:
#line 409 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_DONT_SERIALIZE */ ;}
    break;

  case 72:
#line 410 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_SERIALIZE */ ;}
    break;

  case 73:
#line 411 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_CONTROL); ;}
    break;

  case 74:
#line 412 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULT); ;}
    break;

  case 75:
#line 413 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULTCOLLELEM); ;}
    break;

  case 76:
#line 414 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrp(ATTR_DEFAULTVALUE_EXPR, (yyvsp[-1].expr)); ;}
    break;

  case 77:
#line 415 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrp(ATTR_DEFAULTVALUE_STRING, (yyvsp[-1].str)); ;}
    break;

  case 78:
#line 416 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULTVTABLE); ;}
    break;

  case 79:
#line 417 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_DISPLAYBIND); ;}
    break;

  case 80:
#line 418 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrp(ATTR_DLLNAME, (yyvsp[-1].str)); ;}
    break;

  case 81:
#line 419 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_DUAL); ;}
    break;

  case 82:
#line 420 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrp(ATTR_ENDPOINT, (yyvsp[-1].str_list)); ;}
    break;

  case 83:
#line 421 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrp(ATTR_ENTRY_STRING, (yyvsp[-1].str)); ;}
    break;

  case 84:
#line 422 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrp(ATTR_ENTRY_ORDINAL, (yyvsp[-1].expr)); ;}
    break;

  case 85:
#line 423 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_EXPLICIT_HANDLE); ;}
    break;

  case 86:
#line 424 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_HANDLE); ;}
    break;

  case 87:
#line 425 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPCONTEXT, (yyvsp[-1].expr)); ;}
    break;

  case 88:
#line 426 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPFILE, (yyvsp[-1].str)); ;}
    break;

  case 89:
#line 427 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRING, (yyvsp[-1].str)); ;}
    break;

  case 90:
#line 428 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRINGCONTEXT, (yyvsp[-1].expr)); ;}
    break;

  case 91:
#line 429 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRINGDLL, (yyvsp[-1].str)); ;}
    break;

  case 92:
#line 430 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_HIDDEN); ;}
    break;

  case 93:
#line 431 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrp(ATTR_ID, (yyvsp[-1].expr)); ;}
    break;

  case 94:
#line 432 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_IDEMPOTENT); ;}
    break;

  case 95:
#line 433 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrp(ATTR_IIDIS, (yyvsp[-1].var)); ;}
    break;

  case 96:
#line 434 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_IMMEDIATEBIND); ;}
    break;

  case 97:
#line 435 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrp(ATTR_IMPLICIT_HANDLE, (yyvsp[-1].str)); ;}
    break;

  case 98:
#line 436 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_IN); ;}
    break;

  case 99:
#line 437 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_INPUTSYNC); ;}
    break;

  case 100:
#line 438 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrp(ATTR_LENGTHIS, (yyvsp[-1].expr_list)); ;}
    break;

  case 101:
#line 439 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_LOCAL); ;}
    break;

  case 102:
#line 440 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_NONBROWSABLE); ;}
    break;

  case 103:
#line 441 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_NONCREATABLE); ;}
    break;

  case 104:
#line 442 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_NONEXTENSIBLE); ;}
    break;

  case 105:
#line 443 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_OBJECT); ;}
    break;

  case 106:
#line 444 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_ODL); ;}
    break;

  case 107:
#line 445 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_OLEAUTOMATION); ;}
    break;

  case 108:
#line 446 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_OPTIONAL); ;}
    break;

  case 109:
#line 447 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_OUT); ;}
    break;

  case 110:
#line 448 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrv(ATTR_POINTERDEFAULT, (yyvsp[-1].num)); ;}
    break;

  case 111:
#line 449 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPGET); ;}
    break;

  case 112:
#line 450 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPPUT); ;}
    break;

  case 113:
#line 451 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPPUTREF); ;}
    break;

  case 114:
#line 452 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_PUBLIC); ;}
    break;

  case 115:
#line 453 "tools\\widl_new\\parser.y"
    { expr_list_t *list = append_expr( NULL, (yyvsp[-3].expr) );
                                                     list = append_expr( list, (yyvsp[-1].expr) );
                                                     (yyval.attr) = make_attrp(ATTR_RANGE, list); ;}
    break;

  case 116:
#line 456 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_READONLY); ;}
    break;

  case 117:
#line 457 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_REQUESTEDIT); ;}
    break;

  case 118:
#line 458 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_RESTRICTED); ;}
    break;

  case 119:
#line 459 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_RETVAL); ;}
    break;

  case 120:
#line 460 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrp(ATTR_SIZEIS, (yyvsp[-1].expr_list)); ;}
    break;

  case 121:
#line 461 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_SOURCE); ;}
    break;

  case 122:
#line 462 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_STRING); ;}
    break;

  case 123:
#line 463 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrp(ATTR_SWITCHIS, (yyvsp[-1].expr)); ;}
    break;

  case 124:
#line 464 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrp(ATTR_SWITCHTYPE, (yyvsp[-1].type)); ;}
    break;

  case 125:
#line 465 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrp(ATTR_TRANSMITAS, (yyvsp[-1].type)); ;}
    break;

  case 126:
#line 466 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrp(ATTR_UUID, (yyvsp[-1].uuid)); ;}
    break;

  case 127:
#line 467 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_V1ENUM); ;}
    break;

  case 128:
#line 468 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attr(ATTR_VARARG); ;}
    break;

  case 129:
#line 469 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrv(ATTR_VERSION, (yyvsp[-1].num)); ;}
    break;

  case 130:
#line 470 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrp(ATTR_WIREMARSHAL, (yyvsp[-1].type)); ;}
    break;

  case 131:
#line 471 "tools\\widl_new\\parser.y"
    { (yyval.attr) = make_attrv(ATTR_POINTERTYPE, (yyvsp[0].num)); ;}
    break;

  case 134:
#line 478 "tools\\widl_new\\parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 135:
#line 479 "tools\\widl_new\\parser.y"
    { (yyval.var_list) = append_var( (yyvsp[-1].var_list), (yyvsp[0].var) ); ;}
    break;

  case 136:
#line 482 "tools\\widl_new\\parser.y"
    { attr_t *a = make_attrp(ATTR_CASE, (yyvsp[-2].expr));
						  (yyval.var) = (yyvsp[0].var); if (!(yyval.var)) (yyval.var) = make_var(NULL);
						  (yyval.var)->attrs = append_attr( (yyval.var)->attrs, a );
						;}
    break;

  case 137:
#line 486 "tools\\widl_new\\parser.y"
    { attr_t *a = make_attr(ATTR_DEFAULT);
						  (yyval.var) = (yyvsp[0].var); if (!(yyval.var)) (yyval.var) = make_var(NULL);
						  (yyval.var)->attrs = append_attr( (yyval.var)->attrs, a );
						;}
    break;

  case 138:
#line 492 "tools\\widl_new\\parser.y"
    { (yyval.var) = reg_const((yyvsp[-2].var));
						  set_type((yyval.var), (yyvsp[-3].type), 0, NULL);
						  (yyval.var)->eval = (yyvsp[0].expr);
						;}
    break;

  case 139:
#line 498 "tools\\widl_new\\parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 140:
#line 499 "tools\\widl_new\\parser.y"
    { (yyval.var_list) = (yyvsp[-1].var_list); ;}
    break;

  case 142:
#line 503 "tools\\widl_new\\parser.y"
    { if (!(yyvsp[0].var)->eval)
						    (yyvsp[0].var)->eval = make_exprl(EXPR_NUM, 0 /* default for first enum entry */);
                                                  (yyval.var_list) = append_var( NULL, (yyvsp[0].var) );
						;}
    break;

  case 143:
#line 507 "tools\\widl_new\\parser.y"
    { if (!(yyvsp[0].var)->eval)
                                                  {
                                                    var_t *last = LIST_ENTRY( list_tail((yyval.var_list)), var_t, entry );
                                                    (yyvsp[0].var)->eval = make_exprl(EXPR_NUM, last->eval->cval + 1);
                                                  }
                                                  (yyval.var_list) = append_var( (yyvsp[-2].var_list), (yyvsp[0].var) );
						;}
    break;

  case 144:
#line 516 "tools\\widl_new\\parser.y"
    { (yyval.var) = reg_const((yyvsp[-2].var));
						  (yyval.var)->eval = (yyvsp[0].expr);
                                                  (yyval.var)->type = make_int(0);
						;}
    break;

  case 145:
#line 520 "tools\\widl_new\\parser.y"
    { (yyval.var) = reg_const((yyvsp[0].var));
                                                  (yyval.var)->type = make_int(0);
						;}
    break;

  case 146:
#line 525 "tools\\widl_new\\parser.y"
    { (yyval.type) = get_typev(RPC_FC_ENUM16, (yyvsp[-3].var), tsENUM);
						  (yyval.type)->kind = TKIND_ENUM;
						  (yyval.type)->fields = (yyvsp[-1].var_list);
						  (yyval.type)->defined = TRUE;
                                                  if(in_typelib)
                                                      add_typelib_entry((yyval.type));
						;}
    break;

  case 147:
#line 534 "tools\\widl_new\\parser.y"
    { (yyval.expr_list) = append_expr( NULL, (yyvsp[0].expr) ); ;}
    break;

  case 148:
#line 535 "tools\\widl_new\\parser.y"
    { (yyval.expr_list) = append_expr( (yyvsp[-2].expr_list), (yyvsp[0].expr) ); ;}
    break;

  case 149:
#line 548 "tools\\widl_new\\parser.y"
    { (yyval.expr) = make_expr(EXPR_VOID); ;}
    break;

  case 151:
#line 552 "tools\\widl_new\\parser.y"
    { (yyval.expr) = make_exprl(EXPR_NUM, (yyvsp[0].num)); ;}
    break;

  case 152:
#line 553 "tools\\widl_new\\parser.y"
    { (yyval.expr) = make_exprl(EXPR_HEXNUM, (yyvsp[0].num)); ;}
    break;

  case 153:
#line 554 "tools\\widl_new\\parser.y"
    { (yyval.expr) = make_exprl(EXPR_TRUEFALSE, 0); ;}
    break;

  case 154:
#line 555 "tools\\widl_new\\parser.y"
    { (yyval.expr) = make_exprl(EXPR_TRUEFALSE, 1); ;}
    break;

  case 155:
#line 556 "tools\\widl_new\\parser.y"
    { (yyval.expr) = make_exprs(EXPR_IDENTIFIER, (yyvsp[0].str)); ;}
    break;

  case 156:
#line 557 "tools\\widl_new\\parser.y"
    { (yyval.expr) = make_expr3(EXPR_COND, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 157:
#line 558 "tools\\widl_new\\parser.y"
    { (yyval.expr) = make_expr2(EXPR_OR , (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 158:
#line 559 "tools\\widl_new\\parser.y"
    { (yyval.expr) = make_expr2(EXPR_AND, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 159:
#line 560 "tools\\widl_new\\parser.y"
    { (yyval.expr) = make_expr2(EXPR_ADD, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 160:
#line 561 "tools\\widl_new\\parser.y"
    { (yyval.expr) = make_expr2(EXPR_SUB, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 161:
#line 562 "tools\\widl_new\\parser.y"
    { (yyval.expr) = make_expr2(EXPR_MUL, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 162:
#line 563 "tools\\widl_new\\parser.y"
    { (yyval.expr) = make_expr2(EXPR_DIV, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 163:
#line 564 "tools\\widl_new\\parser.y"
    { (yyval.expr) = make_expr2(EXPR_SHL, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 164:
#line 565 "tools\\widl_new\\parser.y"
    { (yyval.expr) = make_expr2(EXPR_SHR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 165:
#line 566 "tools\\widl_new\\parser.y"
    { (yyval.expr) = make_expr1(EXPR_NOT, (yyvsp[0].expr)); ;}
    break;

  case 166:
#line 567 "tools\\widl_new\\parser.y"
    { (yyval.expr) = make_expr1(EXPR_NEG, (yyvsp[0].expr)); ;}
    break;

  case 167:
#line 568 "tools\\widl_new\\parser.y"
    { (yyval.expr) = make_expr1(EXPR_PPTR, (yyvsp[0].expr)); ;}
    break;

  case 168:
#line 569 "tools\\widl_new\\parser.y"
    { (yyval.expr) = make_exprt(EXPR_CAST, (yyvsp[-2].type), (yyvsp[0].expr)); ;}
    break;

  case 169:
#line 570 "tools\\widl_new\\parser.y"
    { (yyval.expr) = make_exprt(EXPR_SIZEOF, (yyvsp[-1].type), NULL); ;}
    break;

  case 170:
#line 571 "tools\\widl_new\\parser.y"
    { (yyval.expr) = (yyvsp[-1].expr); ;}
    break;

  case 171:
#line 574 "tools\\widl_new\\parser.y"
    { (yyval.expr_list) = append_expr( NULL, (yyvsp[0].expr) ); ;}
    break;

  case 172:
#line 575 "tools\\widl_new\\parser.y"
    { (yyval.expr_list) = append_expr( (yyvsp[-2].expr_list), (yyvsp[0].expr) ); ;}
    break;

  case 173:
#line 578 "tools\\widl_new\\parser.y"
    { (yyval.expr) = (yyvsp[0].expr);
						  if (!(yyval.expr)->is_const)
						      yyerror("expression is not constant");
						;}
    break;

  case 174:
#line 584 "tools\\widl_new\\parser.y"
    { (yyval.var) = (yyvsp[0].var);
						  set_type((yyval.var), (yyvsp[-1].type), 0, NULL);
						;}
    break;

  case 175:
#line 589 "tools\\widl_new\\parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 176:
#line 590 "tools\\widl_new\\parser.y"
    { (yyval.var_list) = append_var( (yyvsp[-1].var_list), (yyvsp[0].var) ); ;}
    break;

  case 177:
#line 593 "tools\\widl_new\\parser.y"
    { (yyval.var) = (yyvsp[-1].var); ;}
    break;

  case 178:
#line 594 "tools\\widl_new\\parser.y"
    { (yyval.var) = make_var(NULL); (yyval.var)->type = (yyvsp[-1].type); (yyval.var)->attrs = (yyvsp[-2].attr_list); ;}
    break;

  case 179:
#line 595 "tools\\widl_new\\parser.y"
    { (yyval.var) = make_var(NULL); (yyval.var)->attrs = (yyvsp[-1].attr_list); ;}
    break;

  case 180:
#line 596 "tools\\widl_new\\parser.y"
    { (yyval.var) = NULL; ;}
    break;

  case 181:
#line 599 "tools\\widl_new\\parser.y"
    { (yyval.var) = (yyvsp[-1].pident)->var;
						  set_type((yyval.var), (yyvsp[-2].type), (yyvsp[-1].pident)->ptr_level, (yyvsp[0].array_dims));
						  free((yyvsp[-1].pident));
						  (yyval.var)->attrs = (yyvsp[-3].attr_list);
						;}
    break;

  case 182:
#line 608 "tools\\widl_new\\parser.y"
    { var_t *v = (yyvsp[-3].pident)->var;
						  set_type(v, (yyvsp[-5].type), (yyvsp[-3].pident)->ptr_level, NULL);
						  free((yyvsp[-3].pident));
						  v->attrs = (yyvsp[-6].attr_list);
						  (yyval.func) = make_func(v, (yyvsp[-1].var_list));
						  if (is_attr(v->attrs, ATTR_IN)) {
						    yyerror("inapplicable attribute [in] for function '%s'",(yyval.func)->def->name);
						  }
						;}
    break;

  case 183:
#line 619 "tools\\widl_new\\parser.y"
    { (yyval.var) = NULL; ;}
    break;

  case 185:
#line 623 "tools\\widl_new\\parser.y"
    { (yyval.var) = NULL; ;}
    break;

  case 186:
#line 624 "tools\\widl_new\\parser.y"
    { (yyval.var) = make_var((yyvsp[0].str)); ;}
    break;

  case 187:
#line 625 "tools\\widl_new\\parser.y"
    { (yyval.var) = make_var((yyvsp[0].str)); ;}
    break;

  case 188:
#line 628 "tools\\widl_new\\parser.y"
    { (yyval.var) = make_var((yyvsp[0].str)); ;}
    break;

  case 189:
#line 630 "tools\\widl_new\\parser.y"
    { (yyval.var) = make_var((yyvsp[0].str)); ;}
    break;

  case 190:
#line 633 "tools\\widl_new\\parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 191:
#line 634 "tools\\widl_new\\parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 193:
#line 636 "tools\\widl_new\\parser.y"
    { (yyval.type) = (yyvsp[0].type); (yyval.type)->sign = 1; ;}
    break;

  case 194:
#line 637 "tools\\widl_new\\parser.y"
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

  case 195:
#line 653 "tools\\widl_new\\parser.y"
    { (yyval.type) = make_int(-1); ;}
    break;

  case 196:
#line 654 "tools\\widl_new\\parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 197:
#line 655 "tools\\widl_new\\parser.y"
    { (yyval.type) = duptype(find_type("float", 0), 1); ;}
    break;

  case 198:
#line 656 "tools\\widl_new\\parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 199:
#line 657 "tools\\widl_new\\parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 200:
#line 658 "tools\\widl_new\\parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 201:
#line 659 "tools\\widl_new\\parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 204:
#line 666 "tools\\widl_new\\parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 205:
#line 667 "tools\\widl_new\\parser.y"
    { (yyval.type) = make_builtin((yyvsp[-1].str)); ;}
    break;

  case 206:
#line 668 "tools\\widl_new\\parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 207:
#line 669 "tools\\widl_new\\parser.y"
    { (yyval.type) = make_builtin((yyvsp[-1].str)); ;}
    break;

  case 208:
#line 670 "tools\\widl_new\\parser.y"
    { (yyval.type) = make_builtin((yyvsp[-1].str)); ;}
    break;

  case 209:
#line 671 "tools\\widl_new\\parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 210:
#line 672 "tools\\widl_new\\parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 211:
#line 675 "tools\\widl_new\\parser.y"
    { (yyval.type) = make_class((yyvsp[0].str)); ;}
    break;

  case 212:
#line 676 "tools\\widl_new\\parser.y"
    { (yyval.type) = find_type((yyvsp[0].str), 0);
						  if ((yyval.type)->defined) yyerror("multiple definition error");
						  if ((yyval.type)->kind != TKIND_COCLASS) yyerror("%s was not declared a coclass", (yyvsp[0].str));
						;}
    break;

  case 213:
#line 682 "tools\\widl_new\\parser.y"
    { (yyval.type) = (yyvsp[0].type);
						  (yyval.type)->attrs = (yyvsp[-1].attr_list);
						  if (!parse_only && do_header)
						    write_coclass((yyval.type));
						  if (!parse_only && do_idfile)
						    write_clsid((yyval.type));
						;}
    break;

  case 214:
#line 691 "tools\\widl_new\\parser.y"
    { (yyval.type) = (yyvsp[-3].type);
						  (yyval.type)->ifaces = (yyvsp[-1].ifref_list);
						  (yyval.type)->defined = TRUE;
						;}
    break;

  case 215:
#line 697 "tools\\widl_new\\parser.y"
    { (yyval.ifref_list) = NULL; ;}
    break;

  case 216:
#line 698 "tools\\widl_new\\parser.y"
    { (yyval.ifref_list) = append_ifref( (yyvsp[-1].ifref_list), (yyvsp[0].ifref) ); ;}
    break;

  case 217:
#line 702 "tools\\widl_new\\parser.y"
    { (yyval.ifref) = make_ifref((yyvsp[0].type)); (yyval.ifref)->attrs = (yyvsp[-1].attr_list); ;}
    break;

  case 218:
#line 705 "tools\\widl_new\\parser.y"
    { (yyval.type) = get_type(0, (yyvsp[0].str), 0); (yyval.type)->kind = TKIND_DISPATCH; ;}
    break;

  case 219:
#line 706 "tools\\widl_new\\parser.y"
    { (yyval.type) = get_type(0, (yyvsp[0].str), 0); (yyval.type)->kind = TKIND_DISPATCH; ;}
    break;

  case 220:
#line 709 "tools\\widl_new\\parser.y"
    { attr_t *attrs;
						  (yyval.type) = (yyvsp[0].type);
						  if ((yyval.type)->defined) yyerror("multiple definition error");
						  attrs = make_attr(ATTR_DISPINTERFACE);
						  (yyval.type)->attrs = append_attr( (yyvsp[-1].attr_list), attrs );
						  (yyval.type)->ref = find_type("IDispatch", 0);
						  if (!(yyval.type)->ref) yyerror("IDispatch is undefined");
						  (yyval.type)->defined = TRUE;
						  if (!parse_only && do_header) write_forward((yyval.type));
						;}
    break;

  case 221:
#line 721 "tools\\widl_new\\parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 222:
#line 722 "tools\\widl_new\\parser.y"
    { (yyval.var_list) = append_var( (yyvsp[-2].var_list), (yyvsp[-1].var) ); ;}
    break;

  case 223:
#line 725 "tools\\widl_new\\parser.y"
    { (yyval.func_list) = NULL; ;}
    break;

  case 224:
#line 726 "tools\\widl_new\\parser.y"
    { (yyval.func_list) = append_func( (yyvsp[-2].func_list), (yyvsp[-1].func) ); ;}
    break;

  case 225:
#line 732 "tools\\widl_new\\parser.y"
    { (yyval.type) = (yyvsp[-4].type);
						  (yyval.type)->fields = (yyvsp[-2].var_list);
						  (yyval.type)->funcs = (yyvsp[-1].func_list);
						  if (!parse_only && do_header) write_dispinterface((yyval.type));
						  if (!parse_only && do_idfile) write_diid((yyval.type));
						;}
    break;

  case 226:
#line 739 "tools\\widl_new\\parser.y"
    { (yyval.type) = (yyvsp[-4].type);
						  (yyval.type)->fields = (yyvsp[-2].type)->fields;
						  (yyval.type)->funcs = (yyvsp[-2].type)->funcs;
						  if (!parse_only && do_header) write_dispinterface((yyval.type));
						  if (!parse_only && do_idfile) write_diid((yyval.type));
						;}
    break;

  case 227:
#line 747 "tools\\widl_new\\parser.y"
    { (yyval.type) = NULL; ;}
    break;

  case 228:
#line 748 "tools\\widl_new\\parser.y"
    { (yyval.type) = find_type2((yyvsp[0].str), 0); ;}
    break;

  case 229:
#line 751 "tools\\widl_new\\parser.y"
    { (yyval.type) = get_type(RPC_FC_IP, (yyvsp[0].str), 0); (yyval.type)->kind = TKIND_INTERFACE; ;}
    break;

  case 230:
#line 752 "tools\\widl_new\\parser.y"
    { (yyval.type) = get_type(RPC_FC_IP, (yyvsp[0].str), 0); (yyval.type)->kind = TKIND_INTERFACE; ;}
    break;

  case 231:
#line 755 "tools\\widl_new\\parser.y"
    { (yyval.type) = (yyvsp[0].type);
						  if ((yyval.type)->defined) yyerror("multiple definition error");
						  (yyval.type)->attrs = (yyvsp[-1].attr_list);
						  (yyval.type)->defined = TRUE;
						  if (!parse_only && do_header) write_forward((yyval.type));
						;}
    break;

  case 232:
#line 764 "tools\\widl_new\\parser.y"
    { (yyval.type) = (yyvsp[-4].type);
						  (yyval.type)->ref = (yyvsp[-3].type);
						  (yyval.type)->funcs = (yyvsp[-1].func_list);
						  compute_method_indexes((yyval.type));
						  if (!parse_only && do_header) write_interface((yyval.type));
						  if (!parse_only && do_idfile) write_iid((yyval.type));
						;}
    break;

  case 233:
#line 774 "tools\\widl_new\\parser.y"
    { (yyval.type) = (yyvsp[-6].type);
						  (yyval.type)->ref = find_type2((yyvsp[-4].str), 0);
						  if (!(yyval.type)->ref) yyerror("base class '%s' not found in import", (yyvsp[-4].str));
						  (yyval.type)->funcs = (yyvsp[-1].func_list);
						  compute_method_indexes((yyval.type));
						  if (!parse_only && do_header) write_interface((yyval.type));
						  if (!parse_only && do_idfile) write_iid((yyval.type));
						;}
    break;

  case 234:
#line 782 "tools\\widl_new\\parser.y"
    { (yyval.type) = (yyvsp[0].type); ;}
    break;

  case 235:
#line 786 "tools\\widl_new\\parser.y"
    { (yyval.type) = (yyvsp[-1].type); if (!parse_only && do_header) write_forward((yyval.type)); ;}
    break;

  case 236:
#line 787 "tools\\widl_new\\parser.y"
    { (yyval.type) = (yyvsp[-1].type); if (!parse_only && do_header) write_forward((yyval.type)); ;}
    break;

  case 237:
#line 790 "tools\\widl_new\\parser.y"
    { (yyval.type) = make_type(0, NULL); (yyval.type)->name = (yyvsp[0].str); (yyval.type)->kind = TKIND_MODULE; ;}
    break;

  case 238:
#line 791 "tools\\widl_new\\parser.y"
    { (yyval.type) = make_type(0, NULL); (yyval.type)->name = (yyvsp[0].str); (yyval.type)->kind = TKIND_MODULE; ;}
    break;

  case 239:
#line 794 "tools\\widl_new\\parser.y"
    { (yyval.type) = (yyvsp[0].type);
						  (yyval.type)->attrs = (yyvsp[-1].attr_list);
						;}
    break;

  case 240:
#line 799 "tools\\widl_new\\parser.y"
    { (yyval.type) = (yyvsp[-3].type);
						  (yyval.type)->funcs = (yyvsp[-1].func_list);
						  /* FIXME: if (!parse_only && do_header) write_module($$); */
						;}
    break;

  case 241:
#line 805 "tools\\widl_new\\parser.y"
    { (yyval.pident) = (yyvsp[0].pident); (yyval.pident)->ptr_level++; ;}
    break;

  case 242:
#line 806 "tools\\widl_new\\parser.y"
    { (yyval.pident) = (yyvsp[0].pident); /* FIXME */ ;}
    break;

  case 243:
#line 809 "tools\\widl_new\\parser.y"
    { (yyval.pident) = make_pident((yyvsp[0].var)); ;}
    break;

  case 245:
#line 811 "tools\\widl_new\\parser.y"
    { (yyval.pident) = (yyvsp[-1].pident); ;}
    break;

  case 246:
#line 815 "tools\\widl_new\\parser.y"
    { (yyval.pident_list) = append_pident( NULL, (yyvsp[0].pident) ); ;}
    break;

  case 247:
#line 816 "tools\\widl_new\\parser.y"
    { (yyval.pident_list) = append_pident( (yyvsp[-2].pident_list), (yyvsp[0].pident) ); ;}
    break;

  case 248:
#line 820 "tools\\widl_new\\parser.y"
    { (yyval.num) = RPC_FC_RP; ;}
    break;

  case 249:
#line 821 "tools\\widl_new\\parser.y"
    { (yyval.num) = RPC_FC_UP; ;}
    break;

  case 250:
#line 822 "tools\\widl_new\\parser.y"
    { (yyval.num) = RPC_FC_FP; ;}
    break;

  case 251:
#line 825 "tools\\widl_new\\parser.y"
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

  case 252:
#line 836 "tools\\widl_new\\parser.y"
    { (yyval.type) = duptype(find_type("void", 0), 1); ;}
    break;

  case 253:
#line 837 "tools\\widl_new\\parser.y"
    { (yyval.type) = find_type((yyvsp[0].str), 0); ;}
    break;

  case 254:
#line 838 "tools\\widl_new\\parser.y"
    { (yyval.type) = (yyvsp[0].type); ;}
    break;

  case 255:
#line 839 "tools\\widl_new\\parser.y"
    { (yyval.type) = duptype((yyvsp[0].type), 1); (yyval.type)->is_const = TRUE; ;}
    break;

  case 256:
#line 840 "tools\\widl_new\\parser.y"
    { (yyval.type) = (yyvsp[0].type); ;}
    break;

  case 257:
#line 841 "tools\\widl_new\\parser.y"
    { (yyval.type) = find_type2((yyvsp[0].str), tsENUM); ;}
    break;

  case 258:
#line 842 "tools\\widl_new\\parser.y"
    { (yyval.type) = (yyvsp[0].type); ;}
    break;

  case 259:
#line 843 "tools\\widl_new\\parser.y"
    { (yyval.type) = get_type(RPC_FC_STRUCT, (yyvsp[0].str), tsSTRUCT); ;}
    break;

  case 260:
#line 844 "tools\\widl_new\\parser.y"
    { (yyval.type) = (yyvsp[0].type); ;}
    break;

  case 261:
#line 845 "tools\\widl_new\\parser.y"
    { (yyval.type) = find_type2((yyvsp[0].str), tsUNION); ;}
    break;

  case 262:
#line 846 "tools\\widl_new\\parser.y"
    { (yyval.type) = make_safearray((yyvsp[-1].type)); ;}
    break;

  case 263:
#line 849 "tools\\widl_new\\parser.y"
    { reg_typedefs((yyvsp[-1].type), (yyvsp[0].pident_list), (yyvsp[-2].attr_list));
						  process_typedefs((yyvsp[0].pident_list));
						;}
    break;

  case 264:
#line 854 "tools\\widl_new\\parser.y"
    { (yyval.type) = get_typev(RPC_FC_NON_ENCAPSULATED_UNION, (yyvsp[-3].var), tsUNION);
						  (yyval.type)->kind = TKIND_UNION;
						  (yyval.type)->fields = (yyvsp[-1].var_list);
						  (yyval.type)->defined = TRUE;
						;}
    break;

  case 265:
#line 861 "tools\\widl_new\\parser.y"
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

  case 266:
#line 876 "tools\\widl_new\\parser.y"
    { (yyval.num) = MAKELONG((yyvsp[0].num), 0); ;}
    break;

  case 267:
#line 877 "tools\\widl_new\\parser.y"
    { (yyval.num) = MAKELONG((yyvsp[-2].num), (yyvsp[0].num)); ;}
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
#line 3561 "tools\\widl_new\\parser.tab.c"

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


#line 880 "tools\\widl_new\\parser.y"


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
  t->typestring_offset = 0;
  t->ignore = parse_only;
  t->is_const = FALSE;
  t->sign = 0;
  t->defined = FALSE;
  t->written = FALSE;
  t->user_types_registered = FALSE;
  t->typelib_idx = -1;
  return t;
}

static void set_type(var_t *v, type_t *type, int ptr_level, array_dims_t *arr)
{
  v->type = type;
  v->array = arr;

  for ( ; 0 < ptr_level; --ptr_level)
    v->type = make_type(RPC_FC_RP, v->type);
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
  v->array = NULL;
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

    if (is_ptr(field->type))
    {
        has_pointer = 1;
        continue;
    }

    if (is_string_type(field->attrs, field->type, field->array))
    {
        has_conformance = 1;
        has_variance = 1;
        continue;
    }

    if (is_array_type(field->attrs, field->type, field->array))
    {
        if (field->array && is_conformant_array(field->array))
        {
            has_conformance = 1;
            if (list_next( fields, &field->entry ))
                yyerror("field '%s' deriving from a conformant array must be the last field in the structure",
                        field->name);
        }
        if (is_attr(field->attrs, ATTR_LENGTHIS))
            has_variance = 1;
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
      has_pointer = 1;
      break;
    case RPC_FC_CARRAY:
      has_conformance = 1;
      if (list_next( fields, &field->entry ))
          yyerror("field '%s' deriving from a conformant array must be the last field in the structure",
                  field->name);
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
    return RPC_FC_CVSTRUCT;
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

