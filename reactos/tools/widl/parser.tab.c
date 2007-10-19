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
static type_t *make_class(char *name);
static type_t *make_safearray(void);

static type_t *reg_type(type_t *type, const char *name, int t);
static type_t *reg_types(type_t *type, var_t *names, int t);
static type_t *find_type(const char *name, int t);
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
# define YYDEBUG 1
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 109 "parser.y"
typedef union YYSTYPE {
	attr_t *attr;
	expr_t *expr;
	type_t *type;
	typeref_t *tref;
	var_t *var;
	func_t *func;
	ifref_t *ifref;
	char *str;
	UUID *uuid;
	unsigned int num;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 463 "parser.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 475 "parser.tab.c"

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
#define YYLAST   1059

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  153
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  74
/* YYNRULES -- Number of rules. */
#define YYNRULES  272
/* YYNRULES -- Number of states. */
#define YYNSTATES  496

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   388

#define YYTRANSLATE(YYX) 						\
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
static const unsigned short yyprhs[] =
{
       0,     0,     3,     5,     6,     9,    12,    16,    19,    22,
      25,    28,    29,    32,    35,    39,    42,    45,    48,    51,
      52,    56,    59,    61,    64,    66,    69,    72,    74,    77,
      80,    83,    88,    92,    96,   101,   104,   108,   112,   113,
     115,   117,   119,   123,   125,   130,   134,   141,   147,   148,
     152,   156,   158,   162,   167,   168,   170,   174,   176,   180,
     185,   186,   188,   190,   192,   194,   196,   201,   206,   208,
     210,   212,   214,   216,   218,   223,   228,   230,   232,   237,
     239,   244,   249,   254,   256,   258,   263,   268,   273,   278,
     283,   285,   290,   292,   297,   299,   305,   307,   309,   314,
     316,   318,   320,   322,   324,   326,   328,   330,   332,   337,
     339,   341,   343,   345,   352,   354,   356,   358,   360,   365,
     367,   369,   374,   379,   384,   389,   391,   393,   398,   403,
     405,   406,   408,   409,   412,   417,   421,   427,   428,   431,
     433,   435,   439,   443,   445,   451,   453,   457,   458,   460,
     462,   464,   466,   468,   470,   476,   480,   484,   488,   492,
     496,   500,   504,   508,   511,   514,   517,   522,   527,   531,
     533,   537,   539,   544,   545,   548,   551,   555,   558,   560,
     565,   573,   574,   576,   577,   579,   581,   583,   585,   587,
     589,   591,   593,   595,   597,   599,   601,   603,   605,   607,
     610,   613,   615,   617,   619,   621,   623,   625,   627,   628,
     630,   632,   635,   637,   640,   643,   645,   647,   650,   653,
     656,   661,   662,   665,   668,   671,   674,   677,   680,   684,
     687,   691,   697,   698,   701,   704,   707,   710,   716,   724,
     726,   729,   732,   735,   738,   741,   746,   749,   752,   754,
     756,   760,   762,   766,   768,   770,   772,   778,   780,   782,
     784,   787,   789,   792,   794,   797,   799,   802,   807,   812,
     818,   829,   831
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short yyrhs[] =
{
     154,     0,    -1,   155,    -1,    -1,   155,   214,    -1,   155,
     213,    -1,   155,   200,   144,    -1,   155,   202,    -1,   155,
     217,    -1,   155,   165,    -1,   155,   158,    -1,    -1,   156,
     214,    -1,   156,   213,    -1,   156,   200,   144,    -1,   156,
     202,    -1,   156,   217,    -1,   156,   158,    -1,   156,   162,
      -1,    -1,   157,   193,   144,    -1,   157,   158,    -1,   144,
      -1,   179,   144,    -1,   159,    -1,   183,   144,    -1,   189,
     144,    -1,   161,    -1,   222,   144,    -1,   224,   144,    -1,
     225,   144,    -1,    36,   145,     7,   146,    -1,    68,     7,
     144,    -1,   160,   156,     9,    -1,    69,   145,     7,   146,
      -1,    78,     3,    -1,   173,   163,   147,    -1,   164,   156,
     148,    -1,    -1,   168,    -1,   128,    -1,   169,    -1,   168,
     131,   169,    -1,   167,    -1,   173,   223,   219,   170,    -1,
     223,   219,   170,    -1,   173,   223,   219,   145,   166,   146,
      -1,   223,   219,   145,   166,   146,    -1,    -1,   149,   171,
     150,    -1,   149,   138,   150,    -1,   185,    -1,   171,   131,
     186,    -1,   171,   150,   149,   186,    -1,    -1,   173,    -1,
     149,   174,   150,    -1,   175,    -1,   174,   131,   175,    -1,
     174,   150,   149,   175,    -1,    -1,    12,    -1,    14,    -1,
      15,    -1,    17,    -1,    18,    -1,    23,   145,   196,   146,
      -1,    25,   145,   187,   146,    -1,    32,    -1,    33,    -1,
      34,    -1,    35,    -1,    37,    -1,    38,    -1,    39,   145,
     188,   146,    -1,    39,   145,     7,   146,    -1,    40,    -1,
      41,    -1,    43,   145,     7,   146,    -1,    45,    -1,    46,
     145,     7,   146,    -1,    47,   145,     7,   146,    -1,    47,
     145,   188,   146,    -1,    50,    -1,    54,    -1,    56,   145,
     188,   146,    -1,    57,   145,     7,   146,    -1,    58,   145,
       7,   146,    -1,    59,   145,   188,   146,    -1,    60,   145,
       7,   146,    -1,    61,    -1,    63,   145,   188,   146,    -1,
      64,    -1,    65,   145,   196,   146,    -1,    66,    -1,    67,
     145,    55,     3,   146,    -1,    70,    -1,    72,    -1,    77,
     145,   184,   146,    -1,    79,    -1,    83,    -1,    84,    -1,
      85,    -1,    86,    -1,    87,    -1,    88,    -1,    89,    -1,
      90,    -1,    91,   145,   221,   146,    -1,    93,    -1,    94,
      -1,    95,    -1,    97,    -1,    98,   145,   188,   131,   188,
     146,    -1,    99,    -1,   101,    -1,   102,    -1,   103,    -1,
     108,   145,   184,   146,    -1,   111,    -1,   113,    -1,   116,
     145,   186,   146,    -1,   117,   145,   223,   146,    -1,   118,
     145,   223,   146,    -1,   124,   145,     8,   146,    -1,   125,
      -1,   126,    -1,   127,   145,   226,   146,    -1,   130,   145,
     223,   146,    -1,   221,    -1,    -1,   112,    -1,    -1,   177,
     178,    -1,    25,   186,   133,   191,    -1,    37,   133,   191,
      -1,    31,   223,   196,   151,   188,    -1,    -1,   181,   131,
      -1,   181,    -1,   182,    -1,   181,   131,   182,    -1,   196,
     151,   188,    -1,   196,    -1,    48,   195,   147,   180,   148,
      -1,   185,    -1,   184,   131,   185,    -1,    -1,   186,    -1,
       5,    -1,     6,    -1,    52,    -1,   119,    -1,     3,    -1,
     186,   132,   186,   133,   186,    -1,   186,   134,   186,    -1,
     186,   135,   186,    -1,   186,   137,   186,    -1,   186,   136,
     186,    -1,   186,   138,   186,    -1,   186,   139,   186,    -1,
     186,    10,   186,    -1,   186,    11,   186,    -1,   140,   186,
      -1,   136,   186,    -1,   138,   186,    -1,   145,   223,   146,
     186,    -1,   109,   145,   223,   146,    -1,   145,   186,   146,
      -1,   188,    -1,   187,   131,   188,    -1,   186,    -1,    51,
      31,   223,   196,    -1,    -1,   190,   191,    -1,   192,   144,
      -1,   172,   225,   144,    -1,   173,   144,    -1,   144,    -1,
     172,   223,   219,   170,    -1,   172,   223,   176,   219,   145,
     166,   146,    -1,    -1,   196,    -1,    -1,     3,    -1,     4,
      -1,     3,    -1,     4,    -1,    15,    -1,    63,    -1,    76,
      -1,    86,    -1,    98,    -1,   103,    -1,   124,    -1,   127,
      -1,    21,    -1,   129,    -1,   199,    -1,   106,   199,    -1,
     123,   199,    -1,   123,    -1,    53,    -1,   107,    -1,    44,
      -1,    19,    -1,    49,    -1,    55,    -1,    -1,    73,    -1,
      73,    -1,   105,   198,    -1,   110,    -1,    80,   198,    -1,
      62,   198,    -1,    74,    -1,    27,    -1,    28,     3,    -1,
      28,     4,    -1,   173,   200,    -1,   201,   147,   203,   148,
      -1,    -1,   203,   204,    -1,   172,   214,    -1,    42,     3,
      -1,    42,     4,    -1,   173,   205,    -1,    92,   133,    -1,
     207,   192,   144,    -1,    81,   133,    -1,   208,   193,   144,
      -1,   206,   147,   207,   208,   148,    -1,    -1,   133,     4,
      -1,    75,     3,    -1,    75,     4,    -1,   173,   211,    -1,
     212,   210,   147,   157,   148,    -1,   212,   133,     3,   147,
     161,   157,   148,    -1,   209,    -1,   211,   144,    -1,   205,
     144,    -1,    82,     3,    -1,    82,     4,    -1,   173,   215,
      -1,   216,   147,   157,   148,    -1,   138,   219,    -1,    31,
     218,    -1,   196,    -1,   218,    -1,   145,   219,   146,    -1,
     219,    -1,   220,   131,   219,    -1,   100,    -1,   122,    -1,
      96,    -1,   114,   195,   147,   190,   148,    -1,   128,    -1,
       4,    -1,   197,    -1,    31,   223,    -1,   183,    -1,    48,
       3,    -1,   222,    -1,   114,     3,    -1,   225,    -1,   121,
       3,    -1,   104,   145,   223,   146,    -1,   120,   172,   223,
     220,    -1,   121,   195,   147,   190,   148,    -1,   121,   195,
     115,   145,   192,   146,   194,   147,   177,   148,    -1,     5,
      -1,     5,   152,     5,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   239,   239,   242,   243,   244,   245,   249,   253,   254,
     255,   258,   259,   260,   261,   262,   265,   266,   267,   270,
     271,   272,   275,   276,   277,   278,   279,   280,   281,   282,
     283,   286,   288,   291,   294,   297,   299,   302,   305,   306,
     309,   312,   313,   314,   318,   322,   325,   331,   338,   339,
     340,   343,   344,   345,   348,   349,   353,   359,   360,   363,
     368,   369,   370,   371,   372,   373,   374,   375,   376,   377,
     378,   379,   380,   381,   382,   383,   384,   385,   386,   387,
     388,   389,   390,   391,   392,   393,   394,   395,   396,   397,
     398,   399,   400,   401,   402,   403,   404,   405,   406,   407,
     408,   409,   410,   411,   412,   413,   414,   415,   416,   417,
     418,   419,   420,   421,   422,   423,   424,   425,   426,   427,
     428,   429,   430,   431,   432,   433,   434,   435,   436,   437,
     440,   441,   444,   445,   450,   454,   460,   466,   467,   468,
     471,   474,   480,   484,   489,   497,   498,   511,   512,   515,
     516,   517,   518,   519,   520,   521,   522,   523,   524,   525,
     526,   527,   528,   529,   530,   531,   532,   533,   534,   537,
     538,   541,   547,   552,   553,   558,   559,   560,   561,   564,
     568,   578,   579,   582,   583,   584,   587,   589,   590,   591,
     592,   593,   594,   595,   596,   597,   600,   601,   602,   603,
     604,   616,   617,   618,   619,   620,   621,   622,   625,   626,
     629,   630,   631,   632,   633,   634,   635,   638,   639,   645,
     652,   657,   658,   662,   665,   666,   669,   682,   683,   686,
     687,   690,   705,   706,   709,   710,   713,   721,   729,   736,
     740,   741,   744,   745,   748,   753,   759,   760,   763,   764,
     765,   769,   770,   774,   775,   776,   779,   789,   790,   791,
     792,   793,   794,   795,   796,   797,   798,   799,   802,   815,
     819,   833,   834
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
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
  "tREF", "tREQUESTEDIT", "tRESTRICTED", "tRETVAL", "tSAFEARRAY",
  "tSHORT", "tSIGNED", "tSINGLE", "tSIZEIS", "tSIZEOF", "tSMALL",
  "tSOURCE", "tSTDCALL", "tSTRING", "tSTRUCT", "tSWITCH", "tSWITCHIS",
  "tSWITCHTYPE", "tTRANSMITAS", "tTRUE", "tTYPEDEF", "tUNION", "tUNIQUE",
  "tUNSIGNED", "tUUID", "tV1ENUM", "tVARARG", "tVERSION", "tVOID",
  "tWCHAR", "tWIREMARSHAL", "','", "'?'", "':'", "'|'", "'&'", "'-'",
  "'+'", "'*'", "'/'", "'~'", "CAST", "PPTR", "NEG", "';'", "'('", "')'",
  "'{'", "'}'", "'['", "']'", "'='", "'.'", "$accept", "input",
  "gbl_statements", "imp_statements", "int_statements", "statement",
  "cppquote", "import_start", "import", "importlib", "libraryhdr",
  "library_start", "librarydef", "m_args", "no_args", "args", "arg",
  "array", "array_list", "m_attributes", "attributes", "attrib_list",
  "attribute", "callconv", "cases", "case", "constdef", "enums",
  "enum_list", "enum", "enumdef", "m_exprs", "m_expr", "expr",
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
     175,   175,   175,   175,   175,   175,   175,   175,   175,   175,
     175,   175,   175,   175,   175,   175,   175,   175,   175,   175,
     175,   175,   175,   175,   175,   175,   175,   175,   175,   175,
     175,   175,   175,   175,   175,   175,   175,   175,   175,   175,
     175,   175,   175,   175,   175,   175,   175,   175,   175,   175,
     175,   175,   175,   175,   175,   175,   175,   175,   175,   175,
     175,   175,   175,   175,   175,   175,   175,   175,   175,   175,
     176,   176,   177,   177,   178,   178,   179,   180,   180,   180,
     181,   181,   182,   182,   183,   184,   184,   185,   185,   186,
     186,   186,   186,   186,   186,   186,   186,   186,   186,   186,
     186,   186,   186,   186,   186,   186,   186,   186,   186,   187,
     187,   188,   189,   190,   190,   191,   191,   191,   191,   192,
     193,   194,   194,   195,   195,   195,   196,   196,   196,   196,
     196,   196,   196,   196,   196,   196,   197,   197,   197,   197,
     197,   197,   197,   197,   197,   197,   197,   197,   198,   198,
     199,   199,   199,   199,   199,   199,   199,   200,   200,   201,
     202,   203,   203,   204,   205,   205,   206,   207,   207,   208,
     208,   209,   210,   210,   211,   211,   212,   213,   213,   213,
     214,   214,   215,   215,   216,   217,   218,   218,   219,   219,
     219,   220,   220,   221,   221,   221,   222,   223,   223,   223,
     223,   223,   223,   223,   223,   223,   223,   223,   224,   225,
     225,   226,   226
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
       0,     1,     1,     1,     1,     1,     4,     4,     1,     1,
       1,     1,     1,     1,     4,     4,     1,     1,     4,     1,
       4,     4,     4,     1,     1,     4,     4,     4,     4,     4,
       1,     4,     1,     4,     1,     5,     1,     1,     4,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     4,     1,
       1,     1,     1,     6,     1,     1,     1,     1,     4,     1,
       1,     4,     4,     4,     4,     1,     1,     4,     4,     1,
       0,     1,     0,     2,     4,     3,     5,     0,     2,     1,
       1,     3,     3,     1,     5,     1,     3,     0,     1,     1,
       1,     1,     1,     1,     5,     3,     3,     3,     3,     3,
       3,     3,     3,     2,     2,     2,     4,     4,     3,     1,
       3,     1,     4,     0,     2,     2,     3,     2,     1,     4,
       7,     0,     1,     0,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       2,     1,     1,     1,     1,     1,     1,     1,     0,     1,
       1,     2,     1,     2,     2,     1,     1,     2,     2,     2,
       4,     0,     2,     2,     2,     2,     2,     2,     3,     2,
       3,     5,     0,     2,     2,     2,     2,     5,     7,     1,
       2,     2,     2,     2,     2,     4,     2,     2,     1,     1,
       3,     1,     3,     1,     1,     1,     5,     1,     1,     1,
       2,     1,     2,     1,     2,     1,     2,     4,     4,     5,
      10,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned short yydefact[] =
{
       3,     0,     2,     1,     0,     0,     0,     0,   183,     0,
       0,     0,   183,    54,   183,    22,    60,    10,    24,    11,
      27,    11,     9,     0,     0,     0,     0,     0,     0,     7,
       0,     0,   239,     0,   232,     5,     4,     0,     8,     0,
       0,     0,   217,   218,   258,   205,   196,   216,     0,   204,
     183,   206,   202,   207,   208,   210,   215,   208,     0,   208,
       0,   203,   212,   183,   183,   201,   257,   197,   261,   259,
     198,   263,     0,   265,     0,   224,   225,   184,   185,     0,
       0,     0,   234,   235,     0,     0,    55,     0,    61,    62,
      63,    64,    65,     0,     0,    68,    69,    70,    71,    72,
      73,     0,    76,    77,     0,    79,     0,     0,    83,    84,
       0,     0,     0,     0,     0,    90,     0,    92,     0,    94,
       0,    96,    97,     0,    99,   100,   101,   102,   103,   104,
     105,   106,   107,     0,   109,   110,   111,   255,   112,     0,
     114,   253,   115,   116,   117,     0,   119,   120,     0,     0,
       0,   254,     0,   125,   126,     0,     0,     0,    57,   129,
       0,     0,     0,     0,     0,   219,   226,   236,   244,    23,
      25,    26,     6,   221,   241,     0,   240,     0,     0,    19,
      28,    29,    30,   260,   262,   209,   214,   213,     0,   211,
     199,   264,   266,   200,   186,   187,   188,   189,   190,   191,
     192,   193,   194,   195,     0,     0,   137,     0,    32,   173,
       0,     0,   173,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   147,     0,     0,
     147,     0,     0,     0,     0,     0,     0,    60,    56,    33,
       0,    17,    18,     0,     0,    15,    13,    12,    16,    37,
      35,   242,   243,    36,    54,     0,    54,     0,   233,    19,
      54,     0,     0,    31,     0,   139,   140,   143,   172,    54,
       0,     0,     0,   248,   249,   251,   268,    54,    54,     0,
     153,   149,   150,   151,     0,   152,     0,     0,     0,     0,
     171,     0,   169,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   145,   148,
       0,     0,     0,     0,     0,     0,     0,   271,     0,     0,
      58,    60,     0,    14,   220,     0,   222,   227,     0,     0,
       0,    54,     0,    54,   245,    21,     0,     0,   267,   136,
     144,   138,     0,   178,   256,     0,    55,   174,     0,   247,
     246,     0,     0,     0,   269,    66,     0,   164,   165,   163,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    67,    75,    74,    78,    80,    81,    82,    85,
      86,    87,    88,    89,    91,    93,     0,   147,    98,   108,
       0,   118,   121,   122,   123,   124,     0,   127,   128,    59,
       0,   223,   229,     0,   228,   231,     0,    19,   237,   130,
      20,   141,   142,   265,   177,   175,   250,   252,   181,     0,
     168,     0,   161,   162,     0,   155,   156,   158,   157,   159,
     160,   170,    95,   146,     0,   272,    34,    48,   230,    54,
     131,     0,   176,     0,   182,   167,   166,     0,   113,   147,
     179,   238,     0,   132,   154,     0,     0,    51,    38,     0,
      50,     0,    49,   257,     0,    43,    39,    41,     0,     0,
       0,     0,   270,   133,    52,     0,   180,     0,     0,    48,
       0,    54,    53,    42,    48,    38,    45,    54,   135,    38,
      44,     0,   134,     0,    47,    46
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,     1,     2,   160,   260,   335,    18,    19,    20,   242,
     164,    21,    22,   464,   465,   466,   467,   450,   456,   336,
      86,   157,   158,   441,   459,   473,    24,   264,   265,   266,
      68,   307,   308,   290,   291,   292,    26,   269,   347,   348,
     337,   443,    79,   273,    69,   186,    70,   165,    28,   245,
     254,   326,    30,    31,   256,   331,    32,   178,    33,    34,
     246,   247,   168,    37,   248,   274,   275,   276,   159,    71,
     469,    40,    73,   318
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -447
static const short yypact[] =
{
    -447,    43,   696,  -447,    65,   655,  -111,    97,   109,    35,
      78,   118,   109,   -46,   109,  -447,   809,  -447,  -447,  -447,
    -447,  -447,  -447,    66,   -30,     2,    13,    16,    -5,  -447,
      26,    40,  -447,    38,    76,  -447,  -447,    50,  -447,    67,
      69,    71,  -447,  -447,  -447,  -447,  -447,  -447,   655,  -447,
     129,  -447,  -447,  -447,   161,  -447,  -447,   161,    91,   161,
      10,  -447,  -447,   182,   191,    10,  -447,  -447,  -447,  -447,
    -447,  -447,   587,  -447,   235,  -447,  -447,  -447,  -447,    96,
     655,   102,  -447,  -447,   103,   655,  -447,   -94,  -447,  -447,
    -447,  -447,  -447,   108,   110,  -447,  -447,  -447,  -447,  -447,
    -447,   112,  -447,  -447,   114,  -447,   121,   127,  -447,  -447,
     130,   135,   136,   137,   138,  -447,   142,  -447,   144,  -447,
     146,  -447,  -447,   147,  -447,  -447,  -447,  -447,  -447,  -447,
    -447,  -447,  -447,   148,  -447,  -447,  -447,  -447,  -447,   149,
    -447,  -447,  -447,  -447,  -447,   152,  -447,  -447,   156,   158,
     159,  -447,   160,  -447,  -447,   165,   166,  -100,  -447,  -447,
     528,   665,   271,   201,   167,  -447,  -447,  -447,  -447,  -447,
    -447,  -447,  -447,  -447,  -447,   214,  -447,   203,   168,  -447,
    -447,  -447,  -447,  -447,   169,  -447,  -447,  -447,   655,  -447,
    -447,   169,   -82,  -447,  -447,  -447,  -447,  -447,  -447,  -447,
    -447,  -447,  -447,  -447,   172,   173,   587,   587,  -447,  -447,
     223,   175,  -447,   587,   246,     9,   305,   320,    99,   246,
     321,   323,   246,   326,   246,   587,   280,   246,   -68,   246,
     246,   246,   655,   655,   329,   334,   655,   809,   195,  -447,
     200,  -447,  -447,   220,   196,  -447,  -447,  -447,  -447,  -447,
    -447,  -447,  -447,  -447,    72,   218,   -61,   206,  -447,  -447,
     892,   208,   246,  -447,   209,   225,  -447,   207,  -447,   -25,
     -21,   223,   223,  -447,  -447,  -447,   228,   -46,   -19,   216,
    -447,  -447,  -447,  -447,   215,  -447,   246,   246,   246,   500,
     532,   -84,  -447,   217,   221,   224,   226,   231,   239,   241,
     242,   243,   247,   248,   249,   250,   361,   -79,  -447,   532,
     254,   238,   -76,   267,   261,   262,   263,   219,   264,   265,
    -447,   809,   359,  -447,  -447,   -16,  -447,  -447,   279,   655,
     270,    92,   347,   901,  -447,  -447,   655,   272,  -447,  -447,
    -447,   587,   246,  -447,  -447,   655,   273,  -447,   275,  -447,
    -447,   274,   223,   277,  -447,  -447,   655,  -447,  -447,  -447,
     297,   278,   246,   246,   246,   246,   246,   246,   246,   246,
     246,   246,  -447,  -447,  -447,  -447,  -447,  -447,  -447,  -447,
    -447,  -447,  -447,  -447,  -447,  -447,   282,   246,  -447,  -447,
     246,  -447,  -447,  -447,  -447,  -447,   416,  -447,  -447,  -447,
     284,  -447,  -447,   223,  -447,  -447,   281,  -447,  -447,   314,
    -447,  -447,  -447,   294,  -447,  -447,  -447,  -447,   587,   295,
    -447,   246,  -447,  -447,   449,    64,    53,    14,    14,   212,
     212,  -447,  -447,  -447,   300,  -447,  -447,   298,  -447,   910,
    -447,   223,  -447,   301,  -447,  -447,  -447,   246,  -447,   375,
    -447,  -447,   306,  -447,   532,    74,   -93,  -447,   269,    -8,
    -447,   246,   303,   -60,   304,  -447,   322,  -447,   655,   223,
     246,   324,  -447,  -447,   532,   246,  -447,   418,   223,   -18,
     519,   -71,   532,  -447,   -11,   269,  -447,   -71,  -447,   269,
    -447,   308,  -447,   309,  -447,  -447
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
    -447,  -447,  -447,   437,  -251,     7,  -447,  -447,   131,  -447,
    -447,  -447,  -447,  -350,  -447,  -447,    -7,  -397,  -447,    -9,
      -2,  -447,  -214,  -447,  -447,  -447,  -447,  -447,  -447,   120,
       3,   244,  -357,  -191,  -447,  -173,  -447,   253,  -446,  -229,
     145,  -447,    46,   -70,  -447,    34,    51,    20,  -447,   466,
    -447,  -447,   -12,  -447,  -447,  -447,  -447,  -447,   -10,  -447,
     467,     4,  -447,  -447,   475,   211,  -253,  -447,   258,     5,
      -4,  -447,     1,  -447
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -185
static const short yytable[] =
{
      23,    72,   204,    41,    85,    25,    36,    39,   333,    17,
     270,   166,   280,   167,   281,   282,   293,   470,   350,   351,
     328,   211,    27,   320,   362,   363,     7,   330,   137,   471,
     433,   237,   141,  -184,    74,   488,   309,    47,   461,   309,
     313,   492,   294,     3,   183,   298,   299,   371,   353,   302,
     238,   304,   387,   212,   151,   387,   311,   462,    84,    11,
      87,   283,   372,   362,   363,  -184,    80,   388,    42,    43,
     391,   -40,    54,   343,   362,   363,   207,   280,    16,   281,
     282,   210,   486,    55,    56,    81,   -40,   490,    16,   339,
      57,   187,   457,   189,     4,   357,   358,   359,   360,   417,
      75,    76,   280,    16,   281,   282,   297,   399,     7,    84,
      87,   190,    77,    78,   169,    59,   193,   271,   284,   343,
      62,    82,    83,   344,    16,   343,   283,   485,   285,   354,
      16,   449,   184,    78,   489,   491,   267,   268,   449,   493,
     472,    11,   173,   279,   162,   286,   170,   287,   163,   288,
     437,   283,   369,   370,   289,   305,   439,   171,   243,   243,
     172,    41,    41,    25,    25,    39,    39,   241,   241,   412,
     174,   422,   423,   424,   425,   426,   427,   428,   429,   430,
     244,   244,   176,   284,   261,   191,    78,   175,   452,   367,
     368,   369,   370,   285,   192,    78,   309,   179,   431,   366,
     367,   368,   369,   370,   251,   252,   257,   258,   284,   177,
     286,   180,   287,   181,   288,   182,   479,   434,   285,   289,
     324,    16,   362,   363,   460,   484,   194,   195,   314,   315,
     446,   166,   319,   167,   185,   286,   188,   287,   196,   288,
     405,    16,   205,   206,   289,   325,   208,   329,     4,   280,
     209,   281,   282,   213,   270,   214,   454,   215,   309,   216,
     345,    41,     7,    25,   358,    39,   217,   346,   329,   345,
     474,   267,   218,    44,   250,   219,   346,   362,   363,   480,
     220,   221,   222,   223,   482,   361,   197,   224,    45,   225,
      46,   226,   227,   228,   229,    11,    47,   230,   283,   198,
      48,   231,   163,   232,   233,   234,   255,   362,   363,   199,
     235,   236,   295,    49,   253,   259,  -184,    50,    51,   263,
     277,   200,    52,   262,    53,   403,   201,   296,   300,   401,
     301,    54,   409,   303,    41,   306,    25,   316,    39,   317,
     323,   403,    55,    56,   321,   322,   413,   202,   444,    57,
     203,   327,   419,   332,   338,   284,   341,   340,   342,   352,
     356,   271,   355,   373,   386,   285,   400,   374,   272,   390,
     375,   396,   376,    58,    59,    60,    61,   377,   280,    62,
     281,   282,   286,    63,   287,   378,   288,   379,   380,   381,
      64,   289,    65,   382,   383,   384,   385,   463,    67,   364,
     389,   365,   366,   367,   368,   369,   370,   393,   394,   395,
     397,   398,   402,   392,   404,    10,   410,   414,    16,   415,
     416,   435,    44,   418,   421,   438,   440,   283,   432,   364,
     436,   365,   366,   367,   368,   369,   370,    45,   442,    46,
      41,   445,    25,   420,    39,    47,   448,   449,   453,    48,
     476,   458,   475,   477,   494,   495,   468,   481,   161,   362,
     363,   411,    49,   407,   478,   278,    50,    51,    29,    35,
     483,    52,   345,    53,   312,   468,   406,    38,   345,   346,
      54,   349,     0,   468,   284,   346,   310,   468,     0,     0,
       0,    55,    56,     0,   285,     0,     0,     0,    57,     0,
       0,     0,     0,   280,    44,   281,   282,     0,     0,     0,
       0,   286,     0,   455,     0,   288,     0,     0,     0,    45,
     289,    46,    58,    59,    60,    61,     0,    47,    62,   362,
     363,    48,    63,     0,     0,     0,     0,   239,     0,    64,
       0,    65,   362,   363,    49,     0,    66,    67,    50,    51,
       0,     0,   283,    52,     0,    53,     4,     0,     0,     5,
       0,     0,    54,     0,     6,     0,     0,    16,     0,     0,
       7,     0,     0,    55,    56,     0,     8,     0,     0,     9,
      57,   364,   447,   365,   366,   367,   368,   369,   370,     0,
     194,   195,     0,     0,     0,     0,    10,   240,     0,     0,
       0,     0,   196,    11,    58,    59,    60,    61,     0,   284,
      62,     0,     0,     0,    63,     0,     0,     0,     0,   285,
       0,    64,     0,    65,     0,     0,     0,     0,    66,    67,
       0,     0,     0,     0,     0,     0,   286,     0,   287,     0,
     288,     0,    12,     0,     0,   289,     0,     0,    13,    14,
     197,   364,   487,   365,   366,   367,   368,   369,   370,    44,
       0,     0,     0,   198,   364,     0,   365,   366,   367,   368,
     369,   370,    15,   199,    45,     0,    46,    16,     0,     0,
       0,     0,    47,     0,     0,   200,    48,     0,     0,     0,
     201,     0,     0,     4,     0,     0,     5,     0,     0,    49,
       0,     6,     0,    50,    51,     0,     0,     7,    52,     0,
      53,   202,     0,     8,   203,     0,     9,    54,     0,     0,
       0,     0,     0,     0,     4,     0,     0,     5,    55,    56,
       0,     0,     6,    10,   240,    57,     0,     0,     7,     0,
      11,     0,     0,     0,     8,     0,     0,     9,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    58,
      59,    60,    61,     0,    10,    62,     0,     0,     0,    63,
       0,    11,     0,     0,     0,     0,    64,     0,    65,    12,
       0,     0,     0,    66,    67,    13,    14,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    15,
      12,     0,     0,   249,    16,     0,    13,    14,     0,     0,
       0,    88,     0,    89,    90,     0,    91,    92,     0,     0,
       0,     0,    93,     0,    94,     0,     0,     0,     0,     0,
      15,    95,    96,    97,    98,    16,    99,   100,   101,   102,
     103,     0,   104,     0,   105,   106,   107,     0,     0,   108,
       0,     0,     0,   109,     0,   110,   111,   112,   113,   114,
     115,     0,   116,   117,   118,   119,   120,     0,     0,   121,
       0,   122,     0,     0,     0,     0,   123,     0,   124,     0,
       0,     0,   125,   126,   127,   128,   129,   130,   131,   132,
     133,     0,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,     0,     0,     0,     0,   145,     0,     0,
     146,     0,   147,     5,     0,   148,   149,   150,     6,     0,
       0,   151,     5,   152,   153,   154,   155,     6,     0,   156,
       8,     5,     0,     9,     0,     0,     6,     0,     0,     8,
       0,     0,     9,     0,     0,     0,     0,     0,     8,     0,
      10,     9,     0,     0,     0,     0,     0,     0,     0,    10,
       0,     0,     0,     0,     0,     0,     0,     0,    10,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    12,     0,     0,     0,
       0,     0,    13,    14,     0,    12,     0,     0,     0,     0,
       0,    13,    14,     0,    12,     0,     0,     0,     0,     0,
      13,    14,     0,     0,     0,     0,    15,     0,     0,     0,
     334,    16,     0,     0,     0,    15,     0,     0,     0,   408,
      16,     0,     0,     0,    15,     0,     0,     0,   451,    16
};

static const short yycheck[] =
{
       2,     5,    72,     2,    13,     2,     2,     2,   259,     2,
      31,    23,     3,    23,     5,     6,     7,    25,   271,   272,
      81,   115,     2,   237,    10,    11,    42,   256,    96,    37,
     387,   131,   100,   115,   145,   481,   227,    27,   131,   230,
     231,   487,   215,     0,    48,   218,   219,   131,   277,   222,
     150,   224,   131,   147,   122,   131,   229,   150,    12,    75,
      14,    52,   146,    10,    11,   147,    31,   146,     3,     4,
     146,   131,    62,   144,    10,    11,    80,     3,   149,     5,
       6,    85,   479,    73,    74,     7,   146,   484,   149,   262,
      80,    57,   449,    59,    28,   286,   287,   288,   289,   352,
       3,     4,     3,   149,     5,     6,     7,   321,    42,    63,
      64,    60,     3,     4,   144,   105,    65,   138,   109,   144,
     110,     3,     4,   148,   149,   144,    52,   145,   119,   148,
     149,   149,     3,     4,   145,   485,   206,   207,   149,   489,
     148,    75,   147,   213,    78,   136,   144,   138,    82,   140,
     403,    52,   138,   139,   145,   225,   407,   144,   160,   161,
     144,   160,   161,   160,   161,   160,   161,   160,   161,   342,
     144,   362,   363,   364,   365,   366,   367,   368,   369,   370,
     160,   161,   144,   109,   188,     3,     4,   147,   441,   136,
     137,   138,   139,   119,     3,     4,   387,   147,   371,   135,
     136,   137,   138,   139,     3,     4,     3,     4,   109,   133,
     136,   144,   138,   144,   140,   144,   469,   390,   119,   145,
     148,   149,    10,    11,   150,   478,     3,     4,   232,   233,
     421,   243,   236,   243,    73,   136,   145,   138,    15,   140,
     148,   149,     7,   147,   145,   254,   144,   256,    28,     3,
     147,     5,     6,   145,    31,   145,   447,   145,   449,   145,
     269,   260,    42,   260,   455,   260,   145,   269,   277,   278,
     461,   341,   145,     4,     3,   145,   278,    10,    11,   470,
     145,   145,   145,   145,   475,   289,    63,   145,    19,   145,
      21,   145,   145,   145,   145,    75,    27,   145,    52,    76,
      31,   145,    82,   145,   145,   145,    92,    10,    11,    86,
     145,   145,     7,    44,   147,   147,   147,    48,    49,   146,
     145,    98,    53,   151,    55,   329,   103,     7,     7,   325,
       7,    62,   336,     7,   333,    55,   333,     8,   333,     5,
     144,   345,    73,    74,   149,   145,   345,   124,   418,    80,
     127,   133,   356,   147,   146,   109,   131,   148,   151,   131,
     145,   138,   146,   146,     3,   119,     7,   146,   145,   131,
     146,   152,   146,   104,   105,   106,   107,   146,     3,   110,
       5,     6,   136,   114,   138,   146,   140,   146,   146,   146,
     121,   145,   123,   146,   146,   146,   146,   128,   129,   132,
     146,   134,   135,   136,   137,   138,   139,   146,   146,   146,
     146,   146,   133,   146,   144,    68,   144,   144,   149,   144,
     146,     5,     4,   146,   146,   144,   112,    52,   146,   132,
     146,   134,   135,   136,   137,   138,   139,    19,   144,    21,
     439,   146,   439,   146,   439,    27,   146,   149,   147,    31,
     146,   145,   149,   131,   146,   146,   458,   133,    21,    10,
      11,   341,    44,   332,   468,   212,    48,    49,     2,     2,
     477,    53,   481,    55,   230,   477,   331,     2,   487,   481,
      62,   270,    -1,   485,   109,   487,   228,   489,    -1,    -1,
      -1,    73,    74,    -1,   119,    -1,    -1,    -1,    80,    -1,
      -1,    -1,    -1,     3,     4,     5,     6,    -1,    -1,    -1,
      -1,   136,    -1,   138,    -1,   140,    -1,    -1,    -1,    19,
     145,    21,   104,   105,   106,   107,    -1,    27,   110,    10,
      11,    31,   114,    -1,    -1,    -1,    -1,     9,    -1,   121,
      -1,   123,    10,    11,    44,    -1,   128,   129,    48,    49,
      -1,    -1,    52,    53,    -1,    55,    28,    -1,    -1,    31,
      -1,    -1,    62,    -1,    36,    -1,    -1,   149,    -1,    -1,
      42,    -1,    -1,    73,    74,    -1,    48,    -1,    -1,    51,
      80,   132,   133,   134,   135,   136,   137,   138,   139,    -1,
       3,     4,    -1,    -1,    -1,    -1,    68,    69,    -1,    -1,
      -1,    -1,    15,    75,   104,   105,   106,   107,    -1,   109,
     110,    -1,    -1,    -1,   114,    -1,    -1,    -1,    -1,   119,
      -1,   121,    -1,   123,    -1,    -1,    -1,    -1,   128,   129,
      -1,    -1,    -1,    -1,    -1,    -1,   136,    -1,   138,    -1,
     140,    -1,   114,    -1,    -1,   145,    -1,    -1,   120,   121,
      63,   132,   133,   134,   135,   136,   137,   138,   139,     4,
      -1,    -1,    -1,    76,   132,    -1,   134,   135,   136,   137,
     138,   139,   144,    86,    19,    -1,    21,   149,    -1,    -1,
      -1,    -1,    27,    -1,    -1,    98,    31,    -1,    -1,    -1,
     103,    -1,    -1,    28,    -1,    -1,    31,    -1,    -1,    44,
      -1,    36,    -1,    48,    49,    -1,    -1,    42,    53,    -1,
      55,   124,    -1,    48,   127,    -1,    51,    62,    -1,    -1,
      -1,    -1,    -1,    -1,    28,    -1,    -1,    31,    73,    74,
      -1,    -1,    36,    68,    69,    80,    -1,    -1,    42,    -1,
      75,    -1,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   104,
     105,   106,   107,    -1,    68,   110,    -1,    -1,    -1,   114,
      -1,    75,    -1,    -1,    -1,    -1,   121,    -1,   123,   114,
      -1,    -1,    -1,   128,   129,   120,   121,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   144,
     114,    -1,    -1,   148,   149,    -1,   120,   121,    -1,    -1,
      -1,    12,    -1,    14,    15,    -1,    17,    18,    -1,    -1,
      -1,    -1,    23,    -1,    25,    -1,    -1,    -1,    -1,    -1,
     144,    32,    33,    34,    35,   149,    37,    38,    39,    40,
      41,    -1,    43,    -1,    45,    46,    47,    -1,    -1,    50,
      -1,    -1,    -1,    54,    -1,    56,    57,    58,    59,    60,
      61,    -1,    63,    64,    65,    66,    67,    -1,    -1,    70,
      -1,    72,    -1,    -1,    -1,    -1,    77,    -1,    79,    -1,
      -1,    -1,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,    -1,    -1,    -1,    -1,   108,    -1,    -1,
     111,    -1,   113,    31,    -1,   116,   117,   118,    36,    -1,
      -1,   122,    31,   124,   125,   126,   127,    36,    -1,   130,
      48,    31,    -1,    51,    -1,    -1,    36,    -1,    -1,    48,
      -1,    -1,    51,    -1,    -1,    -1,    -1,    -1,    48,    -1,
      68,    51,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    68,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    68,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   114,    -1,    -1,    -1,
      -1,    -1,   120,   121,    -1,   114,    -1,    -1,    -1,    -1,
      -1,   120,   121,    -1,   114,    -1,    -1,    -1,    -1,    -1,
     120,   121,    -1,    -1,    -1,    -1,   144,    -1,    -1,    -1,
     148,   149,    -1,    -1,    -1,   144,    -1,    -1,    -1,   148,
     149,    -1,    -1,    -1,   144,    -1,    -1,    -1,   148,   149
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,   154,   155,     0,    28,    31,    36,    42,    48,    51,
      68,    75,   114,   120,   121,   144,   149,   158,   159,   160,
     161,   164,   165,   173,   179,   183,   189,   200,   201,   202,
     205,   206,   209,   211,   212,   213,   214,   216,   217,   222,
     224,   225,     3,     4,     4,    19,    21,    27,    31,    44,
      48,    49,    53,    55,    62,    73,    74,    80,   104,   105,
     106,   107,   110,   114,   121,   123,   128,   129,   183,   197,
     199,   222,   223,   225,   145,     3,     4,     3,     4,   195,
      31,     7,     3,     4,   195,   172,   173,   195,    12,    14,
      15,    17,    18,    23,    25,    32,    33,    34,    35,    37,
      38,    39,    40,    41,    43,    45,    46,    47,    50,    54,
      56,    57,    58,    59,    60,    61,    63,    64,    65,    66,
      67,    70,    72,    77,    79,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   108,   111,   113,   116,   117,
     118,   122,   124,   125,   126,   127,   130,   174,   175,   221,
     156,   156,    78,    82,   163,   200,   205,   211,   215,   144,
     144,   144,   144,   147,   144,   147,   144,   133,   210,   147,
     144,   144,   144,   223,     3,    73,   198,   198,   145,   198,
     199,     3,     3,   199,     3,     4,    15,    63,    76,    86,
      98,   103,   124,   127,   196,     7,   147,   223,   144,   147,
     223,   115,   147,   145,   145,   145,   145,   145,   145,   145,
     145,   145,   145,   145,   145,   145,   145,   145,   145,   145,
     145,   145,   145,   145,   145,   145,   145,   131,   150,     9,
      69,   158,   162,   173,   200,   202,   213,   214,   217,   148,
       3,     3,     4,   147,   203,    92,   207,     3,     4,   147,
     157,   223,   151,   146,   180,   181,   182,   196,   196,   190,
      31,   138,   145,   196,   218,   219,   220,   145,   190,   196,
       3,     5,     6,    52,   109,   119,   136,   138,   140,   145,
     186,   187,   188,     7,   188,     7,     7,     7,   188,   188,
       7,     7,   188,     7,   188,   196,    55,   184,   185,   186,
     221,   188,   184,   186,   223,   223,     8,     5,   226,   223,
     175,   149,   145,   144,   148,   172,   204,   133,    81,   172,
     192,   208,   147,   157,   148,   158,   172,   193,   146,   188,
     148,   131,   151,   144,   148,   172,   173,   191,   192,   218,
     219,   219,   131,   192,   148,   146,   145,   186,   186,   186,
     186,   223,    10,    11,   132,   134,   135,   136,   137,   138,
     139,   131,   146,   146,   146,   146,   146,   146,   146,   146,
     146,   146,   146,   146,   146,   146,     3,   131,   146,   146,
     131,   146,   146,   146,   146,   146,   152,   146,   146,   175,
       7,   214,   133,   223,   144,   148,   193,   161,   148,   223,
     144,   182,   188,   225,   144,   144,   146,   219,   146,   223,
     146,   146,   186,   186,   186,   186,   186,   186,   186,   186,
     186,   188,   146,   185,   188,     5,   146,   219,   144,   157,
     112,   176,   144,   194,   196,   146,   186,   133,   146,   149,
     170,   148,   219,   147,   186,   138,   171,   185,   145,   177,
     150,   131,   150,   128,   166,   167,   168,   169,   173,   223,
      25,    37,   148,   178,   186,   149,   146,   131,   223,   219,
     186,   133,   186,   169,   219,   145,   170,   133,   191,   145,
     170,   166,   191,   166,   146,   146
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
#line 239 "parser.y"
    { write_proxies(yyvsp[0].ifref); write_client(yyvsp[0].ifref); write_server(yyvsp[0].ifref); ;}
    break;

  case 3:
#line 242 "parser.y"
    { yyval.ifref = NULL; ;}
    break;

  case 4:
#line 243 "parser.y"
    { yyval.ifref = yyvsp[-1].ifref; ;}
    break;

  case 5:
#line 244 "parser.y"
    { yyval.ifref = make_ifref(yyvsp[0].type); LINK(yyval.ifref, yyvsp[-1].ifref); ;}
    break;

  case 6:
#line 245 "parser.y"
    { yyval.ifref = yyvsp[-2].ifref;
						  reg_type(yyvsp[-1].type, yyvsp[-1].type->name, 0);
						  if (!parse_only && do_header) write_coclass_forward(yyvsp[-1].type);
						;}
    break;

  case 7:
#line 249 "parser.y"
    { yyval.ifref = yyvsp[-1].ifref;
						  add_coclass(yyvsp[0].type);
						  reg_type(yyvsp[0].type, yyvsp[0].type->name, 0);
						;}
    break;

  case 8:
#line 253 "parser.y"
    { yyval.ifref = yyvsp[-1].ifref; add_module(yyvsp[0].type); ;}
    break;

  case 9:
#line 254 "parser.y"
    { yyval.ifref = yyvsp[-1].ifref; ;}
    break;

  case 10:
#line 255 "parser.y"
    { yyval.ifref = yyvsp[-1].ifref; ;}
    break;

  case 11:
#line 258 "parser.y"
    {;}
    break;

  case 12:
#line 259 "parser.y"
    { if (!parse_only) add_interface(yyvsp[0].type); ;}
    break;

  case 13:
#line 260 "parser.y"
    { if (!parse_only) add_interface(yyvsp[0].type); ;}
    break;

  case 14:
#line 261 "parser.y"
    { reg_type(yyvsp[-1].type, yyvsp[-1].type->name, 0); if (!parse_only && do_header) write_coclass_forward(yyvsp[-1].type); ;}
    break;

  case 15:
#line 262 "parser.y"
    { if (!parse_only) add_coclass(yyvsp[0].type);
						  reg_type(yyvsp[0].type, yyvsp[0].type->name, 0);
						;}
    break;

  case 16:
#line 265 "parser.y"
    { if (!parse_only) add_module(yyvsp[0].type); ;}
    break;

  case 17:
#line 266 "parser.y"
    {;}
    break;

  case 18:
#line 267 "parser.y"
    {;}
    break;

  case 19:
#line 270 "parser.y"
    { yyval.func = NULL; ;}
    break;

  case 20:
#line 271 "parser.y"
    { yyval.func = yyvsp[-1].func; LINK(yyval.func, yyvsp[-2].func); ;}
    break;

  case 21:
#line 272 "parser.y"
    { yyval.func = yyvsp[-1].func; ;}
    break;

  case 22:
#line 275 "parser.y"
    {;}
    break;

  case 23:
#line 276 "parser.y"
    { if (!parse_only && do_header) { write_constdef(yyvsp[-1].var); } ;}
    break;

  case 24:
#line 277 "parser.y"
    {;}
    break;

  case 25:
#line 278 "parser.y"
    { if (!parse_only && do_header) { write_type(header, yyvsp[-1].type, NULL, NULL); fprintf(header, ";\n\n"); } ;}
    break;

  case 26:
#line 279 "parser.y"
    { if (!parse_only && do_header) { write_externdef(yyvsp[-1].var); } ;}
    break;

  case 27:
#line 280 "parser.y"
    {;}
    break;

  case 28:
#line 281 "parser.y"
    { if (!parse_only && do_header) { write_type(header, yyvsp[-1].type, NULL, NULL); fprintf(header, ";\n\n"); } ;}
    break;

  case 29:
#line 282 "parser.y"
    {;}
    break;

  case 30:
#line 283 "parser.y"
    { if (!parse_only && do_header) { write_type(header, yyvsp[-1].type, NULL, NULL); fprintf(header, ";\n\n"); } ;}
    break;

  case 31:
#line 286 "parser.y"
    { if (!parse_only && do_header) fprintf(header, "%s\n", yyvsp[-1].str); ;}
    break;

  case 32:
#line 288 "parser.y"
    { assert(yychar == YYEMPTY);
						  if (!do_import(yyvsp[-1].str)) yychar = aEOF; ;}
    break;

  case 33:
#line 291 "parser.y"
    {;}
    break;

  case 34:
#line 294 "parser.y"
    { if(!parse_only) add_importlib(yyvsp[-1].str); ;}
    break;

  case 35:
#line 297 "parser.y"
    { yyval.str = yyvsp[0].str; ;}
    break;

  case 36:
#line 299 "parser.y"
    { start_typelib(yyvsp[-1].str, yyvsp[-2].attr);
						  if (!parse_only && do_header) write_library(yyvsp[-1].str, yyvsp[-2].attr); ;}
    break;

  case 37:
#line 302 "parser.y"
    { end_typelib(); ;}
    break;

  case 38:
#line 305 "parser.y"
    { yyval.var = NULL; ;}
    break;

  case 40:
#line 309 "parser.y"
    { yyval.var = NULL; ;}
    break;

  case 42:
#line 313 "parser.y"
    { LINK(yyvsp[0].var, yyvsp[-2].var); yyval.var = yyvsp[0].var; ;}
    break;

  case 44:
#line 318 "parser.y"
    { yyval.var = yyvsp[-1].var;
						  set_type(yyval.var, yyvsp[-2].tref, yyvsp[0].expr);
						  yyval.var->attrs = yyvsp[-3].attr;
						;}
    break;

  case 45:
#line 322 "parser.y"
    { yyval.var = yyvsp[-1].var;
						  set_type(yyval.var, yyvsp[-2].tref, yyvsp[0].expr);
						;}
    break;

  case 46:
#line 325 "parser.y"
    { yyval.var = yyvsp[-3].var;
						  yyval.var->ptr_level--;
						  set_type(yyval.var, yyvsp[-4].tref, NULL);
						  yyval.var->attrs = yyvsp[-5].attr;
						  yyval.var->args = yyvsp[-1].var;
						;}
    break;

  case 47:
#line 331 "parser.y"
    { yyval.var = yyvsp[-3].var;
						  yyval.var->ptr_level--;
						  set_type(yyval.var, yyvsp[-4].tref, NULL);
						  yyval.var->args = yyvsp[-1].var;
						;}
    break;

  case 48:
#line 338 "parser.y"
    { yyval.expr = NULL; ;}
    break;

  case 49:
#line 339 "parser.y"
    { yyval.expr = yyvsp[-1].expr; ;}
    break;

  case 50:
#line 340 "parser.y"
    { yyval.expr = make_expr(EXPR_VOID); ;}
    break;

  case 52:
#line 344 "parser.y"
    { LINK(yyvsp[0].expr, yyvsp[-2].expr); yyval.expr = yyvsp[0].expr; ;}
    break;

  case 53:
#line 345 "parser.y"
    { LINK(yyvsp[0].expr, yyvsp[-3].expr); yyval.expr = yyvsp[0].expr; ;}
    break;

  case 54:
#line 348 "parser.y"
    { yyval.attr = NULL; ;}
    break;

  case 56:
#line 353 "parser.y"
    { yyval.attr = yyvsp[-1].attr;
						  if (!yyval.attr)
						    yyerror("empty attribute lists unsupported");
						;}
    break;

  case 58:
#line 360 "parser.y"
    { if (yyvsp[0].attr) { LINK(yyvsp[0].attr, yyvsp[-2].attr); yyval.attr = yyvsp[0].attr; }
						  else { yyval.attr = yyvsp[-2].attr; }
						;}
    break;

  case 59:
#line 363 "parser.y"
    { if (yyvsp[0].attr) { LINK(yyvsp[0].attr, yyvsp[-3].attr); yyval.attr = yyvsp[0].attr; }
						  else { yyval.attr = yyvsp[-3].attr; }
						;}
    break;

  case 60:
#line 368 "parser.y"
    { yyval.attr = NULL; ;}
    break;

  case 61:
#line 369 "parser.y"
    { yyval.attr = make_attr(ATTR_AGGREGATABLE); ;}
    break;

  case 62:
#line 370 "parser.y"
    { yyval.attr = make_attr(ATTR_APPOBJECT); ;}
    break;

  case 63:
#line 371 "parser.y"
    { yyval.attr = make_attr(ATTR_ASYNC); ;}
    break;

  case 64:
#line 372 "parser.y"
    { yyval.attr = make_attr(ATTR_AUTO_HANDLE); ;}
    break;

  case 65:
#line 373 "parser.y"
    { yyval.attr = make_attr(ATTR_BINDABLE); ;}
    break;

  case 66:
#line 374 "parser.y"
    { yyval.attr = make_attrp(ATTR_CALLAS, yyvsp[-1].var); ;}
    break;

  case 67:
#line 375 "parser.y"
    { yyval.attr = make_attrp(ATTR_CASE, yyvsp[-1].expr); ;}
    break;

  case 68:
#line 376 "parser.y"
    { yyval.attr = make_attrv(ATTR_CONTEXTHANDLE, 0); ;}
    break;

  case 69:
#line 377 "parser.y"
    { yyval.attr = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_DONT_SERIALIZE */ ;}
    break;

  case 70:
#line 378 "parser.y"
    { yyval.attr = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_SERIALIZE */ ;}
    break;

  case 71:
#line 379 "parser.y"
    { yyval.attr = make_attr(ATTR_CONTROL); ;}
    break;

  case 72:
#line 380 "parser.y"
    { yyval.attr = make_attr(ATTR_DEFAULT); ;}
    break;

  case 73:
#line 381 "parser.y"
    { yyval.attr = make_attr(ATTR_DEFAULTCOLLELEM); ;}
    break;

  case 74:
#line 382 "parser.y"
    { yyval.attr = make_attrp(ATTR_DEFAULTVALUE_EXPR, yyvsp[-1].expr); ;}
    break;

  case 75:
#line 383 "parser.y"
    { yyval.attr = make_attrp(ATTR_DEFAULTVALUE_STRING, yyvsp[-1].str); ;}
    break;

  case 76:
#line 384 "parser.y"
    { yyval.attr = make_attr(ATTR_DEFAULTVTABLE); ;}
    break;

  case 77:
#line 385 "parser.y"
    { yyval.attr = make_attr(ATTR_DISPLAYBIND); ;}
    break;

  case 78:
#line 386 "parser.y"
    { yyval.attr = make_attrp(ATTR_DLLNAME, yyvsp[-1].str); ;}
    break;

  case 79:
#line 387 "parser.y"
    { yyval.attr = make_attr(ATTR_DUAL); ;}
    break;

  case 80:
#line 388 "parser.y"
    { yyval.attr = make_attrp(ATTR_ENDPOINT, yyvsp[-1].str); ;}
    break;

  case 81:
#line 389 "parser.y"
    { yyval.attr = make_attrp(ATTR_ENTRY_STRING, yyvsp[-1].str); ;}
    break;

  case 82:
#line 390 "parser.y"
    { yyval.attr = make_attrp(ATTR_ENTRY_ORDINAL, yyvsp[-1].expr); ;}
    break;

  case 83:
#line 391 "parser.y"
    { yyval.attr = make_attr(ATTR_EXPLICIT_HANDLE); ;}
    break;

  case 84:
#line 392 "parser.y"
    { yyval.attr = make_attr(ATTR_HANDLE); ;}
    break;

  case 85:
#line 393 "parser.y"
    { yyval.attr = make_attrp(ATTR_HELPCONTEXT, yyvsp[-1].expr); ;}
    break;

  case 86:
#line 394 "parser.y"
    { yyval.attr = make_attrp(ATTR_HELPFILE, yyvsp[-1].str); ;}
    break;

  case 87:
#line 395 "parser.y"
    { yyval.attr = make_attrp(ATTR_HELPSTRING, yyvsp[-1].str); ;}
    break;

  case 88:
#line 396 "parser.y"
    { yyval.attr = make_attrp(ATTR_HELPSTRINGCONTEXT, yyvsp[-1].expr); ;}
    break;

  case 89:
#line 397 "parser.y"
    { yyval.attr = make_attrp(ATTR_HELPSTRINGDLL, yyvsp[-1].str); ;}
    break;

  case 90:
#line 398 "parser.y"
    { yyval.attr = make_attr(ATTR_HIDDEN); ;}
    break;

  case 91:
#line 399 "parser.y"
    { yyval.attr = make_attrp(ATTR_ID, yyvsp[-1].expr); ;}
    break;

  case 92:
#line 400 "parser.y"
    { yyval.attr = make_attr(ATTR_IDEMPOTENT); ;}
    break;

  case 93:
#line 401 "parser.y"
    { yyval.attr = make_attrp(ATTR_IIDIS, yyvsp[-1].var); ;}
    break;

  case 94:
#line 402 "parser.y"
    { yyval.attr = make_attr(ATTR_IMMEDIATEBIND); ;}
    break;

  case 95:
#line 403 "parser.y"
    { yyval.attr = make_attrp(ATTR_IMPLICIT_HANDLE, yyvsp[-1].str); ;}
    break;

  case 96:
#line 404 "parser.y"
    { yyval.attr = make_attr(ATTR_IN); ;}
    break;

  case 97:
#line 405 "parser.y"
    { yyval.attr = make_attr(ATTR_INPUTSYNC); ;}
    break;

  case 98:
#line 406 "parser.y"
    { yyval.attr = make_attrp(ATTR_LENGTHIS, yyvsp[-1].expr); ;}
    break;

  case 99:
#line 407 "parser.y"
    { yyval.attr = make_attr(ATTR_LOCAL); ;}
    break;

  case 100:
#line 408 "parser.y"
    { yyval.attr = make_attr(ATTR_NONBROWSABLE); ;}
    break;

  case 101:
#line 409 "parser.y"
    { yyval.attr = make_attr(ATTR_NONCREATABLE); ;}
    break;

  case 102:
#line 410 "parser.y"
    { yyval.attr = make_attr(ATTR_NONEXTENSIBLE); ;}
    break;

  case 103:
#line 411 "parser.y"
    { yyval.attr = make_attr(ATTR_OBJECT); ;}
    break;

  case 104:
#line 412 "parser.y"
    { yyval.attr = make_attr(ATTR_ODL); ;}
    break;

  case 105:
#line 413 "parser.y"
    { yyval.attr = make_attr(ATTR_OLEAUTOMATION); ;}
    break;

  case 106:
#line 414 "parser.y"
    { yyval.attr = make_attr(ATTR_OPTIONAL); ;}
    break;

  case 107:
#line 415 "parser.y"
    { yyval.attr = make_attr(ATTR_OUT); ;}
    break;

  case 108:
#line 416 "parser.y"
    { yyval.attr = make_attrv(ATTR_POINTERDEFAULT, yyvsp[-1].num); ;}
    break;

  case 109:
#line 417 "parser.y"
    { yyval.attr = make_attr(ATTR_PROPGET); ;}
    break;

  case 110:
#line 418 "parser.y"
    { yyval.attr = make_attr(ATTR_PROPPUT); ;}
    break;

  case 111:
#line 419 "parser.y"
    { yyval.attr = make_attr(ATTR_PROPPUTREF); ;}
    break;

  case 112:
#line 420 "parser.y"
    { yyval.attr = make_attr(ATTR_PUBLIC); ;}
    break;

  case 113:
#line 421 "parser.y"
    { LINK(yyvsp[-1].expr, yyvsp[-3].expr); yyval.attr = make_attrp(ATTR_RANGE, yyvsp[-1].expr); ;}
    break;

  case 114:
#line 422 "parser.y"
    { yyval.attr = make_attr(ATTR_READONLY); ;}
    break;

  case 115:
#line 423 "parser.y"
    { yyval.attr = make_attr(ATTR_REQUESTEDIT); ;}
    break;

  case 116:
#line 424 "parser.y"
    { yyval.attr = make_attr(ATTR_RESTRICTED); ;}
    break;

  case 117:
#line 425 "parser.y"
    { yyval.attr = make_attr(ATTR_RETVAL); ;}
    break;

  case 118:
#line 426 "parser.y"
    { yyval.attr = make_attrp(ATTR_SIZEIS, yyvsp[-1].expr); ;}
    break;

  case 119:
#line 427 "parser.y"
    { yyval.attr = make_attr(ATTR_SOURCE); ;}
    break;

  case 120:
#line 428 "parser.y"
    { yyval.attr = make_attr(ATTR_STRING); ;}
    break;

  case 121:
#line 429 "parser.y"
    { yyval.attr = make_attrp(ATTR_SWITCHIS, yyvsp[-1].expr); ;}
    break;

  case 122:
#line 430 "parser.y"
    { yyval.attr = make_attrp(ATTR_SWITCHTYPE, type_ref(yyvsp[-1].tref)); ;}
    break;

  case 123:
#line 431 "parser.y"
    { yyval.attr = make_attrp(ATTR_TRANSMITAS, type_ref(yyvsp[-1].tref)); ;}
    break;

  case 124:
#line 432 "parser.y"
    { yyval.attr = make_attrp(ATTR_UUID, yyvsp[-1].uuid); ;}
    break;

  case 125:
#line 433 "parser.y"
    { yyval.attr = make_attr(ATTR_V1ENUM); ;}
    break;

  case 126:
#line 434 "parser.y"
    { yyval.attr = make_attr(ATTR_VARARG); ;}
    break;

  case 127:
#line 435 "parser.y"
    { yyval.attr = make_attrv(ATTR_VERSION, yyvsp[-1].num); ;}
    break;

  case 128:
#line 436 "parser.y"
    { yyval.attr = make_attrp(ATTR_WIREMARSHAL, type_ref(yyvsp[-1].tref)); ;}
    break;

  case 129:
#line 437 "parser.y"
    { yyval.attr = make_attrv(ATTR_POINTERTYPE, yyvsp[0].num); ;}
    break;

  case 132:
#line 444 "parser.y"
    { yyval.var = NULL; ;}
    break;

  case 133:
#line 445 "parser.y"
    { if (yyvsp[0].var) { LINK(yyvsp[0].var, yyvsp[-1].var); yyval.var = yyvsp[0].var; }
						  else { yyval.var = yyvsp[-1].var; }
						;}
    break;

  case 134:
#line 450 "parser.y"
    { attr_t *a = make_attrp(ATTR_CASE, yyvsp[-2].expr);
						  yyval.var = yyvsp[0].var; if (!yyval.var) yyval.var = make_var(NULL);
						  LINK(a, yyval.var->attrs); yyval.var->attrs = a;
						;}
    break;

  case 135:
#line 454 "parser.y"
    { attr_t *a = make_attr(ATTR_DEFAULT);
						  yyval.var = yyvsp[0].var; if (!yyval.var) yyval.var = make_var(NULL);
						  LINK(a, yyval.var->attrs); yyval.var->attrs = a;
						;}
    break;

  case 136:
#line 460 "parser.y"
    { yyval.var = reg_const(yyvsp[-2].var);
						  set_type(yyval.var, yyvsp[-3].tref, NULL);
						  yyval.var->eval = yyvsp[0].expr;
						;}
    break;

  case 137:
#line 466 "parser.y"
    { yyval.var = NULL; ;}
    break;

  case 138:
#line 467 "parser.y"
    { yyval.var = yyvsp[-1].var; ;}
    break;

  case 140:
#line 471 "parser.y"
    { if (!yyval.var->eval)
						    yyval.var->eval = make_exprl(EXPR_NUM, 0 /* default for first enum entry */);
						;}
    break;

  case 141:
#line 474 "parser.y"
    { LINK(yyvsp[0].var, yyvsp[-2].var); yyval.var = yyvsp[0].var;
						  if (!yyval.var->eval)
						    yyval.var->eval = make_exprl(EXPR_NUM, yyvsp[-2].var->eval->cval + 1);
						;}
    break;

  case 142:
#line 480 "parser.y"
    { yyval.var = reg_const(yyvsp[-2].var);
						  yyval.var->eval = yyvsp[0].expr;
                                                  yyval.var->type = make_type(RPC_FC_LONG, &std_int);
						;}
    break;

  case 143:
#line 484 "parser.y"
    { yyval.var = reg_const(yyvsp[0].var);
                                                  yyval.var->type = make_type(RPC_FC_LONG, &std_int);
						;}
    break;

  case 144:
#line 489 "parser.y"
    { yyval.type = get_typev(RPC_FC_ENUM16, yyvsp[-3].var, tsENUM);
						  yyval.type->fields = yyvsp[-1].var;
						  yyval.type->defined = TRUE;
                                                  if(in_typelib)
                                                      add_enum(yyval.type);
						;}
    break;

  case 146:
#line 498 "parser.y"
    { LINK(yyvsp[0].expr, yyvsp[-2].expr); yyval.expr = yyvsp[0].expr; ;}
    break;

  case 147:
#line 511 "parser.y"
    { yyval.expr = make_expr(EXPR_VOID); ;}
    break;

  case 149:
#line 515 "parser.y"
    { yyval.expr = make_exprl(EXPR_NUM, yyvsp[0].num); ;}
    break;

  case 150:
#line 516 "parser.y"
    { yyval.expr = make_exprl(EXPR_HEXNUM, yyvsp[0].num); ;}
    break;

  case 151:
#line 517 "parser.y"
    { yyval.expr = make_exprl(EXPR_TRUEFALSE, 0); ;}
    break;

  case 152:
#line 518 "parser.y"
    { yyval.expr = make_exprl(EXPR_TRUEFALSE, 1); ;}
    break;

  case 153:
#line 519 "parser.y"
    { yyval.expr = make_exprs(EXPR_IDENTIFIER, yyvsp[0].str); ;}
    break;

  case 154:
#line 520 "parser.y"
    { yyval.expr = make_expr3(EXPR_COND, yyvsp[-4].expr, yyvsp[-2].expr, yyvsp[0].expr); ;}
    break;

  case 155:
#line 521 "parser.y"
    { yyval.expr = make_expr2(EXPR_OR , yyvsp[-2].expr, yyvsp[0].expr); ;}
    break;

  case 156:
#line 522 "parser.y"
    { yyval.expr = make_expr2(EXPR_AND, yyvsp[-2].expr, yyvsp[0].expr); ;}
    break;

  case 157:
#line 523 "parser.y"
    { yyval.expr = make_expr2(EXPR_ADD, yyvsp[-2].expr, yyvsp[0].expr); ;}
    break;

  case 158:
#line 524 "parser.y"
    { yyval.expr = make_expr2(EXPR_SUB, yyvsp[-2].expr, yyvsp[0].expr); ;}
    break;

  case 159:
#line 525 "parser.y"
    { yyval.expr = make_expr2(EXPR_MUL, yyvsp[-2].expr, yyvsp[0].expr); ;}
    break;

  case 160:
#line 526 "parser.y"
    { yyval.expr = make_expr2(EXPR_DIV, yyvsp[-2].expr, yyvsp[0].expr); ;}
    break;

  case 161:
#line 527 "parser.y"
    { yyval.expr = make_expr2(EXPR_SHL, yyvsp[-2].expr, yyvsp[0].expr); ;}
    break;

  case 162:
#line 528 "parser.y"
    { yyval.expr = make_expr2(EXPR_SHR, yyvsp[-2].expr, yyvsp[0].expr); ;}
    break;

  case 163:
#line 529 "parser.y"
    { yyval.expr = make_expr1(EXPR_NOT, yyvsp[0].expr); ;}
    break;

  case 164:
#line 530 "parser.y"
    { yyval.expr = make_expr1(EXPR_NEG, yyvsp[0].expr); ;}
    break;

  case 165:
#line 531 "parser.y"
    { yyval.expr = make_expr1(EXPR_PPTR, yyvsp[0].expr); ;}
    break;

  case 166:
#line 532 "parser.y"
    { yyval.expr = make_exprt(EXPR_CAST, yyvsp[-2].tref, yyvsp[0].expr); ;}
    break;

  case 167:
#line 533 "parser.y"
    { yyval.expr = make_exprt(EXPR_SIZEOF, yyvsp[-1].tref, NULL); ;}
    break;

  case 168:
#line 534 "parser.y"
    { yyval.expr = yyvsp[-1].expr; ;}
    break;

  case 170:
#line 538 "parser.y"
    { LINK(yyvsp[0].expr, yyvsp[-2].expr); yyval.expr = yyvsp[0].expr; ;}
    break;

  case 171:
#line 541 "parser.y"
    { yyval.expr = yyvsp[0].expr;
						  if (!yyval.expr->is_const)
						      yyerror("expression is not constant");
						;}
    break;

  case 172:
#line 547 "parser.y"
    { yyval.var = yyvsp[0].var;
						  set_type(yyval.var, yyvsp[-1].tref, NULL);
						;}
    break;

  case 173:
#line 552 "parser.y"
    { yyval.var = NULL; ;}
    break;

  case 174:
#line 553 "parser.y"
    { if (yyvsp[0].var) { LINK(yyvsp[0].var, yyvsp[-1].var); yyval.var = yyvsp[0].var; }
						  else { yyval.var = yyvsp[-1].var; }
						;}
    break;

  case 175:
#line 558 "parser.y"
    { yyval.var = yyvsp[-1].var; ;}
    break;

  case 176:
#line 559 "parser.y"
    { yyval.var = make_var(NULL); yyval.var->type = yyvsp[-1].type; yyval.var->attrs = yyvsp[-2].attr; ;}
    break;

  case 177:
#line 560 "parser.y"
    { yyval.var = make_var(NULL); yyval.var->attrs = yyvsp[-1].attr; ;}
    break;

  case 178:
#line 561 "parser.y"
    { yyval.var = NULL; ;}
    break;

  case 179:
#line 564 "parser.y"
    { yyval.var = yyvsp[-1].var; set_type(yyval.var, yyvsp[-2].tref, yyvsp[0].expr); yyval.var->attrs = yyvsp[-3].attr; ;}
    break;

  case 180:
#line 569 "parser.y"
    { set_type(yyvsp[-3].var, yyvsp[-5].tref, NULL);
						  yyvsp[-3].var->attrs = yyvsp[-6].attr;
						  yyval.func = make_func(yyvsp[-3].var, yyvsp[-1].var);
						  if (is_attr(yyvsp[-3].var->attrs, ATTR_IN)) {
						    yyerror("inapplicable attribute [in] for function '%s'",yyval.func->def->name);
						  }
						;}
    break;

  case 181:
#line 578 "parser.y"
    { yyval.var = NULL; ;}
    break;

  case 183:
#line 582 "parser.y"
    { yyval.var = NULL; ;}
    break;

  case 184:
#line 583 "parser.y"
    { yyval.var = make_var(yyvsp[0].str); ;}
    break;

  case 185:
#line 584 "parser.y"
    { yyval.var = make_var(yyvsp[0].str); ;}
    break;

  case 186:
#line 587 "parser.y"
    { yyval.var = make_var(yyvsp[0].str); ;}
    break;

  case 187:
#line 589 "parser.y"
    { yyval.var = make_var(yyvsp[0].str); ;}
    break;

  case 188:
#line 590 "parser.y"
    { yyval.var = make_var(yyvsp[0].str); ;}
    break;

  case 189:
#line 591 "parser.y"
    { yyval.var = make_var(yyvsp[0].str); ;}
    break;

  case 190:
#line 592 "parser.y"
    { yyval.var = make_var(yyvsp[0].str); ;}
    break;

  case 191:
#line 593 "parser.y"
    { yyval.var = make_var(yyvsp[0].str); ;}
    break;

  case 192:
#line 594 "parser.y"
    { yyval.var = make_var(yyvsp[0].str); ;}
    break;

  case 193:
#line 595 "parser.y"
    { yyval.var = make_var(yyvsp[0].str); ;}
    break;

  case 194:
#line 596 "parser.y"
    { yyval.var = make_var(yyvsp[0].str); ;}
    break;

  case 195:
#line 597 "parser.y"
    { yyval.var = make_var(yyvsp[0].str); ;}
    break;

  case 196:
#line 600 "parser.y"
    { yyval.type = make_type(RPC_FC_BYTE, NULL); ;}
    break;

  case 197:
#line 601 "parser.y"
    { yyval.type = make_type(RPC_FC_WCHAR, NULL); ;}
    break;

  case 199:
#line 603 "parser.y"
    { yyval.type = yyvsp[0].type; yyval.type->sign = 1; ;}
    break;

  case 200:
#line 604 "parser.y"
    { yyval.type = yyvsp[0].type; yyval.type->sign = -1;
						  switch (yyval.type->type) {
						  case RPC_FC_CHAR:  break;
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

  case 201:
#line 616 "parser.y"
    { yyval.type = make_type(RPC_FC_ULONG, &std_int); yyval.type->sign = -1; ;}
    break;

  case 202:
#line 617 "parser.y"
    { yyval.type = make_type(RPC_FC_FLOAT, NULL); ;}
    break;

  case 203:
#line 618 "parser.y"
    { yyval.type = make_type(RPC_FC_FLOAT, NULL); ;}
    break;

  case 204:
#line 619 "parser.y"
    { yyval.type = make_type(RPC_FC_DOUBLE, NULL); ;}
    break;

  case 205:
#line 620 "parser.y"
    { yyval.type = make_type(RPC_FC_BYTE, &std_bool); /* ? */ ;}
    break;

  case 206:
#line 621 "parser.y"
    { yyval.type = make_type(RPC_FC_ERROR_STATUS_T, NULL); ;}
    break;

  case 207:
#line 622 "parser.y"
    { yyval.type = make_type(RPC_FC_BIND_PRIMITIVE, NULL); /* ? */ ;}
    break;

  case 210:
#line 629 "parser.y"
    { yyval.type = make_type(RPC_FC_LONG, &std_int); ;}
    break;

  case 211:
#line 630 "parser.y"
    { yyval.type = make_type(RPC_FC_SHORT, NULL); ;}
    break;

  case 212:
#line 631 "parser.y"
    { yyval.type = make_type(RPC_FC_SMALL, NULL); ;}
    break;

  case 213:
#line 632 "parser.y"
    { yyval.type = make_type(RPC_FC_LONG, NULL); ;}
    break;

  case 214:
#line 633 "parser.y"
    { yyval.type = make_type(RPC_FC_HYPER, NULL); ;}
    break;

  case 215:
#line 634 "parser.y"
    { yyval.type = make_type(RPC_FC_HYPER, &std_int64); ;}
    break;

  case 216:
#line 635 "parser.y"
    { yyval.type = make_type(RPC_FC_CHAR, NULL); ;}
    break;

  case 217:
#line 638 "parser.y"
    { yyval.type = make_class(yyvsp[0].str); ;}
    break;

  case 218:
#line 639 "parser.y"
    { yyval.type = find_type(yyvsp[0].str, 0);
						  if (yyval.type->defined) yyerror("multiple definition error");
						  if (yyval.type->kind != TKIND_COCLASS) yyerror("%s was not declared a coclass", yyvsp[0].str);
						;}
    break;

  case 219:
#line 645 "parser.y"
    { yyval.type = yyvsp[0].type;
						  yyval.type->attrs = yyvsp[-1].attr;
						  if (!parse_only && do_header)
						    write_coclass(yyval.type);
						;}
    break;

  case 220:
#line 652 "parser.y"
    { yyval.type = yyvsp[-3].type;
						  yyval.type->ifaces = yyvsp[-1].ifref;
						;}
    break;

  case 221:
#line 657 "parser.y"
    { yyval.ifref = NULL; ;}
    break;

  case 222:
#line 658 "parser.y"
    { LINK(yyvsp[0].ifref, yyvsp[-1].ifref); yyval.ifref = yyvsp[0].ifref; ;}
    break;

  case 223:
#line 662 "parser.y"
    { yyval.ifref = make_ifref(yyvsp[0].type); yyval.ifref->attrs = yyvsp[-1].attr; ;}
    break;

  case 224:
#line 665 "parser.y"
    { yyval.type = get_type(0, yyvsp[0].str, 0); ;}
    break;

  case 225:
#line 666 "parser.y"
    { yyval.type = get_type(0, yyvsp[0].str, 0); ;}
    break;

  case 226:
#line 669 "parser.y"
    { attr_t *attrs;
						  yyval.type = yyvsp[0].type;
						  if (yyval.type->defined) yyerror("multiple definition error");
						  attrs = make_attr(ATTR_DISPINTERFACE);
						  LINK(attrs, yyvsp[-1].attr);
						  yyval.type->attrs = attrs;
						  yyval.type->ref = find_type("IDispatch", 0);
						  if (!yyval.type->ref) yyerror("IDispatch is undefined");
						  yyval.type->defined = TRUE;
						  if (!parse_only && do_header) write_forward(yyval.type);
						;}
    break;

  case 227:
#line 682 "parser.y"
    { yyval.var = NULL; ;}
    break;

  case 228:
#line 683 "parser.y"
    { LINK(yyvsp[-1].var, yyvsp[-2].var); yyval.var = yyvsp[-1].var; ;}
    break;

  case 229:
#line 686 "parser.y"
    { yyval.func = NULL; ;}
    break;

  case 230:
#line 687 "parser.y"
    { LINK(yyvsp[-1].func, yyvsp[-2].func); yyval.func = yyvsp[-1].func; ;}
    break;

  case 231:
#line 693 "parser.y"
    { yyval.type = yyvsp[-4].type;
						  yyval.type->fields = yyvsp[-2].var;
						  yyval.type->funcs = yyvsp[-1].func;
						  if (!parse_only && do_header) write_dispinterface(yyval.type);
						;}
    break;

  case 232:
#line 705 "parser.y"
    { yyval.type = NULL; ;}
    break;

  case 233:
#line 706 "parser.y"
    { yyval.type = find_type2(yyvsp[0].str, 0); ;}
    break;

  case 234:
#line 709 "parser.y"
    { yyval.type = get_type(RPC_FC_IP, yyvsp[0].str, 0); ;}
    break;

  case 235:
#line 710 "parser.y"
    { yyval.type = get_type(RPC_FC_IP, yyvsp[0].str, 0); ;}
    break;

  case 236:
#line 713 "parser.y"
    { yyval.type = yyvsp[0].type;
						  if (yyval.type->defined) yyerror("multiple definition error");
						  yyval.type->attrs = yyvsp[-1].attr;
						  yyval.type->defined = TRUE;
						  if (!parse_only && do_header) write_forward(yyval.type);
						;}
    break;

  case 237:
#line 722 "parser.y"
    { yyval.type = yyvsp[-4].type;
						  yyval.type->ref = yyvsp[-3].type;
						  yyval.type->funcs = yyvsp[-1].func;
						  if (!parse_only && do_header) write_interface(yyval.type);
						;}
    break;

  case 238:
#line 730 "parser.y"
    { yyval.type = yyvsp[-6].type;
						  yyval.type->ref = find_type2(yyvsp[-4].str, 0);
						  if (!yyval.type->ref) yyerror("base class '%s' not found in import", yyvsp[-4].str);
						  yyval.type->funcs = yyvsp[-1].func;
						  if (!parse_only && do_header) write_interface(yyval.type);
						;}
    break;

  case 239:
#line 736 "parser.y"
    { yyval.type = yyvsp[0].type; ;}
    break;

  case 240:
#line 740 "parser.y"
    { yyval.type = yyvsp[-1].type; if (!parse_only && do_header) write_forward(yyval.type); ;}
    break;

  case 241:
#line 741 "parser.y"
    { yyval.type = yyvsp[-1].type; if (!parse_only && do_header) write_forward(yyval.type); ;}
    break;

  case 242:
#line 744 "parser.y"
    { yyval.type = make_type(0, NULL); yyval.type->name = yyvsp[0].str; ;}
    break;

  case 243:
#line 745 "parser.y"
    { yyval.type = make_type(0, NULL); yyval.type->name = yyvsp[0].str; ;}
    break;

  case 244:
#line 748 "parser.y"
    { yyval.type = yyvsp[0].type;
						  yyval.type->attrs = yyvsp[-1].attr;
						;}
    break;

  case 245:
#line 753 "parser.y"
    { yyval.type = yyvsp[-3].type;
						  yyval.type->funcs = yyvsp[-1].func;
						  /* FIXME: if (!parse_only && do_header) write_module($$); */
						;}
    break;

  case 246:
#line 759 "parser.y"
    { yyval.var = yyvsp[0].var; yyval.var->ptr_level++; ;}
    break;

  case 247:
#line 760 "parser.y"
    { yyval.var = yyvsp[0].var; /* FIXME */ ;}
    break;

  case 250:
#line 765 "parser.y"
    { yyval.var = yyvsp[-1].var; ;}
    break;

  case 252:
#line 770 "parser.y"
    { LINK(yyvsp[0].var, yyvsp[-2].var); yyval.var = yyvsp[0].var; ;}
    break;

  case 253:
#line 774 "parser.y"
    { yyval.num = RPC_FC_RP; ;}
    break;

  case 254:
#line 775 "parser.y"
    { yyval.num = RPC_FC_UP; ;}
    break;

  case 255:
#line 776 "parser.y"
    { yyval.num = RPC_FC_FP; ;}
    break;

  case 256:
#line 779 "parser.y"
    { yyval.type = get_typev(RPC_FC_STRUCT, yyvsp[-3].var, tsSTRUCT);
                                                  /* overwrite RPC_FC_STRUCT with a more exact type */
						  yyval.type->type = get_struct_type( yyvsp[-1].var );
						  yyval.type->fields = yyvsp[-1].var;
						  yyval.type->defined = TRUE;
                                                  if(in_typelib)
                                                      add_struct(yyval.type);
                                                ;}
    break;

  case 257:
#line 789 "parser.y"
    { yyval.tref = make_tref(NULL, make_type(0, NULL)); ;}
    break;

  case 258:
#line 790 "parser.y"
    { yyval.tref = make_tref(yyvsp[0].str, find_type(yyvsp[0].str, 0)); ;}
    break;

  case 259:
#line 791 "parser.y"
    { yyval.tref = make_tref(NULL, yyvsp[0].type); ;}
    break;

  case 260:
#line 792 "parser.y"
    { yyval.tref = uniq_tref(yyvsp[0].tref); yyval.tref->ref->is_const = TRUE; ;}
    break;

  case 261:
#line 793 "parser.y"
    { yyval.tref = make_tref(NULL, yyvsp[0].type); ;}
    break;

  case 262:
#line 794 "parser.y"
    { yyval.tref = make_tref(NULL, find_type2(yyvsp[0].str, tsENUM)); ;}
    break;

  case 263:
#line 795 "parser.y"
    { yyval.tref = make_tref(NULL, yyvsp[0].type); ;}
    break;

  case 264:
#line 796 "parser.y"
    { yyval.tref = make_tref(NULL, get_type(RPC_FC_STRUCT, yyvsp[0].str, tsSTRUCT)); ;}
    break;

  case 265:
#line 797 "parser.y"
    { yyval.tref = make_tref(NULL, yyvsp[0].type); ;}
    break;

  case 266:
#line 798 "parser.y"
    { yyval.tref = make_tref(NULL, find_type2(yyvsp[0].str, tsUNION)); ;}
    break;

  case 267:
#line 799 "parser.y"
    { yyval.tref = make_tref(NULL, make_safearray()); ;}
    break;

  case 268:
#line 802 "parser.y"
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

  case 269:
#line 815 "parser.y"
    { yyval.type = get_typev(RPC_FC_NON_ENCAPSULATED_UNION, yyvsp[-3].var, tsUNION);
						  yyval.type->fields = yyvsp[-1].var;
						  yyval.type->defined = TRUE;
						;}
    break;

  case 270:
#line 821 "parser.y"
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

  case 271:
#line 833 "parser.y"
    { yyval.num = MAKELONG(yyvsp[0].num, 0); ;}
    break;

  case 272:
#line 834 "parser.y"
    { yyval.num = MAKELONG(yyvsp[-2].num, yyvsp[0].num); ;}
    break;


    }

/* Line 991 of yacc.c.  */
#line 3375 "parser.tab.c"

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


#line 837 "parser.y"


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
  INIT_LINK(e);
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
  if (type == EXPR_SIZEOF) {
    switch (tref->ref->type) {
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
  t->kind = TKIND_PRIMITIVE;
  t->type = type;
  t->ref = ref;
  t->attrs = NULL;
  t->funcs = NULL;
  t->fields = NULL;
  t->ifaces = NULL;
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

static type_t *make_class(char *name)
{
  type_t *c = make_type(0, NULL);
  c->name = name;
  c->kind = TKIND_COCLASS;
  INIT_LINK(c);
  return c;
}

static type_t *make_safearray(void)
{
  return make_type(RPC_FC_FP, find_type("SAFEARRAY", 0));
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
  return RPC_FC_FP;
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

static int get_struct_type(var_t *field)
{
  int has_pointer = 0;
  int has_conformance = 0;
  int has_variance = 0;

  for (; field; field = NEXT_LINK(field))
  {
    type_t *t = field->type;

    /* get the base type */
    while( (t->type == 0) && t->ref )
      t = t->ref;

    if (field->ptr_level > 0)
    {
        has_pointer = 1;
        continue;
    }

    if (is_string_type(field->attrs, 0, field->array))
    {
        has_conformance = 1;
        has_variance = 1;
        continue;
    }

    if (is_array_type(field->attrs, 0, field->array))
    {
        if (field->array && !field->array->is_const)
        {
            has_conformance = 1;
            if (PREV_LINK(field))
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

    case RPC_FC_UP:
    case RPC_FC_FP:
      has_pointer = 1;
      break;
    case RPC_FC_CARRAY:
      has_conformance = 1;
      if (PREV_LINK(field))
          yyerror("field '%s' deriving from a conformant array must be the last field in the structure",
                  field->name);
      break;
    case RPC_FC_C_CSTRING:
    case RPC_FC_C_WSTRING:
      has_conformance = 1;
      has_variance = 1;
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
      if (PREV_LINK(field))
          yyerror("field '%s' deriving from a conformant array must be the last field in the structure",
                  field->name);
      has_pointer = 1;
      break;

    case RPC_FC_CSTRUCT:
      has_conformance = 1;
      if (PREV_LINK(field))
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

