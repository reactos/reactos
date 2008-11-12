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
static int is_in_interface = FALSE;
static int is_object_interface = FALSE;
/* are we inside a library block? */
static int is_inside_library = FALSE;

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
static type_t *make_func_type(var_list_t *args);
static type_t *make_pointer_type(type_t *ref, attr_list_t *attrs);
static type_t *append_ptrchain_type(type_t *ptrchain, type_t *type);

static type_t *reg_type(type_t *type, const char *name, int t);
static type_t *reg_typedefs(decl_spec_t *decl_spec, var_list_t *names, attr_list_t *attrs);
static type_t *find_type2(char *name, int t);
static type_t *get_type(unsigned char type, char *name, int t);
static type_t *get_typev(unsigned char type, var_t *name, int t);
static int get_struct_type(var_list_t *fields);

static var_t *reg_const(var_t *var);

static void write_libid(const typelib_t *typelib);
static void write_clsid(type_t *cls);
static void write_diid(type_t *iface);
static void write_iid(type_t *iface);

static int compute_method_indexes(type_t *iface);
static char *gen_name(void);
static statement_t *process_typedefs(var_list_t *names);
static void check_arg(var_t *arg);
static void check_functions(const type_t *iface);
static void check_all_user_types(const statement_list_t *stmts);
static attr_list_t *check_iface_attrs(const char *name, attr_list_t *attrs);
static attr_list_t *check_function_attrs(const char *name, attr_list_t *attrs);
static attr_list_t *check_typedef_attrs(attr_list_t *attrs);
static attr_list_t *check_field_attrs(const char *name, attr_list_t *attrs);
static attr_list_t *check_library_attrs(const char *name, attr_list_t *attrs);
static attr_list_t *check_dispiface_attrs(const char *name, attr_list_t *attrs);
static attr_list_t *check_module_attrs(const char *name, attr_list_t *attrs);
static attr_list_t *check_coclass_attrs(const char *name, attr_list_t *attrs);
const char *get_attr_display_name(enum attr_type type);
static void add_explicit_handle_if_necessary(func_t *func);

static statement_t *make_statement(enum statement_type type);
static statement_t *make_statement_type_decl(type_t *type);
static statement_t *make_statement_reference(type_t *type);
static statement_t *make_statement_declaration(var_t *var);
static statement_t *make_statement_library(typelib_t *typelib);
static statement_t *make_statement_cppquote(const char *str);
static statement_t *make_statement_importlib(const char *str);
static statement_t *make_statement_module(type_t *type);
static statement_t *make_statement_import(const char *str);
static statement_list_t *append_statement(statement_list_t *list, statement_t *stmt);
static func_list_t *append_func_from_statement(func_list_t *list, statement_t *stmt);

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
#line 177 "parser.y"
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
#line 601 "parser.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */
#line 613 "parser.tab.c"

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
#define YYLAST   2015

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  176
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  87
/* YYNRULES -- Number of rules. */
#define YYNRULES  308
/* YYNRULES -- Number of states. */
#define YYNSTATES  546

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
      77,    80,    85,    89,    93,    99,   102,   106,   111,   112,
     114,   116,   118,   122,   124,   128,   131,   135,   139,   140,
     142,   146,   148,   152,   157,   159,   163,   164,   166,   168,
     170,   172,   174,   176,   181,   186,   188,   190,   192,   194,
     196,   198,   203,   205,   207,   212,   214,   219,   224,   226,
     228,   233,   238,   243,   248,   253,   255,   260,   262,   267,
     269,   275,   277,   279,   284,   289,   291,   293,   295,   297,
     299,   301,   303,   305,   307,   312,   314,   316,   318,   320,
     327,   329,   331,   333,   335,   340,   342,   344,   346,   351,
     356,   361,   366,   368,   370,   375,   380,   382,   384,   386,
     388,   390,   392,   394,   395,   398,   403,   407,   408,   411,
     413,   415,   419,   423,   425,   431,   433,   437,   438,   440,
     442,   444,   446,   448,   450,   452,   454,   456,   458,   464,
     468,   472,   476,   480,   484,   488,   492,   496,   500,   504,
     508,   512,   516,   520,   524,   528,   532,   536,   539,   542,
     545,   548,   551,   554,   558,   562,   567,   572,   577,   581,
     583,   587,   589,   591,   592,   595,   600,   604,   607,   610,
     611,   614,   617,   619,   623,   627,   631,   634,   635,   637,
     638,   640,   642,   644,   646,   648,   650,   652,   655,   658,
     660,   662,   664,   666,   668,   670,   672,   673,   675,   677,
     680,   682,   685,   688,   690,   692,   695,   698,   701,   707,
     708,   711,   714,   717,   720,   723,   726,   730,   733,   737,
     743,   749,   750,   753,   756,   759,   762,   769,   778,   781,
     784,   787,   790,   793,   796,   802,   804,   806,   808,   810,
     812,   813,   816,   819,   823,   824,   826,   829,   832,   835,
     839,   842,   844,   846,   850,   853,   858,   860,   864,   866,
     870,   872,   874,   876,   882,   884,   886,   888,   890,   893,
     895,   898,   900,   903,   908,   913,   919,   930,   932
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short int yyrhs[] =
{
     177,     0,    -1,   178,    -1,    -1,   178,   242,    -1,   178,
     241,    -1,   178,   228,   170,    -1,   178,   230,    -1,   178,
     245,    -1,   178,   189,    -1,   178,   182,    -1,    -1,   179,
     242,    -1,   179,   241,    -1,   179,   228,   170,    -1,   179,
     230,    -1,   179,   245,    -1,   179,   182,    -1,   179,   186,
      -1,   179,   189,    -1,    -1,   180,   182,    -1,    -1,   170,
      -1,   183,    -1,   207,   170,    -1,   221,   170,    -1,   185,
      -1,   258,   170,    -1,   260,   170,    -1,   261,   170,    -1,
      45,   171,     8,   172,    -1,    78,     8,   170,    -1,   184,
     179,    11,    -1,    79,   171,     8,   172,   181,    -1,    89,
       3,    -1,   196,   187,   173,    -1,   188,   179,   174,   181,
      -1,    -1,   192,    -1,   144,    -1,   193,    -1,   192,   147,
     193,    -1,   191,    -1,   196,   250,   253,    -1,   250,   253,
      -1,   168,   209,   169,    -1,   168,   157,   169,    -1,    -1,
     196,    -1,   168,   197,   169,    -1,   199,    -1,   197,   147,
     199,    -1,   197,   169,   168,   199,    -1,     8,    -1,   198,
     147,     8,    -1,    -1,    21,    -1,    23,    -1,    24,    -1,
      26,    -1,    27,    -1,    29,    -1,    32,   171,   224,   172,
      -1,    34,   171,   211,   172,    -1,    41,    -1,    42,    -1,
      43,    -1,    44,    -1,    46,    -1,    47,    -1,    48,   171,
     213,   172,    -1,    49,    -1,    50,    -1,    52,   171,     8,
     172,    -1,    54,    -1,    55,   171,   198,   172,    -1,    56,
     171,   213,   172,    -1,    59,    -1,    64,    -1,    66,   171,
     212,   172,    -1,    67,   171,     8,   172,    -1,    68,   171,
       8,   172,    -1,    69,   171,   212,   172,    -1,    70,   171,
       8,   172,    -1,    71,    -1,    73,   171,   212,   172,    -1,
      74,    -1,    75,   171,   210,   172,    -1,    76,    -1,    77,
     171,    65,     3,   172,    -1,    80,    -1,    83,    -1,    88,
     171,   208,   172,    -1,    87,   171,   212,   172,    -1,    90,
      -1,    94,    -1,    95,    -1,    96,    -1,    98,    -1,    99,
      -1,   100,    -1,   101,    -1,   102,    -1,   104,   171,   257,
     172,    -1,   106,    -1,   107,    -1,   108,    -1,   110,    -1,
     111,   171,   212,   147,   212,   172,    -1,   112,    -1,   115,
      -1,   116,    -1,   117,    -1,   122,   171,   208,   172,    -1,
     125,    -1,   128,    -1,   129,    -1,   132,   171,   210,   172,
      -1,   133,   171,   259,   172,    -1,   134,   171,   259,   172,
      -1,   140,   171,   200,   172,    -1,   141,    -1,   142,    -1,
     143,   171,   262,   172,    -1,   146,   171,   259,   172,    -1,
     257,    -1,    10,    -1,     8,    -1,    35,    -1,    62,    -1,
     103,    -1,   127,    -1,    -1,   202,   203,    -1,    34,   212,
     149,   218,    -1,    46,   149,   218,    -1,    -1,   205,   147,
      -1,   205,    -1,   206,    -1,   205,   147,   206,    -1,   224,
     175,   212,    -1,   224,    -1,    57,   223,   173,   204,   174,
      -1,   209,    -1,   208,   147,   209,    -1,    -1,   210,    -1,
       5,    -1,     6,    -1,     7,    -1,    61,    -1,    97,    -1,
     135,    -1,     8,    -1,     9,    -1,     3,    -1,   210,   148,
     210,   149,   210,    -1,   210,    19,   210,    -1,   210,    20,
     210,    -1,   210,   150,   210,    -1,   210,   151,   210,    -1,
     210,   152,   210,    -1,   210,    15,   210,    -1,   210,    16,
     210,    -1,   210,   154,   210,    -1,   210,   153,   210,    -1,
     210,    17,   210,    -1,   210,    18,   210,    -1,   210,    12,
     210,    -1,   210,    13,   210,    -1,   210,   156,   210,    -1,
     210,   155,   210,    -1,   210,   159,   210,    -1,   210,   157,
     210,    -1,   210,   158,   210,    -1,   160,   210,    -1,   161,
     210,    -1,   156,   210,    -1,   155,   210,    -1,   152,   210,
      -1,   157,   210,    -1,   210,    14,     3,    -1,   210,   167,
       3,    -1,   171,   259,   172,   210,    -1,   123,   171,   259,
     172,    -1,   210,   168,   210,   169,    -1,   171,   210,   172,
      -1,   212,    -1,   211,   147,   212,    -1,   210,    -1,   210,
      -1,    -1,   214,   215,    -1,   195,   250,   255,   170,    -1,
     195,   261,   170,    -1,   219,   170,    -1,   196,   170,    -1,
      -1,   217,   216,    -1,   219,   170,    -1,   170,    -1,   195,
     250,   253,    -1,   195,   250,   253,    -1,   196,   250,   256,
      -1,   250,   256,    -1,    -1,   224,    -1,    -1,     3,    -1,
       4,    -1,     3,    -1,     4,    -1,    30,    -1,   145,    -1,
     227,    -1,   120,   227,    -1,   139,   227,    -1,   139,    -1,
      63,    -1,   121,    -1,    53,    -1,    28,    -1,    58,    -1,
      65,    -1,    -1,    84,    -1,    84,    -1,   119,   226,    -1,
     124,    -1,    91,   226,    -1,    72,   226,    -1,    85,    -1,
      36,    -1,    37,     3,    -1,    37,     4,    -1,   196,   228,
      -1,   229,   173,   231,   174,   181,    -1,    -1,   231,   232,
      -1,   195,   242,    -1,    51,     3,    -1,    51,     4,    -1,
     196,   233,    -1,   105,   149,    -1,   235,   219,   170,    -1,
      92,   149,    -1,   236,   220,   170,    -1,   234,   173,   235,
     236,   174,    -1,   234,   173,   239,   170,   174,    -1,    -1,
     149,     4,    -1,    86,     3,    -1,    86,     4,    -1,   196,
     239,    -1,   240,   238,   173,   180,   174,   181,    -1,   240,
     149,     3,   173,   185,   180,   174,   181,    -1,   237,   181,
      -1,   239,   170,    -1,   233,   170,    -1,    93,     3,    -1,
      93,     4,    -1,   196,   243,    -1,   244,   173,   180,   174,
     181,    -1,    60,    -1,   126,    -1,   114,    -1,    82,    -1,
      40,    -1,    -1,   249,   248,    -1,   259,   251,    -1,   252,
     259,   251,    -1,    -1,   252,    -1,   248,   251,    -1,   247,
     251,    -1,   246,   251,    -1,   157,   249,   253,    -1,   201,
     253,    -1,   254,    -1,   224,    -1,   171,   253,   172,    -1,
     254,   194,    -1,   254,   171,   190,   172,    -1,   253,    -1,
     255,   147,   253,    -1,   253,    -1,   253,   175,   213,    -1,
     113,    -1,   138,    -1,   109,    -1,   130,   223,   173,   214,
     174,    -1,   144,    -1,     4,    -1,   225,    -1,   207,    -1,
      57,     3,    -1,   258,    -1,   130,     3,    -1,   261,    -1,
     137,     3,    -1,   118,   171,   259,   172,    -1,   136,   195,
     250,   255,    -1,   137,   223,   173,   217,   174,    -1,   137,
     223,   131,   171,   219,   172,   222,   173,   202,   174,    -1,
       5,    -1,     5,   167,     5,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   349,   349,   358,   359,   360,   361,   365,   370,   373,
     374,   377,   378,   379,   380,   381,   386,   387,   388,   389,
     392,   393,   396,   397,   401,   402,   408,   411,   412,   418,
     419,   427,   429,   437,   444,   448,   450,   457,   465,   466,
     469,   472,   473,   474,   478,   485,   493,   494,   497,   498,
     502,   508,   509,   510,   513,   514,   517,   518,   519,   520,
     521,   522,   523,   524,   525,   526,   527,   528,   529,   530,
     531,   532,   533,   534,   535,   536,   537,   538,   539,   540,
     541,   542,   543,   544,   545,   546,   547,   548,   549,   550,
     551,   552,   553,   554,   555,   556,   557,   558,   559,   560,
     561,   562,   563,   564,   565,   566,   567,   568,   569,   570,
     574,   575,   576,   577,   578,   579,   580,   581,   582,   583,
     584,   585,   586,   587,   588,   589,   590,   594,   595,   600,
     601,   602,   603,   606,   607,   610,   614,   620,   621,   622,
     625,   629,   638,   642,   647,   656,   657,   670,   671,   674,
     675,   676,   677,   678,   679,   680,   681,   682,   683,   684,
     685,   686,   687,   688,   689,   690,   691,   692,   693,   694,
     695,   696,   697,   698,   699,   700,   701,   702,   703,   704,
     705,   706,   707,   708,   709,   710,   711,   712,   713,   716,
     717,   720,   726,   732,   733,   736,   741,   748,   749,   752,
     753,   757,   758,   761,   769,   778,   784,   790,   791,   794,
     795,   796,   799,   801,   804,   805,   806,   807,   808,   824,
     825,   826,   827,   828,   829,   830,   833,   834,   837,   838,
     839,   840,   841,   842,   843,   846,   847,   853,   862,   869,
     870,   874,   877,   878,   881,   895,   896,   899,   900,   903,
     913,   923,   924,   927,   928,   931,   944,   958,   971,   975,
     976,   979,   980,   983,   988,   996,   997,   998,  1002,  1006,
    1009,  1010,  1013,  1014,  1018,  1019,  1023,  1024,  1025,  1029,
    1031,  1032,  1036,  1037,  1038,  1039,  1046,  1047,  1051,  1052,
    1056,  1057,  1058,  1061,  1072,  1073,  1074,  1075,  1076,  1077,
    1078,  1079,  1080,  1081,  1084,  1090,  1096,  1113,  1114
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
  "cppquote", "import_start", "import", "importlib", "libraryhdr",
  "library_start", "librarydef", "m_args", "no_args", "args", "arg",
  "array", "m_attributes", "attributes", "attrib_list", "str_list",
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
     180,   180,   181,   181,   182,   182,   182,   182,   182,   182,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   190,
     191,   192,   192,   192,   193,   193,   194,   194,   195,   195,
     196,   197,   197,   197,   198,   198,   199,   199,   199,   199,
     199,   199,   199,   199,   199,   199,   199,   199,   199,   199,
     199,   199,   199,   199,   199,   199,   199,   199,   199,   199,
     199,   199,   199,   199,   199,   199,   199,   199,   199,   199,
     199,   199,   199,   199,   199,   199,   199,   199,   199,   199,
     199,   199,   199,   199,   199,   199,   199,   199,   199,   199,
     199,   199,   199,   199,   199,   199,   199,   199,   199,   199,
     199,   199,   199,   199,   199,   199,   199,   200,   200,   201,
     201,   201,   201,   202,   202,   203,   203,   204,   204,   204,
     205,   205,   206,   206,   207,   208,   208,   209,   209,   210,
     210,   210,   210,   210,   210,   210,   210,   210,   210,   210,
     210,   210,   210,   210,   210,   210,   210,   210,   210,   210,
     210,   210,   210,   210,   210,   210,   210,   210,   210,   210,
     210,   210,   210,   210,   210,   210,   210,   210,   210,   211,
     211,   212,   213,   214,   214,   215,   215,   216,   216,   217,
     217,   218,   218,   219,   220,   221,   221,   222,   222,   223,
     223,   223,   224,   224,   225,   225,   225,   225,   225,   225,
     225,   225,   225,   225,   225,   225,   226,   226,   227,   227,
     227,   227,   227,   227,   227,   228,   228,   229,   230,   231,
     231,   232,   233,   233,   234,   235,   235,   236,   236,   237,
     237,   238,   238,   239,   239,   240,   241,   241,   241,   242,
     242,   243,   243,   244,   245,   246,   246,   246,   247,   248,
     249,   249,   250,   250,   251,   251,   252,   252,   252,   253,
     253,   253,   254,   254,   254,   254,   255,   255,   256,   256,
     257,   257,   257,   258,   259,   259,   259,   259,   259,   259,
     259,   259,   259,   259,   260,   261,   261,   262,   262
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     0,     2,     2,     3,     2,     2,     2,
       2,     0,     2,     2,     3,     2,     2,     2,     2,     2,
       0,     2,     0,     1,     1,     2,     2,     1,     2,     2,
       2,     4,     3,     3,     5,     2,     3,     4,     0,     1,
       1,     1,     3,     1,     3,     2,     3,     3,     0,     1,
       3,     1,     3,     4,     1,     3,     0,     1,     1,     1,
       1,     1,     1,     4,     4,     1,     1,     1,     1,     1,
       1,     4,     1,     1,     4,     1,     4,     4,     1,     1,
       4,     4,     4,     4,     4,     1,     4,     1,     4,     1,
       5,     1,     1,     4,     4,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     4,     1,     1,     1,     1,     6,
       1,     1,     1,     1,     4,     1,     1,     1,     4,     4,
       4,     4,     1,     1,     4,     4,     1,     1,     1,     1,
       1,     1,     1,     0,     2,     4,     3,     0,     2,     1,
       1,     3,     3,     1,     5,     1,     3,     0,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     5,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     2,     2,     2,
       2,     2,     2,     3,     3,     4,     4,     4,     3,     1,
       3,     1,     1,     0,     2,     4,     3,     2,     2,     0,
       2,     2,     1,     3,     3,     3,     2,     0,     1,     0,
       1,     1,     1,     1,     1,     1,     1,     2,     2,     1,
       1,     1,     1,     1,     1,     1,     0,     1,     1,     2,
       1,     2,     2,     1,     1,     2,     2,     2,     5,     0,
       2,     2,     2,     2,     2,     2,     3,     2,     3,     5,
       5,     0,     2,     2,     2,     2,     6,     8,     2,     2,
       2,     2,     2,     2,     5,     1,     1,     1,     1,     1,
       0,     2,     2,     3,     0,     1,     2,     2,     2,     3,
       2,     1,     1,     3,     2,     4,     1,     3,     1,     3,
       1,     1,     1,     5,     1,     1,     1,     1,     2,     1,
       2,     1,     2,     4,     4,     5,    10,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned short int yydefact[] =
{
       3,     0,     2,     1,   295,   223,   214,   234,     0,   269,
       0,     0,   222,   209,   224,   265,   220,   225,   226,     0,
     268,   228,   233,     0,   226,   267,     0,   226,     0,   221,
     230,   266,   209,    48,   209,   219,   294,   215,    56,    10,
      24,    11,    27,    11,     9,     0,   297,     0,   296,   216,
       0,     0,     7,     0,     0,    22,     0,   251,     5,     4,
       0,     8,   274,   274,   274,     0,     0,   299,   274,     0,
     301,   235,   236,     0,   242,   243,   298,   211,     0,   227,
     232,     0,   253,   254,   231,     0,   229,   217,   300,     0,
       0,    49,   302,     0,   218,    57,    58,    59,    60,    61,
      62,     0,     0,    65,    66,    67,    68,    69,    70,     0,
      72,    73,     0,    75,     0,     0,    78,    79,     0,     0,
       0,     0,     0,    85,     0,    87,     0,    89,     0,    91,
      92,     0,     0,    95,    96,    97,    98,    99,   100,   101,
     102,   103,     0,   105,   106,   107,   292,   108,     0,   110,
     290,   111,   112,   113,     0,   115,   116,   117,     0,     0,
       0,   291,     0,   122,   123,     0,     0,     0,    51,   126,
       0,     0,     0,     0,     0,   297,   237,   244,   255,   263,
       0,   299,   301,    25,    26,     6,   239,   260,     0,    23,
     258,   259,     0,     0,    20,   278,   275,   277,   276,   212,
     213,   129,   130,   131,   132,   270,     0,     0,   282,   288,
     281,   206,   274,    28,   272,    29,    30,     0,   137,    32,
       0,   193,     0,     0,   199,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     147,     0,     0,   147,     0,     0,     0,     0,     0,     0,
      56,    50,    33,     0,    17,    18,    19,     0,    15,    13,
      12,    16,    22,    35,   261,   262,    36,   205,    48,     0,
      48,     0,     0,   252,    20,     0,     0,     0,   280,     0,
     147,    38,   284,   273,    31,     0,   139,   140,   143,   303,
      48,   286,   304,    48,    48,     0,   157,   149,   150,   151,
     155,   156,   152,   153,     0,   154,     0,     0,     0,     0,
       0,     0,     0,   191,     0,   189,   192,     0,     0,    54,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   145,   148,     0,     0,     0,     0,     0,     0,
     128,   127,     0,   307,     0,     0,    52,    56,     0,    14,
      37,    22,     0,   240,   245,     0,     0,     0,    48,     0,
       0,     0,    22,    21,     0,   271,   279,   283,   289,     0,
       0,   294,     0,    43,    39,    41,     0,     0,   144,   138,
       0,   293,     0,   194,     0,     0,   305,    49,   200,     0,
      63,     0,   181,   180,   179,   182,   177,   178,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    64,    71,    74,     0,    76,    77,    80,
      81,    82,    83,    84,    86,    88,     0,    94,   147,    93,
     104,     0,   114,   118,   119,   120,   121,     0,   124,   125,
      53,     0,   238,   241,   247,     0,   246,   249,     0,     0,
     250,    20,    22,   264,    47,    46,   285,     0,     0,    45,
     141,   142,     0,   301,   287,   207,   198,   197,     0,   188,
       0,   170,   171,   183,   164,   165,   168,   169,   159,   160,
       0,   161,   162,   163,   167,   166,   173,   172,   175,   176,
     174,   184,     0,   190,    55,    90,   146,     0,   308,    22,
     203,     0,   248,     0,   256,    42,    44,     0,   196,     0,
     208,   186,   185,     0,   187,   109,    34,   204,    22,   195,
     133,   158,   257,     0,     0,     0,   306,   134,     0,    48,
      48,   202,   136,     0,   135,   201
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,     1,     2,   170,   275,   190,   363,    40,    41,    42,
     255,   174,    43,   256,   372,   373,   374,   375,   282,   356,
      91,   167,   320,   168,   342,   207,   533,   537,   285,   286,
     287,   175,   331,   332,   313,   314,   315,   317,   290,   383,
     388,   294,   542,   543,   459,    47,   519,    78,   208,    48,
      80,    49,   257,    51,   258,   268,   353,    53,    54,   270,
     358,    55,   193,    56,    57,   259,   260,   179,    60,   261,
      62,    63,    64,   276,    65,   195,    66,   209,   210,   292,
     211,   169,   181,    68,    69,   182,   344
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -267
static const short int yypact[] =
{
    -267,    42,  1020,  -267,  -267,  -267,  -267,  -267,    47,  -267,
     -94,    86,  -267,   144,  -267,  -267,  -267,  -267,    40,   137,
    -267,  -267,  -267,   150,    40,  -267,   -20,    40,   199,  -267,
    -267,  -267,   161,    -7,   194,   199,  -267,  -267,  1869,  -267,
    -267,  -267,  -267,  -267,  -267,  1631,   -14,     4,  -267,  -267,
      10,    -6,  -267,    12,    21,    38,    43,    73,  -267,  -267,
      59,  -267,    -3,    -3,    -3,    52,  1744,    55,    -3,    56,
      63,  -267,  -267,   226,  -267,  -267,    65,  -267,    66,  -267,
    -267,    70,  -267,  -267,  -267,  1744,  -267,  -267,    65,    69,
    1674,  -267,  -110,  -108,  -267,  -267,  -267,  -267,  -267,  -267,
    -267,    77,    78,  -267,  -267,  -267,  -267,  -267,  -267,    79,
    -267,  -267,    81,  -267,    82,    83,  -267,  -267,    87,    89,
      91,    93,    98,  -267,    99,  -267,   101,  -267,   110,  -267,
    -267,   115,   116,  -267,  -267,  -267,  -267,  -267,  -267,  -267,
    -267,  -267,   117,  -267,  -267,  -267,  -267,  -267,   118,  -267,
    -267,  -267,  -267,  -267,   120,  -267,  -267,  -267,   129,   130,
     131,  -267,   138,  -267,  -267,   139,   140,  -107,  -267,  -267,
     925,   403,   233,   203,    95,  -267,  -267,  -267,  -267,  -267,
      52,  -267,  -267,  -267,  -267,  -267,  -267,  -267,   -12,  -267,
    -267,  -267,   208,   135,  -267,  -267,  -267,  -267,  -267,  -267,
    -267,  -267,  -267,  -267,  -267,  -267,    52,    52,  -267,    84,
     -42,  -267,    -3,  -267,  -267,  -267,  -267,   113,   212,  -267,
     141,  -267,    52,   151,  -267,   212,   749,   749,   311,   313,
     749,   749,   316,   317,   749,   318,   749,   749,   262,   749,
     749,   -79,   749,   749,   749,  1744,  1744,    97,   323,  1744,
    1869,   165,  -267,   158,  -267,  -267,  -267,   164,  -267,  -267,
    -267,  -267,    38,  -267,  -267,  -267,  -267,  -267,  -114,   186,
     -65,   166,   174,  -267,  -267,    13,    32,   167,  -267,   749,
     789,  1095,  -267,  -267,  -267,   163,   201,  -267,   175,  -267,
     -82,  -267,   202,    -7,   -46,   179,  -267,  -267,  -267,  -267,
    -267,  -267,  -267,  -267,   181,  -267,   749,   749,   749,   749,
     749,   749,   688,  1390,  -103,  -267,  1390,   185,   188,  -267,
     -66,   190,   192,   193,   195,   196,   197,   198,  1229,   355,
     200,   -64,  -267,  1390,   204,   224,   -63,  1272,   205,   206,
    -267,  -267,   207,   213,   209,   211,  -267,  1869,   365,  -267,
    -267,    38,   -22,  -267,  -267,   225,  1674,   214,   -38,   215,
     297,   498,    38,  -267,  1674,  -267,  -267,  -267,  -267,   444,
     216,   -59,   237,  -267,   239,  -267,  1674,    52,  -267,   212,
     749,  -267,  1674,  -267,    52,   238,  -267,   241,  -267,   242,
    -267,  1744,    25,    25,    25,    25,    25,    25,  1295,   243,
     749,   749,   410,   749,   749,   749,   749,   749,   749,   749,
     749,   749,   749,   749,   749,   749,   749,   749,   749,   749,
     413,   749,   749,  -267,  -267,  -267,   409,  -267,  -267,  -267,
    -267,  -267,  -267,  -267,  -267,  -267,   246,  -267,   749,  -267,
    -267,   749,  -267,  -267,  -267,  -267,  -267,   414,  -267,  -267,
    -267,   248,  -267,  -267,  -267,    52,  -267,  -267,  1674,   251,
    -267,  -267,    38,  -267,  -267,  -267,  -267,  1138,    52,  -267,
    -267,  -267,    52,   252,  -267,   212,  -267,  -267,   254,  -267,
     749,   148,   148,  -267,   187,   187,    88,    88,  1497,  1420,
    1340,  1443,  1466,   864,    88,    88,    61,    61,    25,    25,
      25,  -267,  1318,  -267,  -267,  -267,  -267,   255,  -267,    38,
    -267,    52,  -267,   593,  -267,  -267,  -267,  -102,  -267,   256,
    -267,  -267,    25,   749,  -267,  -267,  -267,  -267,    38,  -267,
    -267,  1390,  -267,    -8,   749,   275,  -267,  -267,   279,   -28,
     -28,  -267,  -267,   260,  -267,  -267
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -267,  -267,  -267,   389,  -266,  -252,     7,  -267,  -267,    74,
    -267,  -267,  -267,   433,  -267,  -267,  -267,   -31,  -267,   -27,
      -2,  -267,  -267,  -235,  -267,  -267,  -267,  -267,  -267,  -267,
      58,     2,   219,  -247,   -13,  -267,  -220,  -199,  -267,  -267,
    -267,  -267,   -99,  -174,  -267,  -267,  -267,   112,  -193,  -267,
     114,    90,    16,  -267,   436,  -267,  -267,   397,  -267,  -267,
    -267,  -267,  -267,   -25,  -267,   442,     1,  -267,  -267,   443,
    -267,  -267,   170,  -267,   -44,   -16,    53,  -194,  -267,   -17,
     277,   217,     5,   -61,  -267,     0,  -267
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -211
static const short int yytable[] =
{
      45,   180,    70,    59,    46,   212,    90,    67,   361,    39,
     350,   322,   277,   278,   325,   346,   327,     4,    50,   330,
     178,  -210,   335,   223,   220,   288,   534,   355,   291,    11,
     146,   321,   295,   370,   150,   199,   200,     9,   535,   402,
     250,     5,     3,     6,   422,   384,   222,   197,   198,     7,
      71,    72,   214,     9,    38,   199,   200,    15,    10,   161,
     351,   176,   251,  -210,    23,   224,    12,   201,   529,   423,
      13,    14,     9,    15,    23,   402,    16,    73,    17,    20,
     368,   426,   366,   438,   438,    18,    38,   201,   -40,    74,
      75,    19,   381,   269,   202,    20,   357,    21,    22,   452,
     400,   401,   402,    38,    24,   340,   427,   341,   439,   442,
     463,    25,   450,   -40,   202,   196,   196,   196,    87,   385,
     389,   196,    38,    31,    79,    94,   280,    25,   386,   281,
      38,    26,    27,    28,    29,   203,   457,    30,    84,    31,
      38,    86,   541,    32,    89,    81,    93,    76,    77,    33,
      34,    85,    35,    82,    83,   203,   183,    36,    37,   204,
     471,    38,   402,   271,    88,    77,   536,   186,    45,    45,
      70,    70,    46,    46,   184,    67,    67,   254,   254,   204,
     185,    38,   187,   469,   338,   339,   288,   362,   345,   205,
     474,   506,   420,   421,   188,   513,   283,    92,    77,   400,
     401,   402,   503,   206,   405,   406,   264,   265,   189,   205,
     514,   272,   273,   191,   316,   199,   200,   316,   417,   418,
     419,   507,   192,   206,   328,   213,   215,   333,   420,   421,
     333,   337,   194,   216,   217,     7,   263,   377,  -210,   218,
     219,   352,   221,   415,   416,   417,   418,   419,   225,   226,
     227,   399,   228,   229,   230,   420,   421,   526,   231,   279,
     232,   510,   233,   382,   234,   196,   316,   333,   266,   235,
     236,    18,   237,   364,   516,    70,   532,    46,   291,   376,
      67,   238,   520,    21,    22,   284,   239,   240,   241,   242,
      24,   243,   387,   392,   393,   394,   395,   396,   397,   398,
     244,   245,   246,   415,   416,   417,   418,   419,   274,   247,
     248,   249,   455,   289,   538,   420,   421,   527,    27,   318,
     180,   319,   293,    30,   323,   324,   326,   329,   343,   348,
     478,   458,   468,   347,   349,   354,   359,   378,   472,   367,
     413,   414,   415,   416,   417,   418,   419,   360,   379,   384,
     380,   390,   391,   453,   420,   421,   395,   424,   436,   364,
     425,    70,   428,    46,   429,   430,    67,   431,   432,   433,
     434,   441,   437,   451,   454,    19,   440,   444,   445,   446,
     447,   448,   473,   449,   456,   465,   467,   481,   482,   460,
     484,   485,   486,   487,   488,   489,   490,   491,   492,   493,
     494,   495,   496,   497,   498,   499,   500,     4,   502,   466,
     475,   476,   477,   483,   511,   480,   501,   504,   505,   508,
     509,   512,   518,   377,   539,   333,   521,   525,   540,   530,
     545,     5,   171,     6,   461,    44,   515,   470,    52,     7,
       8,   544,   177,     9,    58,    61,   365,   296,    10,   297,
     298,   299,   300,   301,    11,   517,    12,   267,   334,     0,
      13,    14,   336,    15,     0,   376,    16,   522,    17,     0,
       0,     0,     0,     0,     0,    18,     0,     0,     0,     0,
       0,    19,   253,     0,     0,    20,     0,    21,    22,    23,
       0,     0,     0,     0,    24,     0,     0,     0,     0,     0,
       0,     0,     4,     0,     0,   302,     0,     0,     0,     0,
     531,   364,     0,    70,     0,    46,     0,    25,    67,     0,
       0,    26,    27,    28,    29,     0,     5,    30,     6,    31,
       0,     0,     0,    32,     7,     0,     0,     0,     9,    33,
      34,   303,    35,    10,     0,     0,     0,    36,    37,     0,
       0,    12,     0,     0,     0,    13,    14,     0,    15,     0,
       0,    16,     0,    17,     0,     0,     0,   304,     0,     0,
      18,    38,     0,     0,     0,     0,    19,   262,     0,   305,
      20,     0,    21,    22,     0,     0,     0,     0,     0,    24,
       0,     0,     0,     0,     0,     0,   306,     4,     0,   307,
     308,   309,     0,     0,   310,   311,     0,     0,     0,     0,
       0,     0,    25,   464,     0,   312,    26,    27,    28,    29,
       0,     5,    30,     6,    31,     0,     0,     0,    32,     7,
       0,     0,     0,     9,    33,    34,     0,    35,    10,     0,
       0,     0,    36,    37,     0,     0,    12,     0,     0,     0,
      13,    14,     0,    15,     0,     0,    16,     0,    17,     0,
       0,     0,     0,     0,     0,    18,    38,     0,     0,     0,
       0,    19,   462,     0,     0,    20,     0,    21,    22,     0,
       0,     0,     0,     0,    24,     0,     0,     0,     0,     0,
       0,   296,     4,   297,   298,   299,   300,   301,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    25,     0,     0,
       0,    26,    27,    28,    29,     0,     5,    30,     6,    31,
       0,     0,     0,    32,     7,     0,     0,     0,     0,    33,
      34,     0,    35,     0,     0,     0,     0,    36,    37,     0,
       0,    12,     0,     0,     0,    13,    14,     0,     0,   302,
       0,    16,   296,    17,   297,   298,   299,   300,   301,     0,
      18,    38,     0,     0,     0,     0,     0,   528,     0,     0,
       0,     0,    21,    22,     0,     0,     0,     0,     0,    24,
       0,     0,     0,     0,     0,   303,     0,     0,     0,     0,
       0,     0,   296,     0,   297,   298,   299,   300,   301,     0,
       0,     0,     0,     0,     0,     0,    26,    27,    28,    29,
     302,   304,    30,     0,     0,     0,     0,     0,    32,     0,
       0,     0,     0,   305,     0,    34,     0,    35,     0,     0,
       0,     0,    36,    37,     0,     0,     0,     0,     0,     0,
     306,     0,     0,   307,   308,   309,   303,     0,   310,   311,
     302,     0,     0,     0,     0,     0,     0,     0,     0,   312,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   304,     0,     0,     0,   400,   401,   402,   403,
     404,   405,   406,     0,   305,     0,   303,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   306,     0,     0,   307,   308,   309,     0,     0,   310,
     311,     0,   304,     0,     0,     0,     0,     0,     0,     0,
     312,     0,     0,     0,   305,     0,     0,     0,     0,     4,
       0,     0,     0,     0,     0,     0,   252,     0,     0,     0,
       0,   306,     0,     0,   307,   308,   369,     0,     0,   310,
     311,     0,     0,     5,     0,     6,     0,     0,     0,     0,
     312,     7,     8,     0,     0,     9,     0,     0,     0,     0,
      10,     0,     0,     0,     0,     0,    11,     0,    12,     0,
       0,     0,    13,    14,     0,    15,     0,     0,    16,     0,
      17,     0,     0,     0,     0,     0,     0,    18,     0,     0,
       0,     0,     0,    19,   253,     0,     0,    20,     0,    21,
      22,    23,     0,     0,     0,     0,    24,   413,   414,   415,
     416,   417,   418,   419,     4,     0,     0,     0,     0,     0,
       0,   420,   421,     0,     0,     0,     0,     0,     0,    25,
       0,     0,     0,    26,    27,    28,    29,     0,     5,    30,
       6,    31,     0,     0,     0,    32,     7,     8,     0,     0,
       9,    33,    34,     0,    35,    10,     0,     0,     0,    36,
      37,    11,     0,    12,     0,     0,     0,    13,    14,     0,
      15,     0,     0,    16,     0,    17,     0,     0,     0,     0,
       0,     0,    18,    38,     0,     0,     0,     0,    19,     4,
       0,     0,    20,     0,    21,    22,    23,     0,     0,     0,
       0,    24,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     5,     0,     6,     0,     0,     0,     0,
       0,     7,     0,     0,    25,     9,     0,     0,    26,    27,
      28,    29,     4,     0,    30,     0,    31,     0,    12,     0,
      32,     0,    13,    14,     0,    15,    33,    34,    16,    35,
      17,     0,     0,     0,    36,    37,     5,    18,     6,     0,
       0,     0,     0,     0,     7,     0,     0,    20,     9,    21,
      22,     0,     0,     0,     0,     0,    24,     0,    38,     0,
       0,    12,     0,     0,     0,    13,    14,     0,    15,     0,
       0,    16,     0,    17,     0,     0,     0,     0,     0,    25,
      18,     0,     0,    26,    27,    28,    29,     0,     0,    30,
      20,    31,    21,    22,     0,    32,     0,     0,     0,    24,
       0,     0,    34,     0,    35,     0,     0,     0,     0,   371,
      37,   400,   401,   402,   403,   404,   405,   406,   407,   408,
       0,     0,    25,     0,     0,     0,    26,    27,    28,    29,
       0,     0,    30,    38,    31,     0,     0,     0,    32,     0,
       0,     0,     0,     0,     0,    34,     0,    35,     0,     0,
       0,     0,    36,    37,   400,   401,   402,   403,   404,   405,
     406,   407,   408,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    38,   400,   401,   402,
     403,   404,   405,   406,   407,   408,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     400,   401,   402,   403,   404,   405,   406,   407,   408,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   400,   401,   402,   403,   404,   405,   406,   407,
     408,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   409,     0,   410,
     411,   412,   413,   414,   415,   416,   417,   418,   419,     0,
       0,     0,     0,     0,     0,     0,   420,   421,     0,     0,
       0,   435,   400,   401,   402,   403,   404,   405,   406,   407,
     408,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     409,     0,   410,   411,   412,   413,   414,   415,   416,   417,
     418,   419,   400,   401,   402,   403,   404,   405,   406,   420,
     421,     0,     0,   409,   443,   410,   411,   412,   413,   414,
     415,   416,   417,   418,   419,   400,   401,   402,   403,   404,
     405,   406,   420,   421,     0,     0,   409,   479,   410,   411,
     412,   413,   414,   415,   416,   417,   418,   419,   400,   401,
     402,   403,   404,   405,   406,   420,   421,   524,   409,   523,
     410,   411,   412,   413,   414,   415,   416,   417,   418,   419,
       0,     0,     0,     0,     0,     0,     0,   420,   421,   400,
     401,   402,   403,   404,   405,   406,     0,   408,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   409,     0,
     410,   411,   412,   413,   414,   415,   416,   417,   418,   419,
       0,     0,     0,     0,     0,     0,     0,   420,   421,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     410,   411,   412,   413,   414,   415,   416,   417,   418,   419,
       0,     0,     0,     0,     0,     0,     0,   420,   421,     0,
       0,     0,     0,     0,   411,   412,   413,   414,   415,   416,
     417,   418,   419,     0,     0,     0,     0,     0,     0,     0,
     420,   421,     0,     0,     0,     0,     0,     0,   412,   413,
     414,   415,   416,   417,   418,   419,     0,     0,     0,     0,
       0,     0,     0,   420,   421,     4,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   410,   411,   412,
     413,   414,   415,   416,   417,   418,   419,     0,     0,     5,
       0,     6,     0,     0,   420,   421,     0,     7,     8,     0,
       0,     9,     0,     0,     0,     0,     0,     0,     4,     0,
       0,     0,    11,     0,    12,     0,     0,     0,    13,    14,
       0,    15,     0,     0,    16,     0,    17,     0,     0,     0,
       0,     0,     5,    18,     6,     0,     0,     0,     0,     0,
       7,     0,     0,    20,     9,    21,    22,    23,     0,     0,
     172,     0,    24,     0,   173,     0,     0,    12,     0,     0,
       0,    13,    14,     0,    15,     0,     0,    16,     0,    17,
       0,     0,     0,     0,     0,    25,    18,     0,     4,    26,
      27,    28,    29,     0,     0,    30,    20,    31,    21,    22,
       0,    32,     0,     0,     0,    24,     0,     0,    34,     0,
      35,     0,     5,     0,     6,    36,    37,     0,     0,     0,
       7,     0,     0,     0,     0,     0,     0,     0,    25,     0,
       0,     0,    26,    27,    28,    29,     0,    12,    30,     0,
      31,    13,    14,     0,    32,     0,     0,    16,     0,    17,
       0,    34,     0,    35,     0,     0,    18,     0,    36,    37,
       0,     0,     0,     0,     0,     0,     0,     0,    21,    22,
       0,     0,     0,     0,     0,    24,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    26,    27,    28,    29,     0,     0,    30,     0,
       0,     0,     0,     0,    32,     0,     0,     0,     0,     0,
       0,    34,     0,    35,     0,     0,     0,     0,    36,    37,
      95,     0,    96,    97,     0,    98,    99,     0,   100,     0,
       0,   101,     0,   102,     0,     0,     0,     0,     0,     0,
     103,   104,   105,   106,     0,   107,   108,   109,   110,   111,
       0,   112,     0,   113,   114,   115,     0,     0,   116,     0,
       0,     0,     0,   117,     0,   118,   119,   120,   121,   122,
     123,     0,   124,   125,   126,   127,   128,     0,     0,   129,
       0,     0,   130,     0,     0,     0,   131,   132,     0,   133,
       0,     0,     0,   134,   135,   136,     0,   137,   138,   139,
     140,   141,     0,   142,     0,   143,   144,   145,   146,   147,
     148,   149,   150,     0,   151,   152,   153,     0,     0,     0,
       0,   154,     0,     0,   155,     0,     0,   156,   157,     0,
       0,   158,   159,   160,     0,     0,     0,   161,     0,   162,
     163,   164,   165,     0,     0,   166
};

static const short int yycheck[] =
{
       2,    45,     2,     2,     2,    66,    33,     2,   274,     2,
     262,   231,   206,   207,   234,   250,   236,     4,     2,   239,
      45,   131,   242,   131,    85,   218,    34,    92,   222,    51,
     109,   230,   225,   280,   113,     3,     4,    40,    46,    14,
     147,    28,     0,    30,   147,   147,    90,    63,    64,    36,
       3,     4,    68,    40,   168,     3,     4,    60,    45,   138,
     174,    45,   169,   173,    86,   173,    53,    35,   170,   172,
      57,    58,    40,    60,    86,    14,    63,   171,    65,    82,
     279,   147,   276,   147,   147,    72,   168,    35,   147,     3,
       4,    78,   174,   105,    62,    82,   270,    84,    85,   351,
      12,    13,    14,   168,    91,     8,   172,    10,   172,   172,
     362,   114,   347,   172,    62,    62,    63,    64,    28,   293,
     294,    68,   168,   126,    84,    35,   168,   114,   174,   171,
     168,   118,   119,   120,   121,   103,   174,   124,    24,   126,
     168,    27,   170,   130,    32,     8,    34,     3,     4,   136,
     137,   171,   139,     3,     4,   103,   170,   144,   145,   127,
     380,   168,    14,   188,     3,     4,   174,   173,   170,   171,
     170,   171,   170,   171,   170,   170,   171,   170,   171,   127,
     170,   168,   170,   377,   245,   246,   379,   174,   249,   157,
     384,   438,   167,   168,   173,   461,   212,     3,     4,    12,
      13,    14,   422,   171,    17,    18,     3,     4,   170,   157,
     462,     3,     4,   170,   227,     3,     4,   230,   157,   158,
     159,   441,   149,   171,   237,   170,   170,   240,   167,   168,
     243,   244,   173,   170,     8,    36,     3,   281,   173,   173,
     170,   268,   173,   155,   156,   157,   158,   159,   171,   171,
     171,   312,   171,   171,   171,   167,   168,   509,   171,   175,
     171,   455,   171,   290,   171,   212,   279,   280,   173,   171,
     171,    72,   171,   275,   468,   275,   528,   275,   472,   281,
     275,   171,   475,    84,    85,   172,   171,   171,   171,   171,
      91,   171,   294,   306,   307,   308,   309,   310,   311,   312,
     171,   171,   171,   155,   156,   157,   158,   159,   173,   171,
     171,   171,   356,   172,   534,   167,   168,   511,   119,     8,
     364,     8,   171,   124,     8,     8,     8,    65,     5,   171,
     391,   358,   376,   168,   170,   149,   170,   174,   382,   172,
     153,   154,   155,   156,   157,   158,   159,   173,   147,   147,
     175,   172,   171,   352,   167,   168,   369,   172,     3,   361,
     172,   361,   172,   361,   172,   172,   361,   172,   172,   172,
     172,   147,   172,     8,   149,    78,   172,   172,   172,   172,
     167,   172,   382,   172,   170,   169,   147,   400,   401,   174,
     403,   404,   405,   406,   407,   408,   409,   410,   411,   412,
     413,   414,   415,   416,   417,   418,   419,     4,   421,   172,
     172,   170,   170,     3,   458,   172,     3,     8,   172,     5,
     172,   170,   170,   467,   149,   438,   172,   172,   149,   173,
     170,    28,    43,    30,   360,     2,   467,   379,     2,    36,
      37,   540,    45,    40,     2,     2,   276,     3,    45,     5,
       6,     7,     8,     9,    51,   472,    53,   180,   241,    -1,
      57,    58,   243,    60,    -1,   467,    63,   480,    65,    -1,
      -1,    -1,    -1,    -1,    -1,    72,    -1,    -1,    -1,    -1,
      -1,    78,    79,    -1,    -1,    82,    -1,    84,    85,    86,
      -1,    -1,    -1,    -1,    91,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     4,    -1,    -1,    61,    -1,    -1,    -1,    -1,
     523,   513,    -1,   513,    -1,   513,    -1,   114,   513,    -1,
      -1,   118,   119,   120,   121,    -1,    28,   124,    30,   126,
      -1,    -1,    -1,   130,    36,    -1,    -1,    -1,    40,   136,
     137,    97,   139,    45,    -1,    -1,    -1,   144,   145,    -1,
      -1,    53,    -1,    -1,    -1,    57,    58,    -1,    60,    -1,
      -1,    63,    -1,    65,    -1,    -1,    -1,   123,    -1,    -1,
      72,   168,    -1,    -1,    -1,    -1,    78,   174,    -1,   135,
      82,    -1,    84,    85,    -1,    -1,    -1,    -1,    -1,    91,
      -1,    -1,    -1,    -1,    -1,    -1,   152,     4,    -1,   155,
     156,   157,    -1,    -1,   160,   161,    -1,    -1,    -1,    -1,
      -1,    -1,   114,   169,    -1,   171,   118,   119,   120,   121,
      -1,    28,   124,    30,   126,    -1,    -1,    -1,   130,    36,
      -1,    -1,    -1,    40,   136,   137,    -1,   139,    45,    -1,
      -1,    -1,   144,   145,    -1,    -1,    53,    -1,    -1,    -1,
      57,    58,    -1,    60,    -1,    -1,    63,    -1,    65,    -1,
      -1,    -1,    -1,    -1,    -1,    72,   168,    -1,    -1,    -1,
      -1,    78,   174,    -1,    -1,    82,    -1,    84,    85,    -1,
      -1,    -1,    -1,    -1,    91,    -1,    -1,    -1,    -1,    -1,
      -1,     3,     4,     5,     6,     7,     8,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,    -1,    -1,
      -1,   118,   119,   120,   121,    -1,    28,   124,    30,   126,
      -1,    -1,    -1,   130,    36,    -1,    -1,    -1,    -1,   136,
     137,    -1,   139,    -1,    -1,    -1,    -1,   144,   145,    -1,
      -1,    53,    -1,    -1,    -1,    57,    58,    -1,    -1,    61,
      -1,    63,     3,    65,     5,     6,     7,     8,     9,    -1,
      72,   168,    -1,    -1,    -1,    -1,    -1,   174,    -1,    -1,
      -1,    -1,    84,    85,    -1,    -1,    -1,    -1,    -1,    91,
      -1,    -1,    -1,    -1,    -1,    97,    -1,    -1,    -1,    -1,
      -1,    -1,     3,    -1,     5,     6,     7,     8,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   118,   119,   120,   121,
      61,   123,   124,    -1,    -1,    -1,    -1,    -1,   130,    -1,
      -1,    -1,    -1,   135,    -1,   137,    -1,   139,    -1,    -1,
      -1,    -1,   144,   145,    -1,    -1,    -1,    -1,    -1,    -1,
     152,    -1,    -1,   155,   156,   157,    97,    -1,   160,   161,
      61,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   171,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   123,    -1,    -1,    -1,    12,    13,    14,    15,
      16,    17,    18,    -1,   135,    -1,    97,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   152,    -1,    -1,   155,   156,   157,    -1,    -1,   160,
     161,    -1,   123,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     171,    -1,    -1,    -1,   135,    -1,    -1,    -1,    -1,     4,
      -1,    -1,    -1,    -1,    -1,    -1,    11,    -1,    -1,    -1,
      -1,   152,    -1,    -1,   155,   156,   157,    -1,    -1,   160,
     161,    -1,    -1,    28,    -1,    30,    -1,    -1,    -1,    -1,
     171,    36,    37,    -1,    -1,    40,    -1,    -1,    -1,    -1,
      45,    -1,    -1,    -1,    -1,    -1,    51,    -1,    53,    -1,
      -1,    -1,    57,    58,    -1,    60,    -1,    -1,    63,    -1,
      65,    -1,    -1,    -1,    -1,    -1,    -1,    72,    -1,    -1,
      -1,    -1,    -1,    78,    79,    -1,    -1,    82,    -1,    84,
      85,    86,    -1,    -1,    -1,    -1,    91,   153,   154,   155,
     156,   157,   158,   159,     4,    -1,    -1,    -1,    -1,    -1,
      -1,   167,   168,    -1,    -1,    -1,    -1,    -1,    -1,   114,
      -1,    -1,    -1,   118,   119,   120,   121,    -1,    28,   124,
      30,   126,    -1,    -1,    -1,   130,    36,    37,    -1,    -1,
      40,   136,   137,    -1,   139,    45,    -1,    -1,    -1,   144,
     145,    51,    -1,    53,    -1,    -1,    -1,    57,    58,    -1,
      60,    -1,    -1,    63,    -1,    65,    -1,    -1,    -1,    -1,
      -1,    -1,    72,   168,    -1,    -1,    -1,    -1,    78,     4,
      -1,    -1,    82,    -1,    84,    85,    86,    -1,    -1,    -1,
      -1,    91,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    28,    -1,    30,    -1,    -1,    -1,    -1,
      -1,    36,    -1,    -1,   114,    40,    -1,    -1,   118,   119,
     120,   121,     4,    -1,   124,    -1,   126,    -1,    53,    -1,
     130,    -1,    57,    58,    -1,    60,   136,   137,    63,   139,
      65,    -1,    -1,    -1,   144,   145,    28,    72,    30,    -1,
      -1,    -1,    -1,    -1,    36,    -1,    -1,    82,    40,    84,
      85,    -1,    -1,    -1,    -1,    -1,    91,    -1,   168,    -1,
      -1,    53,    -1,    -1,    -1,    57,    58,    -1,    60,    -1,
      -1,    63,    -1,    65,    -1,    -1,    -1,    -1,    -1,   114,
      72,    -1,    -1,   118,   119,   120,   121,    -1,    -1,   124,
      82,   126,    84,    85,    -1,   130,    -1,    -1,    -1,    91,
      -1,    -1,   137,    -1,   139,    -1,    -1,    -1,    -1,   144,
     145,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      -1,    -1,   114,    -1,    -1,    -1,   118,   119,   120,   121,
      -1,    -1,   124,   168,   126,    -1,    -1,    -1,   130,    -1,
      -1,    -1,    -1,    -1,    -1,   137,    -1,   139,    -1,    -1,
      -1,    -1,   144,   145,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   168,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   148,    -1,   150,
     151,   152,   153,   154,   155,   156,   157,   158,   159,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   167,   168,    -1,    -1,
      -1,   172,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     148,    -1,   150,   151,   152,   153,   154,   155,   156,   157,
     158,   159,    12,    13,    14,    15,    16,    17,    18,   167,
     168,    -1,    -1,   148,   172,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,    12,    13,    14,    15,    16,
      17,    18,   167,   168,    -1,    -1,   148,   172,   150,   151,
     152,   153,   154,   155,   156,   157,   158,   159,    12,    13,
      14,    15,    16,    17,    18,   167,   168,   169,   148,   149,
     150,   151,   152,   153,   154,   155,   156,   157,   158,   159,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   167,   168,    12,
      13,    14,    15,    16,    17,    18,    -1,    20,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   148,    -1,
     150,   151,   152,   153,   154,   155,   156,   157,   158,   159,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   167,   168,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     150,   151,   152,   153,   154,   155,   156,   157,   158,   159,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   167,   168,    -1,
      -1,    -1,    -1,    -1,   151,   152,   153,   154,   155,   156,
     157,   158,   159,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     167,   168,    -1,    -1,    -1,    -1,    -1,    -1,   152,   153,
     154,   155,   156,   157,   158,   159,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   167,   168,     4,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   150,   151,   152,
     153,   154,   155,   156,   157,   158,   159,    -1,    -1,    28,
      -1,    30,    -1,    -1,   167,   168,    -1,    36,    37,    -1,
      -1,    40,    -1,    -1,    -1,    -1,    -1,    -1,     4,    -1,
      -1,    -1,    51,    -1,    53,    -1,    -1,    -1,    57,    58,
      -1,    60,    -1,    -1,    63,    -1,    65,    -1,    -1,    -1,
      -1,    -1,    28,    72,    30,    -1,    -1,    -1,    -1,    -1,
      36,    -1,    -1,    82,    40,    84,    85,    86,    -1,    -1,
      89,    -1,    91,    -1,    93,    -1,    -1,    53,    -1,    -1,
      -1,    57,    58,    -1,    60,    -1,    -1,    63,    -1,    65,
      -1,    -1,    -1,    -1,    -1,   114,    72,    -1,     4,   118,
     119,   120,   121,    -1,    -1,   124,    82,   126,    84,    85,
      -1,   130,    -1,    -1,    -1,    91,    -1,    -1,   137,    -1,
     139,    -1,    28,    -1,    30,   144,   145,    -1,    -1,    -1,
      36,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,    -1,
      -1,    -1,   118,   119,   120,   121,    -1,    53,   124,    -1,
     126,    57,    58,    -1,   130,    -1,    -1,    63,    -1,    65,
      -1,   137,    -1,   139,    -1,    -1,    72,    -1,   144,   145,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    84,    85,
      -1,    -1,    -1,    -1,    -1,    91,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   118,   119,   120,   121,    -1,    -1,   124,    -1,
      -1,    -1,    -1,    -1,   130,    -1,    -1,    -1,    -1,    -1,
      -1,   137,    -1,   139,    -1,    -1,    -1,    -1,   144,   145,
      21,    -1,    23,    24,    -1,    26,    27,    -1,    29,    -1,
      -1,    32,    -1,    34,    -1,    -1,    -1,    -1,    -1,    -1,
      41,    42,    43,    44,    -1,    46,    47,    48,    49,    50,
      -1,    52,    -1,    54,    55,    56,    -1,    -1,    59,    -1,
      -1,    -1,    -1,    64,    -1,    66,    67,    68,    69,    70,
      71,    -1,    73,    74,    75,    76,    77,    -1,    -1,    80,
      -1,    -1,    83,    -1,    -1,    -1,    87,    88,    -1,    90,
      -1,    -1,    -1,    94,    95,    96,    -1,    98,    99,   100,
     101,   102,    -1,   104,    -1,   106,   107,   108,   109,   110,
     111,   112,   113,    -1,   115,   116,   117,    -1,    -1,    -1,
      -1,   122,    -1,    -1,   125,    -1,    -1,   128,   129,    -1,
      -1,   132,   133,   134,    -1,    -1,    -1,   138,    -1,   140,
     141,   142,   143,    -1,    -1,   146
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned short int yystos[] =
{
       0,   177,   178,     0,     4,    28,    30,    36,    37,    40,
      45,    51,    53,    57,    58,    60,    63,    65,    72,    78,
      82,    84,    85,    86,    91,   114,   118,   119,   120,   121,
     124,   126,   130,   136,   137,   139,   144,   145,   168,   182,
     183,   184,   185,   188,   189,   196,   207,   221,   225,   227,
     228,   229,   230,   233,   234,   237,   239,   240,   241,   242,
     244,   245,   246,   247,   248,   250,   252,   258,   259,   260,
     261,     3,     4,   171,     3,     4,     3,     4,   223,    84,
     226,     8,     3,     4,   226,   171,   226,   227,     3,   223,
     195,   196,     3,   223,   227,    21,    23,    24,    26,    27,
      29,    32,    34,    41,    42,    43,    44,    46,    47,    48,
      49,    50,    52,    54,    55,    56,    59,    64,    66,    67,
      68,    69,    70,    71,    73,    74,    75,    76,    77,    80,
      83,    87,    88,    90,    94,    95,    96,    98,    99,   100,
     101,   102,   104,   106,   107,   108,   109,   110,   111,   112,
     113,   115,   116,   117,   122,   125,   128,   129,   132,   133,
     134,   138,   140,   141,   142,   143,   146,   197,   199,   257,
     179,   179,    89,    93,   187,   207,   228,   233,   239,   243,
     250,   258,   261,   170,   170,   170,   173,   170,   173,   170,
     181,   170,   149,   238,   173,   251,   252,   251,   251,     3,
       4,    35,    62,   103,   127,   157,   171,   201,   224,   253,
     254,   256,   259,   170,   251,   170,   170,     8,   173,   170,
     259,   173,   250,   131,   173,   171,   171,   171,   171,   171,
     171,   171,   171,   171,   171,   171,   171,   171,   171,   171,
     171,   171,   171,   171,   171,   171,   171,   171,   171,   171,
     147,   169,    11,    79,   182,   186,   189,   228,   230,   241,
     242,   245,   174,     3,     3,     4,   173,   256,   231,   105,
     235,   239,     3,     4,   173,   180,   249,   253,   253,   175,
     168,   171,   194,   251,   172,   204,   205,   206,   224,   172,
     214,   253,   255,   171,   217,   224,     3,     5,     6,     7,
       8,     9,    61,    97,   123,   135,   152,   155,   156,   157,
     160,   161,   171,   210,   211,   212,   210,   213,     8,     8,
     198,   213,   212,     8,     8,   212,     8,   212,   210,    65,
     212,   208,   209,   210,   257,   212,   208,   210,   259,   259,
       8,    10,   200,     5,   262,   259,   199,   168,   171,   170,
     181,   174,   195,   232,   149,    92,   195,   219,   236,   170,
     173,   180,   174,   182,   196,   248,   253,   172,   213,   157,
     209,   144,   190,   191,   192,   193,   196,   250,   174,   147,
     175,   174,   195,   215,   147,   219,   174,   196,   216,   219,
     172,   171,   210,   210,   210,   210,   210,   210,   210,   259,
      12,    13,    14,    15,    16,    17,    18,    19,    20,   148,
     150,   151,   152,   153,   154,   155,   156,   157,   158,   159,
     167,   168,   147,   172,   172,   172,   147,   172,   172,   172,
     172,   172,   172,   172,   172,   172,     3,   172,   147,   172,
     172,   147,   172,   172,   172,   172,   172,   167,   172,   172,
     199,     8,   181,   242,   149,   250,   170,   174,   195,   220,
     174,   185,   174,   181,   169,   169,   172,   147,   250,   253,
     206,   212,   250,   261,   253,   172,   170,   170,   259,   172,
     172,   210,   210,     3,   210,   210,   210,   210,   210,   210,
     210,   210,   210,   210,   210,   210,   210,   210,   210,   210,
     210,     3,   210,   212,     8,   172,   209,   212,     5,   172,
     253,   250,   170,   180,   181,   193,   253,   255,   170,   222,
     224,   172,   210,   149,   169,   172,   181,   253,   174,   170,
     173,   210,   181,   202,    34,    46,   174,   203,   212,   149,
     149,   170,   218,   219,   218,   170
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
#line 349 "parser.y"
    { fix_incomplete();
						  check_all_user_types((yyvsp[0].stmt_list));
						  write_proxies((yyvsp[0].stmt_list));
						  write_client((yyvsp[0].stmt_list));
						  write_server((yyvsp[0].stmt_list));
						  write_dlldata((yyvsp[0].stmt_list));
						;}
    break;

  case 3:
#line 358 "parser.y"
    { (yyval.stmt_list) = NULL; ;}
    break;

  case 4:
#line 359 "parser.y"
    { (yyval.stmt_list) = (yyvsp[-1].stmt_list); ;}
    break;

  case 5:
#line 360 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_type_decl((yyvsp[0].type))); ;}
    break;

  case 6:
#line 361 "parser.y"
    { (yyval.stmt_list) = (yyvsp[-2].stmt_list);
						  reg_type((yyvsp[-1].type), (yyvsp[-1].type)->name, 0);
						  if (!parse_only && do_header) write_coclass_forward((yyvsp[-1].type));
						;}
    break;

  case 7:
#line 365 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_type_decl((yyvsp[0].type)));
						  add_typelib_entry((yyvsp[0].type));
						  reg_type((yyvsp[0].type), (yyvsp[0].type)->name, 0);
						  if (!parse_only && do_header) write_coclass_forward((yyvsp[0].type));
						;}
    break;

  case 8:
#line 370 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_module((yyvsp[0].type)));
						  add_typelib_entry((yyvsp[0].type));
						;}
    break;

  case 9:
#line 373 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_library((yyvsp[0].typelib))); ;}
    break;

  case 10:
#line 374 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), (yyvsp[0].statement)); ;}
    break;

  case 11:
#line 377 "parser.y"
    { (yyval.stmt_list) = NULL; ;}
    break;

  case 12:
#line 378 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_reference((yyvsp[0].type))); if (!parse_only) add_typelib_entry((yyvsp[0].type)); ;}
    break;

  case 13:
#line 379 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_type_decl((yyvsp[0].type))); if (!parse_only) add_typelib_entry((yyvsp[0].type)); ;}
    break;

  case 14:
#line 380 "parser.y"
    { (yyval.stmt_list) = (yyvsp[-2].stmt_list); reg_type((yyvsp[-1].type), (yyvsp[-1].type)->name, 0); if (!parse_only && do_header) write_coclass_forward((yyvsp[-1].type)); ;}
    break;

  case 15:
#line 381 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_type_decl((yyvsp[0].type)));
						  if (!parse_only) add_typelib_entry((yyvsp[0].type));
						  reg_type((yyvsp[0].type), (yyvsp[0].type)->name, 0);
						  if (!parse_only && do_header) write_coclass_forward((yyvsp[0].type));
						;}
    break;

  case 16:
#line 386 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_module((yyvsp[0].type))); if (!parse_only) add_typelib_entry((yyvsp[0].type)); ;}
    break;

  case 17:
#line 387 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), (yyvsp[0].statement)); ;}
    break;

  case 18:
#line 388 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_importlib((yyvsp[0].str))); ;}
    break;

  case 19:
#line 389 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_library((yyvsp[0].typelib))); ;}
    break;

  case 20:
#line 392 "parser.y"
    { (yyval.func_list) = NULL; ;}
    break;

  case 21:
#line 393 "parser.y"
    { (yyval.func_list) = append_func_from_statement( (yyvsp[-1].func_list), (yyvsp[0].statement) ); ;}
    break;

  case 24:
#line 401 "parser.y"
    { (yyval.statement) = make_statement_cppquote((yyvsp[0].str)); ;}
    break;

  case 25:
#line 402 "parser.y"
    { (yyval.statement) = make_statement_type_decl((yyvsp[-1].type));
						  if (!parse_only && do_header) {
						    write_type_def_or_decl(header, (yyvsp[-1].type), FALSE, NULL);
						    fprintf(header, ";\n\n");
						  }
						;}
    break;

  case 26:
#line 408 "parser.y"
    { (yyval.statement) = make_statement_declaration((yyvsp[-1].var));
						  if (!parse_only && do_header) write_declaration((yyvsp[-1].var), is_in_interface);
						;}
    break;

  case 27:
#line 411 "parser.y"
    { (yyval.statement) = make_statement_import((yyvsp[0].str)); ;}
    break;

  case 28:
#line 412 "parser.y"
    { (yyval.statement) = make_statement_type_decl((yyvsp[-1].type));
						  if (!parse_only && do_header) {
						    write_type_def_or_decl(header, (yyvsp[-1].type), FALSE, NULL);
						    fprintf(header, ";\n\n");
						  }
						;}
    break;

  case 29:
#line 418 "parser.y"
    { (yyval.statement) = (yyvsp[-1].statement); ;}
    break;

  case 30:
#line 419 "parser.y"
    { (yyval.statement) = make_statement_type_decl((yyvsp[-1].type));
						  if (!parse_only && do_header) {
						    write_type_def_or_decl(header, (yyvsp[-1].type), FALSE, NULL);
						    fprintf(header, ";\n\n");
						  }
						;}
    break;

  case 31:
#line 427 "parser.y"
    { (yyval.str) = (yyvsp[-1].str); if (!parse_only && do_header) fprintf(header, "%s\n", (yyvsp[-1].str)); ;}
    break;

  case 32:
#line 429 "parser.y"
    { assert(yychar == YYEMPTY);
						  (yyval.import) = xmalloc(sizeof(struct _import_t));
						  (yyval.import)->name = (yyvsp[-1].str);
						  (yyval.import)->import_performed = do_import((yyvsp[-1].str));
						  if (!(yyval.import)->import_performed) yychar = aEOF;
						;}
    break;

  case 33:
#line 437 "parser.y"
    { (yyval.str) = (yyvsp[-2].import)->name;
						  if ((yyvsp[-2].import)->import_performed) pop_import();
						  free((yyvsp[-2].import));
						  if (!parse_only && do_header) write_import((yyval.str));
						;}
    break;

  case 34:
#line 445 "parser.y"
    { (yyval.str) = (yyvsp[-2].str); if(!parse_only) add_importlib((yyvsp[-2].str)); ;}
    break;

  case 35:
#line 448 "parser.y"
    { (yyval.str) = (yyvsp[0].str); ;}
    break;

  case 36:
#line 450 "parser.y"
    { (yyval.typelib) = make_library((yyvsp[-1].str), check_library_attrs((yyvsp[-1].str), (yyvsp[-2].attr_list)));
						  if (!parse_only) start_typelib((yyval.typelib));
						  if (!parse_only && do_header) write_library((yyval.typelib));
						  if (!parse_only && do_idfile) write_libid((yyval.typelib));
						  is_inside_library = TRUE;
						;}
    break;

  case 37:
#line 458 "parser.y"
    { (yyval.typelib) = (yyvsp[-3].typelib);
						  (yyval.typelib)->stmts = (yyvsp[-2].stmt_list);
						  if (!parse_only) end_typelib();
						  is_inside_library = FALSE;
						;}
    break;

  case 38:
#line 465 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 40:
#line 469 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 41:
#line 472 "parser.y"
    { check_arg((yyvsp[0].var)); (yyval.var_list) = append_var( NULL, (yyvsp[0].var) ); ;}
    break;

  case 42:
#line 473 "parser.y"
    { check_arg((yyvsp[0].var)); (yyval.var_list) = append_var( (yyvsp[-2].var_list), (yyvsp[0].var)); ;}
    break;

  case 44:
#line 478 "parser.y"
    { (yyval.var) = (yyvsp[0].declarator)->var;
						  (yyval.var)->attrs = (yyvsp[-2].attr_list);
						  if ((yyvsp[-1].declspec)->stgclass != STG_NONE && (yyvsp[-1].declspec)->stgclass != STG_REGISTER)
						    error_loc("invalid storage class for function parameter\n");
						  set_type((yyval.var), (yyvsp[-1].declspec), (yyvsp[0].declarator), TRUE);
						  free((yyvsp[0].declarator));
						;}
    break;

  case 45:
#line 485 "parser.y"
    { (yyval.var) = (yyvsp[0].declarator)->var;
						  if ((yyvsp[-1].declspec)->stgclass != STG_NONE && (yyvsp[-1].declspec)->stgclass != STG_REGISTER)
						    error_loc("invalid storage class for function parameter\n");
						  set_type((yyval.var), (yyvsp[-1].declspec), (yyvsp[0].declarator), TRUE);
						  free((yyvsp[0].declarator));
						;}
    break;

  case 46:
#line 493 "parser.y"
    { (yyval.expr) = (yyvsp[-1].expr); ;}
    break;

  case 47:
#line 494 "parser.y"
    { (yyval.expr) = make_expr(EXPR_VOID); ;}
    break;

  case 48:
#line 497 "parser.y"
    { (yyval.attr_list) = NULL; ;}
    break;

  case 50:
#line 502 "parser.y"
    { (yyval.attr_list) = (yyvsp[-1].attr_list);
						  if (!(yyval.attr_list))
						    error_loc("empty attribute lists unsupported\n");
						;}
    break;

  case 51:
#line 508 "parser.y"
    { (yyval.attr_list) = append_attr( NULL, (yyvsp[0].attr) ); ;}
    break;

  case 52:
#line 509 "parser.y"
    { (yyval.attr_list) = append_attr( (yyvsp[-2].attr_list), (yyvsp[0].attr) ); ;}
    break;

  case 53:
#line 510 "parser.y"
    { (yyval.attr_list) = append_attr( (yyvsp[-3].attr_list), (yyvsp[0].attr) ); ;}
    break;

  case 54:
#line 513 "parser.y"
    { (yyval.str_list) = append_str( NULL, (yyvsp[0].str) ); ;}
    break;

  case 55:
#line 514 "parser.y"
    { (yyval.str_list) = append_str( (yyvsp[-2].str_list), (yyvsp[0].str) ); ;}
    break;

  case 56:
#line 517 "parser.y"
    { (yyval.attr) = NULL; ;}
    break;

  case 57:
#line 518 "parser.y"
    { (yyval.attr) = make_attr(ATTR_AGGREGATABLE); ;}
    break;

  case 58:
#line 519 "parser.y"
    { (yyval.attr) = make_attr(ATTR_APPOBJECT); ;}
    break;

  case 59:
#line 520 "parser.y"
    { (yyval.attr) = make_attr(ATTR_ASYNC); ;}
    break;

  case 60:
#line 521 "parser.y"
    { (yyval.attr) = make_attr(ATTR_AUTO_HANDLE); ;}
    break;

  case 61:
#line 522 "parser.y"
    { (yyval.attr) = make_attr(ATTR_BINDABLE); ;}
    break;

  case 62:
#line 523 "parser.y"
    { (yyval.attr) = make_attr(ATTR_BROADCAST); ;}
    break;

  case 63:
#line 524 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_CALLAS, (yyvsp[-1].var)); ;}
    break;

  case 64:
#line 525 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_CASE, (yyvsp[-1].expr_list)); ;}
    break;

  case 65:
#line 526 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); ;}
    break;

  case 66:
#line 527 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_DONT_SERIALIZE */ ;}
    break;

  case 67:
#line 528 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_SERIALIZE */ ;}
    break;

  case 68:
#line 529 "parser.y"
    { (yyval.attr) = make_attr(ATTR_CONTROL); ;}
    break;

  case 69:
#line 530 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULT); ;}
    break;

  case 70:
#line 531 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULTCOLLELEM); ;}
    break;

  case 71:
#line 532 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_DEFAULTVALUE, (yyvsp[-1].expr)); ;}
    break;

  case 72:
#line 533 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULTVTABLE); ;}
    break;

  case 73:
#line 534 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DISPLAYBIND); ;}
    break;

  case 74:
#line 535 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_DLLNAME, (yyvsp[-1].str)); ;}
    break;

  case 75:
#line 536 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DUAL); ;}
    break;

  case 76:
#line 537 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ENDPOINT, (yyvsp[-1].str_list)); ;}
    break;

  case 77:
#line 538 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ENTRY, (yyvsp[-1].expr)); ;}
    break;

  case 78:
#line 539 "parser.y"
    { (yyval.attr) = make_attr(ATTR_EXPLICIT_HANDLE); ;}
    break;

  case 79:
#line 540 "parser.y"
    { (yyval.attr) = make_attr(ATTR_HANDLE); ;}
    break;

  case 80:
#line 541 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPCONTEXT, (yyvsp[-1].expr)); ;}
    break;

  case 81:
#line 542 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPFILE, (yyvsp[-1].str)); ;}
    break;

  case 82:
#line 543 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRING, (yyvsp[-1].str)); ;}
    break;

  case 83:
#line 544 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRINGCONTEXT, (yyvsp[-1].expr)); ;}
    break;

  case 84:
#line 545 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRINGDLL, (yyvsp[-1].str)); ;}
    break;

  case 85:
#line 546 "parser.y"
    { (yyval.attr) = make_attr(ATTR_HIDDEN); ;}
    break;

  case 86:
#line 547 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ID, (yyvsp[-1].expr)); ;}
    break;

  case 87:
#line 548 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IDEMPOTENT); ;}
    break;

  case 88:
#line 549 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_IIDIS, (yyvsp[-1].expr)); ;}
    break;

  case 89:
#line 550 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IMMEDIATEBIND); ;}
    break;

  case 90:
#line 551 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_IMPLICIT_HANDLE, (yyvsp[-1].str)); ;}
    break;

  case 91:
#line 552 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IN); ;}
    break;

  case 92:
#line 553 "parser.y"
    { (yyval.attr) = make_attr(ATTR_INPUTSYNC); ;}
    break;

  case 93:
#line 554 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_LENGTHIS, (yyvsp[-1].expr_list)); ;}
    break;

  case 94:
#line 555 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_LIBLCID, (yyvsp[-1].expr)); ;}
    break;

  case 95:
#line 556 "parser.y"
    { (yyval.attr) = make_attr(ATTR_LOCAL); ;}
    break;

  case 96:
#line 557 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONBROWSABLE); ;}
    break;

  case 97:
#line 558 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONCREATABLE); ;}
    break;

  case 98:
#line 559 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONEXTENSIBLE); ;}
    break;

  case 99:
#line 560 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OBJECT); ;}
    break;

  case 100:
#line 561 "parser.y"
    { (yyval.attr) = make_attr(ATTR_ODL); ;}
    break;

  case 101:
#line 562 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OLEAUTOMATION); ;}
    break;

  case 102:
#line 563 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OPTIONAL); ;}
    break;

  case 103:
#line 564 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OUT); ;}
    break;

  case 104:
#line 565 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_POINTERDEFAULT, (yyvsp[-1].num)); ;}
    break;

  case 105:
#line 566 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPGET); ;}
    break;

  case 106:
#line 567 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPPUT); ;}
    break;

  case 107:
#line 568 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPPUTREF); ;}
    break;

  case 108:
#line 569 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PUBLIC); ;}
    break;

  case 109:
#line 571 "parser.y"
    { expr_list_t *list = append_expr( NULL, (yyvsp[-3].expr) );
						  list = append_expr( list, (yyvsp[-1].expr) );
						  (yyval.attr) = make_attrp(ATTR_RANGE, list); ;}
    break;

  case 110:
#line 574 "parser.y"
    { (yyval.attr) = make_attr(ATTR_READONLY); ;}
    break;

  case 111:
#line 575 "parser.y"
    { (yyval.attr) = make_attr(ATTR_REQUESTEDIT); ;}
    break;

  case 112:
#line 576 "parser.y"
    { (yyval.attr) = make_attr(ATTR_RESTRICTED); ;}
    break;

  case 113:
#line 577 "parser.y"
    { (yyval.attr) = make_attr(ATTR_RETVAL); ;}
    break;

  case 114:
#line 578 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SIZEIS, (yyvsp[-1].expr_list)); ;}
    break;

  case 115:
#line 579 "parser.y"
    { (yyval.attr) = make_attr(ATTR_SOURCE); ;}
    break;

  case 116:
#line 580 "parser.y"
    { (yyval.attr) = make_attr(ATTR_STRICTCONTEXTHANDLE); ;}
    break;

  case 117:
#line 581 "parser.y"
    { (yyval.attr) = make_attr(ATTR_STRING); ;}
    break;

  case 118:
#line 582 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SWITCHIS, (yyvsp[-1].expr)); ;}
    break;

  case 119:
#line 583 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SWITCHTYPE, (yyvsp[-1].type)); ;}
    break;

  case 120:
#line 584 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_TRANSMITAS, (yyvsp[-1].type)); ;}
    break;

  case 121:
#line 585 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_UUID, (yyvsp[-1].uuid)); ;}
    break;

  case 122:
#line 586 "parser.y"
    { (yyval.attr) = make_attr(ATTR_V1ENUM); ;}
    break;

  case 123:
#line 587 "parser.y"
    { (yyval.attr) = make_attr(ATTR_VARARG); ;}
    break;

  case 124:
#line 588 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_VERSION, (yyvsp[-1].num)); ;}
    break;

  case 125:
#line 589 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_WIREMARSHAL, (yyvsp[-1].type)); ;}
    break;

  case 126:
#line 590 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_POINTERTYPE, (yyvsp[0].num)); ;}
    break;

  case 128:
#line 595 "parser.y"
    { if (!is_valid_uuid((yyvsp[0].str)))
						    error_loc("invalid UUID: %s\n", (yyvsp[0].str));
						  (yyval.uuid) = parse_uuid((yyvsp[0].str)); ;}
    break;

  case 129:
#line 600 "parser.y"
    { (yyval.str) = (yyvsp[0].str); ;}
    break;

  case 130:
#line 601 "parser.y"
    { (yyval.str) = (yyvsp[0].str); ;}
    break;

  case 131:
#line 602 "parser.y"
    { (yyval.str) = (yyvsp[0].str); ;}
    break;

  case 132:
#line 603 "parser.y"
    { (yyval.str) = (yyvsp[0].str); ;}
    break;

  case 133:
#line 606 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 134:
#line 607 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[-1].var_list), (yyvsp[0].var) ); ;}
    break;

  case 135:
#line 610 "parser.y"
    { attr_t *a = make_attrp(ATTR_CASE, append_expr( NULL, (yyvsp[-2].expr) ));
						  (yyval.var) = (yyvsp[0].var); if (!(yyval.var)) (yyval.var) = make_var(NULL);
						  (yyval.var)->attrs = append_attr( (yyval.var)->attrs, a );
						;}
    break;

  case 136:
#line 614 "parser.y"
    { attr_t *a = make_attr(ATTR_DEFAULT);
						  (yyval.var) = (yyvsp[0].var); if (!(yyval.var)) (yyval.var) = make_var(NULL);
						  (yyval.var)->attrs = append_attr( (yyval.var)->attrs, a );
						;}
    break;

  case 137:
#line 620 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 138:
#line 621 "parser.y"
    { (yyval.var_list) = (yyvsp[-1].var_list); ;}
    break;

  case 140:
#line 625 "parser.y"
    { if (!(yyvsp[0].var)->eval)
						    (yyvsp[0].var)->eval = make_exprl(EXPR_NUM, 0 /* default for first enum entry */);
                                                  (yyval.var_list) = append_var( NULL, (yyvsp[0].var) );
						;}
    break;

  case 141:
#line 629 "parser.y"
    { if (!(yyvsp[0].var)->eval)
                                                  {
                                                    var_t *last = LIST_ENTRY( list_tail((yyval.var_list)), var_t, entry );
                                                    (yyvsp[0].var)->eval = make_exprl(EXPR_NUM, last->eval->cval + 1);
                                                  }
                                                  (yyval.var_list) = append_var( (yyvsp[-2].var_list), (yyvsp[0].var) );
						;}
    break;

  case 142:
#line 638 "parser.y"
    { (yyval.var) = reg_const((yyvsp[-2].var));
						  (yyval.var)->eval = (yyvsp[0].expr);
                                                  (yyval.var)->type = make_int(0);
						;}
    break;

  case 143:
#line 642 "parser.y"
    { (yyval.var) = reg_const((yyvsp[0].var));
                                                  (yyval.var)->type = make_int(0);
						;}
    break;

  case 144:
#line 647 "parser.y"
    { (yyval.type) = get_typev(RPC_FC_ENUM16, (yyvsp[-3].var), tsENUM);
						  (yyval.type)->kind = TKIND_ENUM;
						  (yyval.type)->fields_or_args = (yyvsp[-1].var_list);
						  (yyval.type)->defined = TRUE;
                                                  if(in_typelib)
                                                      add_typelib_entry((yyval.type));
						;}
    break;

  case 145:
#line 656 "parser.y"
    { (yyval.expr_list) = append_expr( NULL, (yyvsp[0].expr) ); ;}
    break;

  case 146:
#line 657 "parser.y"
    { (yyval.expr_list) = append_expr( (yyvsp[-2].expr_list), (yyvsp[0].expr) ); ;}
    break;

  case 147:
#line 670 "parser.y"
    { (yyval.expr) = make_expr(EXPR_VOID); ;}
    break;

  case 149:
#line 674 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_NUM, (yyvsp[0].num)); ;}
    break;

  case 150:
#line 675 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_HEXNUM, (yyvsp[0].num)); ;}
    break;

  case 151:
#line 676 "parser.y"
    { (yyval.expr) = make_exprd(EXPR_DOUBLE, (yyvsp[0].dbl)); ;}
    break;

  case 152:
#line 677 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_TRUEFALSE, 0); ;}
    break;

  case 153:
#line 678 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_NUM, 0); ;}
    break;

  case 154:
#line 679 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_TRUEFALSE, 1); ;}
    break;

  case 155:
#line 680 "parser.y"
    { (yyval.expr) = make_exprs(EXPR_STRLIT, (yyvsp[0].str)); ;}
    break;

  case 156:
#line 681 "parser.y"
    { (yyval.expr) = make_exprs(EXPR_WSTRLIT, (yyvsp[0].str)); ;}
    break;

  case 157:
#line 682 "parser.y"
    { (yyval.expr) = make_exprs(EXPR_IDENTIFIER, (yyvsp[0].str)); ;}
    break;

  case 158:
#line 683 "parser.y"
    { (yyval.expr) = make_expr3(EXPR_COND, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 159:
#line 684 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_LOGOR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 160:
#line 685 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_LOGAND, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 161:
#line 686 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_OR , (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 162:
#line 687 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_XOR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 163:
#line 688 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_AND, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 164:
#line 689 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_EQUALITY, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 165:
#line 690 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_INEQUALITY, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 166:
#line 691 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_GTR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 167:
#line 692 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_LESS, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 168:
#line 693 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_GTREQL, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 169:
#line 694 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_LESSEQL, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 170:
#line 695 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SHL, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 171:
#line 696 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SHR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 172:
#line 697 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_ADD, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 173:
#line 698 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SUB, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 174:
#line 699 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MOD, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 175:
#line 700 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MUL, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 176:
#line 701 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_DIV, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 177:
#line 702 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_LOGNOT, (yyvsp[0].expr)); ;}
    break;

  case 178:
#line 703 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_NOT, (yyvsp[0].expr)); ;}
    break;

  case 179:
#line 704 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_POS, (yyvsp[0].expr)); ;}
    break;

  case 180:
#line 705 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_NEG, (yyvsp[0].expr)); ;}
    break;

  case 181:
#line 706 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_ADDRESSOF, (yyvsp[0].expr)); ;}
    break;

  case 182:
#line 707 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_PPTR, (yyvsp[0].expr)); ;}
    break;

  case 183:
#line 708 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MEMBER, make_expr1(EXPR_PPTR, (yyvsp[-2].expr)), make_exprs(EXPR_IDENTIFIER, (yyvsp[0].str))); ;}
    break;

  case 184:
#line 709 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MEMBER, (yyvsp[-2].expr), make_exprs(EXPR_IDENTIFIER, (yyvsp[0].str))); ;}
    break;

  case 185:
#line 710 "parser.y"
    { (yyval.expr) = make_exprt(EXPR_CAST, (yyvsp[-2].type), (yyvsp[0].expr)); ;}
    break;

  case 186:
#line 711 "parser.y"
    { (yyval.expr) = make_exprt(EXPR_SIZEOF, (yyvsp[-1].type), NULL); ;}
    break;

  case 187:
#line 712 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_ARRAY, (yyvsp[-3].expr), (yyvsp[-1].expr)); ;}
    break;

  case 188:
#line 713 "parser.y"
    { (yyval.expr) = (yyvsp[-1].expr); ;}
    break;

  case 189:
#line 716 "parser.y"
    { (yyval.expr_list) = append_expr( NULL, (yyvsp[0].expr) ); ;}
    break;

  case 190:
#line 717 "parser.y"
    { (yyval.expr_list) = append_expr( (yyvsp[-2].expr_list), (yyvsp[0].expr) ); ;}
    break;

  case 191:
#line 720 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr);
						  if (!(yyval.expr)->is_const)
						      error_loc("expression is not an integer constant\n");
						;}
    break;

  case 192:
#line 726 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr);
						  if (!(yyval.expr)->is_const && (yyval.expr)->type != EXPR_STRLIT && (yyval.expr)->type != EXPR_WSTRLIT)
						      error_loc("expression is not constant\n");
						;}
    break;

  case 193:
#line 732 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 194:
#line 733 "parser.y"
    { (yyval.var_list) = append_var_list((yyvsp[-1].var_list), (yyvsp[0].var_list)); ;}
    break;

  case 195:
#line 737 "parser.y"
    { const char *first = LIST_ENTRY(list_head((yyvsp[-1].declarator_list)), declarator_t, entry)->var->name;
						  check_field_attrs(first, (yyvsp[-3].attr_list));
						  (yyval.var_list) = set_var_types((yyvsp[-3].attr_list), (yyvsp[-2].declspec), (yyvsp[-1].declarator_list));
						;}
    break;

  case 196:
#line 741 "parser.y"
    { var_t *v = make_var(NULL);
						  v->type = (yyvsp[-1].type); v->attrs = (yyvsp[-2].attr_list);
						  (yyval.var_list) = append_var(NULL, v);
						;}
    break;

  case 197:
#line 748 "parser.y"
    { (yyval.var) = (yyvsp[-1].var); ;}
    break;

  case 198:
#line 749 "parser.y"
    { (yyval.var) = make_var(NULL); (yyval.var)->attrs = (yyvsp[-1].attr_list); ;}
    break;

  case 199:
#line 752 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 200:
#line 753 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[-1].var_list), (yyvsp[0].var) ); ;}
    break;

  case 201:
#line 757 "parser.y"
    { (yyval.var) = (yyvsp[-1].var); ;}
    break;

  case 202:
#line 758 "parser.y"
    { (yyval.var) = NULL; ;}
    break;

  case 203:
#line 761 "parser.y"
    { (yyval.var) = (yyvsp[0].declarator)->var;
						  (yyval.var)->attrs = check_field_attrs((yyval.var)->name, (yyvsp[-2].attr_list));
						  set_type((yyval.var), (yyvsp[-1].declspec), (yyvsp[0].declarator), FALSE);
						  free((yyvsp[0].declarator));
						;}
    break;

  case 204:
#line 769 "parser.y"
    { var_t *v = (yyvsp[0].declarator)->var;
						  v->attrs = check_function_attrs(v->name, (yyvsp[-2].attr_list));
						  set_type(v, (yyvsp[-1].declspec), (yyvsp[0].declarator), FALSE);
						  free((yyvsp[0].declarator));
						  (yyval.func) = make_func(v);
						;}
    break;

  case 205:
#line 779 "parser.y"
    { (yyval.var) = (yyvsp[0].declarator)->var;
						  (yyval.var)->attrs = (yyvsp[-2].attr_list);
						  set_type((yyval.var), (yyvsp[-1].declspec), (yyvsp[0].declarator), FALSE);
						  free((yyvsp[0].declarator));
						;}
    break;

  case 206:
#line 784 "parser.y"
    { (yyval.var) = (yyvsp[0].declarator)->var;
						  set_type((yyval.var), (yyvsp[-1].declspec), (yyvsp[0].declarator), FALSE);
						  free((yyvsp[0].declarator));
						;}
    break;

  case 207:
#line 790 "parser.y"
    { (yyval.var) = NULL; ;}
    break;

  case 209:
#line 794 "parser.y"
    { (yyval.var) = NULL; ;}
    break;

  case 210:
#line 795 "parser.y"
    { (yyval.var) = make_var((yyvsp[0].str)); ;}
    break;

  case 211:
#line 796 "parser.y"
    { (yyval.var) = make_var((yyvsp[0].str)); ;}
    break;

  case 212:
#line 799 "parser.y"
    { (yyval.var) = make_var((yyvsp[0].str)); ;}
    break;

  case 213:
#line 801 "parser.y"
    { (yyval.var) = make_var((yyvsp[0].str)); ;}
    break;

  case 214:
#line 804 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 215:
#line 805 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 217:
#line 807 "parser.y"
    { (yyval.type) = (yyvsp[0].type); (yyval.type)->sign = 1; ;}
    break;

  case 218:
#line 808 "parser.y"
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

  case 219:
#line 824 "parser.y"
    { (yyval.type) = make_int(-1); ;}
    break;

  case 220:
#line 825 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 221:
#line 826 "parser.y"
    { (yyval.type) = duptype(find_type("float", 0), 1); ;}
    break;

  case 222:
#line 827 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 223:
#line 828 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 224:
#line 829 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 225:
#line 830 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 228:
#line 837 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 229:
#line 838 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[-1].str)); ;}
    break;

  case 230:
#line 839 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 231:
#line 840 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[-1].str)); ;}
    break;

  case 232:
#line 841 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[-1].str)); ;}
    break;

  case 233:
#line 842 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 234:
#line 843 "parser.y"
    { (yyval.type) = make_builtin((yyvsp[0].str)); ;}
    break;

  case 235:
#line 846 "parser.y"
    { (yyval.type) = make_class((yyvsp[0].str)); ;}
    break;

  case 236:
#line 847 "parser.y"
    { (yyval.type) = find_type((yyvsp[0].str), 0);
						  if ((yyval.type)->defined) error_loc("multiple definition error\n");
						  if ((yyval.type)->kind != TKIND_COCLASS) error_loc("%s was not declared a coclass\n", (yyvsp[0].str));
						;}
    break;

  case 237:
#line 853 "parser.y"
    { (yyval.type) = (yyvsp[0].type);
						  (yyval.type)->attrs = check_coclass_attrs((yyvsp[0].type)->name, (yyvsp[-1].attr_list));
						  if (!parse_only && do_header)
						    write_coclass((yyval.type));
						  if (!parse_only && do_idfile)
						    write_clsid((yyval.type));
						;}
    break;

  case 238:
#line 863 "parser.y"
    { (yyval.type) = (yyvsp[-4].type);
						  (yyval.type)->ifaces = (yyvsp[-2].ifref_list);
						  (yyval.type)->defined = TRUE;
						;}
    break;

  case 239:
#line 869 "parser.y"
    { (yyval.ifref_list) = NULL; ;}
    break;

  case 240:
#line 870 "parser.y"
    { (yyval.ifref_list) = append_ifref( (yyvsp[-1].ifref_list), (yyvsp[0].ifref) ); ;}
    break;

  case 241:
#line 874 "parser.y"
    { (yyval.ifref) = make_ifref((yyvsp[0].type)); (yyval.ifref)->attrs = (yyvsp[-1].attr_list); ;}
    break;

  case 242:
#line 877 "parser.y"
    { (yyval.type) = get_type(0, (yyvsp[0].str), 0); (yyval.type)->kind = TKIND_DISPATCH; ;}
    break;

  case 243:
#line 878 "parser.y"
    { (yyval.type) = get_type(0, (yyvsp[0].str), 0); (yyval.type)->kind = TKIND_DISPATCH; ;}
    break;

  case 244:
#line 881 "parser.y"
    { attr_t *attrs;
						  is_in_interface = TRUE;
						  is_object_interface = TRUE;
						  (yyval.type) = (yyvsp[0].type);
						  if ((yyval.type)->defined) error_loc("multiple definition error\n");
						  attrs = make_attr(ATTR_DISPINTERFACE);
						  (yyval.type)->attrs = append_attr( check_dispiface_attrs((yyvsp[0].type)->name, (yyvsp[-1].attr_list)), attrs );
						  (yyval.type)->ref = find_type("IDispatch", 0);
						  if (!(yyval.type)->ref) error_loc("IDispatch is undefined\n");
						  (yyval.type)->defined = TRUE;
						  if (!parse_only && do_header) write_forward((yyval.type));
						;}
    break;

  case 245:
#line 895 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 246:
#line 896 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[-2].var_list), (yyvsp[-1].var) ); ;}
    break;

  case 247:
#line 899 "parser.y"
    { (yyval.func_list) = NULL; ;}
    break;

  case 248:
#line 900 "parser.y"
    { (yyval.func_list) = append_func( (yyvsp[-2].func_list), (yyvsp[-1].func) ); ;}
    break;

  case 249:
#line 906 "parser.y"
    { (yyval.type) = (yyvsp[-4].type);
						  (yyval.type)->fields_or_args = (yyvsp[-2].var_list);
						  (yyval.type)->funcs = (yyvsp[-1].func_list);
						  if (!parse_only && do_header) write_interface((yyval.type));
						  if (!parse_only && do_idfile) write_diid((yyval.type));
						  is_in_interface = FALSE;
						;}
    break;

  case 250:
#line 914 "parser.y"
    { (yyval.type) = (yyvsp[-4].type);
						  (yyval.type)->fields_or_args = (yyvsp[-2].type)->fields_or_args;
						  (yyval.type)->funcs = (yyvsp[-2].type)->funcs;
						  if (!parse_only && do_header) write_interface((yyval.type));
						  if (!parse_only && do_idfile) write_diid((yyval.type));
						  is_in_interface = FALSE;
						;}
    break;

  case 251:
#line 923 "parser.y"
    { (yyval.type) = NULL; ;}
    break;

  case 252:
#line 924 "parser.y"
    { (yyval.type) = find_type2((yyvsp[0].str), 0); ;}
    break;

  case 253:
#line 927 "parser.y"
    { (yyval.type) = get_type(RPC_FC_IP, (yyvsp[0].str), 0); (yyval.type)->kind = TKIND_INTERFACE; ;}
    break;

  case 254:
#line 928 "parser.y"
    { (yyval.type) = get_type(RPC_FC_IP, (yyvsp[0].str), 0); (yyval.type)->kind = TKIND_INTERFACE; ;}
    break;

  case 255:
#line 931 "parser.y"
    { (yyval.ifinfo).interface = (yyvsp[0].type);
						  (yyval.ifinfo).old_pointer_default = pointer_default;
						  if (is_attr((yyvsp[-1].attr_list), ATTR_POINTERDEFAULT))
						    pointer_default = get_attrv((yyvsp[-1].attr_list), ATTR_POINTERDEFAULT);
						  is_object_interface = is_object((yyvsp[-1].attr_list));
						  is_in_interface = TRUE;
						  if ((yyvsp[0].type)->defined) error_loc("multiple definition error\n");
						  (yyvsp[0].type)->attrs = check_iface_attrs((yyvsp[0].type)->name, (yyvsp[-1].attr_list));
						  (yyvsp[0].type)->defined = TRUE;
						  if (!parse_only && do_header) write_forward((yyvsp[0].type));
						;}
    break;

  case 256:
#line 945 "parser.y"
    { (yyval.type) = (yyvsp[-5].ifinfo).interface;
						  (yyval.type)->ref = (yyvsp[-4].type);
						  (yyval.type)->funcs = (yyvsp[-2].func_list);
						  check_functions((yyval.type));
						  compute_method_indexes((yyval.type));
						  if (!parse_only && do_header) write_interface((yyval.type));
						  if (!parse_only && local_stubs) write_locals(local_stubs, (yyval.type), TRUE);
						  if (!parse_only && do_idfile) write_iid((yyval.type));
						  pointer_default = (yyvsp[-5].ifinfo).old_pointer_default;
						  is_in_interface = FALSE;
						;}
    break;

  case 257:
#line 960 "parser.y"
    { (yyval.type) = (yyvsp[-7].ifinfo).interface;
						  (yyval.type)->ref = find_type2((yyvsp[-5].str), 0);
						  if (!(yyval.type)->ref) error_loc("base class '%s' not found in import\n", (yyvsp[-5].str));
						  (yyval.type)->funcs = (yyvsp[-2].func_list);
						  compute_method_indexes((yyval.type));
						  if (!parse_only && do_header) write_interface((yyval.type));
						  if (!parse_only && local_stubs) write_locals(local_stubs, (yyval.type), TRUE);
						  if (!parse_only && do_idfile) write_iid((yyval.type));
						  pointer_default = (yyvsp[-7].ifinfo).old_pointer_default;
						  is_in_interface = FALSE;
						;}
    break;

  case 258:
#line 971 "parser.y"
    { (yyval.type) = (yyvsp[-1].type); ;}
    break;

  case 259:
#line 975 "parser.y"
    { (yyval.type) = (yyvsp[-1].type); if (!parse_only && do_header) write_forward((yyval.type)); ;}
    break;

  case 260:
#line 976 "parser.y"
    { (yyval.type) = (yyvsp[-1].type); if (!parse_only && do_header) write_forward((yyval.type)); ;}
    break;

  case 261:
#line 979 "parser.y"
    { (yyval.type) = make_type(0, NULL); (yyval.type)->name = (yyvsp[0].str); (yyval.type)->kind = TKIND_MODULE; ;}
    break;

  case 262:
#line 980 "parser.y"
    { (yyval.type) = make_type(0, NULL); (yyval.type)->name = (yyvsp[0].str); (yyval.type)->kind = TKIND_MODULE; ;}
    break;

  case 263:
#line 983 "parser.y"
    { (yyval.type) = (yyvsp[0].type);
						  (yyval.type)->attrs = check_module_attrs((yyvsp[0].type)->name, (yyvsp[-1].attr_list));
						;}
    break;

  case 264:
#line 989 "parser.y"
    { (yyval.type) = (yyvsp[-4].type);
						  (yyval.type)->funcs = (yyvsp[-2].func_list);
						  /* FIXME: if (!parse_only && do_header) write_module($$); */
						;}
    break;

  case 265:
#line 996 "parser.y"
    { (yyval.stgclass) = STG_EXTERN; ;}
    break;

  case 266:
#line 997 "parser.y"
    { (yyval.stgclass) = STG_STATIC; ;}
    break;

  case 267:
#line 998 "parser.y"
    { (yyval.stgclass) = STG_REGISTER; ;}
    break;

  case 268:
#line 1002 "parser.y"
    { (yyval.attr) = make_attr(ATTR_INLINE); ;}
    break;

  case 269:
#line 1006 "parser.y"
    { (yyval.attr) = make_attr(ATTR_CONST); ;}
    break;

  case 270:
#line 1009 "parser.y"
    { (yyval.attr_list) = NULL; ;}
    break;

  case 271:
#line 1010 "parser.y"
    { (yyval.attr_list) = append_attr((yyvsp[-1].attr_list), (yyvsp[0].attr)); ;}
    break;

  case 272:
#line 1013 "parser.y"
    { (yyval.declspec) = make_decl_spec((yyvsp[-1].type), (yyvsp[0].declspec), NULL, NULL, STG_NONE); ;}
    break;

  case 273:
#line 1015 "parser.y"
    { (yyval.declspec) = make_decl_spec((yyvsp[-1].type), (yyvsp[-2].declspec), (yyvsp[0].declspec), NULL, STG_NONE); ;}
    break;

  case 274:
#line 1018 "parser.y"
    { (yyval.declspec) = NULL; ;}
    break;

  case 276:
#line 1023 "parser.y"
    { (yyval.declspec) = make_decl_spec(NULL, (yyvsp[0].declspec), NULL, (yyvsp[-1].attr), STG_NONE); ;}
    break;

  case 277:
#line 1024 "parser.y"
    { (yyval.declspec) = make_decl_spec(NULL, (yyvsp[0].declspec), NULL, (yyvsp[-1].attr), STG_NONE); ;}
    break;

  case 278:
#line 1025 "parser.y"
    { (yyval.declspec) = make_decl_spec(NULL, (yyvsp[0].declspec), NULL, NULL, (yyvsp[-1].stgclass)); ;}
    break;

  case 279:
#line 1030 "parser.y"
    { (yyval.declarator) = (yyvsp[0].declarator); (yyval.declarator)->type = append_ptrchain_type((yyval.declarator)->type, make_pointer_type(NULL, (yyvsp[-1].attr_list))); ;}
    break;

  case 280:
#line 1031 "parser.y"
    { (yyval.declarator) = (yyvsp[0].declarator); (yyval.declarator)->type->attrs = append_attr((yyval.declarator)->type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[-1].str))); ;}
    break;

  case 282:
#line 1036 "parser.y"
    { (yyval.declarator) = make_declarator((yyvsp[0].var)); ;}
    break;

  case 283:
#line 1037 "parser.y"
    { (yyval.declarator) = (yyvsp[-1].declarator); ;}
    break;

  case 284:
#line 1038 "parser.y"
    { (yyval.declarator) = (yyvsp[-1].declarator); (yyval.declarator)->array = append_array((yyval.declarator)->array, (yyvsp[0].expr)); ;}
    break;

  case 285:
#line 1039 "parser.y"
    { (yyval.declarator) = (yyvsp[-3].declarator);
						  (yyval.declarator)->func_type = append_ptrchain_type((yyval.declarator)->type, make_func_type((yyvsp[-1].var_list)));
						  (yyval.declarator)->type = NULL;
						;}
    break;

  case 286:
#line 1046 "parser.y"
    { (yyval.declarator_list) = append_declarator( NULL, (yyvsp[0].declarator) ); ;}
    break;

  case 287:
#line 1047 "parser.y"
    { (yyval.declarator_list) = append_declarator( (yyvsp[-2].declarator_list), (yyvsp[0].declarator) ); ;}
    break;

  case 288:
#line 1051 "parser.y"
    { (yyval.declarator) = (yyvsp[0].declarator); ;}
    break;

  case 289:
#line 1052 "parser.y"
    { (yyval.declarator) = (yyvsp[-2].declarator); (yyvsp[-2].declarator)->var->eval = (yyvsp[0].expr); ;}
    break;

  case 290:
#line 1056 "parser.y"
    { (yyval.num) = RPC_FC_RP; ;}
    break;

  case 291:
#line 1057 "parser.y"
    { (yyval.num) = RPC_FC_UP; ;}
    break;

  case 292:
#line 1058 "parser.y"
    { (yyval.num) = RPC_FC_FP; ;}
    break;

  case 293:
#line 1061 "parser.y"
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

  case 294:
#line 1072 "parser.y"
    { (yyval.type) = duptype(find_type("void", 0), 1); ;}
    break;

  case 295:
#line 1073 "parser.y"
    { (yyval.type) = find_type((yyvsp[0].str), 0); ;}
    break;

  case 296:
#line 1074 "parser.y"
    { (yyval.type) = (yyvsp[0].type); ;}
    break;

  case 297:
#line 1075 "parser.y"
    { (yyval.type) = (yyvsp[0].type); ;}
    break;

  case 298:
#line 1076 "parser.y"
    { (yyval.type) = find_type2((yyvsp[0].str), tsENUM); ;}
    break;

  case 299:
#line 1077 "parser.y"
    { (yyval.type) = (yyvsp[0].type); ;}
    break;

  case 300:
#line 1078 "parser.y"
    { (yyval.type) = get_type(RPC_FC_STRUCT, (yyvsp[0].str), tsSTRUCT); ;}
    break;

  case 301:
#line 1079 "parser.y"
    { (yyval.type) = (yyvsp[0].type); ;}
    break;

  case 302:
#line 1080 "parser.y"
    { (yyval.type) = find_type2((yyvsp[0].str), tsUNION); ;}
    break;

  case 303:
#line 1081 "parser.y"
    { (yyval.type) = make_safearray((yyvsp[-1].type)); ;}
    break;

  case 304:
#line 1085 "parser.y"
    { reg_typedefs((yyvsp[-1].declspec), (yyvsp[0].declarator_list), check_typedef_attrs((yyvsp[-2].attr_list)));
						  (yyval.statement) = process_typedefs((yyvsp[0].declarator_list));
						;}
    break;

  case 305:
#line 1091 "parser.y"
    { (yyval.type) = get_typev(RPC_FC_NON_ENCAPSULATED_UNION, (yyvsp[-3].var), tsUNION);
						  (yyval.type)->kind = TKIND_UNION;
						  (yyval.type)->fields_or_args = (yyvsp[-1].var_list);
						  (yyval.type)->defined = TRUE;
						;}
    break;

  case 306:
#line 1098 "parser.y"
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

  case 307:
#line 1113 "parser.y"
    { (yyval.num) = MAKEVERSION((yyvsp[0].num), 0); ;}
    break;

  case 308:
#line 1114 "parser.y"
    { (yyval.num) = MAKEVERSION((yyvsp[-2].num), (yyvsp[0].num)); ;}
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
#line 4162 "parser.tab.c"

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


#line 1117 "parser.y"


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

type_t *make_type(unsigned char type, type_t *ref)
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
  t->sign = 0;
  t->defined = FALSE;
  t->written = FALSE;
  t->user_types_registered = FALSE;
  t->tfswrite = FALSE;
  t->checked = FALSE;
  t->typelib_idx = -1;
  return t;
}

static type_t *make_func_type(var_list_t *args)
{
  type_t *t = make_type(RPC_FC_FUNCTION, NULL);
  t->fields_or_args = args;
  return t;
}

static type_t *make_pointer_type(type_t *ref, attr_list_t *attrs)
{
    type_t *t = make_type(pointer_default, ref);
    t->attrs = attrs;
    return t;
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
  int ptr_attr = get_attrv(v->attrs, ATTR_POINTERTYPE);
  int sizeless, has_varconf;
  expr_t *dim;
  type_t *atype, **ptype;
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
      for (t = func_type; is_ptr(t); t = t->ref)
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
    const type_t *ptr = NULL;
    /* pointer attributes on the left side of the type belong to the function
     * pointer, if one is being declared */
    type_t **pt = func_type ? &func_type : &v->type;
    for (ptr = *pt; ptr; )
    {
      if (ptr->kind == TKIND_ALIAS)
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

  sizeless = FALSE;
  if (arr) LIST_FOR_EACH_ENTRY_REV(dim, arr, expr_t, entry)
  {
    if (sizeless)
      error_loc("%s: only the first array dimension can be unspecified\n", v->name);

    if (dim->is_const)
    {
      unsigned int align = 0;
      size_t size = type_memsize(v->type, &align);

      if (dim->cval <= 0)
        error_loc("%s: array dimension must be positive\n", v->name);

      if (0xffffffffuL / size < (unsigned long) dim->cval)
        error_loc("%s: total array size is too large\n", v->name);
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
        error_loc("%s: cannot specify size_is for a fixed sized array\n", v->name);

      if (atype->type != RPC_FC_CARRAY && !is_ptr(atype))
        error_loc("%s: size_is attribute applied to illegal type\n", v->name);

      atype->type = RPC_FC_CARRAY;
      atype->size_is = dim;
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
      has_varconf = TRUE;
      atype = *ptype = duptype(*ptype, 0);

      if (atype->type == RPC_FC_SMFARRAY)
        atype->type = RPC_FC_SMVARRAY;
      else if (atype->type == RPC_FC_LGFARRAY)
        atype->type = RPC_FC_LGVARRAY;
      else if (atype->type == RPC_FC_CARRAY)
        atype->type = RPC_FC_CVARRAY;
      else
        error_loc("%s: length_is attribute applied to illegal type\n", v->name);

      atype->length_is = dim;
    }

    ptype = &(*ptype)->ref;
    if (*ptype == NULL)
      error_loc("%s: too many expressions in length_is attribute\n", v->name);
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

  /* v->type is currently pointing the the type on the left-side of the
   * declaration, so we need to fix this up so that it is the return type of the
   * function and make v->type point to the function side of the declaration */
  if (func_type)
  {
    type_t *ft, *t;
    type_t *return_type = v->type;
    v->type = func_type;
    for (ft = v->type; is_ptr(ft); ft = ft->ref)
      ;
    assert(ft->type == RPC_FC_FUNCTION);
    ft->ref = return_type;
    /* move calling convention attribute, if present, from pointer nodes to
     * function node */
    for (t = v->type; is_ptr(t); t = t->ref)
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
    for (t = v->type; is_ptr(t); t = t->ref)
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
  v->loc_info.input_name = input_name ? input_name : "stdin";
  v->loc_info.line_number = line_number;
  v->loc_info.near_text = parser_text;
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
  f->args = def->type->fields_or_args;
  f->ignore = parse_only;
  f->idx = -1;
  return f;
}

static type_t *make_class(char *name)
{
  type_t *c = make_type(RPC_FC_COCLASS, NULL);
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

static typelib_t *make_library(const char *name, const attr_list_t *attrs)
{
    typelib_t *typelib = xmalloc(sizeof(*typelib));
    typelib->name = xstrdup(name);
    typelib->filename = NULL;
    typelib->attrs = attrs;
    list_init( &typelib->entries );
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
      t = t->ref;

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
  if ((type->kind == TKIND_ENUM || type->kind == TKIND_RECORD
       || type->kind == TKIND_UNION) && ! type->name && ! parse_only)
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

      /* set the attributes to allow set_type to do some checks on them */
      name->attrs = attrs;
      set_type(name, decl_spec, decl, 0);
      cur = alias(name->type, name->name);
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

static void write_libid(const typelib_t *typelib)
{
  const UUID *uuid = get_attrp(typelib->attrs, ATTR_UUID);
  write_guid(idfile, "LIBID", typelib->name, uuid);
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

struct allowed_attr
{
    unsigned int dce_compatible : 1;
    unsigned int acf : 1;
    unsigned int on_interface : 1;
    unsigned int on_function : 1;
    unsigned int on_arg : 1;
    unsigned int on_type : 1;
    unsigned int on_field : 1;
    unsigned int on_library : 1;
    unsigned int on_dispinterface : 1;
    unsigned int on_module : 1;
    unsigned int on_coclass : 1;
    const char *display_name;
};

struct allowed_attr allowed_attr[] =
{
    /* attr                     { D ACF I Fn ARG T Fi  L  DI M  C  <display name> } */
    /* ATTR_AGGREGATABLE */     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "aggregatable" },
    /* ATTR_APPOBJECT */        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "appobject" },
    /* ATTR_ASYNC */            { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, "async" },
    /* ATTR_AUTO_HANDLE */      { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, "auto_handle" },
    /* ATTR_BINDABLE */         { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "bindable" },
    /* ATTR_BROADCAST */        { 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "broadcast" },
    /* ATTR_CALLAS */           { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "call_as" },
    /* ATTR_CALLCONV */         { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, NULL },
    /* ATTR_CASE */             { 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, "case" },
    /* ATTR_CONST */            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "const" },
    /* ATTR_CONTEXTHANDLE */    { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, "context_handle" },
    /* ATTR_CONTROL */          { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, "control" },
    /* ATTR_DEFAULT */          { 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, "default" },
    /* ATTR_DEFAULTCOLLELEM */  { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "defaultcollelem" },
    /* ATTR_DEFAULTVALUE */     { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, "defaultvalue" },
    /* ATTR_DEFAULTVTABLE */    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "defaultvtable" },
    /* ATTR_DISPINTERFACE */    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL },
    /* ATTR_DISPLAYBIND */      { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "displaybind" },
    /* ATTR_DLLNAME */          { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, "dllname" },
    /* ATTR_DUAL */             { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "dual" },
    /* ATTR_ENDPOINT */         { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "endpoint" },
    /* ATTR_ENTRY */            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "entry" },
    /* ATTR_EXPLICIT_HANDLE */  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, "explicit_handle" },
    /* ATTR_HANDLE */           { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, "handle" },
    /* ATTR_HELPCONTEXT */      { 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, "helpcontext" },
    /* ATTR_HELPFILE */         { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, "helpfile" },
    /* ATTR_HELPSTRING */       { 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, "helpstring" },
    /* ATTR_HELPSTRINGCONTEXT */ { 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, "helpstringcontext" },
    /* ATTR_HELPSTRINGDLL */    { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, "helpstringdll" },
    /* ATTR_HIDDEN */           { 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, "hidden" },
    /* ATTR_ID */               { 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, "id" },
    /* ATTR_IDEMPOTENT */       { 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "idempotent" },
    /* ATTR_IIDIS */            { 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, "iid_is" },
    /* ATTR_IMMEDIATEBIND */    { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "immediatebind" },
    /* ATTR_IMPLICIT_HANDLE */  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, "implicit_handle" },
    /* ATTR_IN */               { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, "in" },
    /* ATTR_INLINE */           { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "inline" },
    /* ATTR_INPUTSYNC */        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "inputsync" },
    /* ATTR_LENGTHIS */         { 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, "length_is" },
    /* ATTR_LIBLCID */          { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, "lcid" },
    /* ATTR_LOCAL */            { 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, "local" },
    /* ATTR_NONBROWSABLE */     { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "nonbrowsable" },
    /* ATTR_NONCREATABLE */     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "noncreatable" },
    /* ATTR_NONEXTENSIBLE */    { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "nonextensible" },
    /* ATTR_OBJECT */           { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "object" },
    /* ATTR_ODL */              { 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, "odl" },
    /* ATTR_OLEAUTOMATION */    { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "oleautomation" },
    /* ATTR_OPTIONAL */         { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, "optional" },
    /* ATTR_OUT */              { 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, "out" },
    /* ATTR_POINTERDEFAULT */   { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "pointer_default" },
    /* ATTR_POINTERTYPE */      { 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, "ref, unique or ptr" },
    /* ATTR_PROPGET */          { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "propget" },
    /* ATTR_PROPPUT */          { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "propput" },
    /* ATTR_PROPPUTREF */       { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "propputref" },
    /* ATTR_PUBLIC */           { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, "public" },
    /* ATTR_RANGE */            { 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, "range" },
    /* ATTR_READONLY */         { 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, "readonly" },
    /* ATTR_REQUESTEDIT */      { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "requestedit" },
    /* ATTR_RESTRICTED */       { 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, "restricted" },
    /* ATTR_RETVAL */           { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, "retval" },
    /* ATTR_SIZEIS */           { 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, "size_is" },
    /* ATTR_SOURCE */           { 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, "source" },
    /* ATTR_STRICTCONTEXTHANDLE */ { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "strict_context_handle" },
    /* ATTR_STRING */           { 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, "string" },
    /* ATTR_SWITCHIS */         { 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, "switch_is" },
    /* ATTR_SWITCHTYPE */       { 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, "switch_type" },
    /* ATTR_TRANSMITAS */       { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, "transmit_as" },
    /* ATTR_UUID */             { 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, "uuid" },
    /* ATTR_V1ENUM */           { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, "v1_enum" },
    /* ATTR_VARARG */           { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "vararg" },
    /* ATTR_VERSION */          { 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, "version" },
    /* ATTR_WIREMARSHAL */      { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, "wire_marshal" },
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
    for (;;)
    {
        if (type->kind == TKIND_ALIAS)
            type = type->orig;
        else if (is_ptr(type))
        {
            type = type->ref;
            break;
        }
        else
            return FALSE;
    }
    return (type_memsize(type, &align) == 16);
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
        if (type->kind == TKIND_ALIAS)
            type = type->orig;
        else if (is_ptr(type) || is_array(type))
            type = type->ref;
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

    if (type->checked)
        return;

    type->checked = TRUE;

    if (is_struct(type->type))
        fields = type->fields_or_args;
    else if (is_union(type->type))
    {
        if (type->type == RPC_FC_ENCAPSULATED_UNION)
        {
            const var_t *uv = LIST_ENTRY(list_tail(type->fields_or_args), const var_t, entry);
            fields = uv->type->fields_or_args;
        }
        else
            fields = type->fields_or_args;
    }

    if (fields) LIST_FOR_EACH_ENTRY( field, type->fields_or_args, const var_t, entry )
        if (field->type) check_field_common(type, type->name, field);
}

/* checks that arguments for a function make sense for marshalling and unmarshalling */
static void check_remoting_args(const func_t *func)
{
    const char *funcname = func->def->name;
    const var_t *arg;

    if (func->args) LIST_FOR_EACH_ENTRY( arg, func->args, const var_t, entry )
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
            if (type->kind == TKIND_ALIAS)
                type = type->orig;
            else if (is_ptr(type))
            {
                ptr_level++;
                type = type->ref;
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

        check_field_common(func->def->type, funcname, arg);
    }
}

static void add_explicit_handle_if_necessary(func_t *func)
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
                idl_handle->type = find_type("handle_t", 0);
                if (!func->def->type->fields_or_args)
                {
                    func->def->type->fields_or_args = xmalloc( sizeof(*func->def->type->fields_or_args) );
                    list_init( func->def->type->fields_or_args );
                }
                list_add_head( func->def->type->fields_or_args, &idl_handle->entry );
                func->args = func->def->type->fields_or_args;
            }
        }
    }
}

static void check_functions(const type_t *iface)
{
    if (is_attr(iface->attrs, ATTR_EXPLICIT_HANDLE) && iface->funcs)
    {
        func_t *func;
        LIST_FOR_EACH_ENTRY( func, iface->funcs, func_t, entry )
            add_explicit_handle_if_necessary(func);
    }
    if (!is_inside_library && !is_attr(iface->attrs, ATTR_LOCAL))
    {
        const func_t *func;
        if (iface->funcs) LIST_FOR_EACH_ENTRY( func, iface->funcs, const func_t, entry )
        {
            if (!is_attr(func->def->attrs, ATTR_LOCAL))
                check_remoting_args(func);
        }
    }
}

static void check_all_user_types(const statement_list_t *stmts)
{
  const statement_t *stmt;

  if (stmts) LIST_FOR_EACH_ENTRY(stmt, stmts, const statement_t, entry)
  {
    if (stmt->type == STMT_LIBRARY)
      check_all_user_types(stmt->u.lib->stmts);
    else if (stmt->type == STMT_TYPE && stmt->u.type->type == RPC_FC_IP)
    {
      const func_t *f;
      const func_list_t *fs = stmt->u.type->funcs;
      if (fs) LIST_FOR_EACH_ENTRY(f, fs, const func_t, entry)
        check_for_additional_prototype_types(f->args);
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

static statement_t *process_typedefs(declarator_list_t *decls)
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
        type_t *type = find_type(var->name, 0);
        *type_list = xmalloc(sizeof(type_list_t));
        (*type_list)->type = type;
        (*type_list)->next = NULL;

        if (! parse_only && do_header)
            write_typedef(type);
        if (in_typelib && is_attr(type->attrs, ATTR_PUBLIC))
            add_typelib_entry(type);

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

static func_list_t *append_func_from_statement(func_list_t *list, statement_t *stmt)
{
    if (stmt->type == STMT_DECLARATION)
    {
        var_t *var = stmt->u.var;
        if (var->stgclass == STG_NONE && var->type->type == RPC_FC_FUNCTION)
        {
            check_function_attrs(var->name, var->type->attrs);
            return append_func(list, make_func(stmt->u.var));
        }
    }
    return list;
}

