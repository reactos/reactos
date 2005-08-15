/* A Bison parser, made by GNU Bison 1.875.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

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

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



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
     tARRAYS = 270,
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
     tDEFAULTVALUE = 294,
     tDISPINTERFACE = 295,
     tDLLNAME = 296,
     tDOUBLE = 297,
     tDUAL = 298,
     tENDPOINT = 299,
     tENTRY = 300,
     tENUM = 301,
     tERRORSTATUST = 302,
     tEXPLICITHANDLE = 303,
     tEXTERN = 304,
     tFLOAT = 305,
     tHANDLE = 306,
     tHANDLET = 307,
     tHELPCONTEXT = 308,
     tHELPFILE = 309,
     tHELPSTRING = 310,
     tHELPSTRINGCONTEXT = 311,
     tHELPSTRINGDLL = 312,
     tHIDDEN = 313,
     tHYPER = 314,
     tID = 315,
     tIDEMPOTENT = 316,
     tIIDIS = 317,
     tIMPLICITHANDLE = 318,
     tIMPORT = 319,
     tIMPORTLIB = 320,
     tIN = 321,
     tINCLUDE = 322,
     tINLINE = 323,
     tINPUTSYNC = 324,
     tINT = 325,
     tINT64 = 326,
     tINTERFACE = 327,
     tLENGTHIS = 328,
     tLIBRARY = 329,
     tLOCAL = 330,
     tLONG = 331,
     tMETHODS = 332,
     tMODULE = 333,
     tNONCREATABLE = 334,
     tOBJECT = 335,
     tODL = 336,
     tOLEAUTOMATION = 337,
     tOPTIONAL = 338,
     tOUT = 339,
     tPOINTERDEFAULT = 340,
     tPROPERTIES = 341,
     tPROPGET = 342,
     tPROPPUT = 343,
     tPROPPUTREF = 344,
     tPTR = 345,
     tPUBLIC = 346,
     tREADONLY = 347,
     tREF = 348,
     tRESTRICTED = 349,
     tRETVAL = 350,
     tSHORT = 351,
     tSIGNED = 352,
     tSINGLE = 353,
     tSIZEIS = 354,
     tSIZEOF = 355,
     tSMALL = 356,
     tSOURCE = 357,
     tSTDCALL = 358,
     tSTRING = 359,
     tSTRUCT = 360,
     tSWITCH = 361,
     tSWITCHIS = 362,
     tSWITCHTYPE = 363,
     tTRANSMITAS = 364,
     tTYPEDEF = 365,
     tUNION = 366,
     tUNIQUE = 367,
     tUNSIGNED = 368,
     tUUID = 369,
     tV1ENUM = 370,
     tVARARG = 371,
     tVERSION = 372,
     tVOID = 373,
     tWCHAR = 374,
     tWIREMARSHAL = 375,
     tPOINTERTYPE = 376,
     COND = 377,
     CAST = 378,
     PPTR = 379,
     NEG = 380
   };
#endif
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
#define tARRAYS 270
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
#define tDEFAULTVALUE 294
#define tDISPINTERFACE 295
#define tDLLNAME 296
#define tDOUBLE 297
#define tDUAL 298
#define tENDPOINT 299
#define tENTRY 300
#define tENUM 301
#define tERRORSTATUST 302
#define tEXPLICITHANDLE 303
#define tEXTERN 304
#define tFLOAT 305
#define tHANDLE 306
#define tHANDLET 307
#define tHELPCONTEXT 308
#define tHELPFILE 309
#define tHELPSTRING 310
#define tHELPSTRINGCONTEXT 311
#define tHELPSTRINGDLL 312
#define tHIDDEN 313
#define tHYPER 314
#define tID 315
#define tIDEMPOTENT 316
#define tIIDIS 317
#define tIMPLICITHANDLE 318
#define tIMPORT 319
#define tIMPORTLIB 320
#define tIN 321
#define tINCLUDE 322
#define tINLINE 323
#define tINPUTSYNC 324
#define tINT 325
#define tINT64 326
#define tINTERFACE 327
#define tLENGTHIS 328
#define tLIBRARY 329
#define tLOCAL 330
#define tLONG 331
#define tMETHODS 332
#define tMODULE 333
#define tNONCREATABLE 334
#define tOBJECT 335
#define tODL 336
#define tOLEAUTOMATION 337
#define tOPTIONAL 338
#define tOUT 339
#define tPOINTERDEFAULT 340
#define tPROPERTIES 341
#define tPROPGET 342
#define tPROPPUT 343
#define tPROPPUTREF 344
#define tPTR 345
#define tPUBLIC 346
#define tREADONLY 347
#define tREF 348
#define tRESTRICTED 349
#define tRETVAL 350
#define tSHORT 351
#define tSIGNED 352
#define tSINGLE 353
#define tSIZEIS 354
#define tSIZEOF 355
#define tSMALL 356
#define tSOURCE 357
#define tSTDCALL 358
#define tSTRING 359
#define tSTRUCT 360
#define tSWITCH 361
#define tSWITCHIS 362
#define tSWITCHTYPE 363
#define tTRANSMITAS 364
#define tTYPEDEF 365
#define tUNION 366
#define tUNIQUE 367
#define tUNSIGNED 368
#define tUUID 369
#define tV1ENUM 370
#define tVARARG 371
#define tVERSION 372
#define tVOID 373
#define tWCHAR 374
#define tWIREMARSHAL 375
#define tPOINTERTYPE 376
#define COND 377
#define CAST 378
#define PPTR 379
#define NEG 380




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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

static attr_t *make_attr(enum attr_type type);
static attr_t *make_attrv(enum attr_type type, unsigned long val);
static attr_t *make_attrp(enum attr_type type, void *val);
static expr_t *make_expr(enum expr_type type);
static expr_t *make_exprl(enum expr_type type, long val);
static expr_t *make_exprs(enum expr_type type, char *val);
static expr_t *make_exprt(enum expr_type type, typeref_t *tref, expr_t *expr);
static expr_t *make_expr1(enum expr_type type, expr_t *expr);
static expr_t *make_expr2(enum expr_type type, expr_t *exp1, expr_t *exp2);
static expr_t *make_expr3(enum expr_type type, expr_t *expr1, expr_t *expr2, expr_t *expr3);
static type_t *make_type(unsigned char type, type_t *ref);
static typeref_t *make_tref(char *name, type_t *ref);
static typeref_t *uniq_tref(typeref_t *ref);
static type_t *type_ref(typeref_t *ref);
static void set_type(var_t *v, typeref_t *ref, expr_t *arr);
static ifref_t *make_ifref(type_t *iface);
static var_t *make_var(char *name);
static func_t *make_func(var_t *def, var_t *args);
static class_t *make_class(char *name);

static type_t *reg_type(type_t *type, char *name, int t);
static type_t *reg_types(type_t *type, var_t *names, int t);
static type_t *find_type(char *name, int t);
static type_t *find_type2(char *name, int t);
static type_t *get_type(unsigned char type, char *name, int t);
static type_t *get_typev(unsigned char type, var_t *name, int t);
static int get_struct_type(var_t *fields);

static var_t *reg_const(var_t *var);
static var_t *find_const(char *name, int f);

#define tsENUM   1
#define tsSTRUCT 2
#define tsUNION  3

static type_t std_bool = { "boolean" };
static type_t std_int = { "int" };
static type_t std_int64 = { "__int64" };
static type_t std_uhyper = { "MIDL_uhyper" };



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

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 106 "parser.y"
typedef union YYSTYPE {
	attr_t *attr;
	expr_t *expr;
	type_t *type;
	typeref_t *tref;
	var_t *var;
	func_t *func;
	ifref_t *ifref;
	class_t *clas;
	char *str;
	UUID *uuid;
	unsigned int num;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 445 "y.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 457 "y.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
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
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   934

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  145
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  72
/* YYNRULES -- Number of rules. */
#define YYNRULES  249
/* YYNRULES -- Number of states. */
#define YYNSTATES  461

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   380

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   125,     2,
     135,   136,   128,   127,   122,   126,   144,   129,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   141,   134,
       2,   142,     2,   143,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   139,     2,   140,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   137,   124,   138,   130,     2,     2,     2,
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
     115,   116,   117,   118,   119,   120,   121,   123,   131,   132,
     133
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short yyprhs[] =
{
       0,     0,     3,     5,     6,     9,    12,    15,    18,    21,
      24,    25,    28,    31,    34,    37,    40,    41,    45,    48,
      50,    53,    55,    58,    61,    63,    66,    69,    72,    77,
      81,    85,    88,    92,    96,    97,    99,   101,   103,   107,
     109,   114,   118,   125,   131,   132,   136,   140,   142,   146,
     151,   152,   154,   158,   160,   164,   169,   171,   173,   178,
     183,   185,   187,   189,   191,   193,   198,   203,   208,   210,
     215,   220,   225,   227,   229,   234,   239,   244,   249,   254,
     256,   261,   263,   268,   274,   276,   278,   283,   285,   287,
     289,   291,   293,   295,   297,   302,   304,   306,   308,   310,
     312,   314,   316,   318,   320,   325,   327,   329,   334,   339,
     344,   346,   351,   353,   355,   360,   365,   367,   368,   370,
     371,   374,   379,   383,   389,   390,   393,   395,   397,   401,
     405,   407,   413,   414,   416,   418,   420,   422,   428,   432,
     436,   440,   444,   448,   452,   456,   460,   463,   466,   469,
     474,   479,   483,   485,   489,   491,   496,   497,   500,   503,
     507,   510,   512,   517,   525,   526,   528,   529,   531,   533,
     535,   537,   539,   541,   543,   545,   547,   549,   551,   554,
     557,   559,   561,   563,   565,   567,   569,   571,   572,   574,
     576,   579,   582,   585,   588,   590,   592,   595,   598,   601,
     606,   607,   610,   613,   616,   619,   622,   625,   629,   632,
     636,   642,   643,   646,   649,   652,   655,   661,   669,   671,
     674,   677,   680,   683,   686,   691,   694,   697,   699,   701,
     705,   707,   711,   713,   715,   721,   723,   725,   727,   730,
     732,   735,   737,   740,   742,   745,   750,   756,   767,   769
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short yyrhs[] =
{
     146,     0,    -1,   147,    -1,    -1,   147,   204,    -1,   147,
     203,    -1,   147,   192,    -1,   147,   207,    -1,   147,   156,
      -1,   147,   150,    -1,    -1,   148,   204,    -1,   148,   203,
      -1,   148,   192,    -1,   148,   207,    -1,   148,   150,    -1,
      -1,   149,   183,   134,    -1,   149,   150,    -1,   134,    -1,
     170,   134,    -1,   151,    -1,   174,   134,    -1,   179,   134,
      -1,   153,    -1,   212,   134,    -1,   214,   134,    -1,   215,
     134,    -1,    37,   135,     7,   136,    -1,    64,     7,   134,
      -1,   152,   148,     9,    -1,    74,     3,    -1,   164,   154,
     137,    -1,   155,   148,   138,    -1,    -1,   159,    -1,   118,
      -1,   160,    -1,   159,   122,   160,    -1,   158,    -1,   164,
     213,   209,   161,    -1,   213,   209,   161,    -1,   164,   213,
     209,   135,   157,   136,    -1,   213,   209,   135,   157,   136,
      -1,    -1,   139,   162,   140,    -1,   139,   128,   140,    -1,
     175,    -1,   162,   122,   176,    -1,   162,   140,   139,   176,
      -1,    -1,   164,    -1,   139,   165,   140,    -1,   166,    -1,
     165,   122,   166,    -1,   165,   140,   139,   166,    -1,    16,
      -1,    18,    -1,    24,   135,   186,   136,    -1,    26,   135,
     177,   136,    -1,    33,    -1,    34,    -1,    35,    -1,    36,
      -1,    38,    -1,    39,   135,   178,   136,    -1,    39,   135,
       7,   136,    -1,    41,   135,     7,   136,    -1,    43,    -1,
      44,   135,     7,   136,    -1,    45,   135,     7,   136,    -1,
      45,   135,   178,   136,    -1,    48,    -1,    51,    -1,    53,
     135,   178,   136,    -1,    54,   135,     7,   136,    -1,    55,
     135,     7,   136,    -1,    56,   135,   178,   136,    -1,    57,
     135,     7,   136,    -1,    58,    -1,    60,   135,   178,   136,
      -1,    61,    -1,    62,   135,   186,   136,    -1,    63,   135,
      52,     3,   136,    -1,    66,    -1,    69,    -1,    73,   135,
     210,   136,    -1,    75,    -1,    79,    -1,    80,    -1,    81,
      -1,    82,    -1,    83,    -1,    84,    -1,    85,   135,   211,
     136,    -1,    87,    -1,    88,    -1,    89,    -1,    90,    -1,
      91,    -1,    92,    -1,    93,    -1,    94,    -1,    95,    -1,
      99,   135,   210,   136,    -1,   102,    -1,   104,    -1,   107,
     135,   176,   136,    -1,   108,   135,   213,   136,    -1,   109,
     135,   213,   136,    -1,   112,    -1,   114,   135,     8,   136,
      -1,   115,    -1,   116,    -1,   117,   135,   216,   136,    -1,
     120,   135,   213,   136,    -1,   211,    -1,    -1,   103,    -1,
      -1,   168,   169,    -1,    26,   176,   141,   181,    -1,    38,
     141,   181,    -1,    32,   213,   186,   142,   178,    -1,    -1,
     172,   122,    -1,   172,    -1,   173,    -1,   172,   122,   173,
      -1,   186,   142,   178,    -1,   186,    -1,    46,   185,   137,
     171,   138,    -1,    -1,   176,    -1,     5,    -1,     6,    -1,
       3,    -1,   176,   143,   176,   141,   176,    -1,   176,   124,
     176,    -1,   176,   125,   176,    -1,   176,   127,   176,    -1,
     176,   126,   176,    -1,   176,   128,   176,    -1,   176,   129,
     176,    -1,   176,    10,   176,    -1,   176,    11,   176,    -1,
     130,   176,    -1,   126,   176,    -1,   128,   176,    -1,   135,
     213,   136,   176,    -1,   100,   135,   213,   136,    -1,   135,
     176,   136,    -1,   178,    -1,   177,   122,   178,    -1,   176,
      -1,    49,    32,   213,   186,    -1,    -1,   180,   181,    -1,
     182,   134,    -1,   163,   215,   134,    -1,   164,   134,    -1,
     134,    -1,   163,   213,   209,   161,    -1,   163,   213,   167,
     209,   135,   157,   136,    -1,    -1,   186,    -1,    -1,     3,
      -1,     4,    -1,     3,    -1,     4,    -1,    16,    -1,    60,
      -1,    95,    -1,   117,    -1,    22,    -1,   119,    -1,   189,
      -1,    97,   189,    -1,   113,   189,    -1,   113,    -1,    50,
      -1,    98,    -1,    42,    -1,    20,    -1,    47,    -1,    52,
      -1,    -1,    70,    -1,    70,    -1,   101,   188,    -1,    96,
     188,    -1,    76,   188,    -1,    59,   188,    -1,    71,    -1,
      28,    -1,    29,     3,    -1,    29,     4,    -1,   164,   190,
      -1,   191,   137,   193,   138,    -1,    -1,   193,   194,    -1,
     163,   204,    -1,    40,     3,    -1,    40,     4,    -1,   164,
     195,    -1,    86,   141,    -1,   197,   182,   134,    -1,    77,
     141,    -1,   198,   183,   134,    -1,   196,   137,   197,   198,
     138,    -1,    -1,   141,     4,    -1,    72,     3,    -1,    72,
       4,    -1,   164,   201,    -1,   202,   200,   137,   149,   138,
      -1,   202,   141,     3,   137,   153,   149,   138,    -1,   199,
      -1,   201,   134,    -1,   195,   134,    -1,    78,     3,    -1,
      78,     4,    -1,   164,   205,    -1,   206,   137,   149,   138,
      -1,   128,   209,    -1,    32,   208,    -1,   186,    -1,   208,
      -1,   135,   209,   136,    -1,   209,    -1,   210,   122,   209,
      -1,    93,    -1,   112,    -1,   105,   185,   137,   180,   138,
      -1,   118,    -1,     4,    -1,   187,    -1,    32,   213,    -1,
     174,    -1,    46,     3,    -1,   212,    -1,   105,     3,    -1,
     215,    -1,   111,     3,    -1,   110,   163,   213,   210,    -1,
     111,   185,   137,   180,   138,    -1,   111,   185,   106,   135,
     182,   136,   184,   137,   168,   138,    -1,     5,    -1,     5,
     144,     5,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   228,   228,   231,   232,   233,   234,   235,   236,   237,
     240,   241,   242,   243,   244,   245,   248,   249,   250,   253,
     254,   255,   256,   257,   258,   259,   260,   261,   264,   266,
     269,   272,   274,   276,   279,   280,   283,   286,   287,   288,
     292,   296,   299,   305,   312,   313,   314,   317,   318,   319,
     322,   323,   327,   330,   331,   332,   336,   337,   338,   339,
     340,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,   355,   356,   357,   358,   359,
     360,   361,   362,   363,   364,   365,   366,   367,   368,   369,
     370,   371,   372,   373,   374,   375,   376,   377,   378,   379,
     380,   381,   382,   383,   384,   385,   386,   387,   388,   389,
     390,   391,   392,   393,   394,   395,   396,   399,   400,   403,
     404,   409,   413,   419,   426,   427,   428,   431,   432,   438,
     443,   449,   471,   472,   475,   476,   477,   478,   479,   480,
     481,   482,   483,   484,   485,   486,   487,   488,   489,   490,
     491,   492,   495,   496,   499,   505,   510,   511,   516,   517,
     518,   519,   522,   526,   536,   537,   540,   541,   542,   545,
     547,   548,   549,   550,   551,   554,   555,   556,   557,   558,
     570,   571,   572,   573,   574,   575,   576,   579,   580,   583,
     584,   585,   586,   587,   588,   589,   592,   593,   596,   603,
     608,   609,   613,   616,   617,   620,   632,   633,   636,   637,
     640,   655,   656,   659,   660,   663,   671,   679,   686,   690,
     691,   694,   695,   698,   703,   709,   710,   713,   714,   715,
     719,   720,   724,   725,   728,   738,   739,   740,   741,   742,
     743,   744,   745,   746,   747,   750,   763,   767,   781,   782
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "aIDENTIFIER", "aKNOWNTYPE", "aNUM", 
  "aHEXNUM", "aSTRING", "aUUID", "aEOF", "SHL", "SHR", "tAGGREGATABLE", 
  "tALLOCATE", "tAPPOBJECT", "tARRAYS", "tASYNC", "tASYNCUUID", 
  "tAUTOHANDLE", "tBINDABLE", "tBOOLEAN", "tBROADCAST", "tBYTE", 
  "tBYTECOUNT", "tCALLAS", "tCALLBACK", "tCASE", "tCDECL", "tCHAR", 
  "tCOCLASS", "tCODE", "tCOMMSTATUS", "tCONST", "tCONTEXTHANDLE", 
  "tCONTEXTHANDLENOSERIALIZE", "tCONTEXTHANDLESERIALIZE", "tCONTROL", 
  "tCPPQUOTE", "tDEFAULT", "tDEFAULTVALUE", "tDISPINTERFACE", "tDLLNAME", 
  "tDOUBLE", "tDUAL", "tENDPOINT", "tENTRY", "tENUM", "tERRORSTATUST", 
  "tEXPLICITHANDLE", "tEXTERN", "tFLOAT", "tHANDLE", "tHANDLET", 
  "tHELPCONTEXT", "tHELPFILE", "tHELPSTRING", "tHELPSTRINGCONTEXT", 
  "tHELPSTRINGDLL", "tHIDDEN", "tHYPER", "tID", "tIDEMPOTENT", "tIIDIS", 
  "tIMPLICITHANDLE", "tIMPORT", "tIMPORTLIB", "tIN", "tINCLUDE", 
  "tINLINE", "tINPUTSYNC", "tINT", "tINT64", "tINTERFACE", "tLENGTHIS", 
  "tLIBRARY", "tLOCAL", "tLONG", "tMETHODS", "tMODULE", "tNONCREATABLE", 
  "tOBJECT", "tODL", "tOLEAUTOMATION", "tOPTIONAL", "tOUT", 
  "tPOINTERDEFAULT", "tPROPERTIES", "tPROPGET", "tPROPPUT", "tPROPPUTREF", 
  "tPTR", "tPUBLIC", "tREADONLY", "tREF", "tRESTRICTED", "tRETVAL", 
  "tSHORT", "tSIGNED", "tSINGLE", "tSIZEIS", "tSIZEOF", "tSMALL", 
  "tSOURCE", "tSTDCALL", "tSTRING", "tSTRUCT", "tSWITCH", "tSWITCHIS", 
  "tSWITCHTYPE", "tTRANSMITAS", "tTYPEDEF", "tUNION", "tUNIQUE", 
  "tUNSIGNED", "tUUID", "tV1ENUM", "tVARARG", "tVERSION", "tVOID", 
  "tWCHAR", "tWIREMARSHAL", "tPOINTERTYPE", "','", "COND", "'|'", "'&'", 
  "'-'", "'+'", "'*'", "'/'", "'~'", "CAST", "PPTR", "NEG", "';'", "'('", 
  "')'", "'{'", "'}'", "'['", "']'", "':'", "'='", "'?'", "'.'", 
  "$accept", "input", "gbl_statements", "imp_statements", 
  "int_statements", "statement", "cppquote", "import_start", "import", 
  "libraryhdr", "library_start", "librarydef", "m_args", "no_args", 
  "args", "arg", "array", "array_list", "m_attributes", "attributes", 
  "attrib_list", "attribute", "callconv", "cases", "case", "constdef", 
  "enums", "enum_list", "enum", "enumdef", "m_expr", "expr", 
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
static const unsigned short yytoknum[] =
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
     375,   376,    44,   377,   124,    38,    45,    43,    42,    47,
     126,   378,   379,   380,    59,    40,    41,   123,   125,    91,
      93,    58,    61,    63,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,   145,   146,   147,   147,   147,   147,   147,   147,   147,
     148,   148,   148,   148,   148,   148,   149,   149,   149,   150,
     150,   150,   150,   150,   150,   150,   150,   150,   151,   152,
     153,   154,   155,   156,   157,   157,   158,   159,   159,   159,
     160,   160,   160,   160,   161,   161,   161,   162,   162,   162,
     163,   163,   164,   165,   165,   165,   166,   166,   166,   166,
     166,   166,   166,   166,   166,   166,   166,   166,   166,   166,
     166,   166,   166,   166,   166,   166,   166,   166,   166,   166,
     166,   166,   166,   166,   166,   166,   166,   166,   166,   166,
     166,   166,   166,   166,   166,   166,   166,   166,   166,   166,
     166,   166,   166,   166,   166,   166,   166,   166,   166,   166,
     166,   166,   166,   166,   166,   166,   166,   167,   167,   168,
     168,   169,   169,   170,   171,   171,   171,   172,   172,   173,
     173,   174,   175,   175,   176,   176,   176,   176,   176,   176,
     176,   176,   176,   176,   176,   176,   176,   176,   176,   176,
     176,   176,   177,   177,   178,   179,   180,   180,   181,   181,
     181,   181,   182,   183,   184,   184,   185,   185,   185,   186,
     186,   186,   186,   186,   186,   187,   187,   187,   187,   187,
     187,   187,   187,   187,   187,   187,   187,   188,   188,   189,
     189,   189,   189,   189,   189,   189,   190,   190,   191,   192,
     193,   193,   194,   195,   195,   196,   197,   197,   198,   198,
     199,   200,   200,   201,   201,   202,   203,   203,   203,   204,
     204,   205,   205,   206,   207,   208,   208,   209,   209,   209,
     210,   210,   211,   211,   212,   213,   213,   213,   213,   213,
     213,   213,   213,   213,   213,   214,   215,   215,   216,   216
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     0,     2,     2,     2,     2,     2,     2,
       0,     2,     2,     2,     2,     2,     0,     3,     2,     1,
       2,     1,     2,     2,     1,     2,     2,     2,     4,     3,
       3,     2,     3,     3,     0,     1,     1,     1,     3,     1,
       4,     3,     6,     5,     0,     3,     3,     1,     3,     4,
       0,     1,     3,     1,     3,     4,     1,     1,     4,     4,
       1,     1,     1,     1,     1,     4,     4,     4,     1,     4,
       4,     4,     1,     1,     4,     4,     4,     4,     4,     1,
       4,     1,     4,     5,     1,     1,     4,     1,     1,     1,
       1,     1,     1,     1,     4,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     4,     1,     1,     4,     4,     4,
       1,     4,     1,     1,     4,     4,     1,     0,     1,     0,
       2,     4,     3,     5,     0,     2,     1,     1,     3,     3,
       1,     5,     0,     1,     1,     1,     1,     5,     3,     3,
       3,     3,     3,     3,     3,     3,     2,     2,     2,     4,
       4,     3,     1,     3,     1,     4,     0,     2,     2,     3,
       2,     1,     4,     7,     0,     1,     0,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     2,     2,
       1,     1,     1,     1,     1,     1,     1,     0,     1,     1,
       2,     2,     2,     2,     1,     1,     2,     2,     2,     4,
       0,     2,     2,     2,     2,     2,     2,     3,     2,     3,
       5,     0,     2,     2,     2,     2,     5,     7,     1,     2,
       2,     2,     2,     2,     4,     2,     2,     1,     1,     3,
       1,     3,     1,     1,     5,     1,     1,     1,     2,     1,
       2,     1,     2,     1,     2,     4,     5,    10,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       3,     0,     2,     1,     0,     0,     0,   166,     0,     0,
       0,   166,    50,   166,    19,     0,     9,    21,    10,    24,
      10,     8,     0,     0,     0,     0,     0,     6,     0,     0,
     218,     0,   211,     5,     4,     0,     7,     0,     0,     0,
     236,   184,   175,   195,     0,   183,   166,   185,   181,   186,
     187,   189,   194,   187,   187,     0,   182,   187,   166,   166,
     180,   235,   176,   239,   237,   177,   241,     0,   243,     0,
     203,   204,   167,   168,     0,     0,     0,   213,   214,     0,
       0,    51,     0,    56,    57,     0,     0,    60,    61,    62,
      63,    64,     0,     0,    68,     0,     0,    72,    73,     0,
       0,     0,     0,     0,    79,     0,    81,     0,     0,    84,
      85,     0,    87,    88,    89,    90,    91,    92,    93,     0,
      95,    96,    97,    98,    99,   100,   101,   102,   103,     0,
     105,   106,     0,     0,     0,   110,     0,   112,   113,     0,
       0,     0,    53,   116,     0,     0,     0,     0,     0,     0,
     198,   205,   215,   223,    20,    22,    23,   200,   220,     0,
     219,     0,     0,    16,    25,    26,    27,   238,   240,   188,
     193,   192,   191,   178,   190,   242,   244,   179,   169,   170,
     171,   172,   173,   174,     0,     0,   124,     0,    29,   156,
       0,     0,   156,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    52,    30,    15,
       0,    13,    12,    11,    14,    33,   196,   197,    31,   221,
     222,    32,    50,     0,    50,     0,   212,    16,    50,     0,
      28,     0,   126,   127,   130,   155,    50,     0,     0,     0,
     227,   228,   230,   245,    50,    50,     0,   136,   134,   135,
       0,     0,     0,     0,     0,   154,     0,   152,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   232,   233,     0,     0,     0,     0,     0,
       0,   248,     0,     0,    54,     0,   199,     0,   201,   206,
       0,     0,     0,    50,     0,    50,   224,    18,     0,     0,
     123,   131,   125,     0,   161,   234,     0,    51,   157,     0,
     226,   225,     0,     0,     0,   246,    58,     0,   147,   148,
     146,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    59,    66,    65,    67,    69,    70,    71,
      74,    75,    76,    77,    78,    80,    82,     0,    86,    94,
     104,   107,   108,   109,   111,     0,   114,   115,    55,   202,
     208,     0,   207,   210,     0,    16,   216,   117,    17,   128,
     129,   243,   160,   158,   229,   231,   164,     0,   151,     0,
     144,   145,   138,   139,   141,   140,   142,   143,     0,   153,
      83,   249,    44,   209,    50,   118,     0,   159,     0,   165,
     150,   149,     0,   132,   162,   217,     0,   119,   137,     0,
       0,    47,   133,    34,     0,    46,     0,    45,   235,     0,
      39,    35,    37,     0,     0,     0,     0,   247,   120,    48,
       0,   163,     0,     0,    44,     0,    50,    49,    38,    44,
      34,    41,    50,   122,    34,    40,     0,   121,     0,    43,
      42
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,     1,     2,   144,   238,   307,    17,    18,    19,   149,
      20,    21,   429,   430,   431,   432,   414,   420,   308,    81,
     141,   142,   406,   424,   438,    23,   241,   242,   243,    63,
     421,   265,   266,   267,    25,   246,   318,   319,   309,   408,
      74,   250,    64,   170,    65,   150,    26,   221,   232,   298,
      28,    29,   234,   303,    30,   162,    31,    32,   222,   223,
     153,    35,   224,   251,   252,   253,   143,    66,   434,    38,
      68,   292
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -395
static const short yypact[] =
{
    -395,    59,   631,  -395,   603,   -65,   119,   134,    46,    88,
     136,   134,   -34,   134,  -395,   814,  -395,  -395,  -395,  -395,
    -395,  -395,    16,   -27,    10,    34,    35,  -395,    63,    38,
    -395,    68,    69,  -395,  -395,    76,  -395,    86,    93,    98,
    -395,  -395,  -395,  -395,   603,  -395,   176,  -395,  -395,  -395,
      62,  -395,  -395,    62,    62,    -5,  -395,    62,   185,   187,
      -5,  -395,  -395,  -395,  -395,  -395,  -395,    13,  -395,   127,
    -395,  -395,  -395,  -395,    87,   603,   102,  -395,  -395,   101,
     603,  -395,   -82,  -395,  -395,    15,   104,  -395,  -395,  -395,
    -395,  -395,   116,   120,  -395,   121,   123,  -395,  -395,   124,
     125,   128,   129,   130,  -395,   131,  -395,   132,   135,  -395,
    -395,   137,  -395,  -395,  -395,  -395,  -395,  -395,  -395,   138,
    -395,  -395,  -395,  -395,  -395,  -395,  -395,  -395,  -395,   139,
    -395,  -395,   140,   141,   142,  -395,   143,  -395,  -395,   144,
     145,   -79,  -395,  -395,   504,   620,   189,   251,   192,   146,
    -395,  -395,  -395,  -395,  -395,  -395,  -395,  -395,  -395,   195,
    -395,   196,   162,  -395,  -395,  -395,  -395,  -395,   164,  -395,
    -395,  -395,  -395,  -395,  -395,   164,   -70,  -395,  -395,  -395,
    -395,  -395,  -395,  -395,   158,   166,    13,    13,  -395,  -395,
     485,   173,  -395,    13,   470,   279,   264,   304,   458,   470,
     305,   308,   470,   309,   470,    13,   265,   485,   -55,   485,
     470,   603,   603,   312,   317,   603,   814,   184,  -395,  -395,
      20,  -395,  -395,  -395,  -395,  -395,  -395,  -395,  -395,  -395,
    -395,  -395,    66,   188,   -57,   191,  -395,  -395,   656,   470,
    -395,   194,   203,  -395,   200,  -395,   -25,   -22,   485,   485,
    -395,  -395,  -395,   208,   -34,   -19,   197,  -395,  -395,  -395,
     199,   470,   470,   470,   464,   239,   -85,  -395,   207,   212,
     214,   217,   219,   220,   221,   223,   224,   225,   226,   235,
     236,   341,   -83,  -395,  -395,   237,   -72,    58,   238,   240,
     241,   234,   244,   245,  -395,   814,  -395,     8,  -395,  -395,
     206,   603,   254,    92,   325,   686,  -395,  -395,   603,   256,
    -395,  -395,    13,   470,  -395,  -395,   603,   257,  -395,   259,
    -395,  -395,   262,   485,   263,  -395,  -395,   603,   249,   249,
     249,    90,   266,   470,   470,   470,   470,   470,   470,   470,
     470,   470,   470,  -395,  -395,  -395,  -395,  -395,  -395,  -395,
    -395,  -395,  -395,  -395,  -395,  -395,  -395,   267,  -395,  -395,
    -395,  -395,  -395,  -395,  -395,   389,  -395,  -395,  -395,  -395,
    -395,   485,  -395,  -395,   281,  -395,  -395,   313,  -395,  -395,
    -395,   285,  -395,  -395,  -395,  -395,    13,   284,  -395,   470,
     249,   249,   258,   167,   100,   100,    23,    23,   163,  -395,
    -395,  -395,   283,  -395,   707,  -395,   485,  -395,   286,  -395,
    -395,   249,   470,   516,  -395,  -395,   289,  -395,   239,    41,
     -59,  -395,   239,   299,    -7,  -395,   470,   287,   -33,   292,
    -395,   303,  -395,   603,   485,   470,   290,  -395,  -395,   239,
     470,  -395,   407,   485,   -23,   211,   -37,   239,  -395,   -11,
     299,  -395,   -37,  -395,   299,  -395,   296,  -395,   297,  -395,
    -395
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
    -395,  -395,  -395,   414,  -211,     7,  -395,  -395,   147,  -395,
    -395,  -395,  -325,  -395,  -395,    -6,  -370,  -395,    -9,    -2,
    -395,  -202,  -395,  -395,  -395,  -395,  -395,  -395,   133,     4,
    -395,  -178,  -395,  -177,  -395,   250,  -394,  -219,   152,  -395,
      17,   -60,  -395,    64,    44,  -395,  -395,   439,  -395,  -395,
     -17,  -395,  -395,  -395,  -395,  -395,   -14,  -395,   444,     0,
    -395,  -395,   445,   209,  -236,  -167,   252,     9,    -3,  -395,
       2,  -395
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -168
static const short yytable[] =
{
      22,    67,    34,    80,    39,   151,    24,   184,   152,    16,
     247,    37,   321,   322,   294,   302,   178,   179,   269,   435,
     300,   273,   274,    43,   191,   277,   305,   279,    79,   180,
      82,   436,   287,   333,   334,   324,  -167,   342,   283,   323,
     282,   167,   286,   216,   257,   146,   258,   259,     6,   146,
     323,   343,   453,   358,    50,   192,     6,   284,   457,     3,
       6,   217,   310,   426,   360,    51,    52,  -167,   333,   334,
      69,    53,   187,   181,   451,    79,    82,   190,    75,   455,
      10,   427,    15,   328,   329,   330,   331,   385,    10,   -36,
     147,    54,    10,   368,   148,    76,    57,   314,   148,   173,
     333,   334,    15,   -36,   177,    15,   248,   154,   182,   314,
     333,   334,   450,   315,    15,   314,   413,   171,   172,   325,
      15,   174,    70,    71,   454,   456,   244,   245,   413,   458,
     183,   437,   169,   256,   185,   402,   380,    72,    73,    77,
      78,   260,   220,   220,   155,   280,    39,    39,    24,    24,
     193,   219,   219,    37,    37,   390,   391,   392,   393,   394,
     395,   396,   397,   398,   404,   399,   341,   261,   156,   262,
     416,   263,   157,   333,   334,   159,   264,   333,   334,   168,
      73,   425,   335,   336,   337,   338,   339,   340,   175,    73,
     176,    73,   226,   227,   361,   229,   230,   158,   444,   235,
     236,   341,   160,   151,   296,    15,   152,   449,   288,   289,
     161,   411,   293,   163,   335,   336,   337,   338,   339,   340,
     164,   333,   334,   297,   186,   301,   388,   165,   339,   340,
     373,    15,   166,   341,   418,   422,   188,   316,   189,   194,
      39,   329,    24,   341,   317,   301,   316,    37,   439,   333,
     334,   195,   244,   317,   228,   196,   197,   445,   198,   199,
     200,   332,   447,   201,   202,   203,   204,   205,   333,   334,
     206,   270,   207,   208,   209,   210,   211,   212,   213,   214,
     215,   233,   257,   231,   258,   259,   268,   335,   336,   337,
     338,   339,   340,   337,   338,   339,   340,   369,   371,   237,
     239,  -167,   240,    40,   412,   377,   341,    39,   254,    24,
     341,   271,   275,   371,    37,   276,   278,   281,   381,    41,
     290,    42,   291,   295,   387,   312,   409,    43,   304,   299,
     323,    44,   311,   326,   327,   335,   336,   337,   338,   339,
     340,    45,   313,   344,   357,    46,    47,   370,   345,    48,
     346,    49,   452,   347,   341,   348,   349,   350,    50,   351,
     352,   353,   354,   335,   336,   337,   338,   339,   340,    51,
      52,   355,   356,   359,   362,    53,   363,   364,   365,   260,
     366,   367,   341,   336,   337,   338,   339,   340,   372,     9,
     378,   382,   341,   383,   401,    54,    55,    56,   384,   386,
      57,   341,   389,   400,    58,   261,    39,   262,    24,   263,
      59,    40,    60,    37,   264,   403,   405,   428,    62,   407,
     410,   433,   413,   417,   423,   442,   440,    41,   441,    42,
     443,   446,   459,   460,   145,    43,   448,   316,    15,    44,
     433,    27,   255,   316,   317,   379,    33,    36,   433,    45,
     317,   375,   433,    46,    47,   374,   320,    48,     0,    49,
     285,   257,     0,   258,   259,   272,    50,   257,    40,   258,
     259,     0,     0,   257,     0,   258,   259,    51,    52,     0,
       0,     0,     0,    53,    41,     0,    42,     0,   178,   179,
       0,     0,    43,     0,     0,     0,    44,     0,     0,     0,
       0,   180,     0,    54,    55,    56,    45,     0,    57,     0,
      46,    47,    58,   218,    48,     0,    49,   247,    59,   257,
      60,   258,   259,    50,     0,    61,    62,     0,     0,     0,
       0,     0,     0,     0,    51,    52,     4,     0,     0,     0,
      53,     5,     0,     0,     6,   181,    15,     0,     0,     0,
       7,     0,     0,     8,     0,     0,     0,     0,   260,     0,
      54,    55,    56,     0,   260,    57,     0,     0,     9,    58,
     260,     0,     0,     0,     0,    59,    10,    60,     0,     0,
     182,     0,    61,    62,   261,     0,   262,     0,   263,     0,
     261,     0,   262,   264,   263,     0,   261,     0,   262,   264,
     263,     0,   183,     0,     0,   264,     0,    40,     0,    11,
       0,     0,     0,   248,    12,    13,   260,     0,     0,     0,
     249,     0,     0,    41,     0,    42,     0,     0,     0,     0,
       0,    43,     0,     0,     0,    44,     0,     0,    14,     0,
       0,     0,   261,    15,   419,    45,   263,     0,     0,    46,
      47,   264,     4,    48,     0,    49,     0,     5,     0,     0,
       6,     0,    50,     4,     0,     0,     7,     0,     5,     8,
       0,     6,     0,    51,    52,     0,     0,     7,     0,    53,
       8,     0,     0,     0,     9,     0,     0,     0,     4,     0,
       0,     0,    10,     5,     0,     9,     0,     0,     0,    54,
      55,    56,     7,    10,    57,     8,     0,     0,    58,     0,
       0,     0,     0,     0,    59,     0,    60,     0,     4,     0,
       9,    61,    62,     5,     0,    11,     0,     0,     0,     0,
      12,    13,     7,     0,     0,     8,    11,     0,     0,     4,
       0,    12,    13,     0,     5,     0,     0,     0,     0,     0,
       9,     0,     0,     7,    14,     0,     8,     0,   225,    15,
       0,    11,     0,     0,     0,    14,    12,    13,     0,     0,
      15,     9,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      14,    11,     0,     0,   306,    15,    12,    13,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    11,     0,     0,     0,     0,    12,    13,     0,
      14,     0,     0,     0,   376,    15,     0,     0,     0,     0,
      83,     0,    84,     0,     0,     0,     0,     0,    85,     0,
      86,    14,     0,     0,     0,   415,    15,    87,    88,    89,
      90,     0,    91,    92,     0,    93,     0,    94,    95,    96,
       0,     0,    97,     0,     0,    98,     0,    99,   100,   101,
     102,   103,   104,     0,   105,   106,   107,   108,     0,     0,
     109,     0,     0,   110,     0,     0,     0,   111,     0,   112,
       0,     0,     0,   113,   114,   115,   116,   117,   118,   119,
       0,   120,   121,   122,   123,   124,   125,   126,   127,   128,
       0,     0,     0,   129,     0,     0,   130,     0,   131,     0,
       0,   132,   133,   134,     0,     0,   135,     0,   136,   137,
     138,   139,     0,     0,   140
};

static const short yycheck[] =
{
       2,     4,     2,    12,     2,    22,     2,    67,    22,     2,
      32,     2,   248,   249,   216,   234,     3,     4,   195,    26,
      77,   198,   199,    28,   106,   202,   237,   204,    11,    16,
      13,    38,   210,    10,    11,   254,   106,   122,    93,   122,
     207,    44,   209,   122,     3,    29,     5,     6,    40,    29,
     122,   136,   446,   136,    59,   137,    40,   112,   452,     0,
      40,   140,   239,   122,   136,    70,    71,   137,    10,    11,
     135,    76,    75,    60,   444,    58,    59,    80,    32,   449,
      72,   140,   139,   261,   262,   263,   264,   323,    72,   122,
      74,    96,    72,   295,    78,     7,   101,   134,    78,    55,
      10,    11,   139,   136,    60,   139,   128,   134,    95,   134,
      10,    11,   135,   138,   139,   134,   139,    53,    54,   138,
     139,    57,     3,     4,   135,   450,   186,   187,   139,   454,
     117,   138,    70,   193,     7,   371,   313,     3,     4,     3,
       4,   100,   144,   145,   134,   205,   144,   145,   144,   145,
     135,   144,   145,   144,   145,   333,   334,   335,   336,   337,
     338,   339,   340,   341,   375,   342,   143,   126,   134,   128,
     406,   130,   137,    10,    11,   137,   135,    10,    11,     3,
       4,   140,   124,   125,   126,   127,   128,   129,     3,     4,
       3,     4,     3,     4,   136,     3,     4,   134,   434,     3,
       4,   143,   134,   220,   138,   139,   220,   443,   211,   212,
     141,   389,   215,   137,   124,   125,   126,   127,   128,   129,
     134,    10,    11,   232,   137,   234,   136,   134,   128,   129,
     138,   139,   134,   143,   412,   413,   134,   246,   137,   135,
     238,   419,   238,   143,   246,   254,   255,   238,   426,    10,
      11,   135,   312,   255,     3,   135,   135,   435,   135,   135,
     135,   264,   440,   135,   135,   135,   135,   135,    10,    11,
     135,     7,   135,   135,   135,   135,   135,   135,   135,   135,
     135,    86,     3,   137,     5,     6,     7,   124,   125,   126,
     127,   128,   129,   126,   127,   128,   129,   297,   301,   137,
     142,   137,   136,     4,   141,   308,   143,   305,   135,   305,
     143,     7,     7,   316,   305,     7,     7,    52,   316,    20,
       8,    22,     5,   139,   327,   122,   386,    28,   137,   141,
     122,    32,   138,   136,   135,   124,   125,   126,   127,   128,
     129,    42,   142,   136,     3,    46,    47,   141,   136,    50,
     136,    52,   141,   136,   143,   136,   136,   136,    59,   136,
     136,   136,   136,   124,   125,   126,   127,   128,   129,    70,
      71,   136,   136,   136,   136,    76,   136,   136,   144,   100,
     136,   136,   143,   125,   126,   127,   128,   129,   134,    64,
     134,   134,   143,   134,     5,    96,    97,    98,   136,   136,
     101,   143,   136,   136,   105,   126,   404,   128,   404,   130,
     111,     4,   113,   404,   135,   134,   103,   118,   119,   134,
     136,   423,   139,   137,   135,   122,   139,    20,   136,    22,
     433,   141,   136,   136,    20,    28,   442,   446,   139,    32,
     442,     2,   192,   452,   446,   312,     2,     2,   450,    42,
     452,   304,   454,    46,    47,   303,   247,    50,    -1,    52,
     208,     3,    -1,     5,     6,     7,    59,     3,     4,     5,
       6,    -1,    -1,     3,    -1,     5,     6,    70,    71,    -1,
      -1,    -1,    -1,    76,    20,    -1,    22,    -1,     3,     4,
      -1,    -1,    28,    -1,    -1,    -1,    32,    -1,    -1,    -1,
      -1,    16,    -1,    96,    97,    98,    42,    -1,   101,    -1,
      46,    47,   105,     9,    50,    -1,    52,    32,   111,     3,
     113,     5,     6,    59,    -1,   118,   119,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    70,    71,    32,    -1,    -1,    -1,
      76,    37,    -1,    -1,    40,    60,   139,    -1,    -1,    -1,
      46,    -1,    -1,    49,    -1,    -1,    -1,    -1,   100,    -1,
      96,    97,    98,    -1,   100,   101,    -1,    -1,    64,   105,
     100,    -1,    -1,    -1,    -1,   111,    72,   113,    -1,    -1,
      95,    -1,   118,   119,   126,    -1,   128,    -1,   130,    -1,
     126,    -1,   128,   135,   130,    -1,   126,    -1,   128,   135,
     130,    -1,   117,    -1,    -1,   135,    -1,     4,    -1,   105,
      -1,    -1,    -1,   128,   110,   111,   100,    -1,    -1,    -1,
     135,    -1,    -1,    20,    -1,    22,    -1,    -1,    -1,    -1,
      -1,    28,    -1,    -1,    -1,    32,    -1,    -1,   134,    -1,
      -1,    -1,   126,   139,   128,    42,   130,    -1,    -1,    46,
      47,   135,    32,    50,    -1,    52,    -1,    37,    -1,    -1,
      40,    -1,    59,    32,    -1,    -1,    46,    -1,    37,    49,
      -1,    40,    -1,    70,    71,    -1,    -1,    46,    -1,    76,
      49,    -1,    -1,    -1,    64,    -1,    -1,    -1,    32,    -1,
      -1,    -1,    72,    37,    -1,    64,    -1,    -1,    -1,    96,
      97,    98,    46,    72,   101,    49,    -1,    -1,   105,    -1,
      -1,    -1,    -1,    -1,   111,    -1,   113,    -1,    32,    -1,
      64,   118,   119,    37,    -1,   105,    -1,    -1,    -1,    -1,
     110,   111,    46,    -1,    -1,    49,   105,    -1,    -1,    32,
      -1,   110,   111,    -1,    37,    -1,    -1,    -1,    -1,    -1,
      64,    -1,    -1,    46,   134,    -1,    49,    -1,   138,   139,
      -1,   105,    -1,    -1,    -1,   134,   110,   111,    -1,    -1,
     139,    64,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     134,   105,    -1,    -1,   138,   139,   110,   111,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   105,    -1,    -1,    -1,    -1,   110,   111,    -1,
     134,    -1,    -1,    -1,   138,   139,    -1,    -1,    -1,    -1,
      16,    -1,    18,    -1,    -1,    -1,    -1,    -1,    24,    -1,
      26,   134,    -1,    -1,    -1,   138,   139,    33,    34,    35,
      36,    -1,    38,    39,    -1,    41,    -1,    43,    44,    45,
      -1,    -1,    48,    -1,    -1,    51,    -1,    53,    54,    55,
      56,    57,    58,    -1,    60,    61,    62,    63,    -1,    -1,
      66,    -1,    -1,    69,    -1,    -1,    -1,    73,    -1,    75,
      -1,    -1,    -1,    79,    80,    81,    82,    83,    84,    85,
      -1,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      -1,    -1,    -1,    99,    -1,    -1,   102,    -1,   104,    -1,
      -1,   107,   108,   109,    -1,    -1,   112,    -1,   114,   115,
     116,   117,    -1,    -1,   120
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,   146,   147,     0,    32,    37,    40,    46,    49,    64,
      72,   105,   110,   111,   134,   139,   150,   151,   152,   153,
     155,   156,   164,   170,   174,   179,   191,   192,   195,   196,
     199,   201,   202,   203,   204,   206,   207,   212,   214,   215,
       4,    20,    22,    28,    32,    42,    46,    47,    50,    52,
      59,    70,    71,    76,    96,    97,    98,   101,   105,   111,
     113,   118,   119,   174,   187,   189,   212,   213,   215,   135,
       3,     4,     3,     4,   185,    32,     7,     3,     4,   185,
     163,   164,   185,    16,    18,    24,    26,    33,    34,    35,
      36,    38,    39,    41,    43,    44,    45,    48,    51,    53,
      54,    55,    56,    57,    58,    60,    61,    62,    63,    66,
      69,    73,    75,    79,    80,    81,    82,    83,    84,    85,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    99,
     102,   104,   107,   108,   109,   112,   114,   115,   116,   117,
     120,   165,   166,   211,   148,   148,    29,    74,    78,   154,
     190,   195,   201,   205,   134,   134,   134,   137,   134,   137,
     134,   141,   200,   137,   134,   134,   134,   213,     3,    70,
     188,   188,   188,   189,   188,     3,     3,   189,     3,     4,
      16,    60,    95,   117,   186,     7,   137,   213,   134,   137,
     213,   106,   137,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   135,   135,   122,   140,     9,   150,
     164,   192,   203,   204,   207,   138,     3,     4,     3,     3,
       4,   137,   193,    86,   197,     3,     4,   137,   149,   142,
     136,   171,   172,   173,   186,   186,   180,    32,   128,   135,
     186,   208,   209,   210,   135,   180,   186,     3,     5,     6,
     100,   126,   128,   130,   135,   176,   177,   178,     7,   178,
       7,     7,     7,   178,   178,     7,     7,   178,     7,   178,
     186,    52,   210,    93,   112,   211,   210,   176,   213,   213,
       8,     5,   216,   213,   166,   139,   138,   163,   194,   141,
      77,   163,   182,   198,   137,   149,   138,   150,   163,   183,
     178,   138,   122,   142,   134,   138,   163,   164,   181,   182,
     208,   209,   209,   122,   182,   138,   136,   135,   176,   176,
     176,   176,   213,    10,    11,   124,   125,   126,   127,   128,
     129,   143,   122,   136,   136,   136,   136,   136,   136,   136,
     136,   136,   136,   136,   136,   136,   136,     3,   136,   136,
     136,   136,   136,   136,   136,   144,   136,   136,   166,   204,
     141,   213,   134,   138,   183,   153,   138,   213,   134,   173,
     178,   215,   134,   134,   136,   209,   136,   213,   136,   136,
     176,   176,   176,   176,   176,   176,   176,   176,   176,   178,
     136,     5,   209,   134,   149,   103,   167,   134,   184,   186,
     136,   176,   141,   139,   161,   138,   209,   137,   176,   128,
     162,   175,   176,   135,   168,   140,   122,   140,   118,   157,
     158,   159,   160,   164,   213,    26,    38,   138,   169,   176,
     139,   136,   122,   213,   209,   176,   141,   176,   160,   209,
     135,   161,   141,   181,   135,   161,   157,   181,   157,   136,
     136
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrlab1

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
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
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

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
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
  unsigned int yylineno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylineno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
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
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
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
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

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
  register const char *yys = yystr;

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
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



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
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

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
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

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



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
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

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



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
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
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
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
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
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
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

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

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
#line 228 "parser.y"
    { write_proxies(yyvsp[0].ifref); write_client(yyvsp[0].ifref); write_server(yyvsp[0].ifref); ;}
    break;

  case 3:
#line 231 "parser.y"
    { yyval.ifref = NULL; ;}
    break;

  case 4:
#line 232 "parser.y"
    { yyval.ifref = yyvsp[-1].ifref; ;}
    break;

  case 5:
#line 233 "parser.y"
    { yyval.ifref = make_ifref(yyvsp[0].type); LINK(yyval.ifref, yyvsp[-1].ifref); ;}
    break;

  case 6:
#line 234 "parser.y"
    { yyval.ifref = yyvsp[-1].ifref; add_coclass(yyvsp[0].clas); ;}
    break;

  case 7:
#line 235 "parser.y"
    { yyval.ifref = yyvsp[-1].ifref; add_module(yyvsp[0].type); ;}
    break;

  case 8:
#line 236 "parser.y"
    { yyval.ifref = yyvsp[-1].ifref; ;}
    break;

  case 9:
#line 237 "parser.y"
    { yyval.ifref = yyvsp[-1].ifref; ;}
    break;

  case 10:
#line 240 "parser.y"
    {;}
    break;

  case 11:
#line 241 "parser.y"
    { if (!parse_only) add_interface(yyvsp[0].type); ;}
    break;

  case 12:
#line 242 "parser.y"
    { if (!parse_only) add_interface(yyvsp[0].type); ;}
    break;

  case 13:
#line 243 "parser.y"
    { if (!parse_only) add_coclass(yyvsp[0].clas); ;}
    break;

  case 14:
#line 244 "parser.y"
    { if (!parse_only) add_module(yyvsp[0].type); ;}
    break;

  case 15:
#line 245 "parser.y"
    {;}
    break;

  case 16:
#line 248 "parser.y"
    { yyval.func = NULL; ;}
    break;

  case 17:
#line 249 "parser.y"
    { yyval.func = yyvsp[-1].func; LINK(yyval.func, yyvsp[-2].func); ;}
    break;

  case 18:
#line 250 "parser.y"
    { yyval.func = yyvsp[-1].func; ;}
    break;

  case 19:
#line 253 "parser.y"
    {;}
    break;

  case 20:
#line 254 "parser.y"
    { if (!parse_only && do_header) { write_constdef(yyvsp[-1].var); } ;}
    break;

  case 21:
#line 255 "parser.y"
    {;}
    break;

  case 22:
#line 256 "parser.y"
    { if (!parse_only && do_header) { write_type(header, yyvsp[-1].type, NULL, NULL); fprintf(header, ";\n\n"); } ;}
    break;

  case 23:
#line 257 "parser.y"
    { if (!parse_only && do_header) { write_externdef(yyvsp[-1].var); } ;}
    break;

  case 24:
#line 258 "parser.y"
    {;}
    break;

  case 25:
#line 259 "parser.y"
    { if (!parse_only && do_header) { write_type(header, yyvsp[-1].type, NULL, NULL); fprintf(header, ";\n\n"); } ;}
    break;

  case 26:
#line 260 "parser.y"
    {;}
    break;

  case 27:
#line 261 "parser.y"
    { if (!parse_only && do_header) { write_type(header, yyvsp[-1].type, NULL, NULL); fprintf(header, ";\n\n"); } ;}
    break;

  case 28:
#line 264 "parser.y"
    { if (!parse_only && do_header) fprintf(header, "%s\n", yyvsp[-1].str); ;}
    break;

  case 29:
#line 266 "parser.y"
    { assert(yychar == YYEMPTY);
						  if (!do_import(yyvsp[-1].str)) yychar = aEOF; ;}
    break;

  case 30:
#line 269 "parser.y"
    {;}
    break;

  case 31:
#line 272 "parser.y"
    { yyval.str = yyvsp[0].str; ;}
    break;

  case 32:
#line 274 "parser.y"
    { start_typelib(yyvsp[-1].str, yyvsp[-2].attr); ;}
    break;

  case 33:
#line 276 "parser.y"
    { end_typelib(); ;}
    break;

  case 34:
#line 279 "parser.y"
    { yyval.var = NULL; ;}
    break;

  case 36:
#line 283 "parser.y"
    { yyval.var = NULL; ;}
    break;

  case 38:
#line 287 "parser.y"
    { LINK(yyvsp[0].var, yyvsp[-2].var); yyval.var = yyvsp[0].var; ;}
    break;

  case 40:
#line 292 "parser.y"
    { yyval.var = yyvsp[-1].var;
						  set_type(yyval.var, yyvsp[-2].tref, yyvsp[0].expr);
						  yyval.var->attrs = yyvsp[-3].attr;
						;}
    break;

  case 41:
#line 296 "parser.y"
    { yyval.var = yyvsp[-1].var;
						  set_type(yyval.var, yyvsp[-2].tref, yyvsp[0].expr);
						;}
    break;

  case 42:
#line 299 "parser.y"
    { yyval.var = yyvsp[-3].var;
						  yyval.var->ptr_level--;
						  set_type(yyval.var, yyvsp[-4].tref, NULL);
						  yyval.var->attrs = yyvsp[-5].attr;
						  yyval.var->args = yyvsp[-1].var;
						;}
    break;

  case 43:
#line 305 "parser.y"
    { yyval.var = yyvsp[-3].var;
						  yyval.var->ptr_level--;
						  set_type(yyval.var, yyvsp[-4].tref, NULL);
						  yyval.var->args = yyvsp[-1].var;
						;}
    break;

  case 44:
#line 312 "parser.y"
    { yyval.expr = NULL; ;}
    break;

  case 45:
#line 313 "parser.y"
    { yyval.expr = yyvsp[-1].expr; ;}
    break;

  case 46:
#line 314 "parser.y"
    { yyval.expr = make_expr(EXPR_VOID); ;}
    break;

  case 48:
#line 318 "parser.y"
    { LINK(yyvsp[0].expr, yyvsp[-2].expr); yyval.expr = yyvsp[0].expr; ;}
    break;

  case 49:
#line 319 "parser.y"
    { LINK(yyvsp[0].expr, yyvsp[-3].expr); yyval.expr = yyvsp[0].expr; ;}
    break;

  case 50:
#line 322 "parser.y"
    { yyval.attr = NULL; ;}
    break;

  case 52:
#line 327 "parser.y"
    { yyval.attr = yyvsp[-1].attr; ;}
    break;

  case 54:
#line 331 "parser.y"
    { LINK(yyvsp[0].attr, yyvsp[-2].attr); yyval.attr = yyvsp[0].attr; ;}
    break;

  case 55:
#line 332 "parser.y"
    { LINK(yyvsp[0].attr, yyvsp[-3].attr); yyval.attr = yyvsp[0].attr; ;}
    break;

  case 56:
#line 336 "parser.y"
    { yyval.attr = make_attr(ATTR_ASYNC); ;}
    break;

  case 57:
#line 337 "parser.y"
    { yyval.attr = make_attr(ATTR_AUTO_HANDLE); ;}
    break;

  case 58:
#line 338 "parser.y"
    { yyval.attr = make_attrp(ATTR_CALLAS, yyvsp[-1].var); ;}
    break;

  case 59:
#line 339 "parser.y"
    { yyval.attr = make_attrp(ATTR_CASE, yyvsp[-1].expr); ;}
    break;

  case 60:
#line 340 "parser.y"
    { yyval.attr = make_attrv(ATTR_CONTEXTHANDLE, 0); ;}
    break;

  case 61:
#line 341 "parser.y"
    { yyval.attr = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_DONT_SERIALIZE */ ;}
    break;

  case 62:
#line 342 "parser.y"
    { yyval.attr = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_SERIALIZE */ ;}
    break;

  case 63:
#line 343 "parser.y"
    { yyval.attr = make_attr(ATTR_CONTROL); ;}
    break;

  case 64:
#line 344 "parser.y"
    { yyval.attr = make_attr(ATTR_DEFAULT); ;}
    break;

  case 65:
#line 345 "parser.y"
    { yyval.attr = make_attrp(ATTR_DEFAULTVALUE_EXPR, yyvsp[-1].expr); ;}
    break;

  case 66:
#line 346 "parser.y"
    { yyval.attr = make_attrp(ATTR_DEFAULTVALUE_STRING, yyvsp[-1].str); ;}
    break;

  case 67:
#line 347 "parser.y"
    { yyval.attr = make_attrp(ATTR_DLLNAME, yyvsp[-1].str); ;}
    break;

  case 68:
#line 348 "parser.y"
    { yyval.attr = make_attr(ATTR_DUAL); ;}
    break;

  case 69:
#line 349 "parser.y"
    { yyval.attr = make_attrp(ATTR_ENDPOINT, yyvsp[-1].str); ;}
    break;

  case 70:
#line 350 "parser.y"
    { yyval.attr = make_attrp(ATTR_ENTRY_STRING, yyvsp[-1].str); ;}
    break;

  case 71:
#line 351 "parser.y"
    { yyval.attr = make_attrp(ATTR_ENTRY_ORDINAL, yyvsp[-1].expr); ;}
    break;

  case 72:
#line 352 "parser.y"
    { yyval.attr = make_attr(ATTR_EXPLICIT_HANDLE); ;}
    break;

  case 73:
#line 353 "parser.y"
    { yyval.attr = make_attr(ATTR_HANDLE); ;}
    break;

  case 74:
#line 354 "parser.y"
    { yyval.attr = make_attrp(ATTR_HELPCONTEXT, yyvsp[-1].expr); ;}
    break;

  case 75:
#line 355 "parser.y"
    { yyval.attr = make_attrp(ATTR_HELPFILE, yyvsp[-1].str); ;}
    break;

  case 76:
#line 356 "parser.y"
    { yyval.attr = make_attrp(ATTR_HELPSTRING, yyvsp[-1].str); ;}
    break;

  case 77:
#line 357 "parser.y"
    { yyval.attr = make_attrp(ATTR_HELPSTRINGCONTEXT, yyvsp[-1].expr); ;}
    break;

  case 78:
#line 358 "parser.y"
    { yyval.attr = make_attrp(ATTR_HELPSTRINGDLL, yyvsp[-1].str); ;}
    break;

  case 79:
#line 359 "parser.y"
    { yyval.attr = make_attr(ATTR_HIDDEN); ;}
    break;

  case 80:
#line 360 "parser.y"
    { yyval.attr = make_attrp(ATTR_ID, yyvsp[-1].expr); ;}
    break;

  case 81:
#line 361 "parser.y"
    { yyval.attr = make_attr(ATTR_IDEMPOTENT); ;}
    break;

  case 82:
#line 362 "parser.y"
    { yyval.attr = make_attrp(ATTR_IIDIS, yyvsp[-1].var); ;}
    break;

  case 83:
#line 363 "parser.y"
    { yyval.attr = make_attrp(ATTR_IMPLICIT_HANDLE, yyvsp[-1].str); ;}
    break;

  case 84:
#line 364 "parser.y"
    { yyval.attr = make_attr(ATTR_IN); ;}
    break;

  case 85:
#line 365 "parser.y"
    { yyval.attr = make_attr(ATTR_INPUTSYNC); ;}
    break;

  case 86:
#line 366 "parser.y"
    { yyval.attr = make_attrp(ATTR_LENGTHIS, yyvsp[-1].var); ;}
    break;

  case 87:
#line 367 "parser.y"
    { yyval.attr = make_attr(ATTR_LOCAL); ;}
    break;

  case 88:
#line 368 "parser.y"
    { yyval.attr = make_attr(ATTR_NONCREATABLE); ;}
    break;

  case 89:
#line 369 "parser.y"
    { yyval.attr = make_attr(ATTR_OBJECT); ;}
    break;

  case 90:
#line 370 "parser.y"
    { yyval.attr = make_attr(ATTR_ODL); ;}
    break;

  case 91:
#line 371 "parser.y"
    { yyval.attr = make_attr(ATTR_OLEAUTOMATION); ;}
    break;

  case 92:
#line 372 "parser.y"
    { yyval.attr = make_attr(ATTR_OPTIONAL); ;}
    break;

  case 93:
#line 373 "parser.y"
    { yyval.attr = make_attr(ATTR_OUT); ;}
    break;

  case 94:
#line 374 "parser.y"
    { yyval.attr = make_attrv(ATTR_POINTERDEFAULT, yyvsp[-1].num); ;}
    break;

  case 95:
#line 375 "parser.y"
    { yyval.attr = make_attr(ATTR_PROPGET); ;}
    break;

  case 96:
#line 376 "parser.y"
    { yyval.attr = make_attr(ATTR_PROPPUT); ;}
    break;

  case 97:
#line 377 "parser.y"
    { yyval.attr = make_attr(ATTR_PROPPUTREF); ;}
    break;

  case 98:
#line 378 "parser.y"
    { yyval.attr = make_attr(ATTR_PTR); ;}
    break;

  case 99:
#line 379 "parser.y"
    { yyval.attr = make_attr(ATTR_PUBLIC); ;}
    break;

  case 100:
#line 380 "parser.y"
    { yyval.attr = make_attr(ATTR_READONLY); ;}
    break;

  case 101:
#line 381 "parser.y"
    { yyval.attr = make_attr(ATTR_REF); ;}
    break;

  case 102:
#line 382 "parser.y"
    { yyval.attr = make_attr(ATTR_RESTRICTED); ;}
    break;

  case 103:
#line 383 "parser.y"
    { yyval.attr = make_attr(ATTR_RETVAL); ;}
    break;

  case 104:
#line 384 "parser.y"
    { yyval.attr = make_attrp(ATTR_SIZEIS, yyvsp[-1].var); ;}
    break;

  case 105:
#line 385 "parser.y"
    { yyval.attr = make_attr(ATTR_SOURCE); ;}
    break;

  case 106:
#line 386 "parser.y"
    { yyval.attr = make_attr(ATTR_STRING); ;}
    break;

  case 107:
#line 387 "parser.y"
    { yyval.attr = make_attrp(ATTR_SWITCHIS, yyvsp[-1].expr); ;}
    break;

  case 108:
#line 388 "parser.y"
    { yyval.attr = make_attrp(ATTR_SWITCHTYPE, type_ref(yyvsp[-1].tref)); ;}
    break;

  case 109:
#line 389 "parser.y"
    { yyval.attr = make_attrp(ATTR_TRANSMITAS, type_ref(yyvsp[-1].tref)); ;}
    break;

  case 110:
#line 390 "parser.y"
    { yyval.attr = make_attr(ATTR_UNIQUE); ;}
    break;

  case 111:
#line 391 "parser.y"
    { yyval.attr = make_attrp(ATTR_UUID, yyvsp[-1].uuid); ;}
    break;

  case 112:
#line 392 "parser.y"
    { yyval.attr = make_attr(ATTR_V1ENUM); ;}
    break;

  case 113:
#line 393 "parser.y"
    { yyval.attr = make_attr(ATTR_VARARG); ;}
    break;

  case 114:
#line 394 "parser.y"
    { yyval.attr = make_attrv(ATTR_VERSION, yyvsp[-1].num); ;}
    break;

  case 115:
#line 395 "parser.y"
    { yyval.attr = make_attrp(ATTR_WIREMARSHAL, type_ref(yyvsp[-1].tref)); ;}
    break;

  case 116:
#line 396 "parser.y"
    { yyval.attr = make_attrv(ATTR_POINTERTYPE, yyvsp[0].num); ;}
    break;

  case 119:
#line 403 "parser.y"
    { yyval.var = NULL; ;}
    break;

  case 120:
#line 404 "parser.y"
    { if (yyvsp[0].var) { LINK(yyvsp[0].var, yyvsp[-1].var); yyval.var = yyvsp[0].var; }
						  else { yyval.var = yyvsp[-1].var; }
						;}
    break;

  case 121:
#line 409 "parser.y"
    { attr_t *a = make_attrp(ATTR_CASE, yyvsp[-2].expr);
						  yyval.var = yyvsp[0].var; if (!yyval.var) yyval.var = make_var(NULL);
						  LINK(a, yyval.var->attrs); yyval.var->attrs = a;
						;}
    break;

  case 122:
#line 413 "parser.y"
    { attr_t *a = make_attr(ATTR_DEFAULT);
						  yyval.var = yyvsp[0].var; if (!yyval.var) yyval.var = make_var(NULL);
						  LINK(a, yyval.var->attrs); yyval.var->attrs = a;
						;}
    break;

  case 123:
#line 419 "parser.y"
    { yyval.var = reg_const(yyvsp[-2].var);
						  set_type(yyval.var, yyvsp[-3].tref, NULL);
						  yyval.var->eval = yyvsp[0].expr;
						  yyval.var->lval = yyvsp[0].expr->cval;
						;}
    break;

  case 124:
#line 426 "parser.y"
    { yyval.var = NULL; ;}
    break;

  case 125:
#line 427 "parser.y"
    { yyval.var = yyvsp[-1].var; ;}
    break;

  case 128:
#line 432 "parser.y"
    { LINK(yyvsp[0].var, yyvsp[-2].var); yyval.var = yyvsp[0].var;
						  if (yyvsp[-2].var && !yyvsp[0].var->eval)
						    yyvsp[0].var->lval = yyvsp[-2].var->lval + 1;
						;}
    break;

  case 129:
#line 438 "parser.y"
    { yyval.var = reg_const(yyvsp[-2].var);
						  yyval.var->eval = yyvsp[0].expr;
						  yyval.var->lval = yyvsp[0].expr->cval;
						  yyval.var->type = make_type(RPC_FC_LONG, &std_int);
						;}
    break;

  case 130:
#line 443 "parser.y"
    { yyval.var = reg_const(yyvsp[0].var);
						  yyval.var->lval = 0; /* default for first enum entry */
						  yyval.var->type = make_type(RPC_FC_LONG, &std_int);
						;}
    break;

  case 131:
#line 449 "parser.y"
    { yyval.type = get_typev(RPC_FC_ENUM16, yyvsp[-3].var, tsENUM);
						  yyval.type->fields = yyvsp[-1].var;
						  yyval.type->defined = TRUE;
						  if(in_typelib)
						      add_enum(yyval.type);
						;}
    break;

  case 132:
#line 471 "parser.y"
    { yyval.expr = make_expr(EXPR_VOID); ;}
    break;

  case 134:
#line 475 "parser.y"
    { yyval.expr = make_exprl(EXPR_NUM, yyvsp[0].num); ;}
    break;

  case 135:
#line 476 "parser.y"
    { yyval.expr = make_exprl(EXPR_HEXNUM, yyvsp[0].num); ;}
    break;

  case 136:
#line 477 "parser.y"
    { yyval.expr = make_exprs(EXPR_IDENTIFIER, yyvsp[0].str); ;}
    break;

  case 137:
#line 478 "parser.y"
    { yyval.expr = make_expr3(EXPR_COND, yyvsp[-4].expr, yyvsp[-2].expr, yyvsp[0].expr); ;}
    break;

  case 138:
#line 479 "parser.y"
    { yyval.expr = make_expr2(EXPR_OR , yyvsp[-2].expr, yyvsp[0].expr); ;}
    break;

  case 139:
#line 480 "parser.y"
    { yyval.expr = make_expr2(EXPR_AND, yyvsp[-2].expr, yyvsp[0].expr); ;}
    break;

  case 140:
#line 481 "parser.y"
    { yyval.expr = make_expr2(EXPR_ADD, yyvsp[-2].expr, yyvsp[0].expr); ;}
    break;

  case 141:
#line 482 "parser.y"
    { yyval.expr = make_expr2(EXPR_SUB, yyvsp[-2].expr, yyvsp[0].expr); ;}
    break;

  case 142:
#line 483 "parser.y"
    { yyval.expr = make_expr2(EXPR_MUL, yyvsp[-2].expr, yyvsp[0].expr); ;}
    break;

  case 143:
#line 484 "parser.y"
    { yyval.expr = make_expr2(EXPR_DIV, yyvsp[-2].expr, yyvsp[0].expr); ;}
    break;

  case 144:
#line 485 "parser.y"
    { yyval.expr = make_expr2(EXPR_SHL, yyvsp[-2].expr, yyvsp[0].expr); ;}
    break;

  case 145:
#line 486 "parser.y"
    { yyval.expr = make_expr2(EXPR_SHR, yyvsp[-2].expr, yyvsp[0].expr); ;}
    break;

  case 146:
#line 487 "parser.y"
    { yyval.expr = make_expr1(EXPR_NOT, yyvsp[0].expr); ;}
    break;

  case 147:
#line 488 "parser.y"
    { yyval.expr = make_expr1(EXPR_NEG, yyvsp[0].expr); ;}
    break;

  case 148:
#line 489 "parser.y"
    { yyval.expr = make_expr1(EXPR_PPTR, yyvsp[0].expr); ;}
    break;

  case 149:
#line 490 "parser.y"
    { yyval.expr = make_exprt(EXPR_CAST, yyvsp[-2].tref, yyvsp[0].expr); ;}
    break;

  case 150:
#line 491 "parser.y"
    { yyval.expr = make_exprt(EXPR_SIZEOF, yyvsp[-1].tref, NULL); ;}
    break;

  case 151:
#line 492 "parser.y"
    { yyval.expr = yyvsp[-1].expr; ;}
    break;

  case 153:
#line 496 "parser.y"
    { LINK(yyvsp[0].expr, yyvsp[-2].expr); yyval.expr = yyvsp[0].expr; ;}
    break;

  case 154:
#line 499 "parser.y"
    { yyval.expr = yyvsp[0].expr;
						  if (!yyval.expr->is_const)
						      yyerror("expression is not constant\n");
						;}
    break;

  case 155:
#line 505 "parser.y"
    { yyval.var = yyvsp[0].var;
						  set_type(yyval.var, yyvsp[-1].tref, NULL);
						;}
    break;

  case 156:
#line 510 "parser.y"
    { yyval.var = NULL; ;}
    break;

  case 157:
#line 511 "parser.y"
    { if (yyvsp[0].var) { LINK(yyvsp[0].var, yyvsp[-1].var); yyval.var = yyvsp[0].var; }
						  else { yyval.var = yyvsp[-1].var; }
						;}
    break;

  case 158:
#line 516 "parser.y"
    { yyval.var = yyvsp[-1].var; ;}
    break;

  case 159:
#line 517 "parser.y"
    { yyval.var = make_var(NULL); yyval.var->type = yyvsp[-1].type; yyval.var->attrs = yyvsp[-2].attr; ;}
    break;

  case 160:
#line 518 "parser.y"
    { yyval.var = make_var(NULL); yyval.var->attrs = yyvsp[-1].attr; ;}
    break;

  case 161:
#line 519 "parser.y"
    { yyval.var = NULL; ;}
    break;

  case 162:
#line 522 "parser.y"
    { yyval.var = yyvsp[-1].var; set_type(yyval.var, yyvsp[-2].tref, yyvsp[0].expr); yyval.var->attrs = yyvsp[-3].attr; ;}
    break;

  case 163:
#line 527 "parser.y"
    { set_type(yyvsp[-3].var, yyvsp[-5].tref, NULL);
						  yyvsp[-3].var->attrs = yyvsp[-6].attr;
						  yyval.func = make_func(yyvsp[-3].var, yyvsp[-1].var);
						  if (is_attr(yyvsp[-3].var->attrs, ATTR_IN)) {
						    yyerror("Inapplicable attribute");
						  }
						;}
    break;

  case 164:
#line 536 "parser.y"
    { yyval.var = NULL; ;}
    break;

  case 166:
#line 540 "parser.y"
    { yyval.var = NULL; ;}
    break;

  case 167:
#line 541 "parser.y"
    { yyval.var = make_var(yyvsp[0].str); ;}
    break;

  case 168:
#line 542 "parser.y"
    { yyval.var = make_var(yyvsp[0].str); ;}
    break;

  case 169:
#line 545 "parser.y"
    { yyval.var = make_var(yyvsp[0].str); ;}
    break;

  case 170:
#line 547 "parser.y"
    { yyval.var = make_var(yyvsp[0].str); ;}
    break;

  case 171:
#line 548 "parser.y"
    { yyval.var = make_var(yyvsp[0].str); ;}
    break;

  case 172:
#line 549 "parser.y"
    { yyval.var = make_var(yyvsp[0].str); ;}
    break;

  case 173:
#line 550 "parser.y"
    { yyval.var = make_var(yyvsp[0].str); ;}
    break;

  case 174:
#line 551 "parser.y"
    { yyval.var = make_var(yyvsp[0].str); ;}
    break;

  case 175:
#line 554 "parser.y"
    { yyval.type = make_type(RPC_FC_BYTE, NULL); ;}
    break;

  case 176:
#line 555 "parser.y"
    { yyval.type = make_type(RPC_FC_WCHAR, NULL); ;}
    break;

  case 178:
#line 557 "parser.y"
    { yyval.type = yyvsp[0].type; yyval.type->sign = 1; ;}
    break;

  case 179:
#line 558 "parser.y"
    { yyval.type = yyvsp[0].type; yyval.type->sign = -1;
						  switch (yyval.type->type) {
						  case RPC_FC_CHAR: break;
						  case RPC_FC_SMALL: yyval.type->type = RPC_FC_USMALL; break;
						  case RPC_FC_SHORT: yyval.type->type = RPC_FC_USHORT; break;
						  case RPC_FC_LONG:  yyval.type->type = RPC_FC_ULONG;  break;
						  case RPC_FC_HYPER:
						    if (!yyval.type->ref) { yyval.type->ref = &std_uhyper; yyval.type->sign = 0; }
						    break;
						  default: break;
						  }
						;}
    break;

  case 180:
#line 570 "parser.y"
    { yyval.type = make_type(RPC_FC_ULONG, &std_int); yyval.type->sign = -1; ;}
    break;

  case 181:
#line 571 "parser.y"
    { yyval.type = make_type(RPC_FC_FLOAT, NULL); ;}
    break;

  case 182:
#line 572 "parser.y"
    { yyval.type = make_type(RPC_FC_FLOAT, NULL); ;}
    break;

  case 183:
#line 573 "parser.y"
    { yyval.type = make_type(RPC_FC_DOUBLE, NULL); ;}
    break;

  case 184:
#line 574 "parser.y"
    { yyval.type = make_type(RPC_FC_SMALL, &std_bool); ;}
    break;

  case 185:
#line 575 "parser.y"
    { yyval.type = make_type(RPC_FC_ERROR_STATUS_T, NULL); ;}
    break;

  case 186:
#line 576 "parser.y"
    { yyval.type = make_type(RPC_FC_IGNORE, NULL); ;}
    break;

  case 189:
#line 583 "parser.y"
    { yyval.type = make_type(RPC_FC_LONG, &std_int); ;}
    break;

  case 190:
#line 584 "parser.y"
    { yyval.type = make_type(RPC_FC_SMALL, NULL); ;}
    break;

  case 191:
#line 585 "parser.y"
    { yyval.type = make_type(RPC_FC_SHORT, NULL); ;}
    break;

  case 192:
#line 586 "parser.y"
    { yyval.type = make_type(RPC_FC_LONG, NULL); ;}
    break;

  case 193:
#line 587 "parser.y"
    { yyval.type = make_type(RPC_FC_HYPER, NULL); ;}
    break;

  case 194:
#line 588 "parser.y"
    { yyval.type = make_type(RPC_FC_HYPER, &std_int64); ;}
    break;

  case 195:
#line 589 "parser.y"
    { yyval.type = make_type(RPC_FC_CHAR, NULL); ;}
    break;

  case 196:
#line 592 "parser.y"
    { yyval.clas = make_class(yyvsp[0].str); ;}
    break;

  case 197:
#line 593 "parser.y"
    { yyval.clas = make_class(yyvsp[0].str); ;}
    break;

  case 198:
#line 596 "parser.y"
    { yyval.clas = yyvsp[0].clas;
						  yyval.clas->attrs = yyvsp[-1].attr;
						  if (!parse_only && do_header)
						    write_coclass(yyval.clas);
						;}
    break;

  case 199:
#line 603 "parser.y"
    { yyval.clas = yyvsp[-3].clas;
						  yyval.clas->ifaces = yyvsp[-1].ifref;
						;}
    break;

  case 200:
#line 608 "parser.y"
    { yyval.ifref = NULL; ;}
    break;

  case 201:
#line 609 "parser.y"
    { LINK(yyvsp[0].ifref, yyvsp[-1].ifref); yyval.ifref = yyvsp[0].ifref; ;}
    break;

  case 202:
#line 613 "parser.y"
    { yyval.ifref = make_ifref(yyvsp[0].type); yyval.ifref->attrs = yyvsp[-1].attr; ;}
    break;

  case 203:
#line 616 "parser.y"
    { yyval.type = get_type(0, yyvsp[0].str, 0); ;}
    break;

  case 204:
#line 617 "parser.y"
    { yyval.type = get_type(0, yyvsp[0].str, 0); ;}
    break;

  case 205:
#line 620 "parser.y"
    { yyval.type = yyvsp[0].type;
						  if (yyval.type->defined) yyerror("multiple definition error\n");
						  yyval.type->attrs = yyvsp[-1].attr;
						  yyval.type->attrs = make_attr(ATTR_DISPINTERFACE);
						  LINK(yyval.type->attrs, yyvsp[-1].attr);
						  yyval.type->ref = find_type("IDispatch", 0);
						  if (!yyval.type->ref) yyerror("IDispatch is undefined\n");
						  yyval.type->defined = TRUE;
						  if (!parse_only && do_header) write_forward(yyval.type);
						;}
    break;

  case 206:
#line 632 "parser.y"
    { yyval.var = NULL; ;}
    break;

  case 207:
#line 633 "parser.y"
    { LINK(yyvsp[-1].var, yyvsp[-2].var); yyval.var = yyvsp[-1].var; ;}
    break;

  case 208:
#line 636 "parser.y"
    { yyval.func = NULL; ;}
    break;

  case 209:
#line 637 "parser.y"
    { LINK(yyvsp[-1].func, yyvsp[-2].func); yyval.func = yyvsp[-1].func; ;}
    break;

  case 210:
#line 643 "parser.y"
    { yyval.type = yyvsp[-4].type;
						  yyval.type->fields = yyvsp[-2].var;
						  yyval.type->funcs = yyvsp[-1].func;
						  if (!parse_only && do_header) write_dispinterface(yyval.type);
						;}
    break;

  case 211:
#line 655 "parser.y"
    { yyval.type = NULL; ;}
    break;

  case 212:
#line 656 "parser.y"
    { yyval.type = find_type2(yyvsp[0].str, 0); ;}
    break;

  case 213:
#line 659 "parser.y"
    { yyval.type = get_type(RPC_FC_IP, yyvsp[0].str, 0); ;}
    break;

  case 214:
#line 660 "parser.y"
    { yyval.type = get_type(RPC_FC_IP, yyvsp[0].str, 0); ;}
    break;

  case 215:
#line 663 "parser.y"
    { yyval.type = yyvsp[0].type;
						  if (yyval.type->defined) yyerror("multiple definition error\n");
						  yyval.type->attrs = yyvsp[-1].attr;
						  yyval.type->defined = TRUE;
						  if (!parse_only && do_header) write_forward(yyval.type);
						;}
    break;

  case 216:
#line 672 "parser.y"
    { yyval.type = yyvsp[-4].type;
						  yyval.type->ref = yyvsp[-3].type;
						  yyval.type->funcs = yyvsp[-1].func;
						  if (!parse_only && do_header) write_interface(yyval.type);
						;}
    break;

  case 217:
#line 680 "parser.y"
    { yyval.type = yyvsp[-6].type;
						  yyval.type->ref = find_type2(yyvsp[-4].str, 0);
						  if (!yyval.type->ref) yyerror("base class %s not found in import\n", yyvsp[-4].str);
						  yyval.type->funcs = yyvsp[-1].func;
						  if (!parse_only && do_header) write_interface(yyval.type);
						;}
    break;

  case 218:
#line 686 "parser.y"
    { yyval.type = yyvsp[0].type; ;}
    break;

  case 219:
#line 690 "parser.y"
    { yyval.type = yyvsp[-1].type; if (!parse_only && do_header) write_forward(yyval.type); ;}
    break;

  case 220:
#line 691 "parser.y"
    { yyval.type = yyvsp[-1].type; if (!parse_only && do_header) write_forward(yyval.type); ;}
    break;

  case 221:
#line 694 "parser.y"
    { yyval.type = make_type(0, NULL); yyval.type->name = yyvsp[0].str; ;}
    break;

  case 222:
#line 695 "parser.y"
    { yyval.type = make_type(0, NULL); yyval.type->name = yyvsp[0].str; ;}
    break;

  case 223:
#line 698 "parser.y"
    { yyval.type = yyvsp[0].type;
						  yyval.type->attrs = yyvsp[-1].attr;
						;}
    break;

  case 224:
#line 703 "parser.y"
    { yyval.type = yyvsp[-3].type;
						  yyval.type->funcs = yyvsp[-1].func;
						  /* FIXME: if (!parse_only && do_header) write_module($$); */
						;}
    break;

  case 225:
#line 709 "parser.y"
    { yyval.var = yyvsp[0].var; yyval.var->ptr_level++; ;}
    break;

  case 226:
#line 710 "parser.y"
    { yyval.var = yyvsp[0].var; /* FIXME */ ;}
    break;

  case 229:
#line 715 "parser.y"
    { yyval.var = yyvsp[-1].var; ;}
    break;

  case 231:
#line 720 "parser.y"
    { LINK(yyvsp[0].var, yyvsp[-2].var); yyval.var = yyvsp[0].var; ;}
    break;

  case 232:
#line 724 "parser.y"
    { yyval.num = RPC_FC_RP; ;}
    break;

  case 233:
#line 725 "parser.y"
    { yyval.num = RPC_FC_UP; ;}
    break;

  case 234:
#line 728 "parser.y"
    { yyval.type = get_typev(RPC_FC_STRUCT, yyvsp[-3].var, tsSTRUCT);
						  /* overwrite RPC_FC_STRUCT with a more exact type */
						  yyval.type->type = get_struct_type( yyvsp[-1].var );
						  yyval.type->fields = yyvsp[-1].var;
						  yyval.type->defined = TRUE;
						  if(in_typelib)
						    add_struct(yyval.type);
						;}
    break;

  case 235:
#line 738 "parser.y"
    { yyval.tref = make_tref(NULL, make_type(0, NULL)); ;}
    break;

  case 236:
#line 739 "parser.y"
    { yyval.tref = make_tref(yyvsp[0].str, find_type(yyvsp[0].str, 0)); ;}
    break;

  case 237:
#line 740 "parser.y"
    { yyval.tref = make_tref(NULL, yyvsp[0].type); ;}
    break;

  case 238:
#line 741 "parser.y"
    { yyval.tref = uniq_tref(yyvsp[0].tref); yyval.tref->ref->is_const = TRUE; ;}
    break;

  case 239:
#line 742 "parser.y"
    { yyval.tref = make_tref(NULL, yyvsp[0].type); ;}
    break;

  case 240:
#line 743 "parser.y"
    { yyval.tref = make_tref(NULL, find_type2(yyvsp[0].str, tsENUM)); ;}
    break;

  case 241:
#line 744 "parser.y"
    { yyval.tref = make_tref(NULL, yyvsp[0].type); ;}
    break;

  case 242:
#line 745 "parser.y"
    { yyval.tref = make_tref(NULL, get_type(RPC_FC_STRUCT, yyvsp[0].str, tsSTRUCT)); ;}
    break;

  case 243:
#line 746 "parser.y"
    { yyval.tref = make_tref(NULL, yyvsp[0].type); ;}
    break;

  case 244:
#line 747 "parser.y"
    { yyval.tref = make_tref(NULL, find_type2(yyvsp[0].str, tsUNION)); ;}
    break;

  case 245:
#line 750 "parser.y"
    { typeref_t *tref = uniq_tref(yyvsp[-1].tref);
						  yyvsp[0].var->tname = tref->name;
						  tref->name = NULL;
						  yyval.type = type_ref(tref);
						  yyval.type->attrs = yyvsp[-2].attr;
						  if (!parse_only && do_header)
						    write_typedef(yyval.type, yyvsp[0].var);
						  if (in_typelib && yyval.type->attrs)
						    add_typedef(yyval.type, yyvsp[0].var);
						  reg_types(yyval.type, yyvsp[0].var, 0);
						;}
    break;

  case 246:
#line 763 "parser.y"
    { yyval.type = get_typev(RPC_FC_NON_ENCAPSULATED_UNION, yyvsp[-3].var, tsUNION);
						  yyval.type->fields = yyvsp[-1].var;
						  yyval.type->defined = TRUE;
						;}
    break;

  case 247:
#line 769 "parser.y"
    { var_t *u = yyvsp[-3].var;
						  yyval.type = get_typev(RPC_FC_ENCAPSULATED_UNION, yyvsp[-8].var, tsUNION);
						  if (!u) u = make_var("tagged_union");
						  u->type = make_type(RPC_FC_NON_ENCAPSULATED_UNION, NULL);
						  u->type->fields = yyvsp[-1].var;
						  u->type->defined = TRUE;
						  LINK(u, yyvsp[-5].var); yyval.type->fields = u;
						  yyval.type->defined = TRUE;
						;}
    break;

  case 248:
#line 781 "parser.y"
    { yyval.num = MAKELONG(yyvsp[0].num, 0); ;}
    break;

  case 249:
#line 782 "parser.y"
    { yyval.num = MAKELONG(yyvsp[-2].num, yyvsp[0].num); ;}
    break;


    }

/* Line 991 of yacc.c.  */
#line 3169 "y.tab.c"

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
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("syntax error, unexpected ") + 1;
	  yysize += yystrlen (yytname[yytype]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab2;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
yyerrlab1:

  /* Suppress GCC warning that yyerrlab1 is unused when no action
     invokes YYERROR.  */
#if defined (__GNUC_MINOR__) && 2093 <= (__GNUC__ * 1000 + __GNUC_MINOR__)
  __attribute__ ((__unused__))
#endif


  goto yyerrlab2;


/*---------------------------------------------------------------.
| yyerrlab2 -- pop states until the error token can be shifted.  |
`---------------------------------------------------------------*/
yyerrlab2:
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

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      yyvsp--;
      yystate = *--yyssp;

      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


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
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 785 "parser.y"


static attr_t *make_attr(enum attr_type type)
{
  attr_t *a = xmalloc(sizeof(attr_t));
  a->type = type;
  a->u.ival = 0;
  INIT_LINK(a);
  return a;
}

static attr_t *make_attrv(enum attr_type type, unsigned long val)
{
  attr_t *a = xmalloc(sizeof(attr_t));
  a->type = type;
  a->u.ival = val;
  INIT_LINK(a);
  return a;
}

static attr_t *make_attrp(enum attr_type type, void *val)
{
  attr_t *a = xmalloc(sizeof(attr_t));
  a->type = type;
  a->u.pval = val;
  INIT_LINK(a);
  return a;
}

static expr_t *make_expr(enum expr_type type)
{
  expr_t *e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = NULL;
  e->u.lval = 0;
  e->is_const = FALSE;
  INIT_LINK(e);
  return e;
}

static expr_t *make_exprl(enum expr_type type, long val)
{
  expr_t *e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = NULL;
  e->u.lval = val;
  e->is_const = FALSE;
  INIT_LINK(e);
  /* check for numeric constant */
  if (type == EXPR_NUM || type == EXPR_HEXNUM) {
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
  INIT_LINK(e);
  /* check for predefined constants */
  if (type == EXPR_IDENTIFIER) {
    var_t *c = find_const(val, 0);
    if (c) {
      e->u.sval = c->name;
      free(val);
      e->is_const = TRUE;
      e->cval = c->lval;
    }
  }
  return e;
}

static expr_t *make_exprt(enum expr_type type, typeref_t *tref, expr_t *expr)
{
  expr_t *e;
  e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = expr;
  e->u.tref = tref;
  e->is_const = FALSE;
  INIT_LINK(e);
  /* check for cast of constant expression */
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
  INIT_LINK(e);
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
  INIT_LINK(e);
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
  INIT_LINK(e);
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

static type_t *make_type(unsigned char type, type_t *ref)
{
  type_t *t = xmalloc(sizeof(type_t));
  t->name = NULL;
  t->type = type;
  t->ref = ref;
  t->rname = NULL;
  t->attrs = NULL;
  t->funcs = NULL;
  t->fields = NULL;
  t->ignore = parse_only;
  t->is_const = FALSE;
  t->sign = 0;
  t->defined = FALSE;
  t->written = FALSE;
  t->typelib_idx = -1;
  INIT_LINK(t);
  return t;
}

static typeref_t *make_tref(char *name, type_t *ref)
{
  typeref_t *t = xmalloc(sizeof(typeref_t));
  t->name = name;
  t->ref = ref;
  t->uniq = ref ? 0 : 1;
  return t;
}

static typeref_t *uniq_tref(typeref_t *ref)
{
  typeref_t *t = ref;
  type_t *tp;
  if (t->uniq) return t;
  tp = make_type(0, t->ref);
  tp->name = t->name;
  t->name = NULL;
  t->ref = tp;
  t->uniq = 1;
  return t;
}

static type_t *type_ref(typeref_t *ref)
{
  type_t *t = ref->ref;
  if (ref->name) free(ref->name);
  free(ref);
  return t;
}

static void set_type(var_t *v, typeref_t *ref, expr_t *arr)
{
  v->type = ref->ref;
  v->tname = ref->name;
  ref->name = NULL;
  free(ref);
  v->array = arr;
}

static ifref_t *make_ifref(type_t *iface)
{
  ifref_t *l = xmalloc(sizeof(ifref_t));
  l->iface = iface;
  l->attrs = NULL;
  INIT_LINK(l);
  return l;
}

static var_t *make_var(char *name)
{
  var_t *v = xmalloc(sizeof(var_t));
  v->name = name;
  v->ptr_level = 0;
  v->type = NULL;
  v->tname = NULL;
  v->attrs = NULL;
  v->array = NULL;
  v->eval = NULL;
  v->lval = 0;
  INIT_LINK(v);
  return v;
}

static func_t *make_func(var_t *def, var_t *args)
{
  func_t *f = xmalloc(sizeof(func_t));
  f->def = def;
  f->args = args;
  f->ignore = parse_only;
  f->idx = -1;
  INIT_LINK(f);
  return f;
}

static class_t *make_class(char *name)
{
  class_t *c = xmalloc(sizeof(class_t));
  c->name = name;
  c->attrs = NULL;
  c->ifaces = NULL;
  INIT_LINK(c);
  return c;
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
  char *name;
  type_t *type;
  int t;
  struct rtype *next;
};

struct rtype *type_hash[HASHMAX];

static type_t *reg_type(type_t *type, char *name, int t)
{
  struct rtype *nt;
  int hash;
  if (!name) {
    yyerror("registering named type without name\n");
    return type;
  }
  hash = hash_ident(name);
  nt = xmalloc(sizeof(struct rtype));
  nt->name = name;
  nt->type = type;
  nt->t = t;
  nt->next = type_hash[hash];
  type_hash[hash] = nt;
  type->name = name;
  return type;
}

/* determine pointer type from attrs */
static unsigned char get_pointer_type( type_t *type )
{
  int t;
  if (is_attr( type->attrs, ATTR_STRING ))
  {
    type_t *t = type;
    while( t->type == 0 && t->ref )
      t = t->ref;
    switch( t->type )
    {
    case RPC_FC_CHAR:
      return RPC_FC_C_CSTRING;
    case RPC_FC_WCHAR:
      return RPC_FC_C_WSTRING;
    }
  }
  t = get_attrv( type->attrs, ATTR_POINTERTYPE );
  if (t) return t;

  if(is_attr( type->attrs, ATTR_PTR ))
    return RPC_FC_FP;

  if(is_attr( type->attrs, ATTR_UNIQUE ))
    return RPC_FC_UP;

  return RPC_FC_RP;
}

static type_t *reg_types(type_t *type, var_t *names, int t)
{
  type_t *ptr = type;
  int ptrc = 0;

  while (names) {
    var_t *next = NEXT_LINK(names);
    if (names->name) {
      type_t *cur = ptr;
      int cptr = names->ptr_level;
      if (cptr > ptrc) {
        while (cptr > ptrc) {
          int t = get_pointer_type( cur );
          cur = ptr = make_type(t, cur);
          ptrc++;
        }
      } else {
        while (cptr < ptrc) {
          cur = cur->ref;
          cptr++;
        }
      }
      reg_type(cur, names->name, t);
    }
    free(names);
    names = next;
  }
  return type;
}

static type_t *find_type(char *name, int t)
{
  struct rtype *cur = type_hash[hash_ident(name)];
  while (cur && (cur->t != t || strcmp(cur->name, name)))
    cur = cur->next;
  if (!cur) {
    yyerror("type %s not found\n", name);
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

static int get_struct_type(var_t *field)
{
  int has_pointer = 0;
  int has_conformant_array = 0;
  int has_conformant_string = 0;

  while (field)
  {
    type_t *t = field->type;

    /* get the base type */
    while( (t->type == 0) && t->ref )
      t = t->ref;

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

    case RPC_FC_UP:
    case RPC_FC_FP:
      has_pointer = 1;
      break;
    case RPC_FC_CARRAY:
      has_conformant_array = 1;
      break;
    case RPC_FC_C_CSTRING:
    case RPC_FC_C_WSTRING:
      has_conformant_string = 1;
      break;

    /*
     * Propagate member attributes
     *  a struct should be at least as complex as its member
     */
    case RPC_FC_CVSTRUCT:
      has_conformant_string = 1;
      has_pointer = 1;
      break;

    case RPC_FC_CPSTRUCT:
      has_conformant_array = 1;
      has_pointer = 1;
      break;

    case RPC_FC_CSTRUCT:
      has_conformant_array = 1;
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
    field = NEXT_LINK(field);
  }

  if( has_conformant_string && has_pointer )
    return RPC_FC_CVSTRUCT;
  if( has_conformant_array && has_pointer )
    return RPC_FC_CPSTRUCT;
  if( has_conformant_array )
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
    yyerror("registering constant without name\n");
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
    if (f) yyerror("constant %s not found\n", name);
    return NULL;
  }
  return cur->var;
}

