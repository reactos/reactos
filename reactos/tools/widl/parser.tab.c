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
     aWSTRING = 264,
     aUUID = 265,
     aEOF = 266,
     SHL = 267,
     SHR = 268,
     MEMBERPTR = 269,
     EQUALITY = 270,
     INEQUALITY = 271,
     GREATEREQUAL = 272,
     LESSEQUAL = 273,
     LOGICALOR = 274,
     LOGICALAND = 275,
     tAGGREGATABLE = 276,
     tALLOCATE = 277,
     tAPPOBJECT = 278,
     tASYNC = 279,
     tASYNCUUID = 280,
     tAUTOHANDLE = 281,
     tBINDABLE = 282,
     tBOOLEAN = 283,
     tBROADCAST = 284,
     tBYTE = 285,
     tBYTECOUNT = 286,
     tCALLAS = 287,
     tCALLBACK = 288,
     tCASE = 289,
     tCDECL = 290,
     tCHAR = 291,
     tCOCLASS = 292,
     tCODE = 293,
     tCOMMSTATUS = 294,
     tCONST = 295,
     tCONTEXTHANDLE = 296,
     tCONTEXTHANDLENOSERIALIZE = 297,
     tCONTEXTHANDLESERIALIZE = 298,
     tCONTROL = 299,
     tCPPQUOTE = 300,
     tDEFAULT = 301,
     tDEFAULTCOLLELEM = 302,
     tDEFAULTVALUE = 303,
     tDEFAULTVTABLE = 304,
     tDISPLAYBIND = 305,
     tDISPINTERFACE = 306,
     tDLLNAME = 307,
     tDOUBLE = 308,
     tDUAL = 309,
     tENDPOINT = 310,
     tENTRY = 311,
     tENUM = 312,
     tERRORSTATUST = 313,
     tEXPLICITHANDLE = 314,
     tEXTERN = 315,
     tFALSE = 316,
     tFASTCALL = 317,
     tFLOAT = 318,
     tHANDLE = 319,
     tHANDLET = 320,
     tHELPCONTEXT = 321,
     tHELPFILE = 322,
     tHELPSTRING = 323,
     tHELPSTRINGCONTEXT = 324,
     tHELPSTRINGDLL = 325,
     tHIDDEN = 326,
     tHYPER = 327,
     tID = 328,
     tIDEMPOTENT = 329,
     tIIDIS = 330,
     tIMMEDIATEBIND = 331,
     tIMPLICITHANDLE = 332,
     tIMPORT = 333,
     tIMPORTLIB = 334,
     tIN = 335,
     tIN_LINE = 336,
     tINLINE = 337,
     tINPUTSYNC = 338,
     tINT = 339,
     tINT64 = 340,
     tINTERFACE = 341,
     tLCID = 342,
     tLENGTHIS = 343,
     tLIBRARY = 344,
     tLOCAL = 345,
     tLONG = 346,
     tMETHODS = 347,
     tMODULE = 348,
     tNONBROWSABLE = 349,
     tNONCREATABLE = 350,
     tNONEXTENSIBLE = 351,
     tNULL = 352,
     tOBJECT = 353,
     tODL = 354,
     tOLEAUTOMATION = 355,
     tOPTIONAL = 356,
     tOUT = 357,
     tPASCAL = 358,
     tPOINTERDEFAULT = 359,
     tPROPERTIES = 360,
     tPROPGET = 361,
     tPROPPUT = 362,
     tPROPPUTREF = 363,
     tPTR = 364,
     tPUBLIC = 365,
     tRANGE = 366,
     tREADONLY = 367,
     tREF = 368,
     tREGISTER = 369,
     tREQUESTEDIT = 370,
     tRESTRICTED = 371,
     tRETVAL = 372,
     tSAFEARRAY = 373,
     tSHORT = 374,
     tSIGNED = 375,
     tSINGLE = 376,
     tSIZEIS = 377,
     tSIZEOF = 378,
     tSMALL = 379,
     tSOURCE = 380,
     tSTATIC = 381,
     tSTDCALL = 382,
     tSTRICTCONTEXTHANDLE = 383,
     tSTRING = 384,
     tSTRUCT = 385,
     tSWITCH = 386,
     tSWITCHIS = 387,
     tSWITCHTYPE = 388,
     tTRANSMITAS = 389,
     tTRUE = 390,
     tTYPEDEF = 391,
     tUNION = 392,
     tUNIQUE = 393,
     tUNSIGNED = 394,
     tUUID = 395,
     tV1ENUM = 396,
     tVARARG = 397,
     tVERSION = 398,
     tVOID = 399,
     tWCHAR = 400,
     tWIREMARSHAL = 401,
     ADDRESSOF = 402,
     NEG = 403,
     POS = 404,
     PPTR = 405,
     CAST = 406
   };
#endif
/* Tokens.  */
#define aIDENTIFIER 258
#define aKNOWNTYPE 259
#define aNUM 260
#define aHEXNUM 261
#define aDOUBLE 262
#define aSTRING 263
#define aWSTRING 264
#define aUUID 265
#define aEOF 266
#define SHL 267
#define SHR 268
#define MEMBERPTR 269
#define EQUALITY 270
#define INEQUALITY 271
#define GREATEREQUAL 272
#define LESSEQUAL 273
#define LOGICALOR 274
#define LOGICALAND 275
#define tAGGREGATABLE 276
#define tALLOCATE 277
#define tAPPOBJECT 278
#define tASYNC 279
#define tASYNCUUID 280
#define tAUTOHANDLE 281
#define tBINDABLE 282
#define tBOOLEAN 283
#define tBROADCAST 284
#define tBYTE 285
#define tBYTECOUNT 286
#define tCALLAS 287
#define tCALLBACK 288
#define tCASE 289
#define tCDECL 290
#define tCHAR 291
#define tCOCLASS 292
#define tCODE 293
#define tCOMMSTATUS 294
#define tCONST 295
#define tCONTEXTHANDLE 296
#define tCONTEXTHANDLENOSERIALIZE 297
#define tCONTEXTHANDLESERIALIZE 298
#define tCONTROL 299
#define tCPPQUOTE 300
#define tDEFAULT 301
#define tDEFAULTCOLLELEM 302
#define tDEFAULTVALUE 303
#define tDEFAULTVTABLE 304
#define tDISPLAYBIND 305
#define tDISPINTERFACE 306
#define tDLLNAME 307
#define tDOUBLE 308
#define tDUAL 309
#define tENDPOINT 310
#define tENTRY 311
#define tENUM 312
#define tERRORSTATUST 313
#define tEXPLICITHANDLE 314
#define tEXTERN 315
#define tFALSE 316
#define tFASTCALL 317
#define tFLOAT 318
#define tHANDLE 319
#define tHANDLET 320
#define tHELPCONTEXT 321
#define tHELPFILE 322
#define tHELPSTRING 323
#define tHELPSTRINGCONTEXT 324
#define tHELPSTRINGDLL 325
#define tHIDDEN 326
#define tHYPER 327
#define tID 328
#define tIDEMPOTENT 329
#define tIIDIS 330
#define tIMMEDIATEBIND 331
#define tIMPLICITHANDLE 332
#define tIMPORT 333
#define tIMPORTLIB 334
#define tIN 335
#define tIN_LINE 336
#define tINLINE 337
#define tINPUTSYNC 338
#define tINT 339
#define tINT64 340
#define tINTERFACE 341
#define tLCID 342
#define tLENGTHIS 343
#define tLIBRARY 344
#define tLOCAL 345
#define tLONG 346
#define tMETHODS 347
#define tMODULE 348
#define tNONBROWSABLE 349
#define tNONCREATABLE 350
#define tNONEXTENSIBLE 351
#define tNULL 352
#define tOBJECT 353
#define tODL 354
#define tOLEAUTOMATION 355
#define tOPTIONAL 356
#define tOUT 357
#define tPASCAL 358
#define tPOINTERDEFAULT 359
#define tPROPERTIES 360
#define tPROPGET 361
#define tPROPPUT 362
#define tPROPPUTREF 363
#define tPTR 364
#define tPUBLIC 365
#define tRANGE 366
#define tREADONLY 367
#define tREF 368
#define tREGISTER 369
#define tREQUESTEDIT 370
#define tRESTRICTED 371
#define tRETVAL 372
#define tSAFEARRAY 373
#define tSHORT 374
#define tSIGNED 375
#define tSINGLE 376
#define tSIZEIS 377
#define tSIZEOF 378
#define tSMALL 379
#define tSOURCE 380
#define tSTATIC 381
#define tSTDCALL 382
#define tSTRICTCONTEXTHANDLE 383
#define tSTRING 384
#define tSTRUCT 385
#define tSWITCH 386
#define tSWITCHIS 387
#define tSWITCHTYPE 388
#define tTRANSMITAS 389
#define tTRUE 390
#define tTYPEDEF 391
#define tUNION 392
#define tUNIQUE 393
#define tUNSIGNED 394
#define tUUID 395
#define tV1ENUM 396
#define tVARARG 397
#define tVERSION 398
#define tVOID 399
#define tWCHAR 400
#define tWIREMARSHAL 401
#define ADDRESSOF 402
#define NEG 403
#define POS 404
#define PPTR 405
#define CAST 406




/* Copy the first part of user declarations.  */
#line 1 "parser.y"

/*
 * IDL Compiler
 *
 * Copyright 2002 Ove Kaaven
 * Copyright 2006-2008 Robert Shearman
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
#include "expr.h"
#include "typetree.h"

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

struct _import_t
{
  char *name;
  int import_performed;
};

typedef struct _decl_spec_t
{
  type_t *type;
  attr_list_t *attrs;
  enum storage_class stgclass;
} decl_spec_t;

typelist_t incomplete_types = LIST_INIT(incomplete_types);

static void add_incomplete(type_t *t);
static void fix_incomplete(void);
static void fix_incomplete_types(type_t *complete_type);

static str_list_t *append_str(str_list_t *list, char *str);
static attr_list_t *append_attr(attr_list_t *list, attr_t *attr);
static attr_list_t *append_attr_list(attr_list_t *new_list, attr_list_t *old_list);
static decl_spec_t *make_decl_spec(type_t *type, decl_spec_t *left, decl_spec_t *right, attr_t *attr, enum storage_class stgclass);
static attr_t *make_attr(enum attr_type type);
static attr_t *make_attrv(enum attr_type type, unsigned long val);
static attr_t *make_attrp(enum attr_type type, void *val);
static expr_list_t *append_expr(expr_list_t *list, expr_t *expr);
static array_dims_t *append_array(array_dims_t *list, expr_t *expr);
static void set_type(var_t *v, decl_spec_t *decl_spec, const declarator_t *decl, int top);
static var_list_t *set_var_types(attr_list_t *attrs, decl_spec_t *decl_spec, declarator_list_t *decls);
static ifref_list_t *append_ifref(ifref_list_t *list, ifref_t *iface);
static ifref_t *make_ifref(type_t *iface);
static var_list_t *append_var(var_list_t *list, var_t *var);
static var_list_t *append_var_list(var_list_t *list, var_list_t *vars);
static var_t *make_var(char *name);
static declarator_list_t *append_declarator(declarator_list_t *list, declarator_t *p);
static declarator_t *make_declarator(var_t *var);
static func_list_t *append_func(func_list_t *list, func_t *func);
static func_t *make_func(var_t *def);
static type_t *make_class(char *name);
static type_t *make_safearray(type_t *type);
static type_t *make_builtin(char *name);
static type_t *make_int(int sign);
static typelib_t *make_library(const char *name, const attr_list_t *attrs);
static type_t *append_ptrchain_type(type_t *ptrchain, type_t *type);

static type_t *type_new_enum(char *name, var_list_t *enums);
static type_t *type_new_struct(char *name, int defined, var_list_t *fields);
static type_t *type_new_nonencapsulated_union(char *name, var_list_t *fields);
static type_t *type_new_encapsulated_union(char *name, var_t *switch_field, var_t *union_field, var_list_t *cases);

static type_t *reg_type(type_t *type, const char *name, int t);
static type_t *reg_typedefs(decl_spec_t *decl_spec, var_list_t *names, attr_list_t *attrs);
static type_t *find_type_or_error(const char *name, int t);
static type_t *find_type_or_error2(char *name, int t);
static type_t *get_type(unsigned char type, char *name, int t);

static var_t *reg_const(var_t *var);

static char *gen_name(void);
static void check_arg(var_t *arg);
static void check_statements(const statement_list_t *stmts, int is_inside_library);
static void check_all_user_types(const statement_list_t *stmts);
static attr_list_t *check_iface_attrs(const char *name, attr_list_t *attrs);
static attr_list_t *check_function_attrs(const char *name, attr_list_t *attrs);
static attr_list_t *check_typedef_attrs(attr_list_t *attrs);
static attr_list_t *check_enum_attrs(attr_list_t *attrs);
static attr_list_t *check_struct_attrs(attr_list_t *attrs);
static attr_list_t *check_union_attrs(attr_list_t *attrs);
static attr_list_t *check_field_attrs(const char *name, attr_list_t *attrs);
static attr_list_t *check_library_attrs(const char *name, attr_list_t *attrs);
static attr_list_t *check_dispiface_attrs(const char *name, attr_list_t *attrs);
static attr_list_t *check_module_attrs(const char *name, attr_list_t *attrs);
static attr_list_t *check_coclass_attrs(const char *name, attr_list_t *attrs);
const char *get_attr_display_name(enum attr_type type);
static void add_explicit_handle_if_necessary(var_t *func);
static void check_def(const type_t *t);

static statement_t *make_statement(enum statement_type type);
static statement_t *make_statement_type_decl(type_t *type);
static statement_t *make_statement_reference(type_t *type);
static statement_t *make_statement_declaration(var_t *var);
static statement_t *make_statement_library(typelib_t *typelib);
static statement_t *make_statement_cppquote(const char *str);
static statement_t *make_statement_importlib(const char *str);
static statement_t *make_statement_module(type_t *type);
static statement_t *make_statement_typedef(var_list_t *names);
static statement_t *make_statement_import(const char *str);
static statement_t *make_statement_typedef(var_list_t *names);
static statement_list_t *append_statement(statement_list_t *list, statement_t *stmt);

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
#line 176 "parser.y"
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
	declarator_t *declarator;
	declarator_list_t *declarator_list;
	func_t *func;
	func_list_t *func_list;
	statement_t *statement;
	statement_list_t *stmt_list;
	ifref_t *ifref;
	ifref_list_t *ifref_list;
	char *str;
	UUID *uuid;
	unsigned int num;
	double dbl;
	interface_info_t ifinfo;
	typelib_t *typelib;
	struct _import_t *import;
	struct _decl_spec_t *declspec;
	enum storage_class stgclass;
} YYSTYPE;
/* Line 196 of yacc.c.  */
#line 600 "parser.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */
#line 612 "parser.tab.c"

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
#define YYLAST   2034

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  176
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  88
/* YYNRULES -- Number of rules. */
#define YYNRULES  312
/* YYNRULES -- Number of states. */
#define YYNSTATES  548

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   406

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   160,     2,     2,     2,   159,   152,     2,
     171,   172,   157,   156,   147,   155,   167,   158,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   149,   170,
     153,   175,   154,   148,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   168,     2,   169,   151,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   173,   150,   174,   161,     2,     2,     2,
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
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   162,   163,   164,   165,   166
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,     5,     6,     9,    12,    16,    19,    22,
      25,    28,    29,    32,    35,    39,    42,    45,    48,    51,
      54,    55,    58,    59,    61,    63,    66,    69,    71,    74,
      76,    78,    80,    83,    86,    89,    94,    98,   102,   108,
     111,   115,   120,   121,   123,   125,   127,   131,   133,   137,
     140,   144,   148,   149,   151,   155,   157,   161,   166,   168,
     172,   173,   175,   177,   179,   181,   183,   185,   190,   195,
     197,   199,   201,   203,   205,   207,   212,   214,   216,   221,
     223,   228,   233,   235,   237,   242,   247,   252,   257,   262,
     264,   269,   271,   276,   278,   284,   286,   288,   293,   298,
     300,   302,   304,   306,   308,   310,   312,   314,   316,   321,
     323,   325,   327,   329,   336,   338,   340,   342,   344,   349,
     351,   353,   355,   360,   365,   370,   375,   377,   379,   384,
     389,   391,   393,   395,   397,   399,   401,   403,   404,   407,
     412,   416,   417,   420,   422,   424,   428,   432,   434,   440,
     442,   446,   447,   449,   451,   453,   455,   457,   459,   461,
     463,   465,   467,   473,   477,   481,   485,   489,   493,   497,
     501,   505,   509,   513,   517,   521,   525,   529,   533,   537,
     541,   545,   548,   551,   554,   557,   560,   563,   567,   571,
     576,   581,   586,   590,   592,   596,   598,   600,   601,   604,
     609,   613,   616,   619,   620,   623,   626,   628,   632,   636,
     640,   643,   644,   646,   647,   649,   651,   653,   655,   657,
     659,   661,   664,   667,   669,   671,   673,   675,   677,   679,
     681,   682,   684,   686,   689,   691,   694,   697,   699,   701,
     704,   707,   710,   716,   717,   720,   723,   726,   729,   732,
     735,   739,   742,   746,   752,   758,   759,   762,   765,   768,
     771,   778,   787,   790,   793,   796,   799,   802,   805,   811,
     813,   815,   817,   819,   821,   822,   825,   828,   832,   833,
     835,   838,   841,   844,   848,   851,   853,   855,   859,   862,
     867,   869,   873,   875,   879,   881,   883,   885,   891,   893,
     895,   897,   899,   902,   904,   907,   909,   912,   917,   922,
     928,   939,   941
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short int yyrhs[] =
{
     177,     0,    -1,   178,    -1,    -1,   178,   243,    -1,   178,
     242,    -1,   178,   229,   170,    -1,   178,   231,    -1,   178,
     246,    -1,   178,   190,    -1,   178,   182,    -1,    -1,   179,
     243,    -1,   179,   242,    -1,   179,   229,   170,    -1,   179,
     231,    -1,   179,   246,    -1,   179,   182,    -1,   179,   187,
      -1,   179,   190,    -1,    -1,   180,   182,    -1,    -1,   170,
      -1,   184,    -1,   183,   170,    -1,   222,   170,    -1,   186,
      -1,   261,   170,    -1,   208,    -1,   259,    -1,   262,    -1,
     197,   208,    -1,   197,   259,    -1,   197,   262,    -1,    45,
     171,     8,   172,    -1,    78,     8,   170,    -1,   185,   179,
      11,    -1,    79,   171,     8,   172,   181,    -1,    89,     3,
      -1,   197,   188,   173,    -1,   189,   179,   174,   181,    -1,
      -1,   193,    -1,   144,    -1,   194,    -1,   193,   147,   194,
      -1,   192,    -1,   197,   251,   254,    -1,   251,   254,    -1,
     168,   210,   169,    -1,   168,   157,   169,    -1,    -1,   197,
      -1,   168,   198,   169,    -1,   200,    -1,   198,   147,   200,
      -1,   198,   169,   168,   200,    -1,     8,    -1,   199,   147,
       8,    -1,    -1,    21,    -1,    23,    -1,    24,    -1,    26,
      -1,    27,    -1,    29,    -1,    32,   171,   225,   172,    -1,
      34,   171,   212,   172,    -1,    41,    -1,    42,    -1,    43,
      -1,    44,    -1,    46,    -1,    47,    -1,    48,   171,   214,
     172,    -1,    49,    -1,    50,    -1,    52,   171,     8,   172,
      -1,    54,    -1,    55,   171,   199,   172,    -1,    56,   171,
     214,   172,    -1,    59,    -1,    64,    -1,    66,   171,   213,
     172,    -1,    67,   171,     8,   172,    -1,    68,   171,     8,
     172,    -1,    69,   171,   213,   172,    -1,    70,   171,     8,
     172,    -1,    71,    -1,    73,   171,   213,   172,    -1,    74,
      -1,    75,   171,   211,   172,    -1,    76,    -1,    77,   171,
      65,     3,   172,    -1,    80,    -1,    83,    -1,    88,   171,
     209,   172,    -1,    87,   171,   213,   172,    -1,    90,    -1,
      94,    -1,    95,    -1,    96,    -1,    98,    -1,    99,    -1,
     100,    -1,   101,    -1,   102,    -1,   104,   171,   258,   172,
      -1,   106,    -1,   107,    -1,   108,    -1,   110,    -1,   111,
     171,   213,   147,   213,   172,    -1,   112,    -1,   115,    -1,
     116,    -1,   117,    -1,   122,   171,   209,   172,    -1,   125,
      -1,   128,    -1,   129,    -1,   132,   171,   211,   172,    -1,
     133,   171,   260,   172,    -1,   134,   171,   260,   172,    -1,
     140,   171,   201,   172,    -1,   141,    -1,   142,    -1,   143,
     171,   263,   172,    -1,   146,   171,   260,   172,    -1,   258,
      -1,    10,    -1,     8,    -1,    35,    -1,    62,    -1,   103,
      -1,   127,    -1,    -1,   203,   204,    -1,    34,   213,   149,
     219,    -1,    46,   149,   219,    -1,    -1,   206,   147,    -1,
     206,    -1,   207,    -1,   206,   147,   207,    -1,   225,   175,
     213,    -1,   225,    -1,    57,   224,   173,   205,   174,    -1,
     210,    -1,   209,   147,   210,    -1,    -1,   211,    -1,     5,
      -1,     6,    -1,     7,    -1,    61,    -1,    97,    -1,   135,
      -1,     8,    -1,     9,    -1,     3,    -1,   211,   148,   211,
     149,   211,    -1,   211,    19,   211,    -1,   211,    20,   211,
      -1,   211,   150,   211,    -1,   211,   151,   211,    -1,   211,
     152,   211,    -1,   211,    15,   211,    -1,   211,    16,   211,
      -1,   211,   154,   211,    -1,   211,   153,   211,    -1,   211,
      17,   211,    -1,   211,    18,   211,    -1,   211,    12,   211,
      -1,   211,    13,   211,    -1,   211,   156,   211,    -1,   211,
     155,   211,    -1,   211,   159,   211,    -1,   211,   157,   211,
      -1,   211,   158,   211,    -1,   160,   211,    -1,   161,   211,
      -1,   156,   211,    -1,   155,   211,    -1,   152,   211,    -1,
     157,   211,    -1,   211,    14,     3,    -1,   211,   167,     3,
      -1,   171,   260,   172,   211,    -1,   123,   171,   260,   172,
      -1,   211,   168,   211,   169,    -1,   171,   211,   172,    -1,
     213,    -1,   212,   147,   213,    -1,   211,    -1,   211,    -1,
      -1,   215,   216,    -1,   196,   251,   256,   170,    -1,   196,
     262,   170,    -1,   220,   170,    -1,   197,   170,    -1,    -1,
     218,   217,    -1,   220,   170,    -1,   170,    -1,   196,   251,
     254,    -1,   196,   251,   254,    -1,   197,   251,   257,    -1,
     251,   257,    -1,    -1,   225,    -1,    -1,     3,    -1,     4,
      -1,     3,    -1,     4,    -1,    30,    -1,   145,    -1,   228,
      -1,   120,   228,    -1,   139,   228,    -1,   139,    -1,    63,
      -1,   121,    -1,    53,    -1,    28,    -1,    58,    -1,    65,
      -1,    -1,    84,    -1,    84,    -1,   119,   227,    -1,   124,
      -1,    91,   227,    -1,    72,   227,    -1,    85,    -1,    36,
      -1,    37,     3,    -1,    37,     4,    -1,   197,   229,    -1,
     230,   173,   232,   174,   181,    -1,    -1,   232,   233,    -1,
     196,   243,    -1,    51,     3,    -1,    51,     4,    -1,   197,
     234,    -1,   105,   149,    -1,   236,   220,   170,    -1,    92,
     149,    -1,   237,   221,   170,    -1,   235,   173,   236,   237,
     174,    -1,   235,   173,   240,   170,   174,    -1,    -1,   149,
       4,    -1,    86,     3,    -1,    86,     4,    -1,   197,   240,
      -1,   241,   239,   173,   180,   174,   181,    -1,   241,   149,
       3,   173,   186,   180,   174,   181,    -1,   238,   181,    -1,
     240,   170,    -1,   234,   170,    -1,    93,     3,    -1,    93,
       4,    -1,   197,   244,    -1,   245,   173,   180,   174,   181,
      -1,    60,    -1,   126,    -1,   114,    -1,    82,    -1,    40,
      -1,    -1,   250,   249,    -1,   260,   252,    -1,   253,   260,
     252,    -1,    -1,   253,    -1,   249,   252,    -1,   248,   252,
      -1,   247,   252,    -1,   157,   250,   254,    -1,   202,   254,
      -1,   255,    -1,   225,    -1,   171,   254,   172,    -1,   255,
     195,    -1,   255,   171,   191,   172,    -1,   254,    -1,   256,
     147,   254,    -1,   254,    -1,   254,   175,   214,    -1,   113,
      -1,   138,    -1,   109,    -1,   130,   224,   173,   215,   174,
      -1,   144,    -1,     4,    -1,   226,    -1,   208,    -1,    57,
       3,    -1,   259,    -1,   130,     3,    -1,   262,    -1,   137,
       3,    -1,   118,   171,   260,   172,    -1,   136,   196,   251,
     256,    -1,   137,   224,   173,   218,   174,    -1,   137,   224,
     131,   171,   220,   172,   223,   173,   203,   174,    -1,     5,
      -1,     5,   167,     5,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   347,   347,   360,   361,   362,   363,   366,   369,   370,
     371,   374,   375,   376,   377,   378,   381,   382,   383,   384,
     387,   388,   391,   392,   396,   397,   398,   399,   400,   404,
     405,   406,   407,   408,   409,   412,   414,   422,   428,   432,
     434,   438,   445,   446,   449,   452,   453,   454,   458,   465,
     473,   474,   477,   478,   482,   485,   486,   487,   490,   491,
     494,   495,   496,   497,   498,   499,   500,   501,   502,   503,
     504,   505,   506,   507,   508,   509,   510,   511,   512,   513,
     514,   515,   516,   517,   518,   519,   520,   521,   522,   523,
     524,   525,   526,   527,   528,   529,   530,   531,   532,   533,
     534,   535,   536,   537,   538,   539,   540,   541,   542,   543,
     544,   545,   546,   547,   551,   552,   553,   554,   555,   556,
     557,   558,   559,   560,   561,   562,   563,   564,   565,   566,
     567,   571,   572,   577,   578,   579,   580,   583,   584,   587,
     591,   597,   598,   599,   602,   606,   615,   619,   624,   627,
     628,   641,   642,   645,   646,   647,   648,   649,   650,   651,
     652,   653,   654,   655,   656,   657,   658,   659,   660,   661,
     662,   663,   664,   665,   666,   667,   668,   669,   670,   671,
     672,   673,   674,   675,   676,   677,   678,   679,   680,   681,
     682,   683,   684,   687,   688,   691,   697,   703,   704,   707,
     712,   719,   720,   723,   724,   728,   729,   732,   740,   749,
     755,   761,   762,   765,   766,   767,   770,   772,   775,   776,
     777,   778,   779,   795,   796,   797,   798,   799,   800,   801,
     804,   805,   808,   809,   810,   811,   812,   813,   814,   817,
     818,   826,   832,   836,   837,   841,   844,   845,   848,   858,
     859,   862,   863,   866,   872,   878,   879,   882,   883,   886,
     897,   904,   910,   914,   915,   918,   919,   922,   927,   934,
     935,   936,   940,   944,   947,   948,   951,   952,   956,   957,
     961,   962,   963,   967,   969,   970,   974,   975,   976,   977,
     984,   985,   989,   990,   994,   995,   996,   999,  1002,  1003,
    1004,  1005,  1006,  1007,  1008,  1009,  1010,  1011,  1014,  1020,
    1022,  1028,  1029
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "aIDENTIFIER", "aKNOWNTYPE", "aNUM",
  "aHEXNUM", "aDOUBLE", "aSTRING", "aWSTRING", "aUUID", "aEOF", "SHL",
  "SHR", "MEMBERPTR", "EQUALITY", "INEQUALITY", "GREATEREQUAL",
  "LESSEQUAL", "LOGICALOR", "LOGICALAND", "tAGGREGATABLE", "tALLOCATE",
  "tAPPOBJECT", "tASYNC", "tASYNCUUID", "tAUTOHANDLE", "tBINDABLE",
  "tBOOLEAN", "tBROADCAST", "tBYTE", "tBYTECOUNT", "tCALLAS", "tCALLBACK",
  "tCASE", "tCDECL", "tCHAR", "tCOCLASS", "tCODE", "tCOMMSTATUS", "tCONST",
  "tCONTEXTHANDLE", "tCONTEXTHANDLENOSERIALIZE", "tCONTEXTHANDLESERIALIZE",
  "tCONTROL", "tCPPQUOTE", "tDEFAULT", "tDEFAULTCOLLELEM", "tDEFAULTVALUE",
  "tDEFAULTVTABLE", "tDISPLAYBIND", "tDISPINTERFACE", "tDLLNAME",
  "tDOUBLE", "tDUAL", "tENDPOINT", "tENTRY", "tENUM", "tERRORSTATUST",
  "tEXPLICITHANDLE", "tEXTERN", "tFALSE", "tFASTCALL", "tFLOAT", "tHANDLE",
  "tHANDLET", "tHELPCONTEXT", "tHELPFILE", "tHELPSTRING",
  "tHELPSTRINGCONTEXT", "tHELPSTRINGDLL", "tHIDDEN", "tHYPER", "tID",
  "tIDEMPOTENT", "tIIDIS", "tIMMEDIATEBIND", "tIMPLICITHANDLE", "tIMPORT",
  "tIMPORTLIB", "tIN", "tIN_LINE", "tINLINE", "tINPUTSYNC", "tINT",
  "tINT64", "tINTERFACE", "tLCID", "tLENGTHIS", "tLIBRARY", "tLOCAL",
  "tLONG", "tMETHODS", "tMODULE", "tNONBROWSABLE", "tNONCREATABLE",
  "tNONEXTENSIBLE", "tNULL", "tOBJECT", "tODL", "tOLEAUTOMATION",
  "tOPTIONAL", "tOUT", "tPASCAL", "tPOINTERDEFAULT", "tPROPERTIES",
  "tPROPGET", "tPROPPUT", "tPROPPUTREF", "tPTR", "tPUBLIC", "tRANGE",
  "tREADONLY", "tREF", "tREGISTER", "tREQUESTEDIT", "tRESTRICTED",
  "tRETVAL", "tSAFEARRAY", "tSHORT", "tSIGNED", "tSINGLE", "tSIZEIS",
  "tSIZEOF", "tSMALL", "tSOURCE", "tSTATIC", "tSTDCALL",
  "tSTRICTCONTEXTHANDLE", "tSTRING", "tSTRUCT", "tSWITCH", "tSWITCHIS",
  "tSWITCHTYPE", "tTRANSMITAS", "tTRUE", "tTYPEDEF", "tUNION", "tUNIQUE",
  "tUNSIGNED", "tUUID", "tV1ENUM", "tVARARG", "tVERSION", "tVOID",
  "tWCHAR", "tWIREMARSHAL", "','", "'?'", "':'", "'|'", "'^'", "'&'",
  "'<'", "'>'", "'-'", "'+'", "'*'", "'/'", "'%'", "'!'", "'~'",
  "ADDRESSOF", "NEG", "POS", "PPTR", "CAST", "'.'", "'['", "']'", "';'",
  "'('", "')'", "'{'", "'}'", "'='", "$accept", "input", "gbl_statements",
  "imp_statements", "int_statements", "semicolon_opt", "statement",
  "typedecl", "cppquote", "import_start", "import", "importlib",
  "libraryhdr", "library_start", "librarydef", "m_args", "no_args", "args",
  "arg", "array", "m_attributes", "attributes", "attrib_list", "str_list",
  "attribute", "uuid_string", "callconv", "cases", "case", "enums",
  "enum_list", "enum", "enumdef", "m_exprs", "m_expr", "expr",
  "expr_list_int_const", "expr_int_const", "expr_const", "fields", "field",
  "ne_union_field", "ne_union_fields", "union_field", "s_field", "funcdef",
  "declaration", "m_ident", "t_ident", "ident", "base_type", "m_int",
  "int_std", "coclass", "coclasshdr", "coclassdef", "coclass_ints",
  "coclass_int", "dispinterface", "dispinterfacehdr", "dispint_props",
  "dispint_meths", "dispinterfacedef", "inherit", "interface",
  "interfacehdr", "interfacedef", "interfacedec", "module", "modulehdr",
  "moduledef", "storage_cls_spec", "function_specifier", "type_qualifier",
  "m_type_qual_list", "decl_spec", "m_decl_spec_no_type",
  "decl_spec_no_type", "declarator", "direct_declarator",
  "declarator_list", "init_declarator", "pointer_type", "structdef",
  "type", "typedef", "uniondef", "version", 0
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
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
     395,   396,   397,   398,   399,   400,   401,    44,    63,    58,
     124,    94,    38,    60,    62,    45,    43,    42,    47,    37,
      33,   126,   402,   403,   404,   405,   406,    46,    91,    93,
      59,    40,    41,   123,   125,    61
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned short int yyr1[] =
{
       0,   176,   177,   178,   178,   178,   178,   178,   178,   178,
     178,   179,   179,   179,   179,   179,   179,   179,   179,   179,
     180,   180,   181,   181,   182,   182,   182,   182,   182,   183,
     183,   183,   183,   183,   183,   184,   185,   186,   187,   188,
     189,   190,   191,   191,   192,   193,   193,   193,   194,   194,
     195,   195,   196,   196,   197,   198,   198,   198,   199,   199,
     200,   200,   200,   200,   200,   200,   200,   200,   200,   200,
     200,   200,   200,   200,   200,   200,   200,   200,   200,   200,
     200,   200,   200,   200,   200,   200,   200,   200,   200,   200,
     200,   200,   200,   200,   200,   200,   200,   200,   200,   200,
     200,   200,   200,   200,   200,   200,   200,   200,   200,   200,
     200,   200,   200,   200,   200,   200,   200,   200,   200,   200,
     200,   200,   200,   200,   200,   200,   200,   200,   200,   200,
     200,   201,   201,   202,   202,   202,   202,   203,   203,   204,
     204,   205,   205,   205,   206,   206,   207,   207,   208,   209,
     209,   210,   210,   211,   211,   211,   211,   211,   211,   211,
     211,   211,   211,   211,   211,   211,   211,   211,   211,   211,
     211,   211,   211,   211,   211,   211,   211,   211,   211,   211,
     211,   211,   211,   211,   211,   211,   211,   211,   211,   211,
     211,   211,   211,   212,   212,   213,   214,   215,   215,   216,
     216,   217,   217,   218,   218,   219,   219,   220,   221,   222,
     222,   223,   223,   224,   224,   224,   225,   225,   226,   226,
     226,   226,   226,   226,   226,   226,   226,   226,   226,   226,
     227,   227,   228,   228,   228,   228,   228,   228,   228,   229,
     229,   230,   231,   232,   232,   233,   234,   234,   235,   236,
     236,   237,   237,   238,   238,   239,   239,   240,   240,   241,
     242,   242,   242,   243,   243,   244,   244,   245,   246,   247,
     247,   247,   248,   249,   250,   250,   251,   251,   252,   252,
     253,   253,   253,   254,   254,   254,   255,   255,   255,   255,
     256,   256,   257,   257,   258,   258,   258,   259,   260,   260,
     260,   260,   260,   260,   260,   260,   260,   260,   261,   262,
     262,   263,   263
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     0,     2,     2,     3,     2,     2,     2,
       2,     0,     2,     2,     3,     2,     2,     2,     2,     2,
       0,     2,     0,     1,     1,     2,     2,     1,     2,     1,
       1,     1,     2,     2,     2,     4,     3,     3,     5,     2,
       3,     4,     0,     1,     1,     1,     3,     1,     3,     2,
       3,     3,     0,     1,     3,     1,     3,     4,     1,     3,
       0,     1,     1,     1,     1,     1,     1,     4,     4,     1,
       1,     1,     1,     1,     1,     4,     1,     1,     4,     1,
       4,     4,     1,     1,     4,     4,     4,     4,     4,     1,
       4,     1,     4,     1,     5,     1,     1,     4,     4,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     4,     1,
       1,     1,     1,     6,     1,     1,     1,     1,     4,     1,
       1,     1,     4,     4,     4,     4,     1,     1,     4,     4,
       1,     1,     1,     1,     1,     1,     1,     0,     2,     4,
       3,     0,     2,     1,     1,     3,     3,     1,     5,     1,
       3,     0,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     5,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     2,     2,     2,     2,     2,     2,     3,     3,     4,
       4,     4,     3,     1,     3,     1,     1,     0,     2,     4,
       3,     2,     2,     0,     2,     2,     1,     3,     3,     3,
       2,     0,     1,     0,     1,     1,     1,     1,     1,     1,
       1,     2,     2,     1,     1,     1,     1,     1,     1,     1,
       0,     1,     1,     2,     1,     2,     2,     1,     1,     2,
       2,     2,     5,     0,     2,     2,     2,     2,     2,     2,
       3,     2,     3,     5,     5,     0,     2,     2,     2,     2,
       6,     8,     2,     2,     2,     2,     2,     2,     5,     1,
       1,     1,     1,     1,     0,     2,     2,     3,     0,     1,
       2,     2,     2,     3,     2,     1,     1,     3,     2,     4,
       1,     3,     1,     3,     1,     1,     1,     5,     1,     1,
       1,     1,     2,     1,     2,     1,     2,     4,     4,     5,
      10,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned short int yydefact[] =
{
       3,     0,     2,     1,   299,   227,   218,   238,     0,   273,
       0,     0,   226,   213,   228,   269,   224,   229,   230,     0,
     272,   232,   237,     0,   230,   271,     0,   230,     0,   225,
     234,   270,   213,    52,   213,   223,   298,   219,    60,    10,
       0,    24,    11,    27,    11,     9,     0,   301,     0,   300,
     220,     0,     0,     7,     0,     0,    22,     0,   255,     5,
       4,     0,     8,   278,   278,   278,     0,     0,   303,   278,
       0,   305,   239,   240,     0,   246,   247,   302,   215,     0,
     231,   236,     0,   257,   258,   235,     0,   233,   221,   304,
       0,     0,    53,   306,     0,   222,    61,    62,    63,    64,
      65,    66,     0,     0,    69,    70,    71,    72,    73,    74,
       0,    76,    77,     0,    79,     0,     0,    82,    83,     0,
       0,     0,     0,     0,    89,     0,    91,     0,    93,     0,
      95,    96,     0,     0,    99,   100,   101,   102,   103,   104,
     105,   106,   107,     0,   109,   110,   111,   296,   112,     0,
     114,   294,   115,   116,   117,     0,   119,   120,   121,     0,
       0,     0,   295,     0,   126,   127,     0,     0,     0,    55,
     130,    25,     0,     0,     0,     0,     0,   301,   241,   248,
     259,   267,     0,   303,   305,    26,     6,   243,   264,     0,
      23,   262,   263,     0,     0,    20,   282,   279,   281,   280,
     216,   217,   133,   134,   135,   136,   274,     0,     0,   286,
     292,   285,   210,   301,   303,   278,   305,   276,    28,     0,
     141,    36,     0,   197,     0,     0,   203,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   151,     0,     0,   151,     0,     0,     0,     0,
       0,     0,    60,    54,    37,     0,    17,    18,    19,     0,
      15,    13,    12,    16,    22,    39,   265,   266,    40,   209,
      52,     0,    52,     0,     0,   256,    20,     0,     0,     0,
     284,     0,   151,    42,   288,   277,    35,     0,   143,   144,
     147,   307,    52,   290,   308,    52,    52,     0,   161,   153,
     154,   155,   159,   160,   156,   157,     0,   158,     0,     0,
       0,     0,     0,     0,     0,   195,     0,   193,   196,     0,
       0,    58,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   149,   152,     0,     0,     0,     0,
       0,     0,   132,   131,     0,   311,     0,     0,    56,    60,
       0,    14,    41,    22,     0,   244,   249,     0,     0,     0,
      52,     0,     0,     0,    22,    21,     0,   275,   283,   287,
     293,     0,     0,   298,     0,    47,    43,    45,     0,     0,
     148,   142,     0,   297,     0,   198,     0,     0,   309,    53,
     204,     0,    67,     0,   185,   184,   183,   186,   181,   182,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    68,    75,    78,     0,    80,
      81,    84,    85,    86,    87,    88,    90,    92,     0,    98,
     151,    97,   108,     0,   118,   122,   123,   124,   125,     0,
     128,   129,    57,     0,   242,   245,   251,     0,   250,   253,
       0,     0,   254,    20,    22,   268,    51,    50,   289,     0,
       0,    49,   145,   146,     0,   305,   291,   211,   202,   201,
       0,   192,     0,   174,   175,   187,   168,   169,   172,   173,
     163,   164,     0,   165,   166,   167,   171,   170,   177,   176,
     179,   180,   178,   188,     0,   194,    59,    94,   150,     0,
     312,    22,   207,     0,   252,     0,   260,    46,    48,     0,
     200,     0,   212,   190,   189,     0,   191,   113,    38,   208,
      22,   199,   137,   162,   261,     0,     0,     0,   310,   138,
       0,    52,    52,   206,   140,     0,   139,   205
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,     1,     2,   172,   277,   191,   365,    40,    41,    42,
      43,   257,   176,    44,   258,   374,   375,   376,   377,   284,
     358,    92,   168,   322,   169,   344,   208,   535,   539,   287,
     288,   289,   213,   333,   334,   315,   316,   317,   319,   292,
     385,   390,   296,   544,   545,   461,    48,   521,    79,   209,
      49,    81,    50,   259,    52,   260,   270,   355,    54,    55,
     272,   360,    56,   194,    57,    58,   261,   262,   181,    61,
     263,    63,    64,    65,   278,    66,   196,    67,   210,   211,
     294,   212,   170,   214,    69,    70,   216,   346
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -265
static const short int yypact[] =
{
    -265,    38,  1016,  -265,  -265,  -265,  -265,  -265,   133,  -265,
    -118,   135,  -265,   138,  -265,  -265,  -265,  -265,   -26,    63,
    -265,  -265,  -265,   140,   -26,  -265,   -44,   -26,    -3,  -265,
    -265,  -265,   142,   -57,   144,    -3,  -265,  -265,  1888,  -265,
      -8,  -265,  -265,  -265,  -265,  -265,  1650,    21,    23,  -265,
    -265,    26,   -23,  -265,    28,   -14,    30,    31,    55,  -265,
    -265,    32,  -265,    -4,    -4,    -4,   114,  1763,    33,    -4,
      42,    43,  -265,  -265,   212,  -265,  -265,    49,  -265,    51,
    -265,  -265,    57,  -265,  -265,  -265,  1763,  -265,  -265,    49,
      58,  1693,  -265,   -94,   -89,  -265,  -265,  -265,  -265,  -265,
    -265,  -265,    66,    67,  -265,  -265,  -265,  -265,  -265,  -265,
      68,  -265,  -265,    72,  -265,    75,    86,  -265,  -265,    87,
      89,    90,    92,    93,  -265,    94,  -265,    96,  -265,    99,
    -265,  -265,   103,   109,  -265,  -265,  -265,  -265,  -265,  -265,
    -265,  -265,  -265,   111,  -265,  -265,  -265,  -265,  -265,   113,
    -265,  -265,  -265,  -265,  -265,   115,  -265,  -265,  -265,   116,
     117,   118,  -265,   119,  -265,  -265,   120,   121,   -72,  -265,
    -265,  -265,   921,   405,   225,   149,    76,   123,  -265,  -265,
    -265,  -265,   114,   132,   134,  -265,  -265,  -265,  -265,    19,
    -265,  -265,  -265,   151,   130,  -265,  -265,  -265,  -265,  -265,
    -265,  -265,  -265,  -265,  -265,  -265,  -265,   114,   114,  -265,
      73,   -85,  -265,  -265,  -265,    -4,  -265,  -265,  -265,   141,
     154,  -265,   146,  -265,   114,   136,  -265,   154,    95,    95,
     297,   298,    95,    95,   308,   311,    95,   314,    95,    95,
     258,    95,    95,   -62,    95,    95,    95,  1763,  1763,   122,
     319,  1763,  1888,   158,  -265,   156,  -265,  -265,  -265,   159,
    -265,  -265,  -265,  -265,    30,  -265,  -265,  -265,  -265,  -265,
     -48,   179,   -60,   161,   160,  -265,  -265,   500,    37,   162,
    -265,    95,   446,  1091,  -265,  -265,  -265,   170,   188,  -265,
     171,  -265,   -45,  -265,   189,   -57,   -43,   173,  -265,  -265,
    -265,  -265,  -265,  -265,  -265,  -265,   176,  -265,    95,    95,
      95,    95,    95,    95,   785,  1386,   -82,  -265,  1386,   177,
     180,  -265,   -80,   182,   184,   185,   187,   190,   192,   195,
    1225,   345,   198,   -79,  -265,  1386,   199,   208,   -77,  1268,
     201,   202,  -265,  -265,   203,   193,   204,   205,  -265,  1888,
     370,  -265,  -265,    30,   -12,  -265,  -265,   230,  1693,   210,
     -40,   207,   304,   595,    30,  -265,  1693,  -265,  -265,  -265,
    -265,    54,   214,   -53,   213,  -265,   239,  -265,  1693,   114,
    -265,   154,    95,  -265,  1693,  -265,   114,   215,  -265,   218,
    -265,   221,  -265,  1763,     1,     1,     1,     1,     1,     1,
    1291,   240,    95,    95,   408,    95,    95,    95,    95,    95,
      95,    95,    95,    95,    95,    95,    95,    95,    95,    95,
      95,    95,   410,    95,    95,  -265,  -265,  -265,   406,  -265,
    -265,  -265,  -265,  -265,  -265,  -265,  -265,  -265,   243,  -265,
      95,  -265,  -265,    95,  -265,  -265,  -265,  -265,  -265,   411,
    -265,  -265,  -265,   245,  -265,  -265,  -265,   114,  -265,  -265,
    1693,   248,  -265,  -265,    30,  -265,  -265,  -265,  -265,  1134,
     114,  -265,  -265,  -265,   114,   250,  -265,   154,  -265,  -265,
     249,  -265,    95,   183,   183,  -265,   534,   534,   153,   153,
    1493,  1416,  1336,  1439,  1462,  1509,   153,   153,    77,    77,
       1,     1,     1,  -265,  1314,  -265,  -265,  -265,  -265,   251,
    -265,    30,  -265,   114,  -265,   690,  -265,  -265,  -265,   -81,
    -265,   252,  -265,  -265,     1,    95,  -265,  -265,  -265,  -265,
      30,  -265,  -265,  1386,  -265,   -11,    95,   273,  -265,  -265,
     275,   -83,   -83,  -265,  -265,   256,  -265,  -265
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -265,  -265,  -265,   385,  -264,  -257,    11,  -265,  -265,  -265,
      69,  -265,  -265,  -265,   428,  -265,  -265,  -265,   -37,  -265,
     -30,    -2,  -265,  -265,  -235,  -265,  -265,  -265,  -265,  -265,
    -265,    53,     2,   191,  -255,   -13,  -265,  -222,  -208,  -265,
    -265,  -265,  -265,  -105,  -241,  -265,  -265,  -265,   101,  -199,
    -265,    85,    78,    18,  -265,   436,  -265,  -265,   393,  -265,
    -265,  -265,  -265,  -265,   -28,  -265,   438,    -1,  -265,  -265,
     441,  -265,  -265,   166,  -265,   -41,   -35,   -20,  -198,  -265,
     -27,   264,   216,     6,   -61,  -265,     0,  -265
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -215
static const short int yytable[] =
{
      46,    60,    71,    91,    47,   182,   215,   352,    68,   279,
     280,   324,   363,    39,   327,   404,   329,   348,   180,   332,
      51,   290,   337,   536,   323,   222,   293,   372,   297,   198,
     199,   359,   357,     7,   217,   537,     9,  -214,     3,    11,
     200,   201,   225,   197,   197,   197,   184,   147,   177,   197,
     224,   151,   183,    74,   387,   391,    15,   298,    80,   299,
     300,   301,   302,   303,   178,   424,   386,   428,   440,    18,
     440,    82,   202,   370,    23,   252,   162,     9,    20,  -214,
     368,    21,    22,   282,   226,    38,   283,   543,    24,   531,
     425,   404,   429,   441,   -44,   444,   454,   253,   298,   203,
     299,   300,   301,   302,   303,    23,    88,   465,    38,    85,
      25,    38,    87,    95,   452,   304,    27,   200,   201,   -44,
      38,    30,    31,    38,   271,    38,   353,    86,    38,   383,
     342,   388,   343,    90,   459,    94,    72,    73,    75,    76,
     204,    77,    78,    83,    84,    89,    78,    93,    78,   202,
     187,   305,   266,   267,   274,   275,   304,   200,   201,   189,
     473,   273,   171,   538,   205,   402,   403,   404,   422,   423,
      46,    46,    71,    71,    47,    47,   203,   306,    68,    68,
     285,   471,   290,   256,   256,   508,   340,   341,   476,   307,
     347,   -29,   305,   185,   206,   197,   186,   404,   188,   515,
     190,   192,   505,   -30,   193,   195,   308,   516,   207,   309,
     310,   311,   218,   -31,   312,   313,   318,   204,   306,   318,
     219,   509,  -214,   466,   220,   314,   330,   221,   265,   335,
     307,   223,   335,   339,   419,   420,   421,   227,   228,   229,
     354,   205,   379,   230,   422,   423,   231,   308,   281,   268,
     309,   310,   311,   401,   528,   312,   313,   232,   233,   512,
     234,   235,   384,   236,   237,   238,   314,   239,   318,   335,
     240,   206,   518,   534,   241,   366,   293,    71,   522,    47,
     242,   378,   243,    68,   244,   207,   245,   246,   247,   248,
     249,   250,   251,   -32,   389,   394,   395,   396,   397,   398,
     399,   400,   -33,   276,   -34,   320,   321,   295,   417,   418,
     419,   420,   421,   286,   540,   529,   325,   457,   291,   326,
     422,   423,   328,   331,   345,   182,   349,   350,   356,   351,
     460,   361,   480,   362,   369,   381,   386,   470,   417,   418,
     419,   420,   421,   474,   380,   392,   382,   393,   438,   426,
     422,   423,   427,   455,   430,   443,   431,   432,   397,   433,
     449,   366,   434,    71,   435,    47,   184,   436,   177,    68,
     439,   442,   183,   446,   447,   448,   450,   451,   453,   456,
     458,   462,    19,   467,   475,   468,   469,   477,   478,   483,
     484,   479,   486,   487,   488,   489,   490,   491,   492,   493,
     494,   495,   496,   497,   498,   499,   500,   501,   502,     4,
     504,   485,   482,   503,   506,   507,   510,   511,   514,   513,
     520,   523,   541,   527,   542,   532,   547,   335,   379,   173,
      45,   463,   517,     5,   472,     6,   338,   546,    53,   179,
      59,     7,     8,    62,   367,     9,   269,   519,     0,   298,
      10,   299,   300,   301,   302,   303,    11,     0,    12,   336,
       0,     0,    13,    14,     0,    15,     0,   378,    16,   524,
      17,     0,     0,     0,     0,     0,     0,    18,     0,     0,
       0,     0,     0,    19,   255,     0,     0,    20,     0,    21,
      22,    23,     0,     0,     0,     0,    24,     0,     0,     0,
       0,     0,     0,     0,     4,     0,     0,   304,     0,     0,
       0,     0,   533,   366,     0,    71,     0,    47,     0,    25,
       0,    68,     0,    26,    27,    28,    29,     0,     5,    30,
       6,    31,     0,     0,     0,    32,     7,     0,     0,     0,
       9,    33,    34,   305,    35,    10,   402,   403,   404,    36,
      37,   407,   408,    12,     0,     0,     0,    13,    14,     0,
      15,     0,     0,    16,     0,    17,     0,     0,     0,   306,
       0,     0,    18,    38,     0,     0,     0,     0,    19,   264,
       0,   307,    20,     0,    21,    22,     0,     0,     0,     0,
       0,    24,     0,     0,     0,     0,     0,     0,   308,     4,
       0,   309,   310,   371,     0,     0,   312,   313,     0,     0,
       0,     0,     0,     0,    25,     0,     0,   314,    26,    27,
      28,    29,     0,     5,    30,     6,    31,     0,     0,     0,
      32,     7,     0,     0,     0,     9,    33,    34,     0,    35,
      10,     0,     0,     0,    36,    37,     0,     0,    12,     0,
       0,     0,    13,    14,     0,    15,     0,     0,    16,     0,
      17,     0,     0,     0,     0,     0,     0,    18,    38,     0,
       0,     0,     0,    19,   364,     0,     0,    20,     0,    21,
      22,     0,     0,     0,     0,     0,    24,   415,   416,   417,
     418,   419,   420,   421,     4,     0,     0,     0,     0,     0,
       0,   422,   423,     0,     0,     0,     0,     0,     0,    25,
       0,     0,     0,    26,    27,    28,    29,     0,     5,    30,
       6,    31,     0,     0,     0,    32,     7,     0,     0,     0,
       9,    33,    34,     0,    35,    10,     0,     0,     0,    36,
      37,     0,     0,    12,     0,     0,     0,    13,    14,     0,
      15,     0,     0,    16,     0,    17,     0,     0,     0,     0,
       0,     0,    18,    38,     0,     0,     0,     0,    19,   464,
       0,     0,    20,     0,    21,    22,     0,     0,     0,     0,
       0,    24,     0,     0,     0,     0,     0,     0,   298,     4,
     299,   300,   301,   302,   303,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    25,     0,     0,     0,    26,    27,
      28,    29,     0,     5,    30,     6,    31,     0,     0,     0,
      32,     7,     0,     0,     0,     0,    33,    34,     0,    35,
       0,     0,     0,     0,    36,    37,     0,     0,    12,     0,
       0,     0,    13,    14,     0,     0,   304,     0,    16,     0,
      17,     0,     0,     0,     0,     0,     0,    18,    38,     0,
       0,     0,     0,     0,   530,     0,     0,     0,     0,    21,
      22,     0,     0,     0,     0,     0,    24,     0,     0,     0,
       0,     0,   305,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    26,    27,    28,    29,     0,   306,    30,
       0,     0,     0,     0,     0,    32,     0,     0,     0,     0,
     307,     0,    34,     0,    35,     4,     0,     0,     0,    36,
      37,     0,   254,     0,     0,     0,     0,   308,     0,     0,
     309,   310,   311,     0,     0,   312,   313,     0,     0,     5,
       0,     6,     0,     0,     0,     0,   314,     7,     8,     0,
       0,     9,     0,     0,     0,     0,    10,     0,     0,     0,
       0,     0,    11,     0,    12,     0,     0,     0,    13,    14,
       0,    15,     0,     0,    16,     0,    17,     0,     0,     0,
       0,     0,     0,    18,     0,     0,     0,     0,     0,    19,
     255,     0,     0,    20,     0,    21,    22,    23,     0,     0,
       0,     0,    24,     0,     0,     0,     0,     0,     0,     0,
       4,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    25,     0,     0,     0,    26,
      27,    28,    29,     0,     5,    30,     6,    31,     0,     0,
       0,    32,     7,     8,     0,     0,     9,    33,    34,     0,
      35,    10,     0,     0,     0,    36,    37,    11,     0,    12,
       0,     0,     0,    13,    14,     0,    15,     0,     0,    16,
       0,    17,     0,     0,     0,     0,     0,     0,    18,    38,
       0,     0,     0,     0,    19,     4,     0,     0,    20,     0,
      21,    22,    23,     0,     0,     0,     0,    24,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     5,
       0,     6,     0,     0,     0,     0,     0,     7,     0,     0,
      25,     9,     0,     0,    26,    27,    28,    29,     4,     0,
      30,     0,    31,     0,    12,     0,    32,     0,    13,    14,
       0,    15,    33,    34,    16,    35,    17,     0,     0,     0,
      36,    37,     5,    18,     6,     0,     0,     0,     0,     0,
       7,     0,     0,    20,     9,    21,    22,     0,     0,     0,
       0,     0,    24,     0,    38,     0,     0,    12,     0,     0,
       0,    13,    14,     0,    15,     0,     0,    16,     0,    17,
       0,     0,     0,     0,     0,    25,    18,     0,     0,    26,
      27,    28,    29,     0,     0,    30,    20,    31,    21,    22,
       0,    32,     0,     0,     0,    24,     0,     0,    34,     0,
      35,     0,     0,     0,     0,   373,    37,   402,   403,   404,
     405,   406,   407,   408,   409,   410,     0,     0,    25,     0,
       0,     0,    26,    27,    28,    29,     0,     0,    30,    38,
      31,     0,     0,     0,    32,     0,     0,     0,     0,     0,
       0,    34,     0,    35,     0,     0,     0,     0,    36,    37,
     402,   403,   404,   405,   406,   407,   408,   409,   410,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    38,   402,   403,   404,   405,   406,   407,   408,
     409,   410,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   402,   403,   404,   405,
     406,   407,   408,   409,   410,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   402,   403,
     404,   405,   406,   407,   408,   409,   410,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   411,     0,   412,   413,   414,   415,   416,
     417,   418,   419,   420,   421,     0,     0,     0,     0,     0,
       0,     0,   422,   423,     0,     0,     0,   437,   402,   403,
     404,   405,   406,   407,   408,   409,   410,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   411,     0,   412,   413,
     414,   415,   416,   417,   418,   419,   420,   421,   402,   403,
     404,   405,   406,   407,   408,   422,   423,     0,     0,   411,
     445,   412,   413,   414,   415,   416,   417,   418,   419,   420,
     421,   402,   403,   404,   405,   406,   407,   408,   422,   423,
       0,     0,   411,   481,   412,   413,   414,   415,   416,   417,
     418,   419,   420,   421,   402,   403,   404,   405,   406,   407,
     408,   422,   423,   526,   411,   525,   412,   413,   414,   415,
     416,   417,   418,   419,   420,   421,     0,     0,     0,     0,
       0,     0,     0,   422,   423,   402,   403,   404,   405,   406,
     407,   408,     0,   410,     0,     0,     0,     0,     0,     0,
       0,   402,   403,   404,   405,   406,   407,   408,     0,     0,
       0,     0,     0,     0,   411,     0,   412,   413,   414,   415,
     416,   417,   418,   419,   420,   421,     0,     0,     0,     0,
       0,     0,     0,   422,   423,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   412,   413,   414,   415,
     416,   417,   418,   419,   420,   421,     0,     0,     0,     0,
       0,     0,     0,   422,   423,     0,     0,     0,     0,     0,
     413,   414,   415,   416,   417,   418,   419,   420,   421,     0,
       0,     0,     0,     0,     0,     0,   422,   423,     0,     0,
       0,     0,     0,     0,   414,   415,   416,   417,   418,   419,
     420,   421,     0,     0,     0,     0,     0,     0,     0,   422,
     423,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   412,   413,   414,   415,   416,   417,   418,
     419,   420,   421,     0,     4,     0,     0,     0,     0,     0,
     422,   423,   415,   416,   417,   418,   419,   420,   421,     0,
       0,     0,     0,     0,     0,     0,   422,   423,     5,     0,
       6,     0,     0,     0,     0,     0,     7,     8,     0,     0,
       9,     0,     0,     0,     0,     0,     0,     4,     0,     0,
       0,    11,     0,    12,     0,     0,     0,    13,    14,     0,
      15,     0,     0,    16,     0,    17,     0,     0,     0,     0,
       0,     5,    18,     6,     0,     0,     0,     0,     0,     7,
       0,     0,    20,     9,    21,    22,    23,     0,     0,   174,
       0,    24,     0,   175,     0,     0,    12,     0,     0,     0,
      13,    14,     0,    15,     0,     0,    16,     0,    17,     0,
       0,     0,     0,     0,    25,    18,     0,     4,    26,    27,
      28,    29,     0,     0,    30,    20,    31,    21,    22,     0,
      32,     0,     0,     0,    24,     0,     0,    34,     0,    35,
       0,     5,     0,     6,    36,    37,     0,     0,     0,     7,
       0,     0,     0,     0,     0,     0,     0,    25,     0,     0,
       0,    26,    27,    28,    29,     0,    12,    30,     0,    31,
      13,    14,     0,    32,     0,     0,    16,     0,    17,     0,
      34,     0,    35,     0,     0,    18,     0,    36,    37,     0,
       0,     0,     0,     0,     0,     0,     0,    21,    22,     0,
       0,     0,     0,     0,    24,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    26,    27,    28,    29,     0,     0,    30,     0,     0,
       0,     0,     0,    32,     0,     0,     0,     0,     0,     0,
      34,     0,    35,     0,     0,     0,     0,    36,    37,    96,
       0,    97,    98,     0,    99,   100,     0,   101,     0,     0,
     102,     0,   103,     0,     0,     0,     0,     0,     0,   104,
     105,   106,   107,     0,   108,   109,   110,   111,   112,     0,
     113,     0,   114,   115,   116,     0,     0,   117,     0,     0,
       0,     0,   118,     0,   119,   120,   121,   122,   123,   124,
       0,   125,   126,   127,   128,   129,     0,     0,   130,     0,
       0,   131,     0,     0,     0,   132,   133,     0,   134,     0,
       0,     0,   135,   136,   137,     0,   138,   139,   140,   141,
     142,     0,   143,     0,   144,   145,   146,   147,   148,   149,
     150,   151,     0,   152,   153,   154,     0,     0,     0,     0,
     155,     0,     0,   156,     0,     0,   157,   158,     0,     0,
     159,   160,   161,     0,     0,     0,   162,     0,   163,   164,
     165,   166,     0,     0,   167
};

static const short int yycheck[] =
{
       2,     2,     2,    33,     2,    46,    67,   264,     2,   207,
     208,   233,   276,     2,   236,    14,   238,   252,    46,   241,
       2,   220,   244,    34,   232,    86,   224,   282,   227,    64,
      65,   272,    92,    36,    69,    46,    40,   131,     0,    51,
       3,     4,   131,    63,    64,    65,    46,   109,    46,    69,
      91,   113,    46,   171,   295,   296,    60,     3,    84,     5,
       6,     7,     8,     9,    46,   147,   147,   147,   147,    72,
     147,     8,    35,   281,    86,   147,   138,    40,    82,   173,
     278,    84,    85,   168,   173,   168,   171,   170,    91,   170,
     172,    14,   172,   172,   147,   172,   353,   169,     3,    62,
       5,     6,     7,     8,     9,    86,    28,   364,   168,    24,
     114,   168,    27,    35,   349,    61,   119,     3,     4,   172,
     168,   124,   126,   168,   105,   168,   174,   171,   168,   174,
       8,   174,    10,    32,   174,    34,     3,     4,     3,     4,
     103,     3,     4,     3,     4,     3,     4,     3,     4,    35,
     173,    97,     3,     4,     3,     4,    61,     3,     4,   173,
     382,   189,   170,   174,   127,    12,    13,    14,   167,   168,
     172,   173,   172,   173,   172,   173,    62,   123,   172,   173,
     215,   379,   381,   172,   173,   440,   247,   248,   386,   135,
     251,   170,    97,   170,   157,   215,   170,    14,   170,   463,
     170,   170,   424,   170,   149,   173,   152,   464,   171,   155,
     156,   157,   170,   170,   160,   161,   229,   103,   123,   232,
       8,   443,   173,   169,   173,   171,   239,   170,     3,   242,
     135,   173,   245,   246,   157,   158,   159,   171,   171,   171,
     270,   127,   283,   171,   167,   168,   171,   152,   175,   173,
     155,   156,   157,   314,   511,   160,   161,   171,   171,   457,
     171,   171,   292,   171,   171,   171,   171,   171,   281,   282,
     171,   157,   470,   530,   171,   277,   474,   277,   477,   277,
     171,   283,   171,   277,   171,   171,   171,   171,   171,   171,
     171,   171,   171,   170,   296,   308,   309,   310,   311,   312,
     313,   314,   170,   173,   170,     8,     8,   171,   155,   156,
     157,   158,   159,   172,   536,   513,     8,   358,   172,     8,
     167,   168,     8,    65,     5,   366,   168,   171,   149,   170,
     360,   170,   393,   173,   172,   147,   147,   378,   155,   156,
     157,   158,   159,   384,   174,   172,   175,   171,     3,   172,
     167,   168,   172,   354,   172,   147,   172,   172,   371,   172,
     167,   363,   172,   363,   172,   363,   366,   172,   366,   363,
     172,   172,   366,   172,   172,   172,   172,   172,     8,   149,
     170,   174,    78,   169,   384,   172,   147,   172,   170,   402,
     403,   170,   405,   406,   407,   408,   409,   410,   411,   412,
     413,   414,   415,   416,   417,   418,   419,   420,   421,     4,
     423,     3,   172,     3,     8,   172,     5,   172,   170,   460,
     170,   172,   149,   172,   149,   173,   170,   440,   469,    44,
       2,   362,   469,    28,   381,    30,   245,   542,     2,    46,
       2,    36,    37,     2,   278,    40,   182,   474,    -1,     3,
      45,     5,     6,     7,     8,     9,    51,    -1,    53,   243,
      -1,    -1,    57,    58,    -1,    60,    -1,   469,    63,   482,
      65,    -1,    -1,    -1,    -1,    -1,    -1,    72,    -1,    -1,
      -1,    -1,    -1,    78,    79,    -1,    -1,    82,    -1,    84,
      85,    86,    -1,    -1,    -1,    -1,    91,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     4,    -1,    -1,    61,    -1,    -1,
      -1,    -1,   525,   515,    -1,   515,    -1,   515,    -1,   114,
      -1,   515,    -1,   118,   119,   120,   121,    -1,    28,   124,
      30,   126,    -1,    -1,    -1,   130,    36,    -1,    -1,    -1,
      40,   136,   137,    97,   139,    45,    12,    13,    14,   144,
     145,    17,    18,    53,    -1,    -1,    -1,    57,    58,    -1,
      60,    -1,    -1,    63,    -1,    65,    -1,    -1,    -1,   123,
      -1,    -1,    72,   168,    -1,    -1,    -1,    -1,    78,   174,
      -1,   135,    82,    -1,    84,    85,    -1,    -1,    -1,    -1,
      -1,    91,    -1,    -1,    -1,    -1,    -1,    -1,   152,     4,
      -1,   155,   156,   157,    -1,    -1,   160,   161,    -1,    -1,
      -1,    -1,    -1,    -1,   114,    -1,    -1,   171,   118,   119,
     120,   121,    -1,    28,   124,    30,   126,    -1,    -1,    -1,
     130,    36,    -1,    -1,    -1,    40,   136,   137,    -1,   139,
      45,    -1,    -1,    -1,   144,   145,    -1,    -1,    53,    -1,
      -1,    -1,    57,    58,    -1,    60,    -1,    -1,    63,    -1,
      65,    -1,    -1,    -1,    -1,    -1,    -1,    72,   168,    -1,
      -1,    -1,    -1,    78,   174,    -1,    -1,    82,    -1,    84,
      85,    -1,    -1,    -1,    -1,    -1,    91,   153,   154,   155,
     156,   157,   158,   159,     4,    -1,    -1,    -1,    -1,    -1,
      -1,   167,   168,    -1,    -1,    -1,    -1,    -1,    -1,   114,
      -1,    -1,    -1,   118,   119,   120,   121,    -1,    28,   124,
      30,   126,    -1,    -1,    -1,   130,    36,    -1,    -1,    -1,
      40,   136,   137,    -1,   139,    45,    -1,    -1,    -1,   144,
     145,    -1,    -1,    53,    -1,    -1,    -1,    57,    58,    -1,
      60,    -1,    -1,    63,    -1,    65,    -1,    -1,    -1,    -1,
      -1,    -1,    72,   168,    -1,    -1,    -1,    -1,    78,   174,
      -1,    -1,    82,    -1,    84,    85,    -1,    -1,    -1,    -1,
      -1,    91,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,
       5,     6,     7,     8,     9,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   114,    -1,    -1,    -1,   118,   119,
     120,   121,    -1,    28,   124,    30,   126,    -1,    -1,    -1,
     130,    36,    -1,    -1,    -1,    -1,   136,   137,    -1,   139,
      -1,    -1,    -1,    -1,   144,   145,    -1,    -1,    53,    -1,
      -1,    -1,    57,    58,    -1,    -1,    61,    -1,    63,    -1,
      65,    -1,    -1,    -1,    -1,    -1,    -1,    72,   168,    -1,
      -1,    -1,    -1,    -1,   174,    -1,    -1,    -1,    -1,    84,
      85,    -1,    -1,    -1,    -1,    -1,    91,    -1,    -1,    -1,
      -1,    -1,    97,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   118,   119,   120,   121,    -1,   123,   124,
      -1,    -1,    -1,    -1,    -1,   130,    -1,    -1,    -1,    -1,
     135,    -1,   137,    -1,   139,     4,    -1,    -1,    -1,   144,
     145,    -1,    11,    -1,    -1,    -1,    -1,   152,    -1,    -1,
     155,   156,   157,    -1,    -1,   160,   161,    -1,    -1,    28,
      -1,    30,    -1,    -1,    -1,    -1,   171,    36,    37,    -1,
      -1,    40,    -1,    -1,    -1,    -1,    45,    -1,    -1,    -1,
      -1,    -1,    51,    -1,    53,    -1,    -1,    -1,    57,    58,
      -1,    60,    -1,    -1,    63,    -1,    65,    -1,    -1,    -1,
      -1,    -1,    -1,    72,    -1,    -1,    -1,    -1,    -1,    78,
      79,    -1,    -1,    82,    -1,    84,    85,    86,    -1,    -1,
      -1,    -1,    91,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       4,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   114,    -1,    -1,    -1,   118,
     119,   120,   121,    -1,    28,   124,    30,   126,    -1,    -1,
      -1,   130,    36,    37,    -1,    -1,    40,   136,   137,    -1,
     139,    45,    -1,    -1,    -1,   144,   145,    51,    -1,    53,
      -1,    -1,    -1,    57,    58,    -1,    60,    -1,    -1,    63,
      -1,    65,    -1,    -1,    -1,    -1,    -1,    -1,    72,   168,
      -1,    -1,    -1,    -1,    78,     4,    -1,    -1,    82,    -1,
      84,    85,    86,    -1,    -1,    -1,    -1,    91,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    28,
      -1,    30,    -1,    -1,    -1,    -1,    -1,    36,    -1,    -1,
     114,    40,    -1,    -1,   118,   119,   120,   121,     4,    -1,
     124,    -1,   126,    -1,    53,    -1,   130,    -1,    57,    58,
      -1,    60,   136,   137,    63,   139,    65,    -1,    -1,    -1,
     144,   145,    28,    72,    30,    -1,    -1,    -1,    -1,    -1,
      36,    -1,    -1,    82,    40,    84,    85,    -1,    -1,    -1,
      -1,    -1,    91,    -1,   168,    -1,    -1,    53,    -1,    -1,
      -1,    57,    58,    -1,    60,    -1,    -1,    63,    -1,    65,
      -1,    -1,    -1,    -1,    -1,   114,    72,    -1,    -1,   118,
     119,   120,   121,    -1,    -1,   124,    82,   126,    84,    85,
      -1,   130,    -1,    -1,    -1,    91,    -1,    -1,   137,    -1,
     139,    -1,    -1,    -1,    -1,   144,   145,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    -1,    -1,   114,    -1,
      -1,    -1,   118,   119,   120,   121,    -1,    -1,   124,   168,
     126,    -1,    -1,    -1,   130,    -1,    -1,    -1,    -1,    -1,
      -1,   137,    -1,   139,    -1,    -1,    -1,    -1,   144,   145,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   168,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   148,    -1,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   167,   168,    -1,    -1,    -1,   172,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   148,    -1,   150,   151,
     152,   153,   154,   155,   156,   157,   158,   159,    12,    13,
      14,    15,    16,    17,    18,   167,   168,    -1,    -1,   148,
     172,   150,   151,   152,   153,   154,   155,   156,   157,   158,
     159,    12,    13,    14,    15,    16,    17,    18,   167,   168,
      -1,    -1,   148,   172,   150,   151,   152,   153,   154,   155,
     156,   157,   158,   159,    12,    13,    14,    15,    16,    17,
      18,   167,   168,   169,   148,   149,   150,   151,   152,   153,
     154,   155,   156,   157,   158,   159,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   167,   168,    12,    13,    14,    15,    16,
      17,    18,    -1,    20,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    12,    13,    14,    15,    16,    17,    18,    -1,    -1,
      -1,    -1,    -1,    -1,   148,    -1,   150,   151,   152,   153,
     154,   155,   156,   157,   158,   159,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   167,   168,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   150,   151,   152,   153,
     154,   155,   156,   157,   158,   159,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   167,   168,    -1,    -1,    -1,    -1,    -1,
     151,   152,   153,   154,   155,   156,   157,   158,   159,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   167,   168,    -1,    -1,
      -1,    -1,    -1,    -1,   152,   153,   154,   155,   156,   157,
     158,   159,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   167,
     168,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   150,   151,   152,   153,   154,   155,   156,
     157,   158,   159,    -1,     4,    -1,    -1,    -1,    -1,    -1,
     167,   168,   153,   154,   155,   156,   157,   158,   159,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   167,   168,    28,    -1,
      30,    -1,    -1,    -1,    -1,    -1,    36,    37,    -1,    -1,
      40,    -1,    -1,    -1,    -1,    -1,    -1,     4,    -1,    -1,
      -1,    51,    -1,    53,    -1,    -1,    -1,    57,    58,    -1,
      60,    -1,    -1,    63,    -1,    65,    -1,    -1,    -1,    -1,
      -1,    28,    72,    30,    -1,    -1,    -1,    -1,    -1,    36,
      -1,    -1,    82,    40,    84,    85,    86,    -1,    -1,    89,
      -1,    91,    -1,    93,    -1,    -1,    53,    -1,    -1,    -1,
      57,    58,    -1,    60,    -1,    -1,    63,    -1,    65,    -1,
      -1,    -1,    -1,    -1,   114,    72,    -1,     4,   118,   119,
     120,   121,    -1,    -1,   124,    82,   126,    84,    85,    -1,
     130,    -1,    -1,    -1,    91,    -1,    -1,   137,    -1,   139,
      -1,    28,    -1,    30,   144,   145,    -1,    -1,    -1,    36,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,    -1,    -1,
      -1,   118,   119,   120,   121,    -1,    53,   124,    -1,   126,
      57,    58,    -1,   130,    -1,    -1,    63,    -1,    65,    -1,
     137,    -1,   139,    -1,    -1,    72,    -1,   144,   145,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    84,    85,    -1,
      -1,    -1,    -1,    -1,    91,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   118,   119,   120,   121,    -1,    -1,   124,    -1,    -1,
      -1,    -1,    -1,   130,    -1,    -1,    -1,    -1,    -1,    -1,
     137,    -1,   139,    -1,    -1,    -1,    -1,   144,   145,    21,
      -1,    23,    24,    -1,    26,    27,    -1,    29,    -1,    -1,
      32,    -1,    34,    -1,    -1,    -1,    -1,    -1,    -1,    41,
      42,    43,    44,    -1,    46,    47,    48,    49,    50,    -1,
      52,    -1,    54,    55,    56,    -1,    -1,    59,    -1,    -1,
      -1,    -1,    64,    -1,    66,    67,    68,    69,    70,    71,
      -1,    73,    74,    75,    76,    77,    -1,    -1,    80,    -1,
      -1,    83,    -1,    -1,    -1,    87,    88,    -1,    90,    -1,
      -1,    -1,    94,    95,    96,    -1,    98,    99,   100,   101,
     102,    -1,   104,    -1,   106,   107,   108,   109,   110,   111,
     112,   113,    -1,   115,   116,   117,    -1,    -1,    -1,    -1,
     122,    -1,    -1,   125,    -1,    -1,   128,   129,    -1,    -1,
     132,   133,   134,    -1,    -1,    -1,   138,    -1,   140,   141,
     142,   143,    -1,    -1,   146
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned short int yystos[] =
{
       0,   177,   178,     0,     4,    28,    30,    36,    37,    40,
      45,    51,    53,    57,    58,    60,    63,    65,    72,    78,
      82,    84,    85,    86,    91,   114,   118,   119,   120,   121,
     124,   126,   130,   136,   137,   139,   144,   145,   168,   182,
     183,   184,   185,   186,   189,   190,   197,   208,   222,   226,
     228,   229,   230,   231,   234,   235,   238,   240,   241,   242,
     243,   245,   246,   247,   248,   249,   251,   253,   259,   260,
     261,   262,     3,     4,   171,     3,     4,     3,     4,   224,
      84,   227,     8,     3,     4,   227,   171,   227,   228,     3,
     224,   196,   197,     3,   224,   228,    21,    23,    24,    26,
      27,    29,    32,    34,    41,    42,    43,    44,    46,    47,
      48,    49,    50,    52,    54,    55,    56,    59,    64,    66,
      67,    68,    69,    70,    71,    73,    74,    75,    76,    77,
      80,    83,    87,    88,    90,    94,    95,    96,    98,    99,
     100,   101,   102,   104,   106,   107,   108,   109,   110,   111,
     112,   113,   115,   116,   117,   122,   125,   128,   129,   132,
     133,   134,   138,   140,   141,   142,   143,   146,   198,   200,
     258,   170,   179,   179,    89,    93,   188,   208,   229,   234,
     240,   244,   251,   259,   262,   170,   170,   173,   170,   173,
     170,   181,   170,   149,   239,   173,   252,   253,   252,   252,
       3,     4,    35,    62,   103,   127,   157,   171,   202,   225,
     254,   255,   257,   208,   259,   260,   262,   252,   170,     8,
     173,   170,   260,   173,   251,   131,   173,   171,   171,   171,
     171,   171,   171,   171,   171,   171,   171,   171,   171,   171,
     171,   171,   171,   171,   171,   171,   171,   171,   171,   171,
     171,   171,   147,   169,    11,    79,   182,   187,   190,   229,
     231,   242,   243,   246,   174,     3,     3,     4,   173,   257,
     232,   105,   236,   240,     3,     4,   173,   180,   250,   254,
     254,   175,   168,   171,   195,   252,   172,   205,   206,   207,
     225,   172,   215,   254,   256,   171,   218,   225,     3,     5,
       6,     7,     8,     9,    61,    97,   123,   135,   152,   155,
     156,   157,   160,   161,   171,   211,   212,   213,   211,   214,
       8,     8,   199,   214,   213,     8,     8,   213,     8,   213,
     211,    65,   213,   209,   210,   211,   258,   213,   209,   211,
     260,   260,     8,    10,   201,     5,   263,   260,   200,   168,
     171,   170,   181,   174,   196,   233,   149,    92,   196,   220,
     237,   170,   173,   180,   174,   182,   197,   249,   254,   172,
     214,   157,   210,   144,   191,   192,   193,   194,   197,   251,
     174,   147,   175,   174,   196,   216,   147,   220,   174,   197,
     217,   220,   172,   171,   211,   211,   211,   211,   211,   211,
     211,   260,    12,    13,    14,    15,    16,    17,    18,    19,
      20,   148,   150,   151,   152,   153,   154,   155,   156,   157,
     158,   159,   167,   168,   147,   172,   172,   172,   147,   172,
     172,   172,   172,   172,   172,   172,   172,   172,     3,   172,
     147,   172,   172,   147,   172,   172,   172,   172,   172,   167,
     172,   172,   200,     8,   181,   243,   149,   251,   170,   174,
     196,   221,   174,   186,   174,   181,   169,   169,   172,   147,
     251,   254,   207,   213,   251,   262,   254,   172,   170,   170,
     260,   172,   172,   211,   211,     3,   211,   211,   211,   211,
     211,   211,   211,   211,   211,   211,   211,   211,   211,   211,
     211,   211,   211,     3,   211,   213,     8,   172,   210,   213,
       5,   172,   254,   251,   170,   180,   181,   194,   254,   256,
     170,   223,   225,   172,   211,   149,   169,   172,   181,   254,
     174,   170,   173,   211,   181,   203,    34,    46,   174,   204,
     213,   149,   149,   170,   219,   220,   219,   170
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
#line 347 "parser.y"
    { fix_incomplete();
						  check_statements((yyvsp[0].stmt_list), FALSE);
						  check_all_user_types((yyvsp[0].stmt_list));
						  write_header((yyvsp[0].stmt_list));
						  write_id_data((yyvsp[0].stmt_list));
						  write_proxies((yyvsp[0].stmt_list));
						  write_client((yyvsp[0].stmt_list));
						  write_server((yyvsp[0].stmt_list));
						  write_dlldata((yyvsp[0].stmt_list));
						  write_local_stubs((yyvsp[0].stmt_list));
						;}
    break;

  case 3:
#line 360 "parser.y"
    { (yyval.stmt_list) = NULL; ;}
    break;

  case 4:
#line 361 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_reference((yyvsp[0].type))); ;}
    break;

  case 5:
#line 362 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_type_decl((yyvsp[0].type))); ;}
    break;

  case 6:
#line 363 "parser.y"
    { (yyval.stmt_list) = (yyvsp[-2].stmt_list);
						  reg_type((yyvsp[-1].type), (yyvsp[-1].type)->name, 0);
						;}
    break;

  case 7:
#line 366 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_type_decl((yyvsp[0].type)));
						  reg_type((yyvsp[0].type), (yyvsp[0].type)->name, 0);
						;}
    break;

  case 8:
#line 369 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_module((yyvsp[0].type))); ;}
    break;

  case 9:
#line 370 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_library((yyvsp[0].typelib))); ;}
    break;

  case 10:
#line 371 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), (yyvsp[0].statement)); ;}
    break;

  case 11:
#line 374 "parser.y"
    { (yyval.stmt_list) = NULL; ;}
    break;

  case 12:
#line 375 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_reference((yyvsp[0].type))); ;}
    break;

  case 13:
#line 376 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_type_decl((yyvsp[0].type))); ;}
    break;

  case 14:
#line 377 "parser.y"
    { (yyval.stmt_list) = (yyvsp[-2].stmt_list); reg_type((yyvsp[-1].type), (yyvsp[-1].type)->name, 0); ;}
    break;

  case 15:
#line 378 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_type_decl((yyvsp[0].type)));
						  reg_type((yyvsp[0].type), (yyvsp[0].type)->name, 0);
						;}
    break;

  case 16:
#line 381 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_module((yyvsp[0].type))); ;}
    break;

  case 17:
#line 382 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), (yyvsp[0].statement)); ;}
    break;

  case 18:
#line 383 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_importlib((yyvsp[0].str))); ;}
    break;

  case 19:
#line 384 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_library((yyvsp[0].typelib))); ;}
    break;

  case 20:
#line 387 "parser.y"
    { (yyval.stmt_list) = NULL; ;}
    break;

  case 21:
#line 388 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), (yyvsp[0].statement)); ;}
    break;

  case 24:
#line 396 "parser.y"
    { (yyval.statement) = make_statement_cppquote((yyvsp[0].str)); ;}
    break;

  case 25:
#line 397 "parser.y"
    { (yyval.statement) = make_statement_type_decl((yyvsp[-1].type)); ;}
    break;

  case 26:
#line 398 "parser.y"
    { (yyval.statement) = make_statement_declaration((yyvsp[-1].var)); ;}
    break;

  case 27:
#line 399 "parser.y"
    { (yyval.statement) = make_statement_import((yyvsp[0].str)); ;}
    break;

  case 28:
#line 400 "parser.y"
    { (yyval.statement) = (yyvsp[-1].statement); ;}
    break;

  case 32:
#line 407 "parser.y"
    { (yyval.type) = (yyvsp[0].type); (yyval.type)->attrs = check_enum_attrs((yyvsp[-1].attr_list)); ;}
    break;

  case 33:
#line 408 "parser.y"
    { (yyval.type) = (yyvsp[0].type); (yyval.type)->attrs = check_struct_attrs((yyvsp[-1].attr_list)); ;}
    break;

  case 34:
#line 409 "parser.y"
    { (yyval.type) = (yyvsp[0].type); (yyval.type)->attrs = check_union_attrs((yyvsp[-1].attr_list)); ;}
    break;

  case 35:
#line 412 "parser.y"
    { (yyval.str) = (yyvsp[-1].str); ;}
    break;

  case 36:
#line 414 "parser.y"
    { assert(yychar == YYEMPTY);
						  (yyval.import) = xmalloc(sizeof(struct _import_t));
						  (yyval.import)->name = (yyvsp[-1].str);
						  (yyval.import)->import_performed = do_import((yyvsp[-1].str));
						  if (!(yyval.import)->import_performed) yychar = aEOF;
						;}
    break;

  case 37:
#line 422 "parser.y"
    { (yyval.str) = (yyvsp[-2].import)->name;
						  if ((yyvsp[-2].import)->import_performed) pop_import();
						  free((yyvsp[-2].import));
						;}
    break;

  case 38:
#line 429 "parser.y"
    { (yyval.str) = (yyvsp[-2].str); if(!parse_only) add_importlib((yyvsp[-2].str)); ;}
    break;

  case 39:
#line 432 "parser.y"
    { (yyval.str) = (yyvsp[0].str); ;}
    break;

  case 40:
#line 434 "parser.y"
    { (yyval.typelib) = make_library((yyvsp[-1].str), check_library_attrs((yyvsp[-1].str), (yyvsp[-2].attr_list)));
						  if (!parse_only) start_typelib((yyval.typelib));
						;}
    break;

  case 41:
#line 439 "parser.y"
    { (yyval.typelib) = (yyvsp[-3].typelib);
						  (yyval.typelib)->stmts = (yyvsp[-2].stmt_list);
						  if (!parse_only) end_typelib();
						;}
    break;

  case 42:
#line 445 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 44:
#line 449 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 45:
#line 452 "parser.y"
    { check_arg((yyvsp[0].var)); (yyval.var_list) = append_var( NULL, (yyvsp[0].var) ); ;}
    break;

  case 46:
#line 453 "parser.y"
    { check_arg((yyvsp[0].var)); (yyval.var_list) = append_var( (yyvsp[-2].var_list), (yyvsp[0].var)); ;}
    break;

  case 48:
#line 458 "parser.y"
    { (yyval.var) = (yyvsp[0].declarator)->var;
						  (yyval.var)->attrs = (yyvsp[-2].attr_list);
						  if ((yyvsp[-1].declspec)->stgclass != STG_NONE && (yyvsp[-1].declspec)->stgclass != STG_REGISTER)
						    error_loc("invalid storage class for function parameter\n");
						  set_type((yyval.var), (yyvsp[-1].declspec), (yyvsp[0].declarator), TRUE);
						  free((yyvsp[0].declarator));
						;}
    break;

  case 49:
#line 465 "parser.y"
    { (yyval.var) = (yyvsp[0].declarator)->var;
						  if ((yyvsp[-1].declspec)->stgclass != STG_NONE && (yyvsp[-1].declspec)->stgclass != STG_REGISTER)
						    error_loc("invalid storage class for function parameter\n");
						  set_type((yyval.var), (yyvsp[-1].declspec), (yyvsp[0].declarator), TRUE);
						  free((yyvsp[0].declarator));
						;}
    break;

  case 50:
#line 473 "parser.y"
    { (yyval.expr) = (yyvsp[-1].expr); ;}
    break;

  case 51:
#line 474 "parser.y"
    { (yyval.expr) = make_expr(EXPR_VOID); ;}
    break;

  case 52:
#line 477 "parser.y"
    { (yyval.attr_list) = NULL; ;}
    break;

  case 54:
#line 482 "parser.y"
    { (yyval.attr_list) = (yyvsp[-1].attr_list); ;}
    break;

  case 55:
#line 485 "parser.y"
    { (yyval.attr_list) = append_attr( NULL, (yyvsp[0].attr) ); ;}
    break;

  case 56:
#line 486 "parser.y"
    { (yyval.attr_list) = append_attr( (yyvsp[-2].attr_list), (yyvsp[0].attr) ); ;}
    break;

  case 57:
#line 487 "parser.y"
    { (yyval.attr_list) = append_attr( (yyvsp[-3].attr_list), (yyvsp[0].attr) ); ;}
    break;

  case 58:
#line 490 "parser.y"
    { (yyval.str_list) = append_str( NULL, (yyvsp[0].str) ); ;}
    break;

  case 59:
#line 491 "parser.y"
    { (yyval.str_list) = append_str( (yyvsp[-2].str_list), (yyvsp[0].str) ); ;}
    break;

  case 60:
#line 494 "parser.y"
    { (yyval.attr) = NULL; ;}
    break;

  case 61:
#line 495 "parser.y"
    { (yyval.attr) = make_attr(ATTR_AGGREGATABLE); ;}
    break;

  case 62:
#line 496 "parser.y"
    { (yyval.attr) = make_attr(ATTR_APPOBJECT); ;}
    break;

  case 63:
#line 497 "parser.y"
    { (yyval.attr) = make_attr(ATTR_ASYNC); ;}
    break;

  case 64:
#line 498 "parser.y"
    { (yyval.attr) = make_attr(ATTR_AUTO_HANDLE); ;}
    break;

  case 65:
#line 499 "parser.y"
    { (yyval.attr) = make_attr(ATTR_BINDABLE); ;}
    break;

  case 66:
#line 500 "parser.y"
    { (yyval.attr) = make_attr(ATTR_BROADCAST); ;}
    break;

  case 67:
#line 501 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_CALLAS, (yyvsp[-1].var)); ;}
    break;

  case 68:
#line 502 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_CASE, (yyvsp[-1].expr_list)); ;}
    break;

  case 69:
#line 503 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); ;}
    break;

  case 70:
#line 504 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_DONT_SERIALIZE */ ;}
    break;

  case 71:
#line 505 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_SERIALIZE */ ;}
    break;

  case 72:
#line 506 "parser.y"
    { (yyval.attr) = make_attr(ATTR_CONTROL); ;}
    break;

  case 73:
#line 507 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULT); ;}
    break;

  case 74:
#line 508 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULTCOLLELEM); ;}
    break;

  case 75:
#line 509 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_DEFAULTVALUE, (yyvsp[-1].expr)); ;}
    break;

  case 76:
#line 510 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULTVTABLE); ;}
    break;

  case 77:
#line 511 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DISPLAYBIND); ;}
    break;

  case 78:
#line 512 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_DLLNAME, (yyvsp[-1].str)); ;}
    break;

  case 79:
#line 513 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DUAL); ;}
    break;

  case 80:
#line 514 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ENDPOINT, (yyvsp[-1].str_list)); ;}
    break;

  case 81:
#line 515 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ENTRY, (yyvsp[-1].expr)); ;}
    break;

  case 82:
#line 516 "parser.y"
    { (yyval.attr) = make_attr(ATTR_EXPLICIT_HANDLE); ;}
    break;

  case 83:
#line 517 "parser.y"
    { (yyval.attr) = make_attr(ATTR_HANDLE); ;}
    break;

  case 84:
#line 518 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPCONTEXT, (yyvsp[-1].expr)); ;}
    break;

  case 85:
#line 519 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPFILE, (yyvsp[-1].str)); ;}
    break;

  case 86:
#line 520 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRING, (yyvsp[-1].str)); ;}
    break;

  case 87:
#line 521 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRINGCONTEXT, (yyvsp[-1].expr)); ;}
    break;

  case 88:
#line 522 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRINGDLL, (yyvsp[-1].str)); ;}
    break;

  case 89:
#line 523 "parser.y"
    { (yyval.attr) = make_attr(ATTR_HIDDEN); ;}
    break;

  case 90:
#line 524 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ID, (yyvsp[-1].expr)); ;}
    break;

  case 91:
#line 525 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IDEMPOTENT); ;}
    break;

  case 92:
#line 526 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_IIDIS, (yyvsp[-1].expr)); ;}
    break;

  case 93:
#line 527 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IMMEDIATEBIND); ;}
    break;

  case 94:
#line 528 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_IMPLICIT_HANDLE, (yyvsp[-1].str)); ;}
    break;

  case 95:
#line 529 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IN); ;}
    break;

  case 96:
#line 530 "parser.y"
    { (yyval.attr) = make_attr(ATTR_INPUTSYNC); ;}
    break;

  case 97:
#line 531 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_LENGTHIS, (yyvsp[-1].expr_list)); ;}
    break;

  case 98:
#line 532 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_LIBLCID, (yyvsp[-1].expr)); ;}
    break;

  case 99:
#line 533 "parser.y"
    { (yyval.attr) = make_attr(ATTR_LOCAL); ;}
    break;

  case 100:
#line 534 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONBROWSABLE); ;}
    break;

  case 101:
#line 535 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONCREATABLE); ;}
    break;

  case 102:
#line 536 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONEXTENSIBLE); ;}
    break;

  case 103:
#line 537 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OBJECT); ;}
    break;

  case 104:
#line 538 "parser.y"
    { (yyval.attr) = make_attr(ATTR_ODL); ;}
    break;

  case 105:
#line 539 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OLEAUTOMATION); ;}
    break;

  case 106:
#line 540 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OPTIONAL); ;}
    break;

  case 107:
#line 541 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OUT); ;}
    break;

  case 108:
#line 542 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_POINTERDEFAULT, (yyvsp[-1].num)); ;}
    break;

  case 109:
#line 543 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPGET); ;}
    break;

  case 110:
#line 544 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPPUT); ;}
    break;

  case 111:
#line 545 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPPUTREF); ;}
    break;

  case 112:
#line 546 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PUBLIC); ;}
    break;

  case 113:
#line 548 "parser.y"
    { expr_list_t *list = append_expr( NULL, (yyvsp[-3].expr) );
						  list = append_expr( list, (yyvsp[-1].expr) );
						  (yyval.attr) = make_attrp(ATTR_RANGE, list); ;}
    break;

  case 114:
#line 551 "parser.y"
    { (yyval.attr) = make_attr(ATTR_READONLY); ;}
    break;

  case 115:
#line 552 "parser.y"
    { (yyval.attr) = make_attr(ATTR_REQUESTEDIT); ;}
    break;

  case 116:
#line 553 "parser.y"
    { (yyval.attr) = make_attr(ATTR_RESTRICTED); ;}
    break;

  case 117:
#line 554 "parser.y"
    { (yyval.attr) = make_attr(ATTR_RETVAL); ;}
    break;

  case 118:
#line 555 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SIZEIS, (yyvsp[-1].expr_list)); ;}
    break;

  case 119:
#line 556 "parser.y"
    { (yyval.attr) = make_attr(ATTR_SOURCE); ;}
    break;

  case 120:
#line 557 "parser.y"
    { (yyval.attr) = make_attr(ATTR_STRICTCONTEXTHANDLE); ;}
    break;

  case 121:
#line 558 "parser.y"
    { (yyval.attr) = make_attr(ATTR_STRING); ;}
    break;

  case 122:
#line 559 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SWITCHIS, (yyvsp[-1].expr)); ;}
    break;

  case 123:
#line 560 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SWITCHTYPE, (yyvsp[-1].type)); ;}
    break;

  case 124:
#line 561 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_TRANSMITAS, (yyvsp[-1].type)); ;}
    break;

  case 125:
#line 562 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_UUID, (yyvsp[-1].uuid)); ;}
    break;

  case 126:
#line 563 "parser.y"
    { (yyval.attr) = make_attr(ATTR_V1ENUM); ;}
    break;

  case 127:
#line 564 "parser.y"
    { (yyval.attr) = make_attr(ATTR_VARARG); ;}
    break;

  case 128:
#line 565 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_VERSION, (yyvsp[-1].num)); ;}
    break;

  case 129:
#line 566 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_WIREMARSHAL, (yyvsp[-1].type)); ;}
    break;

  case 130:
#line 567 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_POINTERTYPE, (yyvsp[0].num)); ;}
    break;

  case 132:
#line 572 "parser.y"
    { if (!is_valid_uuid((yyvsp[0].str)))
						    error_loc("invalid UUID: %s\n", (yyvsp[0].str));
						  (yyval.uuid) = parse_uuid((yyvsp[0].str)); ;}
    break;

  case 133:
#line 577 "parser.y"
    { (yyval.str) = (yyvsp[0].str); ;}
    break;

  case 134:
#line 578 "parser.y"
    { (yyval.str) = (yyvsp[0].str); ;}
    break;

  case 135:
#line 579 "parser.y"
    { (yyval.str) = (yyvsp[0].str); ;}
    break;

  case 136:
#line 580 "parser.y"
    { (yyval.str) = (yyvsp[0].str); ;}
    break;

  case 137:
#line 583 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 138:
#line 584 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[-1].var_list), (yyvsp[0].var) ); ;}
    break;

  case 139:
#line 587 "parser.y"
    { attr_t *a = make_attrp(ATTR_CASE, append_expr( NULL, (yyvsp[-2].expr) ));
						  (yyval.var) = (yyvsp[0].var); if (!(yyval.var)) (yyval.var) = make_var(NULL);
						  (yyval.var)->attrs = append_attr( (yyval.var)->attrs, a );
						;}
    break;

  case 140:
#line 591 "parser.y"
    { attr_t *a = make_attr(ATTR_DEFAULT);
						  (yyval.var) = (yyvsp[0].var); if (!(yyval.var)) (yyval.var) = make_var(NULL);
						  (yyval.var)->attrs = append_attr( (yyval.var)->attrs, a );
						;}
    break;

  case 141:
#line 597 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 142:
#line 598 "parser.y"
    { (yyval.var_list) = (yyvsp[-1].var_list); ;}
    break;

  case 144:
#line 602 "parser.y"
    { if (!(yyvsp[0].var)->eval)
						    (yyvsp[0].var)->eval = make_exprl(EXPR_NUM, 0 /* default for first enum entry */);
                                                  (yyval.var_list) = append_var( NULL, (yyvsp[0].var) );
						;}
    break;

  case 145:
#line 606 "parser.y"
    { if (!(yyvsp[0].var)->eval)
                                                  {
                                                    var_t *last = LIST_ENTRY( list_tail((yyval.var_list)), var_t, entry );
                                                    (yyvsp[0].var)->eval = make_exprl(EXPR_NUM, last->eval->cval + 1);
                                                  }
                                                  (yyval.var_list) = append_var( (yyvsp[-2].var_list), (yyvsp[0].var) );
						;}
    break;

  case 146:
#line 615 "parser.y"
    { (yyval.var) = reg_const((yyvsp[-2].var));
						  (yyval.var)->eval = (yyvsp[0].expr);
                                                  (yyval.var)->type = make_int(0);
						;}
    break;

  case 147:
#line 619 "parser.y"
    { (yyval.var) = reg_const((yyvsp[0].var));
                                                  (yyval.var)->type = make_int(0);
						;}
    break;

  case 148:
#line 624 "parser.y"
    { (yyval.type) = type_new_enum((yyvsp[-3].str), (yyvsp[-1].var_list)); ;}
    break;

  case 149:
#line 627 "parser.y"
    { (yyval.expr_list) = append_expr( NULL, (yyvsp[0].expr) ); ;}
    break;

  case 150:
#line 628 "parser.y"
    { (yyval.expr_list) = append_expr( (yyvsp[-2].expr_list), (yyvsp[0].expr) ); ;}
    break;

  case 151:
#line 641 "parser.y"
    { (yyval.expr) = make_expr(EXPR_VOID); ;}
    break;

  case 153:
#line 645 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_NUM, (yyvsp[0].num)); ;}
    break;

  case 154:
#line 646 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_HEXNUM, (yyvsp[0].num)); ;}
    break;

  case 155:
#line 647 "parser.y"
    { (yyval.expr) = make_exprd(EXPR_DOUBLE, (yyvsp[0].dbl)); ;}
    break;

  case 156:
#line 648 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_TRUEFALSE, 0); ;}
    break;

  case 157:
#line 649 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_NUM, 0); ;}
    break;

  case 158:
#line 650 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_TRUEFALSE, 1); ;}
    break;

  case 159:
#line 651 "parser.y"
    { (yyval.expr) = make_exprs(EXPR_STRLIT, (yyvsp[0].str)); ;}
    break;

  case 160:
#line 652 "parser.y"
    { (yyval.expr) = make_exprs(EXPR_WSTRLIT, (yyvsp[0].str)); ;}
    break;

  case 161:
#line 653 "parser.y"
    { (yyval.expr) = make_exprs(EXPR_IDENTIFIER, (yyvsp[0].str)); ;}
    break;

  case 162:
#line 654 "parser.y"
    { (yyval.expr) = make_expr3(EXPR_COND, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 163:
#line 655 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_LOGOR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 164:
#line 656 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_LOGAND, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 165:
#line 657 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_OR , (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 166:
#line 658 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_XOR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 167:
#line 659 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_AND, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 168:
#line 660 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_EQUALITY, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 169:
#line 661 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_INEQUALITY, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 170:
#line 662 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_GTR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 171:
#line 663 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_LESS, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 172:
#line 664 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_GTREQL, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 173:
#line 665 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_LESSEQL, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 174:
#line 666 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SHL, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 175:
#line 667 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SHR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 176:
#line 668 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_ADD, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 177:
#line 669 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SUB, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 178:
#line 670 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MOD, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 179:
#line 671 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MUL, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 180:
#line 672 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_DIV, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 181:
#line 673 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_LOGNOT, (yyvsp[0].expr)); ;}
    break;

  case 182:
#line 674 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_NOT, (yyvsp[0].expr)); ;}
    break;

  case 183:
#line 675 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_POS, (yyvsp[0].expr)); ;}
    break;

  case 184:
#line 676 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_NEG, (yyvsp[0].expr)); ;}
    break;

  case 185:
#line 677 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_ADDRESSOF, (yyvsp[0].expr)); ;}
    break;

  case 186:
#line 678 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_PPTR, (yyvsp[0].expr)); ;}
    break;

  case 187:
#line 679 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MEMBER, make_expr1(EXPR_PPTR, (yyvsp[-2].expr)), make_exprs(EXPR_IDENTIFIER, (yyvsp[0].str))); ;}
    break;

  case 188:
#line 680 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MEMBER, (yyvsp[-2].expr), make_exprs(EXPR_IDENTIFIER, (yyvsp[0].str))); ;}
    break;

  case 189:
#line 681 "parser.y"
    { (yyval.expr) = make_exprt(EXPR_CAST, (yyvsp[-2].type), (yyvsp[0].expr)); ;}
    break;

  case 190:
#line 682 "parser.y"
    { (yyval.expr) = make_exprt(EXPR_SIZEOF, (yyvsp[-1].type), NULL); ;}
    break;

  case 191:
#line 683 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_ARRAY, (yyvsp[-3].expr), (yyvsp[-1].expr)); ;}
    break;

  case 192:
#line 684 "parser.y"
    { (yyval.expr) = (yyvsp[-1].expr); ;}
    break;

  case 193:
#line 687 "parser.y"
    { (yyval.expr_list) = append_expr( NULL, (yyvsp[0].expr) ); ;}
    break;

  case 194:
#line 688 "parser.y"
    { (yyval.expr_list) = append_expr( (yyvsp[-2].expr_list), (yyvsp[0].expr) ); ;}
    break;

  case 195:
#line 691 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr);
						  if (!(yyval.expr)->is_const)
						      error_loc("expression is not an integer constant\n");
						;}
    break;

  case 196:
#line 697 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr);
						  if (!(yyval.expr)->is_const && (yyval.expr)->type != EXPR_STRLIT && (yyval.expr)->type != EXPR_WSTRLIT)
						      error_loc("expression is not constant\n");
						;}
    break;

  case 197:
#line 703 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 198:
#line 704 "parser.y"
    { (yyval.var_list) = append_var_list((yyvsp[-1].var_list), (yyvsp[0].var_list)); ;}
    break;

  case 199:
#line 708 "parser.y"
    { const char *first = LIST_ENTRY(list_head((yyvsp[-1].declarator_list)), declarator_t, entry)->var->name;
						  check_field_attrs(first, (yyvsp[-3].attr_list));
						  (yyval.var_list) = set_var_types((yyvsp[-3].attr_list), (yyvsp[-2].declspec), (yyvsp[-1].declarator_list));
						;}
    break;

  case 200:
#line 712 "parser.y"
    { var_t *v = make_var(NULL);
						  v->type = (yyvsp[-1].type); v->attrs = (yyvsp[-2].attr_list);
						  (yyval.var_list) = append_var(NULL, v);
						;}
    break;

  case 201:
#line 719 "parser.y"
    { (yyval.var) = (yyvsp[-1].var); ;}
    break;

  case 202:
#line 720 "parser.y"
    { (yyval.var) = make_var(NULL); (yyval.var)->attrs = (yyvsp[-1].attr_list); ;}
    break;

  case 203:
#line 723 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 204:
#line 724 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[-1].var_list), (yyvsp[0].var) ); ;}
    break;

  case 205:
#line 728 "parser.y"
    { (yyval.var) = (yyvsp[-1].var); ;}
    break;

  case 206:
#line 729 "parser.y"
    { (yyval.var) = NULL; ;}
    break;

  case 207:
#line 732 "parser.y"
    { (yyval.var) = (yyvsp[0].declarator)->var;
						  (yyval.var)->attrs = check_field_attrs((yyval.var)->name, (yyvsp[-2].attr_list));
						  set_type((yyval.var), (yyvsp[-1].declspec), (yyvsp[0].declarator), FALSE);
						  free((yyvsp[0].declarator));
						;}
    break;

  case 208:
#line 740 "parser.y"
    { var_t *v = (yyvsp[0].declarator)->var;
						  v->attrs = check_function_attrs(v->name, (yyvsp[-2].attr_list));
						  set_type(v, (yyvsp[-1].declspec), (yyvsp[0].declarator), FALSE);
						  free((yyvsp[0].declarator));
						  (yyval.func) = make_func(v);
						;}
    break;

  case 209:
#line 750 "parser.y"
    { (yyval.var) = (yyvsp[0].declarator)->var;
						  (yyval.var)->attrs = (yyvsp[-2].attr_list);
						  set_type((yyval.var), (yyvsp[-1].declspec), (yyvsp[0].declarator), FALSE);
						  free((yyvsp[0].declarator));
						;}
    break;

  case 210:
#line 755 "parser.y"
    { (yyval.var) = (yyvsp[0].declarator)->var;
						  set_type((yyval.var), (yyvsp[-1].declspec), (yyvsp[0].declarator), FALSE);
						  free((yyvsp[0].declarator));
						;}
    break;

  case 211:
#line 761 "parser.y"
    { (yyval.var) = NULL; ;}
    break;

  case 213:
#line 765 "parser.y"
    { (yyval.str) = NULL; ;}
    break;

  case 214:
#line 766 "parser.y"
    { (yyval.str) = (yyvsp[0].str); ;}
    break;

  case 215:
#line 767 "parser.y"
    { (yyval.str) = (yyvsp[0].str); ;}
    break;

  case 216:
#line 770 "parser.y"
    { (yyval.var) = make_var((yyvsp[0].str)); ;}
    break;

  case 217:
#line 772 "parser.y"
    { (yyval.var) = make_var((yyvsp[0].str)); ;}
    break;

  case 218:
#line 775 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 219:
#line 776 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 221:
#line 778 "parser.y"
    { (yyval.type) = (yyvsp[0].type); (yyval.type)->sign = 1; ;}
    break;

  case 222:
#line 779 "parser.y"
    { (yyval.type) = (yyvsp[0].type); (yyval.type)->sign = -1;
						  switch ((yyval.type)->type) {
						  case RPC_FC_CHAR:  break;
						  case RPC_FC_SMALL: (yyval.type)->type = RPC_FC_USMALL; break;
						  case RPC_FC_SHORT: (yyval.type)->type = RPC_FC_USHORT; break;
						  case RPC_FC_LONG:  (yyval.type)->type = RPC_FC_ULONG;  break;
						  case RPC_FC_HYPER:
						    if ((yyval.type)->name[0] == 'h') /* hyper, as opposed to __int64 */
                                                    {
                                                      (yyval.type) = type_new_alias((yyval.type), "MIDL_uhyper");
                                                      (yyval.type)->sign = 0;
                                                    }
						    break;
						  default: break;
						  }
						;}
    break;

  case 223:
#line 795 "parser.y"
    { (yyval.type) = make_int(-1); ;}
    break;

  case 224:
#line 796 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 225:
#line 797 "parser.y"
    { (yyval.type) = find_type("float", 0); ;}
    break;

  case 226:
#line 798 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 227:
#line 799 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 228:
#line 800 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 229:
#line 801 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 232:
#line 808 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 233:
#line 809 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[-1].str)); ;}
    break;

  case 234:
#line 810 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 235:
#line 811 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[-1].str)); ;}
    break;

  case 236:
#line 812 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[-1].str)); ;}
    break;

  case 237:
#line 813 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 238:
#line 814 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 239:
#line 817 "parser.y"
    { (yyval.type) = make_class((yyvsp[0].str)); ;}
    break;

  case 240:
#line 818 "parser.y"
    { (yyval.type) = find_type((yyvsp[0].str), 0);
						  if ((yyval.type)->type != RPC_FC_COCLASS)
						    error_loc("%s was not declared a coclass at %s:%d\n",
							      (yyvsp[0].str), (yyval.type)->loc_info.input_name,
							      (yyval.type)->loc_info.line_number);
						;}
    break;

  case 241:
#line 826 "parser.y"
    { (yyval.type) = (yyvsp[0].type);
						  check_def((yyval.type));
						  (yyval.type)->attrs = check_coclass_attrs((yyvsp[0].type)->name, (yyvsp[-1].attr_list));
						;}
    break;

  case 242:
#line 833 "parser.y"
    { (yyval.type) = type_coclass_define((yyvsp[-4].type), (yyvsp[-2].ifref_list)); ;}
    break;

  case 243:
#line 836 "parser.y"
    { (yyval.ifref_list) = NULL; ;}
    break;

  case 244:
#line 837 "parser.y"
    { (yyval.ifref_list) = append_ifref( (yyvsp[-1].ifref_list), (yyvsp[0].ifref) ); ;}
    break;

  case 245:
#line 841 "parser.y"
    { (yyval.ifref) = make_ifref((yyvsp[0].type)); (yyval.ifref)->attrs = (yyvsp[-1].attr_list); ;}
    break;

  case 246:
#line 844 "parser.y"
    { (yyval.type) = get_type(RPC_FC_IP, (yyvsp[0].str), 0); ;}
    break;

  case 247:
#line 845 "parser.y"
    { (yyval.type) = get_type(RPC_FC_IP, (yyvsp[0].str), 0); ;}
    break;

  case 248:
#line 848 "parser.y"
    { attr_t *attrs;
						  is_object_interface = TRUE;
						  (yyval.type) = (yyvsp[0].type);
						  check_def((yyval.type));
						  attrs = make_attr(ATTR_DISPINTERFACE);
						  (yyval.type)->attrs = append_attr( check_dispiface_attrs((yyvsp[0].type)->name, (yyvsp[-1].attr_list)), attrs );
						  (yyval.type)->defined = TRUE;
						;}
    break;

  case 249:
#line 858 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 250:
#line 859 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[-2].var_list), (yyvsp[-1].var) ); ;}
    break;

  case 251:
#line 862 "parser.y"
    { (yyval.stmt_list) = NULL; ;}
    break;

  case 252:
#line 863 "parser.y"
    { (yyval.stmt_list) = append_func( (yyvsp[-2].stmt_list), (yyvsp[-1].func) ); ;}
    break;

  case 253:
#line 869 "parser.y"
    { (yyval.type) = (yyvsp[-4].type);
						  type_dispinterface_define((yyval.type), (yyvsp[-2].var_list), (yyvsp[-1].stmt_list));
						;}
    break;

  case 254:
#line 873 "parser.y"
    { (yyval.type) = (yyvsp[-4].type);
						  type_dispinterface_define_from_iface((yyval.type), (yyvsp[-2].type));
						;}
    break;

  case 255:
#line 878 "parser.y"
    { (yyval.type) = NULL; ;}
    break;

  case 256:
#line 879 "parser.y"
    { (yyval.type) = find_type_or_error2((yyvsp[0].str), 0); ;}
    break;

  case 257:
#line 882 "parser.y"
    { (yyval.type) = get_type(RPC_FC_IP, (yyvsp[0].str), 0); ;}
    break;

  case 258:
#line 883 "parser.y"
    { (yyval.type) = get_type(RPC_FC_IP, (yyvsp[0].str), 0); ;}
    break;

  case 259:
#line 886 "parser.y"
    { (yyval.ifinfo).interface = (yyvsp[0].type);
						  (yyval.ifinfo).old_pointer_default = pointer_default;
						  if (is_attr((yyvsp[-1].attr_list), ATTR_POINTERDEFAULT))
						    pointer_default = get_attrv((yyvsp[-1].attr_list), ATTR_POINTERDEFAULT);
						  is_object_interface = is_object((yyvsp[-1].attr_list));
						  check_def((yyvsp[0].type));
						  (yyvsp[0].type)->attrs = check_iface_attrs((yyvsp[0].type)->name, (yyvsp[-1].attr_list));
						  (yyvsp[0].type)->defined = TRUE;
						;}
    break;

  case 260:
#line 898 "parser.y"
    { (yyval.type) = (yyvsp[-5].ifinfo).interface;
						  type_interface_define((yyval.type), (yyvsp[-4].type), (yyvsp[-2].stmt_list));
						  pointer_default = (yyvsp[-5].ifinfo).old_pointer_default;
						;}
    break;

  case 261:
#line 906 "parser.y"
    { (yyval.type) = (yyvsp[-7].ifinfo).interface;
						  type_interface_define((yyval.type), find_type_or_error2((yyvsp[-5].str), 0), (yyvsp[-2].stmt_list));
						  pointer_default = (yyvsp[-7].ifinfo).old_pointer_default;
						;}
    break;

  case 262:
#line 910 "parser.y"
    { (yyval.type) = (yyvsp[-1].type); ;}
    break;

  case 263:
#line 914 "parser.y"
    { (yyval.type) = (yyvsp[-1].type); ;}
    break;

  case 264:
#line 915 "parser.y"
    { (yyval.type) = (yyvsp[-1].type); ;}
    break;

  case 265:
#line 918 "parser.y"
    { (yyval.type) = type_new_module((yyvsp[0].str)); ;}
    break;

  case 266:
#line 919 "parser.y"
    { (yyval.type) = type_new_module((yyvsp[0].str)); ;}
    break;

  case 267:
#line 922 "parser.y"
    { (yyval.type) = (yyvsp[0].type);
						  (yyval.type)->attrs = check_module_attrs((yyvsp[0].type)->name, (yyvsp[-1].attr_list));
						;}
    break;

  case 268:
#line 928 "parser.y"
    { (yyval.type) = (yyvsp[-4].type);
                                                  type_module_define((yyval.type), (yyvsp[-2].stmt_list));
						;}
    break;

  case 269:
#line 934 "parser.y"
    { (yyval.stgclass) = STG_EXTERN; ;}
    break;

  case 270:
#line 935 "parser.y"
    { (yyval.stgclass) = STG_STATIC; ;}
    break;

  case 271:
#line 936 "parser.y"
    { (yyval.stgclass) = STG_REGISTER; ;}
    break;

  case 272:
#line 940 "parser.y"
    { (yyval.attr) = make_attr(ATTR_INLINE); ;}
    break;

  case 273:
#line 944 "parser.y"
    { (yyval.attr) = make_attr(ATTR_CONST); ;}
    break;

  case 274:
#line 947 "parser.y"
    { (yyval.attr_list) = NULL; ;}
    break;

  case 275:
#line 948 "parser.y"
    { (yyval.attr_list) = append_attr((yyvsp[-1].attr_list), (yyvsp[0].attr)); ;}
    break;

  case 276:
#line 951 "parser.y"
    { (yyval.declspec) = make_decl_spec((yyvsp[-1].type), (yyvsp[0].declspec), NULL, NULL, STG_NONE); ;}
    break;

  case 277:
#line 953 "parser.y"
    { (yyval.declspec) = make_decl_spec((yyvsp[-1].type), (yyvsp[-2].declspec), (yyvsp[0].declspec), NULL, STG_NONE); ;}
    break;

  case 278:
#line 956 "parser.y"
    { (yyval.declspec) = NULL; ;}
    break;

  case 280:
#line 961 "parser.y"
    { (yyval.declspec) = make_decl_spec(NULL, (yyvsp[0].declspec), NULL, (yyvsp[-1].attr), STG_NONE); ;}
    break;

  case 281:
#line 962 "parser.y"
    { (yyval.declspec) = make_decl_spec(NULL, (yyvsp[0].declspec), NULL, (yyvsp[-1].attr), STG_NONE); ;}
    break;

  case 282:
#line 963 "parser.y"
    { (yyval.declspec) = make_decl_spec(NULL, (yyvsp[0].declspec), NULL, NULL, (yyvsp[-1].stgclass)); ;}
    break;

  case 283:
#line 968 "parser.y"
    { (yyval.declarator) = (yyvsp[0].declarator); (yyval.declarator)->type = append_ptrchain_type((yyval.declarator)->type, type_new_pointer(NULL, (yyvsp[-1].attr_list))); ;}
    break;

  case 284:
#line 969 "parser.y"
    { (yyval.declarator) = (yyvsp[0].declarator); (yyval.declarator)->type->attrs = append_attr((yyval.declarator)->type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[-1].str))); ;}
    break;

  case 286:
#line 974 "parser.y"
    { (yyval.declarator) = make_declarator((yyvsp[0].var)); ;}
    break;

  case 287:
#line 975 "parser.y"
    { (yyval.declarator) = (yyvsp[-1].declarator); ;}
    break;

  case 288:
#line 976 "parser.y"
    { (yyval.declarator) = (yyvsp[-1].declarator); (yyval.declarator)->array = append_array((yyval.declarator)->array, (yyvsp[0].expr)); ;}
    break;

  case 289:
#line 977 "parser.y"
    { (yyval.declarator) = (yyvsp[-3].declarator);
						  (yyval.declarator)->func_type = append_ptrchain_type((yyval.declarator)->type, type_new_function((yyvsp[-1].var_list)));
						  (yyval.declarator)->type = NULL;
						;}
    break;

  case 290:
#line 984 "parser.y"
    { (yyval.declarator_list) = append_declarator( NULL, (yyvsp[0].declarator) ); ;}
    break;

  case 291:
#line 985 "parser.y"
    { (yyval.declarator_list) = append_declarator( (yyvsp[-2].declarator_list), (yyvsp[0].declarator) ); ;}
    break;

  case 292:
#line 989 "parser.y"
    { (yyval.declarator) = (yyvsp[0].declarator); ;}
    break;

  case 293:
#line 990 "parser.y"
    { (yyval.declarator) = (yyvsp[-2].declarator); (yyvsp[-2].declarator)->var->eval = (yyvsp[0].expr); ;}
    break;

  case 294:
#line 994 "parser.y"
    { (yyval.num) = RPC_FC_RP; ;}
    break;

  case 295:
#line 995 "parser.y"
    { (yyval.num) = RPC_FC_UP; ;}
    break;

  case 296:
#line 996 "parser.y"
    { (yyval.num) = RPC_FC_FP; ;}
    break;

  case 297:
#line 999 "parser.y"
    { (yyval.type) = type_new_struct((yyvsp[-3].str), TRUE, (yyvsp[-1].var_list)); ;}
    break;

  case 298:
#line 1002 "parser.y"
    { (yyval.type) = find_type_or_error("void", 0); ;}
    break;

  case 299:
#line 1003 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[0].str), 0); ;}
    break;

  case 300:
#line 1004 "parser.y"
    { (yyval.type) = (yyvsp[0].type); ;}
    break;

  case 301:
#line 1005 "parser.y"
    { (yyval.type) = (yyvsp[0].type); ;}
    break;

  case 302:
#line 1006 "parser.y"
    { (yyval.type) = find_type_or_error2((yyvsp[0].str), tsENUM); ;}
    break;

  case 303:
#line 1007 "parser.y"
    { (yyval.type) = (yyvsp[0].type); ;}
    break;

  case 304:
#line 1008 "parser.y"
    { (yyval.type) = type_new_struct((yyvsp[0].str), FALSE, NULL); ;}
    break;

  case 305:
#line 1009 "parser.y"
    { (yyval.type) = (yyvsp[0].type); ;}
    break;

  case 306:
#line 1010 "parser.y"
    { (yyval.type) = find_type_or_error2((yyvsp[0].str), tsUNION); ;}
    break;

  case 307:
#line 1011 "parser.y"
    { (yyval.type) = make_safearray((yyvsp[-1].type)); ;}
    break;

  case 308:
#line 1015 "parser.y"
    { reg_typedefs((yyvsp[-1].declspec), (yyvsp[0].declarator_list), check_typedef_attrs((yyvsp[-2].attr_list)));
						  (yyval.statement) = make_statement_typedef((yyvsp[0].declarator_list));
						;}
    break;

  case 309:
#line 1021 "parser.y"
    { (yyval.type) = type_new_nonencapsulated_union((yyvsp[-3].str), (yyvsp[-1].var_list)); ;}
    break;

  case 310:
#line 1024 "parser.y"
    { (yyval.type) = type_new_encapsulated_union((yyvsp[-8].str), (yyvsp[-5].var), (yyvsp[-3].var), (yyvsp[-1].var_list)); ;}
    break;

  case 311:
#line 1028 "parser.y"
    { (yyval.num) = MAKEVERSION((yyvsp[0].num), 0); ;}
    break;

  case 312:
#line 1029 "parser.y"
    { (yyval.num) = MAKEVERSION((yyvsp[-2].num), (yyvsp[0].num)); ;}
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
#line 4085 "parser.tab.c"

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


#line 1032 "parser.y"


static void decl_builtin(const char *name, unsigned char type)
{
  type_t *t = make_type(type, NULL);
  t->name = xstrdup(name);
  reg_type(t, name, 0);
}

static type_t *make_builtin(char *name)
{
  /* NAME is strdup'd in the lexer */
  type_t *t = duptype(find_type_or_error(name, 0), 0);
  t->name = name;
  return t;
}

static type_t *make_int(int sign)
{
  type_t *t = duptype(find_type_or_error("int", 0), 1);

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
    attr_t *attr_existing;
    if (!attr) return list;
    if (!list)
    {
        list = xmalloc( sizeof(*list) );
        list_init( list );
    }
    LIST_FOR_EACH_ENTRY(attr_existing, list, attr_t, entry)
        if (attr_existing->type == attr->type)
        {
            parser_warning("duplicate attribute %s\n", get_attr_display_name(attr->type));
            /* use the last attribute, like MIDL does */
            list_remove(&attr_existing->entry);
            break;
        }
    list_add_tail( list, &attr->entry );
    return list;
}

static attr_list_t *move_attr(attr_list_t *dst, attr_list_t *src, enum attr_type type)
{
  attr_t *attr;
  if (!src) return dst;
  LIST_FOR_EACH_ENTRY(attr, src, attr_t, entry)
    if (attr->type == type)
    {
      list_remove(&attr->entry);
      return append_attr(dst, attr);
    }
  return dst;
}

static attr_list_t *append_attr_list(attr_list_t *new_list, attr_list_t *old_list)
{
  struct list *entry;

  if (!old_list) return new_list;

  while ((entry = list_head(old_list)))
  {
    attr_t *attr = LIST_ENTRY(entry, attr_t, entry);
    list_remove(entry);
    new_list = append_attr(new_list, attr);
  }
  return new_list;
}

static attr_list_t *dupattrs(const attr_list_t *list)
{
  attr_list_t *new_list;
  const attr_t *attr;

  if (!list) return NULL;

  new_list = xmalloc( sizeof(*list) );
  list_init( new_list );
  LIST_FOR_EACH_ENTRY(attr, list, const attr_t, entry)
  {
    attr_t *new_attr = xmalloc(sizeof(*new_attr));
    *new_attr = *attr;
    list_add_tail(new_list, &new_attr->entry);
  }
  return new_list;
}

static decl_spec_t *make_decl_spec(type_t *type, decl_spec_t *left, decl_spec_t *right, attr_t *attr, enum storage_class stgclass)
{
  decl_spec_t *declspec = left ? left : right;
  if (!declspec)
  {
    declspec = xmalloc(sizeof(*declspec));
    declspec->type = NULL;
    declspec->attrs = NULL;
    declspec->stgclass = STG_NONE;
  }
  declspec->type = type;
  if (left && declspec != left)
  {
    declspec->attrs = append_attr_list(declspec->attrs, left->attrs);
    if (declspec->stgclass == STG_NONE)
      declspec->stgclass = left->stgclass;
    else if (left->stgclass != STG_NONE)
      error_loc("only one storage class can be specified\n");
    assert(!left->type);
    free(left);
  }
  if (right && declspec != right)
  {
    declspec->attrs = append_attr_list(declspec->attrs, right->attrs);
    if (declspec->stgclass == STG_NONE)
      declspec->stgclass = right->stgclass;
    else if (right->stgclass != STG_NONE)
      error_loc("only one storage class can be specified\n");
    assert(!right->type);
    free(right);
  }

  declspec->attrs = append_attr(declspec->attrs, attr);
  if (declspec->stgclass == STG_NONE)
    declspec->stgclass = stgclass;
  else if (stgclass != STG_NONE)
    error_loc("only one storage class can be specified\n");

  /* apply attributes to type */
  if (type && declspec->attrs)
  {
    attr_list_t *attrs;
    declspec->type = duptype(type, 1);
    attrs = dupattrs(type->attrs);
    declspec->type->attrs = append_attr_list(attrs, declspec->attrs);
    declspec->attrs = NULL;
  }

  return declspec;
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

void clear_all_offsets(void)
{
  type_pool_node_t *node;
  LIST_FOR_EACH_ENTRY(node, &type_pool, type_pool_node_t, link)
    node->data.typestring_offset = node->data.ptrdesc = 0;
}

type_t *make_type(unsigned char type, type_t *ref)
{
  type_t *t = alloc_type();
  t->name = NULL;
  t->type = type;
  t->ref = ref;
  t->attrs = NULL;
  t->orig = NULL;
  memset(&t->details, 0, sizeof(t->details));
  t->typestring_offset = 0;
  t->ptrdesc = 0;
  t->declarray = FALSE;
  t->ignore = (parse_only != 0);
  t->sign = 0;
  t->defined = FALSE;
  t->written = FALSE;
  t->user_types_registered = FALSE;
  t->tfswrite = FALSE;
  t->checked = FALSE;
  t->is_alias = FALSE;
  t->typelib_idx = -1;
  init_loc_info(&t->loc_info);
  return t;
}

static type_t *type_new_enum(char *name, var_list_t *enums)
{
    type_t *t = get_type(RPC_FC_ENUM16, name, tsENUM);
    if (enums)
    {
        t->details.enumeration = xmalloc(sizeof(*t->details.enumeration));
        t->details.enumeration->enums = enums;
    }
    else
        t->details.enumeration = NULL;
    t->defined = TRUE;
    return t;
}

static type_t *type_new_struct(char *name, int defined, var_list_t *fields)
{
  type_t *tag_type = name ? find_type(name, tsSTRUCT) : NULL;
  type_t *t = make_type(RPC_FC_STRUCT, NULL);
  t->name = name;
  if (defined || (tag_type && tag_type->details.structure))
  {
    if (tag_type && tag_type->details.structure)
    {
      t->details.structure = tag_type->details.structure;
      t->type = tag_type->type;
    }
    else if (defined)
    {
      t->details.structure = xmalloc(sizeof(*t->details.structure));
      t->details.structure->fields = fields;
      t->defined = TRUE;
    }
  }
  if (name)
  {
    if (fields)
      reg_type(t, name, tsSTRUCT);
    else
      add_incomplete(t);
  }
  return t;
}

static type_t *type_new_nonencapsulated_union(char *name, var_list_t *fields)
{
  type_t *t = get_type(RPC_FC_NON_ENCAPSULATED_UNION, name, tsUNION);
  t->details.structure = xmalloc(sizeof(*t->details.structure));
  t->details.structure->fields = fields;
  t->defined = TRUE;
  return t;
}

static type_t *type_new_encapsulated_union(char *name, var_t *switch_field, var_t *union_field, var_list_t *cases)
{
  type_t *t = get_type(RPC_FC_ENCAPSULATED_UNION, name, tsUNION);
  if (!union_field) union_field = make_var( xstrdup("tagged_union") );
  union_field->type = make_type(RPC_FC_NON_ENCAPSULATED_UNION, NULL);
  union_field->type->details.structure = xmalloc(sizeof(*union_field->type->details.structure));
  union_field->type->details.structure->fields = cases;
  union_field->type->defined = TRUE;
  t->details.structure = xmalloc(sizeof(*t->details.structure));
  t->details.structure->fields = append_var( NULL, switch_field );
  t->details.structure->fields = append_var( t->details.structure->fields, union_field );
  t->defined = TRUE;
  return t;
}

static void type_function_add_head_arg(type_t *type, var_t *arg)
{
    if (!type->details.function->args)
    {
        type->details.function->args = xmalloc( sizeof(*type->details.function->args) );
        list_init( type->details.function->args );
    }
    list_add_head( type->details.function->args, &arg->entry );
}

static type_t *append_ptrchain_type(type_t *ptrchain, type_t *type)
{
  type_t *ptrchain_type;
  if (!ptrchain)
    return type;
  for (ptrchain_type = ptrchain; ptrchain_type->ref; ptrchain_type = ptrchain_type->ref)
    ;
  ptrchain_type->ref = type;
  return ptrchain;
}

static void set_type(var_t *v, decl_spec_t *decl_spec, const declarator_t *decl,
                     int top)
{
  expr_list_t *sizes = get_attrp(v->attrs, ATTR_SIZEIS);
  expr_list_t *lengs = get_attrp(v->attrs, ATTR_LENGTHIS);
  int sizeless;
  expr_t *dim;
  type_t **ptype;
  array_dims_t *arr = decl ? decl->array : NULL;
  type_t *func_type = decl ? decl->func_type : NULL;
  type_t *type = decl_spec->type;

  if (is_attr(type->attrs, ATTR_INLINE))
  {
    if (!func_type)
      error_loc("inline attribute applied to non-function type\n");
    else
    {
      type_t *t;
      /* move inline attribute from return type node to function node */
      for (t = func_type; is_ptr(t); t = type_pointer_get_ref(t))
        ;
      t->attrs = move_attr(t->attrs, type->attrs, ATTR_INLINE);
    }
  }

  /* add type onto the end of the pointers in pident->type */
  v->type = append_ptrchain_type(decl ? decl->type : NULL, type);
  v->stgclass = decl_spec->stgclass;

  /* the highest level of pointer specified should default to the var's ptr attr
   * or (RPC_FC_RP if not specified and it's a top level ptr), not
   * pointer_default so we need to fix that up here */
  if (!arr)
  {
    int ptr_attr = get_attrv(v->attrs, ATTR_POINTERTYPE);
    const type_t *ptr = NULL;
    /* pointer attributes on the left side of the type belong to the function
     * pointer, if one is being declared */
    type_t **pt = func_type ? &func_type : &v->type;
    for (ptr = *pt; ptr && !ptr_attr; )
    {
      ptr_attr = get_attrv(ptr->attrs, ATTR_POINTERTYPE);
      if (!ptr_attr && type_is_alias(ptr))
        ptr = ptr->orig;
      else
        break;
    }
    if (ptr && is_ptr(ptr) && (ptr_attr || top))
    {
      /* duplicate type to avoid changing original type */
      *pt = duptype(*pt, 1);
      (*pt)->type = ptr_attr ? ptr_attr : RPC_FC_RP;
    }
    else if (ptr_attr)
       error_loc("%s: pointer attribute applied to non-pointer type\n", v->name);
  }

  if (is_attr(v->attrs, ATTR_STRING) && !is_ptr(v->type) && !arr)
    error_loc("'%s': [string] attribute applied to non-pointer, non-array type\n",
              v->name);

  if (is_attr(v->attrs, ATTR_V1ENUM))
  {
    if (v->type->type == RPC_FC_ENUM16)
      v->type->type = RPC_FC_ENUM32;
    else
      error_loc("'%s': [v1_enum] attribute applied to non-enum type\n", v->name);
  }

  ptype = &v->type;
  sizeless = FALSE;
  if (arr) LIST_FOR_EACH_ENTRY_REV(dim, arr, expr_t, entry)
  {
    if (sizeless)
      error_loc("%s: only the first array dimension can be unspecified\n", v->name);

    if (dim->is_const)
    {
      if (dim->cval <= 0)
        error_loc("%s: array dimension must be positive\n", v->name);

      /* FIXME: should use a type_memsize that allows us to pass in a pointer size */
      if (0)
      {
        unsigned int align = 0;
        size_t size = type_memsize(v->type, &align);

        if (0xffffffffuL / size < (unsigned long) dim->cval)
          error_loc("%s: total array size is too large\n", v->name);
      }
    }
    else
      sizeless = TRUE;

    *ptype = type_new_array(NULL, *ptype, TRUE,
                            dim->is_const ? dim->cval : 0,
                            dim->is_const ? NULL : dim, NULL);
  }

  ptype = &v->type;
  if (sizes) LIST_FOR_EACH_ENTRY(dim, sizes, expr_t, entry)
  {
    if (dim->type != EXPR_VOID)
    {
      if (is_array(*ptype))
      {
        if (type_array_get_conformance(*ptype)->is_const)
          error_loc("%s: cannot specify size_is for a fixed sized array\n", v->name);
        else
          *ptype = type_new_array((*ptype)->name,
                                  type_array_get_element(*ptype), TRUE,
                                  0, dim, NULL);
      }
      else if (is_ptr(*ptype))
        *ptype = type_new_array((*ptype)->name, type_pointer_get_ref(*ptype), FALSE,
                                0, dim, NULL);
      else
        error_loc("%s: size_is attribute applied to illegal type\n", v->name);
    }

    ptype = &(*ptype)->ref;
    if (*ptype == NULL)
      error_loc("%s: too many expressions in size_is attribute\n", v->name);
  }

  ptype = &v->type;
  if (lengs) LIST_FOR_EACH_ENTRY(dim, lengs, expr_t, entry)
  {
    if (dim->type != EXPR_VOID)
    {
      if (is_array(*ptype))
      {
        *ptype = type_new_array((*ptype)->name,
                                type_array_get_element(*ptype),
                                (*ptype)->declarray,
                                type_array_get_dim(*ptype),
                                type_array_get_conformance(*ptype),
                                dim);
      }
      else
        error_loc("%s: length_is attribute applied to illegal type\n", v->name);
    }

    ptype = &(*ptype)->ref;
    if (*ptype == NULL)
      error_loc("%s: too many expressions in length_is attribute\n", v->name);
  }

  /* v->type is currently pointing to the type on the left-side of the
   * declaration, so we need to fix this up so that it is the return type of the
   * function and make v->type point to the function side of the declaration */
  if (func_type)
  {
    type_t *ft, *t;
    type_t *return_type = v->type;
    v->type = func_type;
    for (ft = v->type; is_ptr(ft); ft = type_pointer_get_ref(ft))
      ;
    assert(ft->type == RPC_FC_FUNCTION);
    ft->ref = return_type;
    /* move calling convention attribute, if present, from pointer nodes to
     * function node */
    for (t = v->type; is_ptr(t); t = type_pointer_get_ref(t))
      ft->attrs = move_attr(ft->attrs, t->attrs, ATTR_CALLCONV);
    if (is_object_interface && !is_attr(ft->attrs, ATTR_CALLCONV))
    {
      static char *stdmethodcalltype;
      if (!stdmethodcalltype) stdmethodcalltype = strdup("STDMETHODCALLTYPE");
      ft->attrs = append_attr(NULL, make_attrp(ATTR_CALLCONV, stdmethodcalltype));
    }
  }
  else
  {
    type_t *t;
    for (t = v->type; is_ptr(t); t = type_pointer_get_ref(t))
      if (is_attr(t->attrs, ATTR_CALLCONV))
        error_loc("calling convention applied to non-function-pointer type\n");
  }
}

static var_list_t *set_var_types(attr_list_t *attrs, decl_spec_t *decl_spec, declarator_list_t *decls)
{
  declarator_t *decl, *next;
  var_list_t *var_list = NULL;

  LIST_FOR_EACH_ENTRY_SAFE( decl, next, decls, declarator_t, entry )
  {
    var_t *var = decl->var;

    var->attrs = attrs;
    set_type(var, decl_spec, decl, 0);
    var_list = append_var(var_list, var);
    free(decl);
  }
  return var_list;
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

static var_list_t *append_var_list(var_list_t *list, var_list_t *vars)
{
    if (!vars) return list;
    if (!list)
    {
        list = xmalloc( sizeof(*list) );
        list_init( list );
    }
    list_move_tail( list, vars );
    return list;
}

static var_t *make_var(char *name)
{
  var_t *v = xmalloc(sizeof(var_t));
  v->name = name;
  v->type = NULL;
  v->attrs = NULL;
  v->eval = NULL;
  v->stgclass = STG_NONE;
  init_loc_info(&v->loc_info);
  return v;
}

static declarator_list_t *append_declarator(declarator_list_t *list, declarator_t *d)
{
  if (!d) return list;
  if (!list) {
    list = xmalloc(sizeof(*list));
    list_init(list);
  }
  list_add_tail(list, &d->entry);
  return list;
}

static declarator_t *make_declarator(var_t *var)
{
  declarator_t *d = xmalloc(sizeof(*d));
  d->var = var;
  d->type = NULL;
  d->func_type = NULL;
  d->array = NULL;
  return d;
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

static func_t *make_func(var_t *def)
{
  func_t *f = xmalloc(sizeof(func_t));
  f->def = def;
  return f;
}

static type_t *make_class(char *name)
{
  type_t *c = make_type(RPC_FC_COCLASS, NULL);
  c->name = name;
  return c;
}

static type_t *make_safearray(type_t *type)
{
  type_t *sa = find_type_or_error("SAFEARRAY", 0);
  sa->ref = type;
  return make_type(pointer_default, sa);
}

static typelib_t *make_library(const char *name, const attr_list_t *attrs)
{
    typelib_t *typelib = xmalloc(sizeof(*typelib));
    typelib->name = xstrdup(name);
    typelib->filename = NULL;
    typelib->attrs = attrs;
    list_init( &typelib->importlibs );
    return typelib;
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
  if ((t == tsSTRUCT || t == tsUNION))
    fix_incomplete_types(type);
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
  if (type_is_alias(t) && is_incomplete(t)) {
    type_t *ot = t->orig;
    fix_type(ot);
    if (is_struct(ot->type) || is_union(ot->type))
      t->details.structure = ot->details.structure;
    t->defined = ot->defined;
  }
}

static void fix_incomplete(void)
{
  struct typenode *tn, *next;

  LIST_FOR_EACH_ENTRY_SAFE(tn, next, &incomplete_types, struct typenode, entry) {
    fix_type(tn->type);
    list_remove(&tn->entry);
    free(tn);
  }
}

static void fix_incomplete_types(type_t *complete_type)
{
  struct typenode *tn, *next;

  LIST_FOR_EACH_ENTRY_SAFE(tn, next, &incomplete_types, struct typenode, entry)
  {
    if (((is_struct(complete_type->type) && is_struct(tn->type->type)) ||
         (is_union(complete_type->type) && is_union(tn->type->type))) &&
        !strcmp(complete_type->name, tn->type->name))
    {
      tn->type->details.structure = complete_type->details.structure;
      tn->type->type = complete_type->type;
      list_remove(&tn->entry);
      free(tn);
    }
  }
}

static type_t *reg_typedefs(decl_spec_t *decl_spec, declarator_list_t *decls, attr_list_t *attrs)
{
  const declarator_t *decl;
  int is_str = is_attr(attrs, ATTR_STRING);
  type_t *type = decl_spec->type;

  if (is_str)
  {
    type_t *t = decl_spec->type;
    unsigned char c;

    while (is_ptr(t))
      t = type_pointer_get_ref(t);

    c = t->type;
    if (c != RPC_FC_CHAR && c != RPC_FC_BYTE && c != RPC_FC_WCHAR)
    {
      decl = LIST_ENTRY( list_head( decls ), const declarator_t, entry );
      error_loc("'%s': [string] attribute is only valid on 'char', 'byte', or 'wchar_t' pointers and arrays\n",
              decl->var->name);
    }
  }

  /* We must generate names for tagless enum, struct or union.
     Typedef-ing a tagless enum, struct or union means we want the typedef
     to be included in a library hence the public attribute.  */
  if ((type->type == RPC_FC_ENUM16 || type->type == RPC_FC_ENUM32 ||
       is_struct(type->type) || is_union(type->type)) &&
      !type->name && !parse_only)
  {
    if (! is_attr(attrs, ATTR_PUBLIC))
      attrs = append_attr( attrs, make_attr(ATTR_PUBLIC) );
    type->name = gen_name();
  }
  else if (is_attr(attrs, ATTR_UUID) && !is_attr(attrs, ATTR_PUBLIC))
    attrs = append_attr( attrs, make_attr(ATTR_PUBLIC) );

  LIST_FOR_EACH_ENTRY( decl, decls, const declarator_t, entry )
  {
    var_t *name = decl->var;

    if (name->name) {
      type_t *cur;

      cur = find_type(name->name, 0);
      if (cur)
          error_loc("%s: redefinition error; original definition was at %s:%d\n",
                    cur->name, cur->loc_info.input_name,
                    cur->loc_info.line_number);

      /* set the attributes to allow set_type to do some checks on them */
      name->attrs = attrs;
      set_type(name, decl_spec, decl, 0);
      cur = type_new_alias(name->type, name->name);
      cur->attrs = attrs;

      if (is_incomplete(cur))
        add_incomplete(cur);
      reg_type(cur, cur->name, 0);
    }
  }
  return type;
}

type_t *find_type(const char *name, int t)
{
  struct rtype *cur = type_hash[hash_ident(name)];
  while (cur && (cur->t != t || strcmp(cur->name, name)))
    cur = cur->next;
  return cur ? cur->type : NULL;
}

static type_t *find_type_or_error(const char *name, int t)
{
  type_t *type = find_type(name, t);
  if (!type) {
    error_loc("type '%s' not found\n", name);
    return NULL;
  }
  return type;
}

static type_t *find_type_or_error2(char *name, int t)
{
  type_t *tp = find_type_or_error(name, t);
  free(name);
  return tp;
}

int is_type(const char *name)
{
  return find_type(name, 0) != NULL;
}

static type_t *get_type(unsigned char type, char *name, int t)
{
  type_t *tp;
  if (name) {
    tp = find_type(name, t);
    if (tp) {
      free(name);
      return tp;
    }
  }
  tp = make_type(type, NULL);
  tp->name = name;
  if (!name) return tp;
  return reg_type(tp, name, t);
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

var_t *find_const(const char *name, int f)
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

struct allowed_attr
{
    unsigned int dce_compatible : 1;
    unsigned int acf : 1;
    unsigned int on_interface : 1;
    unsigned int on_function : 1;
    unsigned int on_arg : 1;
    unsigned int on_type : 1;
    unsigned int on_enum : 1;
    unsigned int on_struct : 1;
    unsigned int on_union : 1;
    unsigned int on_field : 1;
    unsigned int on_library : 1;
    unsigned int on_dispinterface : 1;
    unsigned int on_module : 1;
    unsigned int on_coclass : 1;
    const char *display_name;
};

struct allowed_attr allowed_attr[] =
{
    /* attr                        { D ACF I Fn ARG T En St Un Fi  L  DI M  C  <display name> } */
    /* ATTR_AGGREGATABLE */        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "aggregatable" },
    /* ATTR_APPOBJECT */           { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "appobject" },
    /* ATTR_ASYNC */               { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "async" },
    /* ATTR_AUTO_HANDLE */         { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "auto_handle" },
    /* ATTR_BINDABLE */            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "bindable" },
    /* ATTR_BROADCAST */           { 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "broadcast" },
    /* ATTR_CALLAS */              { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "call_as" },
    /* ATTR_CALLCONV */            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL },
    /* ATTR_CASE */                { 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, "case" },
    /* ATTR_CONST */               { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "const" },
    /* ATTR_CONTEXTHANDLE */       { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "context_handle" },
    /* ATTR_CONTROL */             { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, "control" },
    /* ATTR_DEFAULT */             { 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, "default" },
    /* ATTR_DEFAULTCOLLELEM */     { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "defaultcollelem" },
    /* ATTR_DEFAULTVALUE */        { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "defaultvalue" },
    /* ATTR_DEFAULTVTABLE */       { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "defaultvtable" },
    /* ATTR_DISPINTERFACE */       { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL },
    /* ATTR_DISPLAYBIND */         { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "displaybind" },
    /* ATTR_DLLNAME */             { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, "dllname" },
    /* ATTR_DUAL */                { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "dual" },
    /* ATTR_ENDPOINT */            { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "endpoint" },
    /* ATTR_ENTRY */               { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "entry" },
    /* ATTR_EXPLICIT_HANDLE */     { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "explicit_handle" },
    /* ATTR_HANDLE */              { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "handle" },
    /* ATTR_HELPCONTEXT */         { 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, "helpcontext" },
    /* ATTR_HELPFILE */            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, "helpfile" },
    /* ATTR_HELPSTRING */          { 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, "helpstring" },
    /* ATTR_HELPSTRINGCONTEXT */   { 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, "helpstringcontext" },
    /* ATTR_HELPSTRINGDLL */       { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, "helpstringdll" },
    /* ATTR_HIDDEN */              { 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, "hidden" },
    /* ATTR_ID */                  { 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, "id" },
    /* ATTR_IDEMPOTENT */          { 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "idempotent" },
    /* ATTR_IIDIS */               { 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, "iid_is" },
    /* ATTR_IMMEDIATEBIND */       { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "immediatebind" },
    /* ATTR_IMPLICIT_HANDLE */     { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "implicit_handle" },
    /* ATTR_IN */                  { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "in" },
    /* ATTR_INLINE */              { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "inline" },
    /* ATTR_INPUTSYNC */           { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "inputsync" },
    /* ATTR_LENGTHIS */            { 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, "length_is" },
    /* ATTR_LIBLCID */             { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, "lcid" },
    /* ATTR_LOCAL */               { 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "local" },
    /* ATTR_NONBROWSABLE */        { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "nonbrowsable" },
    /* ATTR_NONCREATABLE */        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "noncreatable" },
    /* ATTR_NONEXTENSIBLE */       { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "nonextensible" },
    /* ATTR_OBJECT */              { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "object" },
    /* ATTR_ODL */                 { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, "odl" },
    /* ATTR_OLEAUTOMATION */       { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "oleautomation" },
    /* ATTR_OPTIONAL */            { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "optional" },
    /* ATTR_OUT */                 { 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "out" },
    /* ATTR_POINTERDEFAULT */      { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "pointer_default" },
    /* ATTR_POINTERTYPE */         { 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, "ref, unique or ptr" },
    /* ATTR_PROPGET */             { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "propget" },
    /* ATTR_PROPPUT */             { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "propput" },
    /* ATTR_PROPPUTREF */          { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "propputref" },
    /* ATTR_PUBLIC */              { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "public" },
    /* ATTR_RANGE */               { 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, "range" },
    /* ATTR_READONLY */            { 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, "readonly" },
    /* ATTR_REQUESTEDIT */         { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "requestedit" },
    /* ATTR_RESTRICTED */          { 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, "restricted" },
    /* ATTR_RETVAL */              { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "retval" },
    /* ATTR_SIZEIS */              { 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, "size_is" },
    /* ATTR_SOURCE */              { 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, "source" },
    /* ATTR_STRICTCONTEXTHANDLE */ { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "strict_context_handle" },
    /* ATTR_STRING */              { 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, "string" },
    /* ATTR_SWITCHIS */            { 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, "switch_is" },
    /* ATTR_SWITCHTYPE */          { 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, "switch_type" },
    /* ATTR_TRANSMITAS */          { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "transmit_as" },
    /* ATTR_UUID */                { 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, "uuid" },
    /* ATTR_V1ENUM */              { 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, "v1_enum" },
    /* ATTR_VARARG */              { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "vararg" },
    /* ATTR_VERSION */             { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, "version" },
    /* ATTR_WIREMARSHAL */         { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "wire_marshal" },
};

const char *get_attr_display_name(enum attr_type type)
{
    return allowed_attr[type].display_name;
}

static attr_list_t *check_iface_attrs(const char *name, attr_list_t *attrs)
{
  const attr_t *attr;
  if (!attrs) return attrs;
  LIST_FOR_EACH_ENTRY(attr, attrs, const attr_t, entry)
  {
    if (!allowed_attr[attr->type].on_interface)
      error_loc("inapplicable attribute %s for interface %s\n",
                allowed_attr[attr->type].display_name, name);
  }
  return attrs;
}

static attr_list_t *check_function_attrs(const char *name, attr_list_t *attrs)
{
  const attr_t *attr;
  if (!attrs) return attrs;
  LIST_FOR_EACH_ENTRY(attr, attrs, const attr_t, entry)
  {
    if (!allowed_attr[attr->type].on_function)
      error_loc("inapplicable attribute %s for function %s\n",
                allowed_attr[attr->type].display_name, name);
  }
  return attrs;
}

static void check_arg(var_t *arg)
{
  const type_t *t = arg->type;
  const attr_t *attr;

  if (t->type == 0 && ! is_var_ptr(arg))
    error_loc("argument '%s' has void type\n", arg->name);

  if (arg->attrs)
  {
    LIST_FOR_EACH_ENTRY(attr, arg->attrs, const attr_t, entry)
    {
      if (!allowed_attr[attr->type].on_arg)
        error_loc("inapplicable attribute %s for argument %s\n",
                  allowed_attr[attr->type].display_name, arg->name);
    }
  }
}

static attr_list_t *check_typedef_attrs(attr_list_t *attrs)
{
  const attr_t *attr;
  if (!attrs) return attrs;
  LIST_FOR_EACH_ENTRY(attr, attrs, const attr_t, entry)
  {
    if (!allowed_attr[attr->type].on_type)
      error_loc("inapplicable attribute %s for typedef\n",
                allowed_attr[attr->type].display_name);
  }
  return attrs;
}

static attr_list_t *check_enum_attrs(attr_list_t *attrs)
{
  const attr_t *attr;
  if (!attrs) return attrs;
  LIST_FOR_EACH_ENTRY(attr, attrs, const attr_t, entry)
  {
    if (!allowed_attr[attr->type].on_enum)
      error_loc("inapplicable attribute %s for enum\n",
                allowed_attr[attr->type].display_name);
  }
  return attrs;
}

static attr_list_t *check_struct_attrs(attr_list_t *attrs)
{
  const attr_t *attr;
  if (!attrs) return attrs;
  LIST_FOR_EACH_ENTRY(attr, attrs, const attr_t, entry)
  {
    if (!allowed_attr[attr->type].on_struct)
      error_loc("inapplicable attribute %s for struct\n",
                allowed_attr[attr->type].display_name);
  }
  return attrs;
}

static attr_list_t *check_union_attrs(attr_list_t *attrs)
{
  const attr_t *attr;
  if (!attrs) return attrs;
  LIST_FOR_EACH_ENTRY(attr, attrs, const attr_t, entry)
  {
    if (!allowed_attr[attr->type].on_union)
      error_loc("inapplicable attribute %s for union\n",
                allowed_attr[attr->type].display_name);
  }
  return attrs;
}

static attr_list_t *check_field_attrs(const char *name, attr_list_t *attrs)
{
  const attr_t *attr;
  if (!attrs) return attrs;
  LIST_FOR_EACH_ENTRY(attr, attrs, const attr_t, entry)
  {
    if (!allowed_attr[attr->type].on_field)
      error_loc("inapplicable attribute %s for field %s\n",
                allowed_attr[attr->type].display_name, name);
  }
  return attrs;
}

static attr_list_t *check_library_attrs(const char *name, attr_list_t *attrs)
{
  const attr_t *attr;
  if (!attrs) return attrs;
  LIST_FOR_EACH_ENTRY(attr, attrs, const attr_t, entry)
  {
    if (!allowed_attr[attr->type].on_library)
      error_loc("inapplicable attribute %s for library %s\n",
                allowed_attr[attr->type].display_name, name);
  }
  return attrs;
}

static attr_list_t *check_dispiface_attrs(const char *name, attr_list_t *attrs)
{
  const attr_t *attr;
  if (!attrs) return attrs;
  LIST_FOR_EACH_ENTRY(attr, attrs, const attr_t, entry)
  {
    if (!allowed_attr[attr->type].on_dispinterface)
      error_loc("inapplicable attribute %s for dispinterface %s\n",
                allowed_attr[attr->type].display_name, name);
  }
  return attrs;
}

static attr_list_t *check_module_attrs(const char *name, attr_list_t *attrs)
{
  const attr_t *attr;
  if (!attrs) return attrs;
  LIST_FOR_EACH_ENTRY(attr, attrs, const attr_t, entry)
  {
    if (!allowed_attr[attr->type].on_module)
      error_loc("inapplicable attribute %s for module %s\n",
                allowed_attr[attr->type].display_name, name);
  }
  return attrs;
}

static attr_list_t *check_coclass_attrs(const char *name, attr_list_t *attrs)
{
  const attr_t *attr;
  if (!attrs) return attrs;
  LIST_FOR_EACH_ENTRY(attr, attrs, const attr_t, entry)
  {
    if (!allowed_attr[attr->type].on_coclass)
      error_loc("inapplicable attribute %s for coclass %s\n",
                allowed_attr[attr->type].display_name, name);
  }
  return attrs;
}

static int is_allowed_conf_type(const type_t *type)
{
    switch (type->type)
    {
    case RPC_FC_CHAR:
    case RPC_FC_SMALL:
    case RPC_FC_BYTE:
    case RPC_FC_USMALL:
    case RPC_FC_WCHAR:
    case RPC_FC_SHORT:
    case RPC_FC_ENUM16:
    case RPC_FC_USHORT:
    case RPC_FC_LONG:
    case RPC_FC_ENUM32:
    case RPC_FC_ULONG:
        return TRUE;
    default:
        return FALSE;
    }
}

static int is_ptr_guid_type(const type_t *type)
{
    unsigned int align = 0;

    /* first, make sure it is a pointer to something */
    if (!is_ptr(type)) return FALSE;

    /* second, make sure it is a pointer to something of size sizeof(GUID),
     * i.e. 16 bytes */
    return (type_memsize(type_pointer_get_ref(type), &align) == 16);
}

static void check_conformance_expr_list(const char *attr_name, const var_t *arg, const type_t *container_type, expr_list_t *expr_list)
{
    expr_t *dim;
    struct expr_loc expr_loc;
    expr_loc.v = arg;
    expr_loc.attr = attr_name;
    if (expr_list) LIST_FOR_EACH_ENTRY(dim, expr_list, expr_t, entry)
    {
        if (dim->type != EXPR_VOID)
        {
            const type_t *expr_type = expr_resolve_type(&expr_loc, container_type, dim);
            if (!is_allowed_conf_type(expr_type))
                error_loc_info(&arg->loc_info, "expression must resolve to integral type <= 32bits for attribute %s\n",
                               attr_name);
        }
    }
}

static void check_remoting_fields(const var_t *var, type_t *type);

/* checks that properties common to fields and arguments are consistent */
static void check_field_common(const type_t *container_type,
                               const char *container_name, const var_t *arg)
{
    type_t *type = arg->type;
    int is_wire_marshal = 0;
    int is_context_handle = 0;
    const char *container_type_name = NULL;

    if (is_struct(container_type->type))
        container_type_name = "struct";
    else if (is_union(container_type->type))
        container_type_name = "union";
    else if (container_type->type == RPC_FC_FUNCTION)
        container_type_name = "function";

    if (is_attr(arg->attrs, ATTR_LENGTHIS) &&
        (is_attr(arg->attrs, ATTR_STRING) || is_aliaschain_attr(arg->type, ATTR_STRING)))
        error_loc_info(&arg->loc_info,
                       "string and length_is specified for argument %s are mutually exclusive attributes\n",
                       arg->name);

    if (is_attr(arg->attrs, ATTR_SIZEIS))
    {
        expr_list_t *size_is_exprs = get_attrp(arg->attrs, ATTR_SIZEIS);
        check_conformance_expr_list("size_is", arg, container_type, size_is_exprs);
    }
    if (is_attr(arg->attrs, ATTR_LENGTHIS))
    {
        expr_list_t *length_is_exprs = get_attrp(arg->attrs, ATTR_LENGTHIS);
        check_conformance_expr_list("length_is", arg, container_type, length_is_exprs);
    }
    if (is_attr(arg->attrs, ATTR_IIDIS))
    {
        struct expr_loc expr_loc;
        expr_t *expr = get_attrp(arg->attrs, ATTR_IIDIS);
        if (expr->type != EXPR_VOID)
        {
            const type_t *expr_type;
            expr_loc.v = arg;
            expr_loc.attr = "iid_is";
            expr_type = expr_resolve_type(&expr_loc, container_type, expr);
            if (!expr_type || !is_ptr_guid_type(expr_type))
                error_loc_info(&arg->loc_info, "expression must resolve to pointer to GUID type for attribute iid_is\n");
        }
    }
    if (is_attr(arg->attrs, ATTR_SWITCHIS))
    {
        struct expr_loc expr_loc;
        expr_t *expr = get_attrp(arg->attrs, ATTR_SWITCHIS);
        if (expr->type != EXPR_VOID)
        {
            const type_t *expr_type;
            expr_loc.v = arg;
            expr_loc.attr = "switch_is";
            expr_type = expr_resolve_type(&expr_loc, container_type, expr);
            if (!expr_type || !is_allowed_conf_type(expr_type))
                error_loc_info(&arg->loc_info, "expression must resolve to integral type <= 32bits for attribute %s\n",
                               expr_loc.attr);
        }
    }

    /* get fundamental type for the argument */
    for (;;)
    {
        if (is_attr(type->attrs, ATTR_WIREMARSHAL))
        {
            is_wire_marshal = 1;
            break;
        }
        if (is_attr(type->attrs, ATTR_CONTEXTHANDLE))
        {
            is_context_handle = 1;
            break;
        }
        if (type_is_alias(type))
            type = type->orig;
        else if (is_ptr(type))
            type = type->ref;
        else if (is_array(type))
            type = type_array_get_element(type);
        else
            break;
    }

    if (type->type == 0 && !is_attr(arg->attrs, ATTR_IIDIS) && !is_wire_marshal && !is_context_handle)
        error_loc_info(&arg->loc_info, "parameter \'%s\' of %s \'%s\' cannot derive from void *\n", arg->name, container_type_name, container_name);
    else if (type->type == RPC_FC_FUNCTION)
        error_loc_info(&arg->loc_info, "parameter \'%s\' of %s \'%s\' cannot be a function pointer\n", arg->name, container_type_name, container_name);
    else if (!is_wire_marshal && (is_struct(type->type) || is_union(type->type)))
        check_remoting_fields(arg, type);
}

static void check_remoting_fields(const var_t *var, type_t *type)
{
    const var_t *field;
    const var_list_t *fields = NULL;

    type = type_get_real_type(type);

    if (type->checked)
        return;

    type->checked = TRUE;

    if (is_struct(type->type))
    {
        if (type_is_complete(type))
            fields = type_struct_get_fields(type);
        else
            error_loc_info(&var->loc_info, "undefined type declaration %s\n", type->name);
    }
    else if (is_union(type->type))
        fields = type_union_get_cases(type);

    if (fields) LIST_FOR_EACH_ENTRY( field, fields, const var_t, entry )
        if (field->type) check_field_common(type, type->name, field);
}

/* checks that arguments for a function make sense for marshalling and unmarshalling */
static void check_remoting_args(const var_t *func)
{
    const char *funcname = func->name;
    const var_t *arg;

    if (func->type->details.function->args) LIST_FOR_EACH_ENTRY( arg, func->type->details.function->args, const var_t, entry )
    {
        int ptr_level = 0;
        const type_t *type = arg->type;

        /* get pointer level and fundamental type for the argument */
        for (;;)
        {
            if (is_attr(type->attrs, ATTR_WIREMARSHAL))
                break;
            if (is_attr(type->attrs, ATTR_CONTEXTHANDLE))
                break;
            if (type_is_alias(type))
                type = type->orig;
            else if (is_ptr(type))
            {
                ptr_level++;
                type = type_pointer_get_ref(type);
            }
            else
                break;
        }

        /* check that [out] parameters have enough pointer levels */
        if (is_attr(arg->attrs, ATTR_OUT))
        {
            if (!is_array(type))
            {
                if (!ptr_level)
                    error_loc_info(&arg->loc_info, "out parameter \'%s\' of function \'%s\' is not a pointer\n", arg->name, funcname);
                if (type->type == RPC_FC_IP && ptr_level == 1)
                    error_loc_info(&arg->loc_info, "out interface pointer \'%s\' of function \'%s\' is not a double pointer\n", arg->name, funcname);
            }
        }

        check_field_common(func->type, funcname, arg);
    }
}

static void add_explicit_handle_if_necessary(var_t *func)
{
    const var_t* explicit_handle_var;
    const var_t* explicit_generic_handle_var = NULL;
    const var_t* context_handle_var = NULL;

    /* check for a defined binding handle */
    explicit_handle_var = get_explicit_handle_var(func);
    if (!explicit_handle_var)
    {
        explicit_generic_handle_var = get_explicit_generic_handle_var(func);
        if (!explicit_generic_handle_var)
        {
            context_handle_var = get_context_handle_var(func);
            if (!context_handle_var)
            {
                /* no explicit handle specified so add
                 * "[in] handle_t IDL_handle" as the first parameter to the
                 * function */
                var_t *idl_handle = make_var(xstrdup("IDL_handle"));
                idl_handle->attrs = append_attr(NULL, make_attr(ATTR_IN));
                idl_handle->type = find_type_or_error("handle_t", 0);
                type_function_add_head_arg(func->type, idl_handle);
            }
        }
    }
}

static void check_functions(const type_t *iface, int is_inside_library)
{
    const statement_t *stmt;
    if (is_attr(iface->attrs, ATTR_EXPLICIT_HANDLE))
    {
        STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
        {
            var_t *func = stmt->u.var;
            add_explicit_handle_if_necessary(func);
        }
    }
    if (!is_inside_library && !is_attr(iface->attrs, ATTR_LOCAL))
    {
        STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
        {
            const var_t *func = stmt->u.var;
            if (!is_attr(func->attrs, ATTR_LOCAL))
                check_remoting_args(func);
        }
    }
}

static void check_statements(const statement_list_t *stmts, int is_inside_library)
{
    const statement_t *stmt;

    if (stmts) LIST_FOR_EACH_ENTRY(stmt, stmts, const statement_t, entry)
    {
      if (stmt->type == STMT_LIBRARY)
          check_statements(stmt->u.lib->stmts, TRUE);
      else if (stmt->type == STMT_TYPE && stmt->u.type->type == RPC_FC_IP)
          check_functions(stmt->u.type, is_inside_library);
    }
}

static void check_all_user_types(const statement_list_t *stmts)
{
  const statement_t *stmt;

  if (stmts) LIST_FOR_EACH_ENTRY(stmt, stmts, const statement_t, entry)
  {
    if (stmt->type == STMT_LIBRARY)
      check_all_user_types(stmt->u.lib->stmts);
    else if (stmt->type == STMT_TYPE && stmt->u.type->type == RPC_FC_IP &&
             !is_local(stmt->u.type->attrs))
    {
      const statement_t *stmt_func;
      STATEMENTS_FOR_EACH_FUNC(stmt_func, type_iface_get_stmts(stmt->u.type)) {
        const var_t *func = stmt_func->u.var;
        check_for_additional_prototype_types(func->type->details.function->args);
      }
    }
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

static statement_t *make_statement(enum statement_type type)
{
    statement_t *stmt = xmalloc(sizeof(*stmt));
    stmt->type = type;
    return stmt;
}

static statement_t *make_statement_type_decl(type_t *type)
{
    statement_t *stmt = make_statement(STMT_TYPE);
    stmt->u.type = type;
    return stmt;
}

static statement_t *make_statement_reference(type_t *type)
{
    statement_t *stmt = make_statement(STMT_TYPEREF);
    stmt->u.type = type;
    return stmt;
}

static statement_t *make_statement_declaration(var_t *var)
{
    statement_t *stmt = make_statement(STMT_DECLARATION);
    stmt->u.var = var;
    if (var->stgclass == STG_EXTERN && var->eval)
        warning("'%s' initialised and declared extern\n", var->name);
    if (is_const_decl(var))
    {
        if (var->eval)
            reg_const(var);
    }
    else if ((var->stgclass == STG_NONE || var->stgclass == STG_REGISTER) &&
	     var->type->type != RPC_FC_FUNCTION)
        error_loc("instantiation of data is illegal\n");
    return stmt;
}

static statement_t *make_statement_library(typelib_t *typelib)
{
    statement_t *stmt = make_statement(STMT_LIBRARY);
    stmt->u.lib = typelib;
    return stmt;
}

static statement_t *make_statement_cppquote(const char *str)
{
    statement_t *stmt = make_statement(STMT_CPPQUOTE);
    stmt->u.str = str;
    return stmt;
}

static statement_t *make_statement_importlib(const char *str)
{
    statement_t *stmt = make_statement(STMT_IMPORTLIB);
    stmt->u.str = str;
    return stmt;
}

static statement_t *make_statement_import(const char *str)
{
    statement_t *stmt = make_statement(STMT_IMPORT);
    stmt->u.str = str;
    return stmt;
}

static statement_t *make_statement_module(type_t *type)
{
    statement_t *stmt = make_statement(STMT_MODULE);
    stmt->u.type = type;
    return stmt;
}

static statement_t *make_statement_typedef(declarator_list_t *decls)
{
    declarator_t *decl, *next;
    statement_t *stmt;
    type_list_t **type_list;

    if (!decls) return NULL;

    stmt = make_statement(STMT_TYPEDEF);
    stmt->u.type_list = NULL;
    type_list = &stmt->u.type_list;

    LIST_FOR_EACH_ENTRY_SAFE( decl, next, decls, declarator_t, entry )
    {
        var_t *var = decl->var;
        type_t *type = find_type_or_error(var->name, 0);
        *type_list = xmalloc(sizeof(type_list_t));
        (*type_list)->type = type;
        (*type_list)->next = NULL;

        type_list = &(*type_list)->next;
        free(decl);
        free(var);
    }

    return stmt;
}

static statement_list_t *append_statement(statement_list_t *list, statement_t *stmt)
{
    if (!stmt) return list;
    if (!list)
    {
        list = xmalloc( sizeof(*list) );
        list_init( list );
    }
    list_add_tail( list, &stmt->entry );
    return list;
}

void init_loc_info(loc_info_t *i)
{
    i->input_name = input_name ? input_name : "stdin";
    i->line_number = line_number;
    i->near_text = parser_text;
}

static void check_def(const type_t *t)
{
    if (t->defined)
        error_loc("%s: redefinition error; original definition was at %s:%d\n",
                  t->name, t->loc_info.input_name, t->loc_info.line_number);
}

