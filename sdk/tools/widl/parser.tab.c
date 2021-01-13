/* A Bison parser, made by GNU Bison 3.5.4.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.5.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         parser_parse
#define yylex           parser_lex
#define yyerror         parser_error
#define yydebug         parser_debug
#define yynerrs         parser_nerrs
#define yylval          parser_lval
#define yychar          parser_char

/* First part of user prologue.  */
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

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"
#include "typelib.h"
#include "typegen.h"
#include "expr.h"
#include "typetree.h"

static unsigned char pointer_default = FC_UP;

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

static void fix_incomplete(void);
static void fix_incomplete_types(type_t *complete_type);

static str_list_t *append_str(str_list_t *list, char *str);
static attr_list_t *append_attr(attr_list_t *list, attr_t *attr);
static attr_list_t *append_attr_list(attr_list_t *new_list, attr_list_t *old_list);
static decl_spec_t *make_decl_spec(type_t *type, decl_spec_t *left, decl_spec_t *right, attr_t *attr, enum storage_class stgclass);
static attr_t *make_attr(enum attr_type type);
static attr_t *make_attrv(enum attr_type type, unsigned int val);
static attr_t *make_attrp(enum attr_type type, void *val);
static expr_list_t *append_expr(expr_list_t *list, expr_t *expr);
static type_t *append_array(type_t *chain, expr_t *expr);
static var_t *declare_var(attr_list_t *attrs, decl_spec_t *decl_spec, const declarator_t *decl, int top);
static var_list_t *set_var_types(attr_list_t *attrs, decl_spec_t *decl_spec, declarator_list_t *decls);
static ifref_list_t *append_ifref(ifref_list_t *list, ifref_t *iface);
static ifref_t *make_ifref(type_t *iface);
static var_list_t *append_var_list(var_list_t *list, var_list_t *vars);
static declarator_list_t *append_declarator(declarator_list_t *list, declarator_t *p);
static declarator_t *make_declarator(var_t *var);
static type_t *make_safearray(type_t *type);
static typelib_t *make_library(const char *name, const attr_list_t *attrs);
static type_t *append_chain_type(type_t *chain, type_t *type);
static warning_list_t *append_warning(warning_list_t *, int);

static type_t *reg_typedefs(decl_spec_t *decl_spec, var_list_t *names, attr_list_t *attrs);
static type_t *find_type_or_error(const char *name, int t);
static type_t *find_type_or_error2(char *name, int t);

static var_t *reg_const(var_t *var);

static void push_namespace(const char *name);
static void pop_namespace(const char *name);

static char *gen_name(void);
static void check_arg_attrs(const var_t *arg);
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
static void add_explicit_handle_if_necessary(const type_t *iface, var_t *func);
static void check_def(const type_t *t);

static void check_async_uuid(type_t *iface);

static statement_t *make_statement(enum statement_type type);
static statement_t *make_statement_type_decl(type_t *type);
static statement_t *make_statement_reference(type_t *type);
static statement_t *make_statement_declaration(var_t *var);
static statement_t *make_statement_library(typelib_t *typelib);
static statement_t *make_statement_pragma(const char *str);
static statement_t *make_statement_cppquote(const char *str);
static statement_t *make_statement_importlib(const char *str);
static statement_t *make_statement_module(type_t *type);
static statement_t *make_statement_typedef(var_list_t *names);
static statement_t *make_statement_import(const char *str);
static statement_list_t *append_statement(statement_list_t *list, statement_t *stmt);
static statement_list_t *append_statements(statement_list_t *, statement_list_t *);
static attr_list_t *append_attribs(attr_list_t *, attr_list_t *);

static struct namespace global_namespace = {
    NULL, NULL, LIST_INIT(global_namespace.entry), LIST_INIT(global_namespace.children)
};

static struct namespace *current_namespace = &global_namespace;

#ifndef __REACTOS__
static typelib_t *current_typelib;
#endif


#line 221 "parser.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
#ifndef YY_PARSER_PARSER_TAB_H_INCLUDED
# define YY_PARSER_PARSER_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int parser_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    aIDENTIFIER = 258,
    aPRAGMA = 259,
    aKNOWNTYPE = 260,
    aNUM = 261,
    aHEXNUM = 262,
    aDOUBLE = 263,
    aSTRING = 264,
    aWSTRING = 265,
    aSQSTRING = 266,
    aUUID = 267,
    aEOF = 268,
    aACF = 269,
    SHL = 270,
    SHR = 271,
    MEMBERPTR = 272,
    EQUALITY = 273,
    INEQUALITY = 274,
    GREATEREQUAL = 275,
    LESSEQUAL = 276,
    LOGICALOR = 277,
    LOGICALAND = 278,
    ELLIPSIS = 279,
    tAGGREGATABLE = 280,
    tALLNODES = 281,
    tALLOCATE = 282,
    tANNOTATION = 283,
    tAPPOBJECT = 284,
    tASYNC = 285,
    tASYNCUUID = 286,
    tAUTOHANDLE = 287,
    tBINDABLE = 288,
    tBOOLEAN = 289,
    tBROADCAST = 290,
    tBYTE = 291,
    tBYTECOUNT = 292,
    tCALLAS = 293,
    tCALLBACK = 294,
    tCASE = 295,
    tCDECL = 296,
    tCHAR = 297,
    tCOCLASS = 298,
    tCODE = 299,
    tCOMMSTATUS = 300,
    tCONST = 301,
    tCONTEXTHANDLE = 302,
    tCONTEXTHANDLENOSERIALIZE = 303,
    tCONTEXTHANDLESERIALIZE = 304,
    tCONTROL = 305,
    tCPPQUOTE = 306,
    tDECODE = 307,
    tDEFAULT = 308,
    tDEFAULTBIND = 309,
    tDEFAULTCOLLELEM = 310,
    tDEFAULTVALUE = 311,
    tDEFAULTVTABLE = 312,
    tDISABLECONSISTENCYCHECK = 313,
    tDISPLAYBIND = 314,
    tDISPINTERFACE = 315,
    tDLLNAME = 316,
    tDONTFREE = 317,
    tDOUBLE = 318,
    tDUAL = 319,
    tENABLEALLOCATE = 320,
    tENCODE = 321,
    tENDPOINT = 322,
    tENTRY = 323,
    tENUM = 324,
    tERRORSTATUST = 325,
    tEXPLICITHANDLE = 326,
    tEXTERN = 327,
    tFALSE = 328,
    tFASTCALL = 329,
    tFAULTSTATUS = 330,
    tFLOAT = 331,
    tFORCEALLOCATE = 332,
    tHANDLE = 333,
    tHANDLET = 334,
    tHELPCONTEXT = 335,
    tHELPFILE = 336,
    tHELPSTRING = 337,
    tHELPSTRINGCONTEXT = 338,
    tHELPSTRINGDLL = 339,
    tHIDDEN = 340,
    tHYPER = 341,
    tID = 342,
    tIDEMPOTENT = 343,
    tIGNORE = 344,
    tIIDIS = 345,
    tIMMEDIATEBIND = 346,
    tIMPLICITHANDLE = 347,
    tIMPORT = 348,
    tIMPORTLIB = 349,
    tIN = 350,
    tIN_LINE = 351,
    tINLINE = 352,
    tINPUTSYNC = 353,
    tINT = 354,
    tINT32 = 355,
    tINT3264 = 356,
    tINT64 = 357,
    tINTERFACE = 358,
    tLCID = 359,
    tLENGTHIS = 360,
    tLIBRARY = 361,
    tLICENSED = 362,
    tLOCAL = 363,
    tLONG = 364,
    tMAYBE = 365,
    tMESSAGE = 366,
    tMETHODS = 367,
    tMODULE = 368,
    tNAMESPACE = 369,
    tNOCODE = 370,
    tNONBROWSABLE = 371,
    tNONCREATABLE = 372,
    tNONEXTENSIBLE = 373,
    tNOTIFY = 374,
    tNOTIFYFLAG = 375,
    tNULL = 376,
    tOBJECT = 377,
    tODL = 378,
    tOLEAUTOMATION = 379,
    tOPTIMIZE = 380,
    tOPTIONAL = 381,
    tOUT = 382,
    tPARTIALIGNORE = 383,
    tPASCAL = 384,
    tPOINTERDEFAULT = 385,
    tPRAGMA_WARNING = 386,
    tPROGID = 387,
    tPROPERTIES = 388,
    tPROPGET = 389,
    tPROPPUT = 390,
    tPROPPUTREF = 391,
    tPROXY = 392,
    tPTR = 393,
    tPUBLIC = 394,
    tRANGE = 395,
    tREADONLY = 396,
    tREF = 397,
    tREGISTER = 398,
    tREPRESENTAS = 399,
    tREQUESTEDIT = 400,
    tRESTRICTED = 401,
    tRETVAL = 402,
    tSAFEARRAY = 403,
    tSHORT = 404,
    tSIGNED = 405,
    tSINGLENODE = 406,
    tSIZEIS = 407,
    tSIZEOF = 408,
    tSMALL = 409,
    tSOURCE = 410,
    tSTATIC = 411,
    tSTDCALL = 412,
    tSTRICTCONTEXTHANDLE = 413,
    tSTRING = 414,
    tSTRUCT = 415,
    tSWITCH = 416,
    tSWITCHIS = 417,
    tSWITCHTYPE = 418,
    tTHREADING = 419,
    tTRANSMITAS = 420,
    tTRUE = 421,
    tTYPEDEF = 422,
    tUIDEFAULT = 423,
    tUNION = 424,
    tUNIQUE = 425,
    tUNSIGNED = 426,
    tUSESGETLASTERROR = 427,
    tUSERMARSHAL = 428,
    tUUID = 429,
    tV1ENUM = 430,
    tVARARG = 431,
    tVERSION = 432,
    tVIPROGID = 433,
    tVOID = 434,
    tWCHAR = 435,
    tWIREMARSHAL = 436,
    tAPARTMENT = 437,
    tNEUTRAL = 438,
    tSINGLE = 439,
    tFREE = 440,
    tBOTH = 441,
    CAST = 442,
    PPTR = 443,
    POS = 444,
    NEG = 445,
    ADDRESSOF = 446
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 144 "parser.y"

	attr_t *attr;
	attr_list_t *attr_list;
	str_list_t *str_list;
	expr_t *expr;
	expr_list_t *expr_list;
	type_t *type;
	var_t *var;
	var_list_t *var_list;
	declarator_t *declarator;
	declarator_list_t *declarator_list;
	statement_t *statement;
	statement_list_t *stmt_list;
	warning_t *warning;
	warning_list_t *warning_list;
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

#line 493 "parser.tab.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE parser_lval;

int parser_parse (void);

#endif /* !YY_PARSER_PARSER_TAB_H_INCLUDED  */



#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))

/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                            \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
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
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
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
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   3170

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  216
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  115
/* YYNRULES -- Number of rules.  */
#define YYNRULES  417
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  729

#define YYUNDEFTOK  2
#define YYMAXUTOK   446


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   200,     2,     2,     2,   199,   192,     2,
     213,   214,   197,   196,   187,   195,   207,   198,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   189,   212,
     193,   215,   194,   188,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   208,     2,   209,   191,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   210,   190,   211,   201,     2,     2,     2,
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
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   202,   203,   204,   205,   206
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   328,   328,   345,   345,   347,   348,   348,   350,   351,
     352,   355,   358,   359,   360,   363,   364,   365,   365,   367,
     368,   369,   372,   373,   374,   375,   378,   379,   382,   383,
     387,   388,   389,   390,   391,   392,   393,   396,   407,   408,
     412,   413,   414,   415,   416,   417,   418,   419,   420,   423,
     425,   433,   439,   447,   448,   450,   458,   469,   470,   473,
     474,   477,   478,   482,   487,   494,   498,   499,   502,   503,
     507,   510,   511,   512,   515,   516,   519,   520,   521,   522,
     523,   524,   525,   526,   527,   528,   529,   530,   531,   532,
     533,   534,   535,   536,   537,   538,   539,   540,   541,   542,
     543,   544,   545,   546,   547,   548,   549,   550,   551,   552,
     553,   554,   555,   556,   557,   558,   559,   560,   561,   562,
     563,   564,   565,   566,   567,   568,   569,   570,   571,   572,
     573,   574,   575,   576,   577,   578,   579,   580,   581,   582,
     583,   584,   585,   586,   587,   588,   589,   590,   591,   592,
     593,   594,   598,   599,   600,   601,   602,   603,   604,   605,
     606,   607,   608,   609,   610,   611,   612,   613,   614,   615,
     616,   617,   618,   619,   620,   621,   625,   626,   631,   632,
     633,   634,   637,   638,   641,   645,   651,   652,   653,   656,
     660,   672,   676,   681,   684,   685,   688,   689,   692,   693,
     694,   695,   696,   697,   698,   699,   700,   701,   702,   703,
     704,   705,   706,   707,   708,   709,   710,   711,   712,   713,
     714,   715,   716,   717,   718,   719,   720,   721,   722,   723,
     724,   725,   726,   727,   728,   729,   731,   733,   734,   737,
     738,   741,   747,   753,   754,   757,   762,   769,   770,   773,
     774,   778,   779,   782,   786,   792,   800,   804,   809,   810,
     813,   814,   815,   818,   820,   823,   824,   825,   826,   827,
     828,   829,   830,   831,   832,   833,   836,   837,   840,   841,
     842,   843,   844,   845,   846,   847,   848,   851,   852,   860,
     866,   870,   873,   874,   878,   881,   882,   885,   894,   895,
     898,   899,   902,   908,   914,   915,   918,   919,   922,   932,
     942,   948,   952,   953,   956,   957,   960,   965,   972,   973,
     974,   978,   982,   985,   986,   989,   990,   994,   995,   999,
    1000,  1001,  1005,  1007,  1009,  1013,  1014,  1015,  1016,  1024,
    1026,  1028,  1033,  1035,  1040,  1041,  1046,  1047,  1048,  1049,
    1054,  1063,  1065,  1066,  1071,  1073,  1077,  1078,  1085,  1086,
    1087,  1088,  1089,  1094,  1102,  1103,  1106,  1107,  1110,  1117,
    1118,  1123,  1124,  1128,  1129,  1130,  1131,  1132,  1136,  1137,
    1138,  1141,  1144,  1145,  1146,  1147,  1148,  1149,  1150,  1151,
    1152,  1153,  1156,  1163,  1165,  1171,  1172,  1173,  1177,  1178,
    1182,  1183,  1187,  1194,  1203,  1204,  1208,  1209,  1213,  1215,
    1216,  1217,  1221,  1222,  1227,  1228,  1229,  1230
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "aIDENTIFIER", "aPRAGMA", "aKNOWNTYPE",
  "aNUM", "aHEXNUM", "aDOUBLE", "aSTRING", "aWSTRING", "aSQSTRING",
  "aUUID", "aEOF", "aACF", "SHL", "SHR", "MEMBERPTR", "EQUALITY",
  "INEQUALITY", "GREATEREQUAL", "LESSEQUAL", "LOGICALOR", "LOGICALAND",
  "ELLIPSIS", "tAGGREGATABLE", "tALLNODES", "tALLOCATE", "tANNOTATION",
  "tAPPOBJECT", "tASYNC", "tASYNCUUID", "tAUTOHANDLE", "tBINDABLE",
  "tBOOLEAN", "tBROADCAST", "tBYTE", "tBYTECOUNT", "tCALLAS", "tCALLBACK",
  "tCASE", "tCDECL", "tCHAR", "tCOCLASS", "tCODE", "tCOMMSTATUS", "tCONST",
  "tCONTEXTHANDLE", "tCONTEXTHANDLENOSERIALIZE", "tCONTEXTHANDLESERIALIZE",
  "tCONTROL", "tCPPQUOTE", "tDECODE", "tDEFAULT", "tDEFAULTBIND",
  "tDEFAULTCOLLELEM", "tDEFAULTVALUE", "tDEFAULTVTABLE",
  "tDISABLECONSISTENCYCHECK", "tDISPLAYBIND", "tDISPINTERFACE", "tDLLNAME",
  "tDONTFREE", "tDOUBLE", "tDUAL", "tENABLEALLOCATE", "tENCODE",
  "tENDPOINT", "tENTRY", "tENUM", "tERRORSTATUST", "tEXPLICITHANDLE",
  "tEXTERN", "tFALSE", "tFASTCALL", "tFAULTSTATUS", "tFLOAT",
  "tFORCEALLOCATE", "tHANDLE", "tHANDLET", "tHELPCONTEXT", "tHELPFILE",
  "tHELPSTRING", "tHELPSTRINGCONTEXT", "tHELPSTRINGDLL", "tHIDDEN",
  "tHYPER", "tID", "tIDEMPOTENT", "tIGNORE", "tIIDIS", "tIMMEDIATEBIND",
  "tIMPLICITHANDLE", "tIMPORT", "tIMPORTLIB", "tIN", "tIN_LINE", "tINLINE",
  "tINPUTSYNC", "tINT", "tINT32", "tINT3264", "tINT64", "tINTERFACE",
  "tLCID", "tLENGTHIS", "tLIBRARY", "tLICENSED", "tLOCAL", "tLONG",
  "tMAYBE", "tMESSAGE", "tMETHODS", "tMODULE", "tNAMESPACE", "tNOCODE",
  "tNONBROWSABLE", "tNONCREATABLE", "tNONEXTENSIBLE", "tNOTIFY",
  "tNOTIFYFLAG", "tNULL", "tOBJECT", "tODL", "tOLEAUTOMATION", "tOPTIMIZE",
  "tOPTIONAL", "tOUT", "tPARTIALIGNORE", "tPASCAL", "tPOINTERDEFAULT",
  "tPRAGMA_WARNING", "tPROGID", "tPROPERTIES", "tPROPGET", "tPROPPUT",
  "tPROPPUTREF", "tPROXY", "tPTR", "tPUBLIC", "tRANGE", "tREADONLY",
  "tREF", "tREGISTER", "tREPRESENTAS", "tREQUESTEDIT", "tRESTRICTED",
  "tRETVAL", "tSAFEARRAY", "tSHORT", "tSIGNED", "tSINGLENODE", "tSIZEIS",
  "tSIZEOF", "tSMALL", "tSOURCE", "tSTATIC", "tSTDCALL",
  "tSTRICTCONTEXTHANDLE", "tSTRING", "tSTRUCT", "tSWITCH", "tSWITCHIS",
  "tSWITCHTYPE", "tTHREADING", "tTRANSMITAS", "tTRUE", "tTYPEDEF",
  "tUIDEFAULT", "tUNION", "tUNIQUE", "tUNSIGNED", "tUSESGETLASTERROR",
  "tUSERMARSHAL", "tUUID", "tV1ENUM", "tVARARG", "tVERSION", "tVIPROGID",
  "tVOID", "tWCHAR", "tWIREMARSHAL", "tAPARTMENT", "tNEUTRAL", "tSINGLE",
  "tFREE", "tBOTH", "','", "'?'", "':'", "'|'", "'^'", "'&'", "'<'", "'>'",
  "'-'", "'+'", "'*'", "'/'", "'%'", "'!'", "'~'", "CAST", "PPTR", "POS",
  "NEG", "ADDRESSOF", "'.'", "'['", "']'", "'{'", "'}'", "';'", "'('",
  "')'", "'='", "$accept", "input", "m_acf", "gbl_statements", "$@1",
  "imp_statements", "$@2", "int_statements", "semicolon_opt", "statement",
  "pragma_warning", "warnings", "typedecl", "cppquote", "import_start",
  "import", "importlib", "libraryhdr", "library_start", "librarydef",
  "m_args", "arg_list", "args", "arg", "array", "m_attributes",
  "attributes", "attrib_list", "str_list", "attribute", "uuid_string",
  "callconv", "cases", "case", "enums", "enum_list", "enum", "enumdef",
  "m_exprs", "m_expr", "expr", "expr_list_int_const", "expr_int_const",
  "expr_const", "fields", "field", "ne_union_field", "ne_union_fields",
  "union_field", "s_field", "funcdef", "declaration", "m_ident", "t_ident",
  "ident", "base_type", "m_int", "int_std", "coclass", "coclasshdr",
  "coclassdef", "namespacedef", "coclass_ints", "coclass_int",
  "dispinterface", "dispinterfacehdr", "dispint_props", "dispint_meths",
  "dispinterfacedef", "inherit", "interface", "interfacehdr",
  "interfacedef", "interfacedec", "module", "modulehdr", "moduledef",
  "storage_cls_spec", "function_specifier", "type_qualifier",
  "m_type_qual_list", "decl_spec", "m_decl_spec_no_type",
  "decl_spec_no_type", "declarator", "direct_declarator",
  "abstract_declarator", "abstract_declarator_no_direct",
  "m_abstract_declarator", "abstract_direct_declarator", "any_declarator",
  "any_declarator_no_direct", "m_any_declarator", "any_direct_declarator",
  "declarator_list", "m_bitfield", "struct_declarator",
  "struct_declarator_list", "init_declarator", "threading_type",
  "pointer_type", "structdef", "type", "typedef", "uniondef", "version",
  "acf_statements", "acf_int_statements", "acf_int_statement",
  "acf_interface", "acf_attributes", "acf_attribute_list", "acf_attribute",
  "allocate_option_list", "allocate_option", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_int16 yytoknum[] =
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
     395,   396,   397,   398,   399,   400,   401,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   414,
     415,   416,   417,   418,   419,   420,   421,   422,   423,   424,
     425,   426,   427,   428,   429,   430,   431,   432,   433,   434,
     435,   436,   437,   438,   439,   440,   441,    44,    63,    58,
     124,    94,    38,    60,    62,    45,    43,    42,    47,    37,
      33,   126,   442,   443,   444,   445,   446,    46,    91,    93,
     123,   125,    59,    40,    41,    61
};
# endif

#define YYPACT_NINF (-564)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-405)

#define yytable_value_is_error(Yyn) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -564,   108,  1696,  -564,  -564,  -564,   -66,  -564,  -564,  -564,
     176,  -564,  -101,   195,  -564,   243,  -564,  -564,  -564,  -564,
      38,   148,  -564,  -564,  -564,  -564,  -564,   269,    38,   165,
     -35,  -564,   -14,    38,    15,  -564,  -564,   291,   321,    15,
    -564,  -564,  2989,  -564,  -564,  -564,    47,  -564,  -564,  -564,
    -564,  -564,    51,  2671,    53,    57,  -564,  -564,    64,    24,
    -564,    81,    80,    85,    87,    91,   117,  -564,  -564,   123,
    -564,   -16,   -16,   -16,    48,  2836,   125,   -16,   130,   136,
      96,  -564,   -66,   152,  -564,  -564,   346,  -564,  -564,   119,
    -564,   139,  -564,  -564,   149,  -564,  -564,  -564,  -564,   368,
    2836,  -564,  -564,   122,   168,   -92,  -117,  -564,  -564,   169,
    -564,  -564,   173,  -564,  -564,  -564,   174,   186,  -564,  -564,
    -564,  -564,  -564,  -564,  -564,  -564,  -564,  -564,   187,  -564,
    -564,  -564,   188,  -564,  -564,  -564,   189,   190,  -564,  -564,
    -564,  -564,   191,   192,   194,   203,   206,  -564,   207,  -564,
    -564,   208,  -564,   213,  -564,  -564,   214,   216,  -564,  -564,
    -564,  -564,  -564,  -564,  -564,  -564,  -564,  -564,  -564,  -564,
    -564,   217,  -564,  -564,  -564,   218,   220,  -564,  -564,  -564,
    -564,  -564,  -564,   223,  -564,  -564,   224,  -564,  -564,  -564,
     225,  -564,  -564,  -564,   226,   228,   230,   231,  -564,  -564,
    -564,   236,   257,  -564,  -564,   259,   260,   262,   -62,  -564,
    -564,  -564,  1836,   878,   200,   335,   340,   349,   359,   364,
     267,   268,  -564,  -564,  -564,  -564,    48,   270,   271,  -564,
    -564,  -564,  -564,  -564,   -30,  -564,  -564,  -564,   369,   274,
    -564,  -564,  -564,  -564,  -564,  -564,  -564,  -564,  -564,  -564,
    -564,  -564,    48,    48,  -564,   272,   -58,  -564,  -564,  -564,
     -16,  -564,  -564,  -564,   275,  -564,  -564,  -564,   -12,  -564,
    -564,   423,   278,   376,  -564,   296,   279,  -564,   276,  -564,
     485,    43,   376,   731,   731,   487,   489,   731,   731,   490,
     491,   731,   492,   731,   731,  2224,   731,   731,   494,   -80,
     495,   731,  2836,   731,   731,  2836,    98,  2836,  2836,    43,
     202,   496,  2836,  2989,   298,  -564,   295,  -564,  -564,  -564,
     299,  -564,   302,  -564,  -564,  -564,    87,  2760,  -564,   303,
    -564,  -564,  -564,  -564,   303,   -97,  -564,  -564,  -112,  -564,
     325,   -72,   305,   308,  -564,  -564,  1239,    45,   306,  -564,
     731,   624,  2224,  -564,  -564,    66,    96,  -564,   311,  -564,
     312,   337,  -564,   307,   520,  -564,  -111,   200,    59,   313,
    -564,  -564,   314,   316,  -564,  -564,  -564,  -564,  -564,  -564,
    -564,  -564,  -564,   318,  -564,   731,   731,   731,   731,   731,
     731,   605,  2473,   -93,  -564,  2473,   320,   322,  -564,   -42,
     328,   330,   331,   332,   333,   334,   336,   374,   338,  2760,
      65,   339,   -41,  -564,  2473,   341,   342,   344,   345,   347,
     -38,  2230,   360,  -564,  -564,  -564,  -564,  -564,   361,   362,
     363,   365,   352,  -564,   366,   370,   372,  -564,  2989,   526,
    -564,  -564,  -564,    48,    87,    27,  -564,  1120,  -564,   371,
    2760,   351,  1575,   343,   456,  1358,    87,  -564,  2760,  -564,
    -564,  -564,  -564,   709,  -564,  2390,   373,   391,  -564,  -564,
    -564,  -564,  -564,  -564,   -34,  -564,  -564,   384,  -564,   376,
     731,  -564,    21,  -564,  2760,  -564,   377,  -564,   378,  -564,
     380,  -564,  -564,  -564,  2760,    32,    32,    32,    32,    32,
      32,  2319,   227,   731,   731,   590,   731,   731,   731,   731,
     731,   731,   731,   731,   731,   731,   731,   731,   731,   731,
     731,   731,   731,   592,   731,   731,  -564,  -564,  -564,   587,
    -564,  -564,  -564,  -564,  -564,  -564,  -564,  -564,  -564,  -564,
      65,  -564,  1957,  -564,    65,  -564,  -564,  -564,     8,  -564,
     731,  -564,  -564,  -564,  -564,   731,  -564,  -564,  -564,  -564,
    -564,  -564,  -564,  -564,   591,  -564,  -564,  -564,  -564,   385,
    -564,  -564,   411,  -564,  -564,  -564,  -564,    48,   171,  -564,
    -564,  2760,   388,  -564,  -564,  -564,    87,  -564,  -564,  -564,
    -564,  2135,    66,  -564,   393,   394,   384,  -564,  -564,  -564,
    -564,    65,   390,   376,  -564,  -564,   227,  -564,  -564,  2046,
    -564,   227,  -564,   392,    28,    90,    90,  -564,   672,   672,
     215,   215,  2345,  2504,  2452,  1277,  1396,  1337,   215,   215,
      30,    30,    32,    32,    32,  -564,  2430,  -564,  -564,  -564,
      36,  -564,   395,    65,   403,  -564,  2224,  -564,  -564,   404,
    -564,    87,   999,    48,  -564,  -564,  1477,  -564,  -564,  -564,
    -564,   599,  -564,  -564,   430,  -564,  -103,  -564,   410,  -564,
     407,   289,  -564,   408,    65,   409,  -564,   731,  2224,  -564,
     731,  -564,  -564,    36,  -564,  -564,  -564,   414,  -564,  -564,
    -564,  -564,    87,   412,   731,  -564,    65,  -564,  -564,  -564,
    -564,    36,  -564,  -564,  -564,    32,   415,  2473,  -564,  -564,
    -564,  -564,  -564,  -564,    -7,  -564,  -564,   731,   436,  -564,
    -564,   447,  -147,  -147,  -564,  -564,   425,  -564,  -564
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       5,     0,     3,     1,    35,   383,   398,   273,   265,   284,
       0,   322,     0,     0,   272,   260,   274,   318,   271,   275,
     276,     0,   321,   278,   285,   286,   283,     0,   276,     0,
       0,   320,     0,   276,     0,   280,   319,   260,   260,   270,
     382,   266,    76,     2,    14,    36,     0,    30,    15,    33,
      15,    13,     0,    69,   385,     0,   384,   267,     0,     0,
      11,     0,     0,     0,    28,     0,   304,     9,     8,     0,
      12,   327,   327,   327,     0,     0,   387,   327,     0,   389,
       0,     4,   398,     0,   287,   288,     0,   295,   296,   386,
     262,     0,   277,   282,     0,   306,   307,   281,   291,     0,
       0,   279,   268,   388,     0,   390,     0,   269,    77,     0,
      79,    80,     0,    81,    82,    83,     0,     0,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,     0,    97,
      98,    99,     0,   101,   102,   103,     0,     0,   106,   107,
     108,   109,     0,     0,     0,     0,     0,   115,     0,   117,
     118,     0,   120,     0,   122,   123,   126,     0,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,     0,   141,   142,   143,     0,     0,   146,   147,   148,
     149,   380,   150,     0,   152,   378,     0,   154,   155,   156,
       0,   158,   159,   160,     0,     0,     0,     0,   165,   379,
     166,     0,     0,   170,   171,     0,     0,     0,     0,    71,
     175,    31,    68,    68,    68,   260,     0,     0,   260,   260,
       0,   385,   289,   297,   308,   316,     0,   387,   389,    32,
      10,   292,     6,   313,     0,    29,   311,   312,     0,     0,
      26,   331,   328,   330,   329,   263,   264,   178,   179,   180,
     181,   323,     0,     0,   335,   371,   334,   257,   385,   387,
     327,   389,   325,    34,     0,   410,   409,   411,     0,   406,
     399,     0,     0,   186,    50,     0,     0,   243,     0,   249,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   196,     0,     0,
       0,     0,     0,   196,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    76,    70,    51,     0,    23,    24,    25,
       0,    21,     0,    19,    16,    22,    28,     0,    69,   386,
      53,    54,   314,   315,   388,   390,    55,   256,    68,     5,
       0,    68,     0,     0,   305,    26,    68,     0,     0,   333,
       0,     0,    57,   337,   326,     0,     0,   405,     0,    49,
       0,   188,   189,   192,     0,   391,    68,    68,    68,     0,
     177,   176,     0,     0,   207,   198,   199,   200,   204,   205,
     206,   201,   202,     0,   203,     0,     0,     0,     0,     0,
       0,     0,   241,     0,   239,   242,     0,     0,    74,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     356,     0,     0,   194,   197,     0,     0,     0,     0,     0,
       0,     0,     0,   373,   374,   375,   376,   377,     0,     0,
       0,     0,   395,   397,     0,     0,     0,    72,    76,     0,
      20,    17,    56,     0,    28,     0,   293,    68,   298,     0,
       0,     0,     0,     0,     0,    68,    28,    27,    69,   324,
     332,   336,   372,     0,    67,     0,     0,    61,    58,    59,
     416,   414,   417,   415,     0,   412,   407,   400,   193,   187,
       0,    38,     0,   381,     0,   244,     0,   393,    69,   250,
       0,    78,   169,    84,     0,   231,   230,   229,   232,   227,
     228,     0,   344,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    85,    96,   100,     0,
     104,   105,   110,   111,   112,   113,   114,   116,   119,   121,
     356,   323,    57,   361,   356,   358,   357,    64,   353,   125,
     196,   124,   140,   144,   145,     0,   153,   157,   161,   162,
     164,   163,   167,   168,     0,   172,   173,   174,    73,     0,
      15,   364,   392,   290,   294,     7,   300,     0,   387,   299,
     302,     0,     0,   255,   303,    26,    28,   317,    66,    65,
     338,     0,     0,   408,   404,     0,   400,   190,   191,    39,
      37,     0,   389,   258,   248,   247,   344,   238,   323,    57,
     348,   344,   345,     0,   341,   220,   221,   233,   214,   215,
     218,   219,   209,   210,     0,   211,   212,   213,   217,   216,
     223,   222,   225,   226,   224,   234,     0,   240,    75,    63,
     356,   323,     0,   356,     0,   352,    57,   360,   195,     0,
     396,    28,    68,     0,   253,   301,    68,   309,    62,    60,
     413,     0,   403,   401,   366,   369,     0,   246,     0,   259,
       0,   344,   323,     0,   356,     0,   340,     0,    57,   347,
       0,   237,   351,   356,   362,   355,   359,     0,   151,    52,
      18,   365,    28,     0,     0,   368,     0,   245,   182,   236,
     339,   356,   349,   343,   346,   235,     0,   208,   354,   363,
     310,   402,   367,   370,     0,   342,   350,     0,     0,   394,
     183,     0,    68,    68,   252,   185,     0,   184,   251
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -564,  -564,  -564,   301,  -564,   -45,  -564,  -331,  -315,     0,
    -564,  -564,  -564,  -564,  -564,   184,  -564,  -564,  -564,    11,
    -483,  -564,  -564,  -263,  -246,  -207,    -2,  -564,  -564,  -284,
     353,   -68,  -564,  -564,  -564,  -564,   163,    13,   355,    93,
    -199,  -564,  -265,  -279,  -564,  -564,  -564,  -564,   -78,  -197,
    -564,   196,  -564,     5,   -70,  -564,   110,   162,    10,  -564,
      17,    19,  -564,  -564,   593,  -564,  -564,  -564,  -564,  -564,
     -28,  -564,    20,    16,  -564,  -564,    22,  -564,  -564,  -313,
    -506,   -52,     6,     3,  -236,  -564,  -564,  -564,  -539,  -564,
    -563,  -564,  -164,  -564,  -564,  -564,   -47,  -564,   426,  -564,
     356,     1,   -55,  -564,     7,  -564,   578,    68,  -564,  -564,
      67,  -564,   310,  -564,    75
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    43,     2,   339,   212,   570,   346,   236,   317,
      45,   482,    46,    47,    48,    49,   318,   220,    50,   319,
     466,   467,   468,   469,   543,    52,   328,   208,   399,   209,
     372,   544,   714,   720,   360,   361,   362,   258,   412,   413,
     392,   393,   394,   396,   366,   485,   489,   368,   725,   726,
     582,    55,   668,    91,   545,    56,    93,    57,   320,    59,
     321,   322,   338,   446,    62,    63,   341,   452,    64,   239,
      65,    66,   323,   324,   225,    69,   325,    71,    72,    73,
     347,    74,   241,    75,   255,   256,   612,   675,   613,   614,
     546,   644,   547,   548,   572,   695,   665,   666,   257,   428,
     210,   259,    77,    78,   261,   434,    81,   595,   596,    82,
      83,   268,   269,   474,   475
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      53,   226,    44,    76,   254,   213,   253,   327,   400,    79,
     353,   442,    58,    51,   455,    54,   348,   349,    68,    60,
     260,    61,    67,   401,    70,   224,   404,   599,   406,   437,
      11,   411,   408,   717,   459,   640,   418,  -404,   664,   245,
     449,   246,   104,   106,   278,   276,   718,   505,   245,   505,
     246,   245,   370,   246,   227,   371,    17,     9,   181,   642,
     228,    42,   185,   222,  -261,   724,   221,   670,   245,  -261,
     246,   462,   676,    27,   242,   242,   242,   247,   243,   244,
     242,    22,    11,   262,   696,   395,   247,    13,   395,   247,
     199,    11,   470,   279,   525,   407,    42,    42,   414,   444,
     483,    20,   671,   340,   414,   421,   247,   505,     3,   697,
     248,   460,    86,  -261,    23,    24,    25,    26,  -261,   248,
     -45,   526,   248,   264,    28,   313,   673,    31,   471,   573,
      27,   445,   700,   664,   450,   683,    42,    92,    97,   248,
      36,   587,    80,   101,   451,   529,   550,   314,   265,   550,
     351,   395,   465,   592,   568,   352,   254,    94,   253,   484,
     450,   450,   266,   687,    33,   249,   701,   267,    98,    35,
     486,   490,   530,   551,   249,   356,   557,   249,    99,    84,
     593,    85,   254,   254,   253,   253,   495,   496,   497,   498,
     499,   500,   501,   250,   249,   706,   102,   357,    87,   100,
      88,   107,   250,   363,   719,   250,   342,   571,   432,   433,
      53,    53,   373,    76,    76,   598,   351,   472,   214,    79,
      79,   646,   250,   104,   106,    54,    54,   520,   521,   522,
     503,   504,   505,   541,   231,   600,   351,   523,   524,   523,
     524,   678,   251,   410,   351,   251,    89,   419,    90,   542,
     422,   473,   429,   430,   656,   271,   610,   436,   252,   211,
     637,   252,   541,   242,   498,   -40,   354,    42,   247,   229,
     487,   657,    95,   351,    96,   443,   230,   254,   542,   253,
     423,   424,   425,   426,   427,   518,   519,   520,   521,   522,
     649,   232,   233,   409,   103,   234,    90,   523,   524,   235,
     410,   248,   647,   237,   615,   616,   238,   618,   619,   620,
     621,   622,   623,   624,   625,   626,   627,   628,   629,   630,
     631,   632,   633,   634,   105,   636,    90,   459,   659,  -261,
     247,   -41,  -261,   240,   -43,    11,   689,   -42,   329,   502,
      90,   654,   263,   330,   458,   331,   457,    76,   -44,   273,
     409,   414,   332,    79,   333,   272,   249,   540,   459,    54,
     610,   274,   334,   248,    90,   610,   488,   335,   679,    90,
     459,   275,   343,   254,   344,   253,   639,   710,   277,   245,
     645,   246,   280,  -254,   250,  -254,   281,   282,   459,   503,
     504,   505,   506,   507,   508,   509,   510,   511,   577,   283,
     284,   285,   286,   287,   288,   289,   226,   290,    42,   363,
     518,   519,   520,   521,   522,   712,   291,   691,   249,   292,
     293,   294,   523,   524,   608,   610,   295,   296,   358,   297,
     298,   299,   601,   300,   611,   351,   301,   302,   303,   304,
     609,   305,   606,   306,   307,    53,   250,    44,    76,   308,
     581,   578,   721,   458,    79,   457,    76,    58,    51,   227,
      54,   574,    79,    68,    60,   228,    61,    67,    54,    70,
     309,   221,   310,   311,   643,   312,   682,   336,   705,   685,
     -46,   707,   -47,   -48,   345,   364,   608,   350,   355,   367,
     410,   602,   359,   365,   369,   395,   397,   351,   398,   402,
     403,   405,   609,   415,   417,   435,   438,   254,   439,   253,
     703,   440,   441,  -261,   448,   450,   450,   453,   454,   708,
     461,   477,   480,   478,   479,   652,   481,   491,   492,   226,
     493,   494,   555,   669,   527,   569,   528,   715,   611,   410,
     409,   674,   531,   611,   532,   533,   534,   535,   536,    21,
     537,   594,   539,   549,   584,   552,   553,   410,   554,   564,
     576,   556,   512,   579,   513,   514,   515,   516,   517,   518,
     519,   520,   521,   522,   559,   560,   561,   562,   591,   563,
     565,   523,   524,   254,   566,   253,   567,   590,   538,   409,
     604,   603,   605,   617,   410,   635,   638,   650,   653,   651,
     655,    80,   667,   611,   693,   662,   677,   409,   374,   684,
       5,   375,   376,   377,   378,   379,   380,   686,   688,   694,
     698,   699,   702,   704,   711,   722,   410,   374,   709,   716,
     375,   376,   377,   378,   379,   380,   723,   728,   585,     7,
     447,     8,   597,   648,   409,   727,   223,     9,   583,   713,
      53,    11,   337,    76,   458,   416,   457,    76,   420,    79,
     270,   661,   431,    79,   663,    54,   476,   660,    14,    54,
       0,     0,     0,     0,   215,    16,   409,    17,   381,     0,
       0,    18,     0,     0,    19,     0,     0,   503,   504,   505,
       0,    20,   508,   509,     0,     0,     0,   381,     0,     0,
       0,     0,    22,     0,    23,    24,    25,    26,     0,     0,
       0,     0,   374,     0,    28,   375,   376,   377,   378,   379,
     380,     0,     0,     0,     0,     0,   382,     0,     0,     0,
       0,     0,     0,     0,   374,     0,     0,   375,   376,   377,
     378,   379,   380,     0,     0,   382,     0,     0,    31,     0,
       0,     0,     0,    32,    33,    34,     0,     0,   383,    35,
       0,    36,     0,     0,     0,   218,     0,     0,     0,     0,
       0,   384,     0,     0,   219,     0,    39,   383,     0,     0,
       0,     0,   381,     0,    40,    41,     0,     0,     0,     0,
     384,     0,     0,     0,     0,     0,     0,   385,     0,     0,
     386,   387,   388,     0,   381,   389,   390,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   385,     0,   391,   386,
     387,   463,     0,     0,   389,   390,     0,     0,     0,     0,
     382,     0,     0,   464,     0,     0,     0,   391,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   382,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   383,     0,     0,   516,   517,   518,   519,   520,
     521,   522,     0,     0,     0,   384,     0,     0,     0,   523,
     524,     0,     4,     5,   383,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   384,     0,     0,
       0,   385,     0,     0,   386,   387,   388,     0,     0,   389,
     390,     0,     7,     0,     8,     0,     0,     0,   588,     0,
       9,    10,   391,   385,    11,     0,   386,   387,   388,    12,
       0,   389,   390,     0,     0,     0,     0,     0,    13,     0,
       0,    14,     0,     0,   391,     0,     0,    15,    16,     0,
      17,     0,     0,     0,    18,     0,     0,    19,     0,     0,
       0,     0,     0,     0,    20,     0,     0,     0,     0,     0,
       0,    21,   316,     0,     0,    22,     0,    23,    24,    25,
      26,    27,     0,     0,     0,     0,     0,    28,     0,     0,
       0,     0,    29,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     4,     5,     0,     0,     0,     0,    30,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    31,     0,     0,     0,     0,    32,    33,    34,     0,
       0,     0,    35,     7,    36,     8,     0,     0,    37,     0,
       0,     9,    10,     0,     0,    11,     0,    38,     0,    39,
      12,     0,     0,     0,     0,     0,     0,    40,    41,    13,
       0,     0,    14,     0,     0,     0,     0,     0,    15,    16,
       0,    17,     0,     0,     0,    18,     0,     0,    19,     0,
       0,     0,     0,     0,     0,    20,    42,     0,     0,   326,
       0,     0,    21,   316,     0,     0,    22,     0,    23,    24,
      25,    26,    27,     0,     0,     0,     0,     0,    28,     0,
       0,     0,     0,    29,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     4,     5,     0,     0,     0,     0,
      30,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    31,     0,     0,     0,     0,    32,    33,    34,
       0,     0,     0,    35,     7,    36,     8,     0,     0,    37,
       0,     0,     9,    10,     0,     0,    11,     0,    38,     0,
      39,    12,     0,     0,     0,     0,     0,     0,    40,    41,
      13,     0,     0,    14,     0,     0,     0,     0,     0,    15,
      16,     0,    17,     0,     0,     0,    18,     0,     0,    19,
       0,     0,     0,     0,     0,     0,    20,    42,     0,     0,
     690,     0,     0,    21,     0,     0,     0,    22,     0,    23,
      24,    25,    26,    27,     0,     0,     0,     0,     0,    28,
       0,     0,     0,     0,    29,     0,     0,     0,     0,     0,
       0,     0,     0,     4,     5,     0,     0,     0,     0,     0,
       0,    30,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    31,     0,     0,     0,     0,    32,    33,
      34,     0,     0,     7,    35,     8,    36,     0,     0,     0,
      37,     9,     0,     0,     0,    11,     0,     0,     0,    38,
      12,    39,   503,   504,   505,   506,   507,   508,   509,    40,
      41,     0,    14,     0,     0,     0,     0,     0,    15,    16,
       0,    17,     0,     0,     0,    18,     0,     0,    19,     0,
       0,     0,     0,     0,     0,    20,     0,     0,    42,     0,
       0,   575,    21,     0,     0,     0,    22,     0,    23,    24,
      25,    26,     0,     0,     0,     0,     0,     0,    28,     0,
       0,     0,   503,   504,   505,   506,   507,   508,   509,     0,
       0,     0,     4,     5,     0,     0,     0,     0,     0,     0,
      30,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    31,     0,     0,     0,     0,    32,    33,    34,
       0,     0,     7,    35,     8,    36,     0,     0,     0,    37,
       9,     0,     0,     0,    11,     0,     0,     0,    38,    12,
      39,   503,   504,   505,   506,   507,   508,   509,    40,    41,
       0,    14,     0,     0,     0,     0,     0,    15,    16,     0,
      17,     0,     0,     0,    18,     0,     0,    19,     0,     0,
       0,     0,     0,     0,    20,     0,     0,    42,     0,     0,
     456,    21,     0,     0,     0,    22,     0,    23,    24,    25,
      26,     0,     0,     0,     0,     0,     0,    28,   514,   515,
     516,   517,   518,   519,   520,   521,   522,     0,     0,     0,
       0,     4,     5,     0,   523,   524,     0,     0,     0,    30,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    31,     0,     0,     0,     0,    32,    33,    34,     0,
       0,     7,    35,     8,    36,     0,     0,     0,    37,     9,
       0,     0,     0,    11,     0,     0,     0,    38,    12,    39,
     516,   517,   518,   519,   520,   521,   522,    40,    41,     0,
      14,     0,     0,     0,   523,   524,    15,    16,     0,    17,
       0,     0,     0,    18,     0,     0,    19,     0,     0,     0,
       0,     0,     0,    20,     0,     0,    42,     0,     0,   586,
      21,     0,     0,     0,    22,     0,    23,    24,    25,    26,
       5,     0,     0,     0,     0,     0,    28,     0,   515,   516,
     517,   518,   519,   520,   521,   522,     0,     0,     0,     0,
       0,     0,     0,   523,   524,     0,     0,     0,    30,     7,
       0,     8,     0,     0,     0,     0,     0,     9,     0,     0,
      31,    11,     0,     0,     0,    32,    33,    34,     0,     0,
       0,    35,     0,    36,     0,     0,     0,    37,    14,     0,
       0,     0,     0,     0,   215,    16,    38,    17,    39,     0,
       0,    18,     0,     0,    19,     0,    40,    41,     0,     0,
       0,    20,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    22,     0,    23,    24,    25,    26,     0,     0,
       0,     0,     0,     0,    28,    42,     0,     0,   692,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       4,     5,     0,     0,     0,     0,     0,     0,     0,     0,
       6,     0,     0,     0,     0,     0,     0,     0,    31,     0,
       0,     0,     0,    32,    33,    34,     0,     0,     0,    35,
       7,    36,     8,     0,     0,   218,     0,     0,     9,    10,
       0,     0,    11,     0,   219,     0,    39,    12,     0,     0,
       0,     0,     0,     0,    40,    41,    13,     0,     0,    14,
       0,     0,     0,     0,     0,    15,    16,     0,    17,     0,
       0,     0,    18,     0,     0,    19,     0,     0,     0,     0,
       0,     0,    20,    42,     0,     0,   580,     0,     0,    21,
       0,     0,     0,    22,     0,    23,    24,    25,    26,    27,
       0,     0,     0,     0,     0,    28,     0,     0,     0,     0,
      29,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    30,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    31,
       4,     5,     0,     0,    32,    33,    34,     0,     0,   315,
      35,     0,    36,     0,     0,     0,    37,     0,     0,     0,
       0,     0,     0,   -68,     0,    38,     0,    39,     0,     0,
       7,     0,     8,     0,     0,    40,    41,     0,     9,    10,
       0,     0,    11,     0,     0,     0,     0,    12,     0,     0,
       0,     0,     0,     0,     0,     0,    13,     0,     0,    14,
       0,     0,     0,     0,    42,    15,    16,     0,    17,     0,
       0,     0,    18,     0,     0,    19,     0,     0,     0,     0,
       0,     0,    20,     0,     0,     0,     0,     0,     0,    21,
     316,     0,     0,    22,     0,    23,    24,    25,    26,    27,
       0,     0,     0,     0,     0,    28,     0,     0,     0,     0,
      29,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     5,     0,     0,     0,     0,    30,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    31,
       0,     0,     0,     0,    32,    33,    34,     0,     0,     0,
      35,     7,    36,     8,     0,     0,    37,     0,   247,     9,
       0,     0,     0,    11,     0,    38,     0,    39,     0,     0,
       0,     0,     0,     0,     0,    40,    41,     0,     0,     0,
      14,     0,     0,     0,     0,     0,   215,    16,     0,    17,
       0,   248,     0,    18,     0,     0,    19,     0,     0,     0,
       0,     0,     0,    20,    42,     0,     0,     0,     0,     0,
       0,     5,     0,     0,    22,     0,    23,    24,    25,    26,
       0,     0,     0,     0,     0,     0,    28,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       7,     0,     8,     0,     0,     0,   249,   247,     9,     0,
       0,     0,    11,     0,     0,     0,     0,     0,     0,     0,
      31,     0,     0,     0,     0,    32,    33,    34,     0,    14,
       0,    35,     0,    36,   250,   215,    16,   218,    17,     0,
     248,     0,    18,     0,     0,    19,   219,     0,    39,     0,
       0,     0,    20,     0,     0,     0,    40,    41,     0,     0,
       5,     0,     0,    22,     0,    23,    24,    25,    26,     0,
       0,     0,     0,     0,   641,    28,     0,     0,     0,   658,
       0,     0,     0,     0,     0,    42,     0,     0,     0,     7,
       0,     8,     0,     0,     0,   249,     0,     9,     0,     0,
       0,    11,     0,     0,     0,     0,     0,     0,     0,    31,
       0,     0,     0,     0,    32,    33,    34,     0,    14,     0,
      35,     0,    36,   250,   215,    16,   218,    17,     0,     0,
       0,    18,     0,     0,    19,   219,     0,    39,     0,     0,
       0,    20,     0,     0,     0,    40,    41,     0,     0,     5,
       0,     0,    22,     0,    23,    24,    25,    26,     0,     0,
       0,     0,     0,   672,    28,   503,   504,   505,   506,   507,
     508,   509,   510,   511,    42,     0,     0,     0,     7,     0,
       8,     0,     0,     0,     0,     0,     9,     0,     0,     0,
      11,     0,     0,     0,     0,     0,     0,     0,    31,     0,
       0,     0,     0,    32,    33,    34,     0,    14,     0,    35,
       0,    36,     0,   215,    16,   218,    17,     0,     0,     0,
      18,     0,     0,    19,   219,     0,    39,     0,     0,     0,
      20,     0,     0,     0,    40,    41,     0,     0,     0,     0,
       0,    22,     0,    23,    24,    25,    26,     0,     0,     0,
       0,     0,     0,    28,   503,   504,   505,   506,   507,   508,
     509,   510,   511,    42,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     503,   504,   505,   506,   507,   508,   509,    31,   511,     0,
       0,     0,    32,    33,    34,     0,     0,     0,    35,     0,
      36,     0,     0,     0,   218,     0,     0,     0,     0,     0,
       0,     0,     0,   219,     0,    39,     0,     0,     0,     0,
       0,     0,     0,    40,    41,   503,   504,   505,   506,   507,
     508,   509,   510,   511,     0,     0,     0,     0,   512,     0,
     513,   514,   515,   516,   517,   518,   519,   520,   521,   522,
       0,     0,    42,     0,     0,     0,     0,   523,   524,     0,
       0,     0,     0,     0,   558,   503,   504,   505,   506,   507,
     508,   509,   510,   511,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   503,   504,   505,
     506,   507,   508,   509,   510,   511,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   503,   504,
     505,   506,   507,   508,   509,   510,   511,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   512,     0,   513,
     514,   515,   516,   517,   518,   519,   520,   521,   522,   503,
     504,   505,   506,   507,   508,   509,   523,   524,     0,     0,
       0,     0,     0,   607,     0,   513,   514,   515,   516,   517,
     518,   519,   520,   521,   522,     0,     0,     0,     0,     0,
       0,     0,   523,   524,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   512,     0,
     513,   514,   515,   516,   517,   518,   519,   520,   521,   522,
       0,     0,     0,     0,     0,     0,     0,   523,   524,   589,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   512,     0,
     513,   514,   515,   516,   517,   518,   519,   520,   521,   522,
       0,     0,     0,     0,     0,     0,     0,   523,   524,   681,
     512,   680,   513,   514,   515,   516,   517,   518,   519,   520,
     521,   522,     0,     0,     0,     0,     0,     0,     0,   523,
     524,   512,     0,   513,   514,   515,   516,   517,   518,   519,
     520,   521,   522,     0,     0,     0,     5,     0,     0,     0,
     523,   524,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   513,   514,   515,   516,   517,   518,
     519,   520,   521,   522,     0,     7,     0,     8,     0,     0,
       0,   523,   524,     9,    10,     0,     0,    11,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    13,     0,     0,    14,     0,     0,     0,     0,     0,
     215,    16,     0,    17,     0,     0,     0,    18,     0,     0,
      19,     0,     0,     0,     0,     0,     0,    20,     0,     0,
       0,     0,     0,     0,     0,     5,     0,     0,    22,     0,
      23,    24,    25,    26,    27,     0,     0,   216,     0,     0,
      28,     0,     0,     0,   217,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     7,     0,     8,     0,     0,     0,
       0,     0,     9,     0,     0,     0,    11,     0,     0,     0,
       0,     0,     0,     0,    31,     0,     0,     0,     0,    32,
      33,    34,     0,    14,     0,    35,     0,    36,     0,   215,
      16,   218,    17,     0,     0,     0,    18,     0,     0,    19,
     219,     5,    39,     0,     0,     0,    20,     0,     0,     0,
      40,    41,     0,     0,     0,     0,     0,    22,     0,    23,
      24,    25,    26,     0,     0,     0,     0,     0,     0,    28,
       7,     0,     8,     0,     0,     0,     0,     0,     9,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    14,
       0,     0,     0,    31,     0,   215,    16,     0,    32,    33,
      34,     0,    18,     0,    35,    19,    36,     0,     0,     0,
     218,     0,    20,     0,     0,     0,     0,     0,     0,   219,
       0,    39,     0,     0,     0,    23,    24,    25,    26,    40,
      41,     0,     0,     0,     0,    28,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    32,    33,    34,     0,     0,     0,
      35,     0,     0,     0,     0,     0,   218,     0,     0,     0,
       0,     0,     0,     0,     0,   219,     0,    39,     0,     0,
       0,     0,     0,     0,   108,    40,    41,   109,   110,   111,
     112,   113,   114,     0,   115,     0,     0,   116,     0,   117,
       0,     0,     0,   118,   119,     0,   120,   121,   122,   123,
       0,   124,   125,   126,   127,   128,   129,   130,   131,     0,
     132,     0,     0,   133,   134,   135,   136,   137,     0,     0,
     138,     0,     0,     0,   139,     0,   140,   141,     0,   142,
     143,   144,   145,   146,   147,     0,   148,   149,   150,   151,
     152,   153,     0,     0,   154,     0,     0,   155,     0,     0,
       0,     0,     0,   156,   157,     0,   158,   159,     0,   160,
     161,     0,     0,     0,   162,   163,   164,   165,   166,   167,
       0,   168,   169,   170,   171,   172,   173,   174,     0,   175,
       0,   176,     0,   177,   178,   179,   180,   181,   182,   183,
     184,   185,     0,   186,   187,   188,   189,     0,     0,     0,
       0,   190,     0,     0,   191,     0,     0,   192,   193,     0,
       0,   194,   195,   196,   197,     0,     0,   198,     0,   199,
       0,   200,   201,   202,   203,   204,   205,   206,     0,     0,
     207
};

static const yytype_int16 yycheck[] =
{
       2,    53,     2,     2,    74,    50,    74,   214,   287,     2,
     256,   326,     2,     2,   345,     2,   252,   253,     2,     2,
      75,     2,     2,   288,     2,    53,   291,     6,   293,   313,
      46,   296,   295,    40,   347,   541,   301,   103,   601,     3,
     112,     5,    37,    38,   161,   100,    53,    17,     3,    17,
       5,     3,     9,     5,    53,    12,    72,    42,   138,   542,
      53,   208,   142,    53,   161,   212,    53,   606,     3,   161,
       5,   350,   611,   103,    71,    72,    73,    41,    72,    73,
      77,    97,    46,    77,   187,   284,    41,    60,   287,    41,
     170,    46,    26,   210,   187,   294,   208,   208,   297,   211,
     211,    86,   608,   133,   303,   304,    41,    17,     0,   212,
      74,   347,   213,   210,    99,   100,   101,   102,   210,    74,
     212,   214,    74,    27,   109,   187,   609,   143,    62,   444,
     103,   338,   671,   696,   341,   641,   208,    99,    28,    74,
     156,   456,   208,    33,   341,   187,   187,   209,    52,   187,
     208,   350,   351,   187,   438,   213,   226,     9,   226,   366,
     367,   368,    66,   646,   149,   129,   672,    71,     3,   154,
     367,   368,   214,   214,   129,   187,   214,   129,   213,     3,
     214,     5,   252,   253,   252,   253,   385,   386,   387,   388,
     389,   390,   391,   157,   129,   678,    34,   209,     3,   213,
       5,    39,   157,   273,   211,   157,   234,   443,     6,     7,
     212,   213,   282,   212,   213,   480,   208,   151,   167,   212,
     213,   213,   157,   218,   219,   212,   213,   197,   198,   199,
      15,    16,    17,   197,   210,   214,   208,   207,   208,   207,
     208,   213,   197,   295,   208,   197,     3,   302,     5,   213,
     305,   185,   307,   308,   585,   103,   502,   312,   213,   212,
     525,   213,   197,   260,   463,   212,   260,   208,    41,   212,
     211,   586,     3,   208,     5,   327,   212,   347,   213,   347,
     182,   183,   184,   185,   186,   195,   196,   197,   198,   199,
     555,   210,   212,   295,     3,   210,     5,   207,   208,   212,
     352,    74,   548,   212,   503,   504,   189,   506,   507,   508,
     509,   510,   511,   512,   513,   514,   515,   516,   517,   518,
     519,   520,   521,   522,     3,   524,     5,   640,   591,   210,
      41,   212,   210,   210,   212,    46,   651,   212,     3,   391,
       5,   577,   212,     3,   346,     5,   346,   346,   212,   210,
     352,   550,     3,   346,     5,     9,   129,   409,   671,   346,
     606,   212,     3,    74,     5,   611,   368,     3,   614,     5,
     683,     3,     3,   443,     5,   443,   540,   692,   210,     3,
     544,     5,   213,   212,   157,   214,   213,   213,   701,    15,
      16,    17,    18,    19,    20,    21,    22,    23,   450,   213,
     213,   213,   213,   213,   213,   213,   458,   213,   208,   479,
     195,   196,   197,   198,   199,   694,   213,   653,   129,   213,
     213,   213,   207,   208,   197,   671,   213,   213,     5,   213,
     213,   213,   484,   213,   502,   208,   213,   213,   213,   213,
     213,   213,   494,   213,   213,   447,   157,   447,   447,   213,
     452,   450,   717,   455,   447,   455,   455,   447,   447,   458,
     447,   445,   455,   447,   447,   458,   447,   447,   455,   447,
     213,   458,   213,   213,   542,   213,   640,   210,   677,   643,
     212,   680,   212,   212,   210,   189,   197,   215,   213,   213,
     542,   484,   214,   214,     9,   694,     9,   208,     9,     9,
       9,     9,   213,     9,     9,     9,   208,   577,   213,   577,
     674,   212,   210,   210,   189,   722,   723,   212,   210,   683,
     214,   210,   215,   211,   187,   570,     6,   214,   214,   581,
     214,   213,   187,   603,   214,     9,   214,   701,   606,   591,
     542,   609,   214,   611,   214,   214,   214,   214,   214,    93,
     214,   167,   214,   214,   211,   214,   214,   609,   214,   207,
     189,   214,   188,   212,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   214,   214,   214,   214,   187,   214,
     214,   207,   208,   653,   214,   653,   214,   214,   214,   591,
     212,   214,   212,     3,   646,     3,     9,     6,   187,   214,
     212,   208,   212,   671,     5,   211,   214,   609,     3,   214,
       5,     6,     7,     8,     9,    10,    11,   214,   214,   189,
     210,   214,   214,   214,   212,   189,   678,     3,   214,   214,
       6,     7,     8,     9,    10,    11,   189,   212,   454,    34,
     339,    36,   479,   550,   646,   723,    53,    42,   452,   696,
     652,    46,   226,   652,   656,   299,   656,   656,   303,   652,
      82,   594,   309,   656,   596,   652,   356,   592,    63,   656,
      -1,    -1,    -1,    -1,    69,    70,   678,    72,    73,    -1,
      -1,    76,    -1,    -1,    79,    -1,    -1,    15,    16,    17,
      -1,    86,    20,    21,    -1,    -1,    -1,    73,    -1,    -1,
      -1,    -1,    97,    -1,    99,   100,   101,   102,    -1,    -1,
      -1,    -1,     3,    -1,   109,     6,     7,     8,     9,    10,
      11,    -1,    -1,    -1,    -1,    -1,   121,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     3,    -1,    -1,     6,     7,     8,
       9,    10,    11,    -1,    -1,   121,    -1,    -1,   143,    -1,
      -1,    -1,    -1,   148,   149,   150,    -1,    -1,   153,   154,
      -1,   156,    -1,    -1,    -1,   160,    -1,    -1,    -1,    -1,
      -1,   166,    -1,    -1,   169,    -1,   171,   153,    -1,    -1,
      -1,    -1,    73,    -1,   179,   180,    -1,    -1,    -1,    -1,
     166,    -1,    -1,    -1,    -1,    -1,    -1,   192,    -1,    -1,
     195,   196,   197,    -1,    73,   200,   201,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   192,    -1,   213,   195,
     196,   197,    -1,    -1,   200,   201,    -1,    -1,    -1,    -1,
     121,    -1,    -1,   209,    -1,    -1,    -1,   213,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   121,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   153,    -1,    -1,   193,   194,   195,   196,   197,
     198,   199,    -1,    -1,    -1,   166,    -1,    -1,    -1,   207,
     208,    -1,     4,     5,   153,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   166,    -1,    -1,
      -1,   192,    -1,    -1,   195,   196,   197,    -1,    -1,   200,
     201,    -1,    34,    -1,    36,    -1,    -1,    -1,   209,    -1,
      42,    43,   213,   192,    46,    -1,   195,   196,   197,    51,
      -1,   200,   201,    -1,    -1,    -1,    -1,    -1,    60,    -1,
      -1,    63,    -1,    -1,   213,    -1,    -1,    69,    70,    -1,
      72,    -1,    -1,    -1,    76,    -1,    -1,    79,    -1,    -1,
      -1,    -1,    -1,    -1,    86,    -1,    -1,    -1,    -1,    -1,
      -1,    93,    94,    -1,    -1,    97,    -1,    99,   100,   101,
     102,   103,    -1,    -1,    -1,    -1,    -1,   109,    -1,    -1,
      -1,    -1,   114,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     4,     5,    -1,    -1,    -1,    -1,   131,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   143,    -1,    -1,    -1,    -1,   148,   149,   150,    -1,
      -1,    -1,   154,    34,   156,    36,    -1,    -1,   160,    -1,
      -1,    42,    43,    -1,    -1,    46,    -1,   169,    -1,   171,
      51,    -1,    -1,    -1,    -1,    -1,    -1,   179,   180,    60,
      -1,    -1,    63,    -1,    -1,    -1,    -1,    -1,    69,    70,
      -1,    72,    -1,    -1,    -1,    76,    -1,    -1,    79,    -1,
      -1,    -1,    -1,    -1,    -1,    86,   208,    -1,    -1,   211,
      -1,    -1,    93,    94,    -1,    -1,    97,    -1,    99,   100,
     101,   102,   103,    -1,    -1,    -1,    -1,    -1,   109,    -1,
      -1,    -1,    -1,   114,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     4,     5,    -1,    -1,    -1,    -1,
     131,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   143,    -1,    -1,    -1,    -1,   148,   149,   150,
      -1,    -1,    -1,   154,    34,   156,    36,    -1,    -1,   160,
      -1,    -1,    42,    43,    -1,    -1,    46,    -1,   169,    -1,
     171,    51,    -1,    -1,    -1,    -1,    -1,    -1,   179,   180,
      60,    -1,    -1,    63,    -1,    -1,    -1,    -1,    -1,    69,
      70,    -1,    72,    -1,    -1,    -1,    76,    -1,    -1,    79,
      -1,    -1,    -1,    -1,    -1,    -1,    86,   208,    -1,    -1,
     211,    -1,    -1,    93,    -1,    -1,    -1,    97,    -1,    99,
     100,   101,   102,   103,    -1,    -1,    -1,    -1,    -1,   109,
      -1,    -1,    -1,    -1,   114,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     4,     5,    -1,    -1,    -1,    -1,    -1,
      -1,   131,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,   148,   149,
     150,    -1,    -1,    34,   154,    36,   156,    -1,    -1,    -1,
     160,    42,    -1,    -1,    -1,    46,    -1,    -1,    -1,   169,
      51,   171,    15,    16,    17,    18,    19,    20,    21,   179,
     180,    -1,    63,    -1,    -1,    -1,    -1,    -1,    69,    70,
      -1,    72,    -1,    -1,    -1,    76,    -1,    -1,    79,    -1,
      -1,    -1,    -1,    -1,    -1,    86,    -1,    -1,   208,    -1,
      -1,   211,    93,    -1,    -1,    -1,    97,    -1,    99,   100,
     101,   102,    -1,    -1,    -1,    -1,    -1,    -1,   109,    -1,
      -1,    -1,    15,    16,    17,    18,    19,    20,    21,    -1,
      -1,    -1,     4,     5,    -1,    -1,    -1,    -1,    -1,    -1,
     131,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   143,    -1,    -1,    -1,    -1,   148,   149,   150,
      -1,    -1,    34,   154,    36,   156,    -1,    -1,    -1,   160,
      42,    -1,    -1,    -1,    46,    -1,    -1,    -1,   169,    51,
     171,    15,    16,    17,    18,    19,    20,    21,   179,   180,
      -1,    63,    -1,    -1,    -1,    -1,    -1,    69,    70,    -1,
      72,    -1,    -1,    -1,    76,    -1,    -1,    79,    -1,    -1,
      -1,    -1,    -1,    -1,    86,    -1,    -1,   208,    -1,    -1,
     211,    93,    -1,    -1,    -1,    97,    -1,    99,   100,   101,
     102,    -1,    -1,    -1,    -1,    -1,    -1,   109,   191,   192,
     193,   194,   195,   196,   197,   198,   199,    -1,    -1,    -1,
      -1,     4,     5,    -1,   207,   208,    -1,    -1,    -1,   131,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   143,    -1,    -1,    -1,    -1,   148,   149,   150,    -1,
      -1,    34,   154,    36,   156,    -1,    -1,    -1,   160,    42,
      -1,    -1,    -1,    46,    -1,    -1,    -1,   169,    51,   171,
     193,   194,   195,   196,   197,   198,   199,   179,   180,    -1,
      63,    -1,    -1,    -1,   207,   208,    69,    70,    -1,    72,
      -1,    -1,    -1,    76,    -1,    -1,    79,    -1,    -1,    -1,
      -1,    -1,    -1,    86,    -1,    -1,   208,    -1,    -1,   211,
      93,    -1,    -1,    -1,    97,    -1,    99,   100,   101,   102,
       5,    -1,    -1,    -1,    -1,    -1,   109,    -1,   192,   193,
     194,   195,   196,   197,   198,   199,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   207,   208,    -1,    -1,    -1,   131,    34,
      -1,    36,    -1,    -1,    -1,    -1,    -1,    42,    -1,    -1,
     143,    46,    -1,    -1,    -1,   148,   149,   150,    -1,    -1,
      -1,   154,    -1,   156,    -1,    -1,    -1,   160,    63,    -1,
      -1,    -1,    -1,    -1,    69,    70,   169,    72,   171,    -1,
      -1,    76,    -1,    -1,    79,    -1,   179,   180,    -1,    -1,
      -1,    86,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    97,    -1,    99,   100,   101,   102,    -1,    -1,
      -1,    -1,    -1,    -1,   109,   208,    -1,    -1,   211,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       4,     5,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,
      -1,    -1,    -1,   148,   149,   150,    -1,    -1,    -1,   154,
      34,   156,    36,    -1,    -1,   160,    -1,    -1,    42,    43,
      -1,    -1,    46,    -1,   169,    -1,   171,    51,    -1,    -1,
      -1,    -1,    -1,    -1,   179,   180,    60,    -1,    -1,    63,
      -1,    -1,    -1,    -1,    -1,    69,    70,    -1,    72,    -1,
      -1,    -1,    76,    -1,    -1,    79,    -1,    -1,    -1,    -1,
      -1,    -1,    86,   208,    -1,    -1,   211,    -1,    -1,    93,
      -1,    -1,    -1,    97,    -1,    99,   100,   101,   102,   103,
      -1,    -1,    -1,    -1,    -1,   109,    -1,    -1,    -1,    -1,
     114,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   131,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,
       4,     5,    -1,    -1,   148,   149,   150,    -1,    -1,    13,
     154,    -1,   156,    -1,    -1,    -1,   160,    -1,    -1,    -1,
      -1,    -1,    -1,   167,    -1,   169,    -1,   171,    -1,    -1,
      34,    -1,    36,    -1,    -1,   179,   180,    -1,    42,    43,
      -1,    -1,    46,    -1,    -1,    -1,    -1,    51,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    60,    -1,    -1,    63,
      -1,    -1,    -1,    -1,   208,    69,    70,    -1,    72,    -1,
      -1,    -1,    76,    -1,    -1,    79,    -1,    -1,    -1,    -1,
      -1,    -1,    86,    -1,    -1,    -1,    -1,    -1,    -1,    93,
      94,    -1,    -1,    97,    -1,    99,   100,   101,   102,   103,
      -1,    -1,    -1,    -1,    -1,   109,    -1,    -1,    -1,    -1,
     114,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     5,    -1,    -1,    -1,    -1,   131,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,
      -1,    -1,    -1,    -1,   148,   149,   150,    -1,    -1,    -1,
     154,    34,   156,    36,    -1,    -1,   160,    -1,    41,    42,
      -1,    -1,    -1,    46,    -1,   169,    -1,   171,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   179,   180,    -1,    -1,    -1,
      63,    -1,    -1,    -1,    -1,    -1,    69,    70,    -1,    72,
      -1,    74,    -1,    76,    -1,    -1,    79,    -1,    -1,    -1,
      -1,    -1,    -1,    86,   208,    -1,    -1,    -1,    -1,    -1,
      -1,     5,    -1,    -1,    97,    -1,    99,   100,   101,   102,
      -1,    -1,    -1,    -1,    -1,    -1,   109,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      34,    -1,    36,    -1,    -1,    -1,   129,    41,    42,    -1,
      -1,    -1,    46,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     143,    -1,    -1,    -1,    -1,   148,   149,   150,    -1,    63,
      -1,   154,    -1,   156,   157,    69,    70,   160,    72,    -1,
      74,    -1,    76,    -1,    -1,    79,   169,    -1,   171,    -1,
      -1,    -1,    86,    -1,    -1,    -1,   179,   180,    -1,    -1,
       5,    -1,    -1,    97,    -1,    99,   100,   101,   102,    -1,
      -1,    -1,    -1,    -1,   197,   109,    -1,    -1,    -1,    24,
      -1,    -1,    -1,    -1,    -1,   208,    -1,    -1,    -1,    34,
      -1,    36,    -1,    -1,    -1,   129,    -1,    42,    -1,    -1,
      -1,    46,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,
      -1,    -1,    -1,    -1,   148,   149,   150,    -1,    63,    -1,
     154,    -1,   156,   157,    69,    70,   160,    72,    -1,    -1,
      -1,    76,    -1,    -1,    79,   169,    -1,   171,    -1,    -1,
      -1,    86,    -1,    -1,    -1,   179,   180,    -1,    -1,     5,
      -1,    -1,    97,    -1,    99,   100,   101,   102,    -1,    -1,
      -1,    -1,    -1,   197,   109,    15,    16,    17,    18,    19,
      20,    21,    22,    23,   208,    -1,    -1,    -1,    34,    -1,
      36,    -1,    -1,    -1,    -1,    -1,    42,    -1,    -1,    -1,
      46,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,
      -1,    -1,    -1,   148,   149,   150,    -1,    63,    -1,   154,
      -1,   156,    -1,    69,    70,   160,    72,    -1,    -1,    -1,
      76,    -1,    -1,    79,   169,    -1,   171,    -1,    -1,    -1,
      86,    -1,    -1,    -1,   179,   180,    -1,    -1,    -1,    -1,
      -1,    97,    -1,    99,   100,   101,   102,    -1,    -1,    -1,
      -1,    -1,    -1,   109,    15,    16,    17,    18,    19,    20,
      21,    22,    23,   208,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      15,    16,    17,    18,    19,    20,    21,   143,    23,    -1,
      -1,    -1,   148,   149,   150,    -1,    -1,    -1,   154,    -1,
     156,    -1,    -1,    -1,   160,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   169,    -1,   171,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   179,   180,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    -1,    -1,    -1,    -1,   188,    -1,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
      -1,    -1,   208,    -1,    -1,    -1,    -1,   207,   208,    -1,
      -1,    -1,    -1,    -1,   214,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   188,    -1,   190,
     191,   192,   193,   194,   195,   196,   197,   198,   199,    15,
      16,    17,    18,    19,    20,    21,   207,   208,    -1,    -1,
      -1,    -1,    -1,   214,    -1,   190,   191,   192,   193,   194,
     195,   196,   197,   198,   199,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   207,   208,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   188,    -1,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   207,   208,   209,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   188,    -1,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   207,   208,   209,
     188,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   207,
     208,   188,    -1,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,    -1,    -1,    -1,     5,    -1,    -1,    -1,
     207,   208,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,    -1,    34,    -1,    36,    -1,    -1,
      -1,   207,   208,    42,    43,    -1,    -1,    46,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    60,    -1,    -1,    63,    -1,    -1,    -1,    -1,    -1,
      69,    70,    -1,    72,    -1,    -1,    -1,    76,    -1,    -1,
      79,    -1,    -1,    -1,    -1,    -1,    -1,    86,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     5,    -1,    -1,    97,    -1,
      99,   100,   101,   102,   103,    -1,    -1,   106,    -1,    -1,
     109,    -1,    -1,    -1,   113,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    34,    -1,    36,    -1,    -1,    -1,
      -1,    -1,    42,    -1,    -1,    -1,    46,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,   148,
     149,   150,    -1,    63,    -1,   154,    -1,   156,    -1,    69,
      70,   160,    72,    -1,    -1,    -1,    76,    -1,    -1,    79,
     169,     5,   171,    -1,    -1,    -1,    86,    -1,    -1,    -1,
     179,   180,    -1,    -1,    -1,    -1,    -1,    97,    -1,    99,
     100,   101,   102,    -1,    -1,    -1,    -1,    -1,    -1,   109,
      34,    -1,    36,    -1,    -1,    -1,    -1,    -1,    42,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    63,
      -1,    -1,    -1,   143,    -1,    69,    70,    -1,   148,   149,
     150,    -1,    76,    -1,   154,    79,   156,    -1,    -1,    -1,
     160,    -1,    86,    -1,    -1,    -1,    -1,    -1,    -1,   169,
      -1,   171,    -1,    -1,    -1,    99,   100,   101,   102,   179,
     180,    -1,    -1,    -1,    -1,   109,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   148,   149,   150,    -1,    -1,    -1,
     154,    -1,    -1,    -1,    -1,    -1,   160,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   169,    -1,   171,    -1,    -1,
      -1,    -1,    -1,    -1,    25,   179,   180,    28,    29,    30,
      31,    32,    33,    -1,    35,    -1,    -1,    38,    -1,    40,
      -1,    -1,    -1,    44,    45,    -1,    47,    48,    49,    50,
      -1,    52,    53,    54,    55,    56,    57,    58,    59,    -1,
      61,    -1,    -1,    64,    65,    66,    67,    68,    -1,    -1,
      71,    -1,    -1,    -1,    75,    -1,    77,    78,    -1,    80,
      81,    82,    83,    84,    85,    -1,    87,    88,    89,    90,
      91,    92,    -1,    -1,    95,    -1,    -1,    98,    -1,    -1,
      -1,    -1,    -1,   104,   105,    -1,   107,   108,    -1,   110,
     111,    -1,    -1,    -1,   115,   116,   117,   118,   119,   120,
      -1,   122,   123,   124,   125,   126,   127,   128,    -1,   130,
      -1,   132,    -1,   134,   135,   136,   137,   138,   139,   140,
     141,   142,    -1,   144,   145,   146,   147,    -1,    -1,    -1,
      -1,   152,    -1,    -1,   155,    -1,    -1,   158,   159,    -1,
      -1,   162,   163,   164,   165,    -1,    -1,   168,    -1,   170,
      -1,   172,   173,   174,   175,   176,   177,   178,    -1,    -1,
     181
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   217,   219,     0,     4,     5,    14,    34,    36,    42,
      43,    46,    51,    60,    63,    69,    70,    72,    76,    79,
      86,    93,    97,    99,   100,   101,   102,   103,   109,   114,
     131,   143,   148,   149,   150,   154,   156,   160,   169,   171,
     179,   180,   208,   218,   225,   226,   228,   229,   230,   231,
     234,   235,   241,   242,   253,   267,   271,   273,   274,   275,
     276,   277,   280,   281,   284,   286,   287,   288,   289,   291,
     292,   293,   294,   295,   297,   299,   317,   318,   319,   320,
     208,   322,   325,   326,     3,     5,   213,     3,     5,     3,
       5,   269,    99,   272,     9,     3,     5,   272,     3,   213,
     213,   272,   273,     3,   269,     3,   269,   273,    25,    28,
      29,    30,    31,    32,    33,    35,    38,    40,    44,    45,
      47,    48,    49,    50,    52,    53,    54,    55,    56,    57,
      58,    59,    61,    64,    65,    66,    67,    68,    71,    75,
      77,    78,    80,    81,    82,    83,    84,    85,    87,    88,
      89,    90,    91,    92,    95,    98,   104,   105,   107,   108,
     110,   111,   115,   116,   117,   118,   119,   120,   122,   123,
     124,   125,   126,   127,   128,   130,   132,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   144,   145,   146,   147,
     152,   155,   158,   159,   162,   163,   164,   165,   168,   170,
     172,   173,   174,   175,   176,   177,   178,   181,   243,   245,
     316,   212,   221,   221,   167,    69,   106,   113,   160,   169,
     233,   253,   274,   280,   286,   290,   297,   317,   320,   212,
     212,   210,   210,   212,   210,   212,   224,   212,   189,   285,
     210,   298,   299,   298,   298,     3,     5,    41,    74,   129,
     157,   197,   213,   247,   270,   300,   301,   314,   253,   317,
     318,   320,   298,   212,    27,    52,    66,    71,   327,   328,
     322,   103,     9,   210,   212,     3,   318,   210,   161,   210,
     213,   213,   213,   213,   213,   213,   213,   213,   213,   213,
     213,   213,   213,   213,   213,   213,   213,   213,   213,   213,
     213,   213,   213,   213,   213,   213,   213,   213,   213,   213,
     213,   213,   213,   187,   209,    13,    94,   225,   232,   235,
     274,   276,   277,   288,   289,   292,   211,   241,   242,     3,
       3,     5,     3,     5,     3,     3,   210,   314,   278,   220,
     133,   282,   286,     3,     5,   210,   223,   296,   300,   300,
     215,   208,   213,   240,   298,   213,   187,   209,     5,   214,
     250,   251,   252,   270,   189,   214,   260,   213,   263,     9,
       9,    12,   246,   270,     3,     6,     7,     8,     9,    10,
      11,    73,   121,   153,   166,   192,   195,   196,   197,   200,
     201,   213,   256,   257,   258,   256,   259,     9,     9,   244,
     259,   258,     9,     9,   258,     9,   258,   256,   239,   242,
     297,   258,   254,   255,   256,     9,   316,     9,   258,   318,
     254,   256,   318,   182,   183,   184,   185,   186,   315,   318,
     318,   246,     6,     7,   321,     9,   318,   245,   208,   213,
     212,   210,   224,   297,   211,   241,   279,   219,   189,   112,
     241,   265,   283,   212,   210,   223,   211,   225,   242,   295,
     300,   214,   259,   197,   209,   256,   236,   237,   238,   239,
      26,    62,   151,   185,   329,   330,   328,   210,   211,   187,
     215,     6,   227,   211,   241,   261,   265,   211,   242,   262,
     265,   214,   214,   214,   213,   256,   256,   256,   256,   256,
     256,   256,   297,    15,    16,    17,    18,    19,    20,    21,
      22,    23,   188,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   207,   208,   187,   214,   214,   214,   187,
     214,   214,   214,   214,   214,   214,   214,   214,   214,   214,
     297,   197,   213,   240,   247,   270,   306,   308,   309,   214,
     187,   214,   214,   214,   214,   187,   214,   214,   214,   214,
     214,   214,   214,   214,   207,   214,   214,   214,   245,     9,
     222,   300,   310,   224,   289,   211,   189,   297,   317,   212,
     211,   242,   266,   267,   211,   231,   211,   224,   209,   209,
     214,   187,   187,   214,   167,   323,   324,   252,   258,     6,
     214,   297,   320,   214,   212,   212,   297,   214,   197,   213,
     240,   247,   302,   304,   305,   256,   256,     3,   256,   256,
     256,   256,   256,   256,   256,   256,   256,   256,   256,   256,
     256,   256,   256,   256,   256,     3,   256,   258,     9,   308,
     296,   197,   236,   247,   307,   308,   213,   240,   255,   258,
       6,   214,   221,   187,   300,   212,   223,   224,    24,   239,
     330,   326,   211,   323,   306,   312,   313,   212,   268,   270,
     304,   296,   197,   236,   247,   303,   304,   214,   213,   240,
     189,   209,   308,   296,   214,   308,   214,   236,   214,   224,
     211,   300,   211,     5,   189,   311,   187,   212,   210,   214,
     304,   296,   214,   308,   214,   256,   236,   256,   308,   214,
     224,   212,   259,   312,   248,   308,   214,    40,    53,   211,
     249,   258,   189,   189,   212,   264,   265,   264,   212
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_int16 yyr1[] =
{
       0,   216,   217,   218,   218,   219,   220,   219,   219,   219,
     219,   219,   219,   219,   219,   221,   221,   222,   221,   221,
     221,   221,   221,   221,   221,   221,   223,   223,   224,   224,
     225,   225,   225,   225,   225,   225,   225,   226,   227,   227,
     228,   228,   228,   228,   228,   228,   228,   228,   228,   229,
     230,   231,   232,   233,   233,   234,   235,   236,   236,   237,
     237,   238,   238,   239,   239,   240,   240,   240,   241,   241,
     242,   243,   243,   243,   244,   244,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   245,   246,   246,   247,   247,
     247,   247,   248,   248,   249,   249,   250,   250,   250,   251,
     251,   252,   252,   253,   254,   254,   255,   255,   256,   256,
     256,   256,   256,   256,   256,   256,   256,   256,   256,   256,
     256,   256,   256,   256,   256,   256,   256,   256,   256,   256,
     256,   256,   256,   256,   256,   256,   256,   256,   256,   256,
     256,   256,   256,   256,   256,   256,   256,   256,   256,   257,
     257,   258,   259,   260,   260,   261,   261,   262,   262,   263,
     263,   264,   264,   265,   265,   266,   267,   267,   268,   268,
     269,   269,   269,   270,   270,   271,   271,   271,   271,   271,
     271,   271,   271,   271,   271,   271,   272,   272,   273,   273,
     273,   273,   273,   273,   273,   273,   273,   274,   274,   275,
     276,   277,   278,   278,   279,   280,   280,   281,   282,   282,
     283,   283,   284,   284,   285,   285,   286,   286,   287,   288,
     288,   288,   289,   289,   290,   290,   291,   292,   293,   293,
     293,   294,   295,   296,   296,   297,   297,   298,   298,   299,
     299,   299,   300,   300,   300,   301,   301,   301,   301,   302,
     302,   302,   303,   303,   304,   304,   305,   305,   305,   305,
     305,   306,   306,   306,   307,   307,   308,   308,   309,   309,
     309,   309,   309,   309,   310,   310,   311,   311,   312,   313,
     313,   314,   314,   315,   315,   315,   315,   315,   316,   316,
     316,   317,   318,   318,   318,   318,   318,   318,   318,   318,
     318,   318,   319,   320,   320,   321,   321,   321,   322,   322,
     323,   323,   324,   325,   326,   326,   327,   327,   328,   328,
     328,   328,   329,   329,   330,   330,   330,   330
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     0,     2,     0,     0,     6,     2,     2,
       3,     2,     2,     2,     2,     0,     2,     0,     6,     2,
       3,     2,     2,     2,     2,     2,     0,     2,     0,     1,
       1,     2,     2,     1,     2,     1,     1,     6,     1,     2,
       1,     2,     1,     2,     1,     2,     2,     2,     2,     4,
       3,     3,     5,     2,     2,     3,     4,     0,     1,     1,
       3,     1,     3,     3,     2,     3,     3,     2,     0,     1,
       3,     1,     3,     4,     1,     3,     0,     1,     4,     1,
       1,     1,     1,     1,     4,     4,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     4,     1,     1,     1,
       4,     1,     1,     1,     4,     4,     1,     1,     1,     1,
       4,     4,     4,     4,     4,     1,     4,     1,     1,     4,
       1,     4,     1,     1,     4,     4,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       4,     1,     1,     1,     4,     4,     1,     1,     1,     1,
       1,     6,     1,     4,     1,     1,     1,     4,     1,     1,
       1,     4,     4,     4,     4,     1,     1,     4,     4,     4,
       1,     1,     4,     4,     4,     1,     1,     1,     1,     1,
       1,     1,     0,     2,     4,     3,     0,     2,     1,     1,
       3,     3,     1,     5,     1,     3,     0,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     5,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     2,     2,     2,
       2,     2,     2,     3,     3,     5,     5,     4,     3,     1,
       3,     1,     1,     0,     2,     4,     3,     2,     2,     0,
       2,     2,     1,     3,     2,     1,     3,     2,     0,     1,
       0,     1,     1,     1,     1,     1,     1,     1,     2,     2,
       1,     1,     1,     1,     1,     1,     0,     1,     1,     2,
       1,     2,     2,     1,     1,     1,     1,     2,     2,     2,
       5,     2,     0,     2,     2,     2,     2,     2,     2,     3,
       2,     3,     5,     5,     0,     2,     2,     2,     2,     6,
       8,     2,     2,     2,     2,     2,     2,     5,     1,     1,
       1,     1,     1,     0,     2,     2,     3,     0,     1,     2,
       2,     2,     3,     2,     1,     1,     3,     2,     4,     3,
       2,     1,     3,     2,     0,     1,     3,     2,     1,     3,
       4,     3,     2,     1,     3,     2,     0,     1,     1,     3,
       2,     1,     3,     4,     1,     3,     0,     2,     2,     1,
       3,     1,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     5,     1,     1,     1,     1,     2,     1,     2,     1,
       2,     4,     5,     5,    10,     1,     3,     1,     0,     2,
       0,     2,     4,     6,     0,     3,     1,     3,     4,     1,
       1,     1,     1,     3,     1,     1,     1,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yytype], *yyvaluep);
# endif
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyo, yytype, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[+yyssp[yyi + 1 - yynrhs]],
                       &yyvsp[(yyi + 1) - (yynrhs)]
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
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
#ifndef YYINITDEPTH
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
#   define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
#  else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
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
static char *
yystpcpy (char *yydest, const char *yysrc)
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
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
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
            else
              goto append;

          append:
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

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                yy_state_t *yyssp, int yytoken)
{
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Actual size of YYARG. */
  int yycount = 0;
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[+*yyssp];
      YYPTRDIFF_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
      yysize = yysize0;
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYPTRDIFF_T yysize1
                    = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
                    yysize = yysize1;
                  else
                    return 2;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    /* Don't count the "%s"s in the final size, but reserve room for
       the terminator.  */
    YYPTRDIFF_T yysize1 = yysize + (yystrlen (yyformat) - 2 * yycount) + 1;
    if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
      yysize = yysize1;
    else
      return 2;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss;
    yy_state_t *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYPTRDIFF_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
# undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
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
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
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
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

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
#line 328 "parser.y"
                                                { fix_incomplete();
						  check_statements((yyvsp[-1].stmt_list), FALSE);
						  check_all_user_types((yyvsp[-1].stmt_list));
						  write_header((yyvsp[-1].stmt_list));
						  write_id_data((yyvsp[-1].stmt_list));
						  write_proxies((yyvsp[-1].stmt_list));
						  write_client((yyvsp[-1].stmt_list));
						  write_server((yyvsp[-1].stmt_list));
						  write_regscript((yyvsp[-1].stmt_list));
#ifndef __REACTOS__
						  write_typelib_regscript((yyvsp[-1].stmt_list));
#endif
						  write_dlldata((yyvsp[-1].stmt_list));
						  write_local_stubs((yyvsp[-1].stmt_list));
						}
#line 2767 "parser.tab.c"
    break;

  case 5:
#line 347 "parser.y"
                                                { (yyval.stmt_list) = NULL; }
#line 2773 "parser.tab.c"
    break;

  case 6:
#line 348 "parser.y"
                                          { push_namespace((yyvsp[-1].str)); }
#line 2779 "parser.tab.c"
    break;

  case 7:
#line 349 "parser.y"
                                                { pop_namespace((yyvsp[-4].str)); (yyval.stmt_list) = append_statements((yyvsp[-5].stmt_list), (yyvsp[-1].stmt_list)); }
#line 2785 "parser.tab.c"
    break;

  case 8:
#line 350 "parser.y"
                                                { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_reference((yyvsp[0].type))); }
#line 2791 "parser.tab.c"
    break;

  case 9:
#line 351 "parser.y"
                                                { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_type_decl((yyvsp[0].type))); }
#line 2797 "parser.tab.c"
    break;

  case 10:
#line 352 "parser.y"
                                                { (yyval.stmt_list) = (yyvsp[-2].stmt_list);
						  reg_type((yyvsp[-1].type), (yyvsp[-1].type)->name, current_namespace, 0);
						}
#line 2805 "parser.tab.c"
    break;

  case 11:
#line 355 "parser.y"
                                                { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_type_decl((yyvsp[0].type)));
						  reg_type((yyvsp[0].type), (yyvsp[0].type)->name, current_namespace, 0);
						}
#line 2813 "parser.tab.c"
    break;

  case 12:
#line 358 "parser.y"
                                                { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_module((yyvsp[0].type))); }
#line 2819 "parser.tab.c"
    break;

  case 13:
#line 359 "parser.y"
                                                { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_library((yyvsp[0].typelib))); }
#line 2825 "parser.tab.c"
    break;

  case 14:
#line 360 "parser.y"
                                                { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), (yyvsp[0].statement)); }
#line 2831 "parser.tab.c"
    break;

  case 15:
#line 363 "parser.y"
                                                { (yyval.stmt_list) = NULL; }
#line 2837 "parser.tab.c"
    break;

  case 16:
#line 364 "parser.y"
                                                { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_reference((yyvsp[0].type))); }
#line 2843 "parser.tab.c"
    break;

  case 17:
#line 365 "parser.y"
                                          { push_namespace((yyvsp[-1].str)); }
#line 2849 "parser.tab.c"
    break;

  case 18:
#line 366 "parser.y"
                                                { pop_namespace((yyvsp[-4].str)); (yyval.stmt_list) = append_statements((yyvsp[-5].stmt_list), (yyvsp[-1].stmt_list)); }
#line 2855 "parser.tab.c"
    break;

  case 19:
#line 367 "parser.y"
                                                { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_type_decl((yyvsp[0].type))); }
#line 2861 "parser.tab.c"
    break;

  case 20:
#line 368 "parser.y"
                                                { (yyval.stmt_list) = (yyvsp[-2].stmt_list); reg_type((yyvsp[-1].type), (yyvsp[-1].type)->name, current_namespace, 0); }
#line 2867 "parser.tab.c"
    break;

  case 21:
#line 369 "parser.y"
                                                { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_type_decl((yyvsp[0].type)));
						  reg_type((yyvsp[0].type), (yyvsp[0].type)->name, current_namespace, 0);
						}
#line 2875 "parser.tab.c"
    break;

  case 22:
#line 372 "parser.y"
                                                { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_module((yyvsp[0].type))); }
#line 2881 "parser.tab.c"
    break;

  case 23:
#line 373 "parser.y"
                                                { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), (yyvsp[0].statement)); }
#line 2887 "parser.tab.c"
    break;

  case 24:
#line 374 "parser.y"
                                                { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_importlib((yyvsp[0].str))); }
#line 2893 "parser.tab.c"
    break;

  case 25:
#line 375 "parser.y"
                                                { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), make_statement_library((yyvsp[0].typelib))); }
#line 2899 "parser.tab.c"
    break;

  case 26:
#line 378 "parser.y"
                                                { (yyval.stmt_list) = NULL; }
#line 2905 "parser.tab.c"
    break;

  case 27:
#line 379 "parser.y"
                                                { (yyval.stmt_list) = append_statement((yyvsp[-1].stmt_list), (yyvsp[0].statement)); }
#line 2911 "parser.tab.c"
    break;

  case 30:
#line 387 "parser.y"
                                                { (yyval.statement) = make_statement_cppquote((yyvsp[0].str)); }
#line 2917 "parser.tab.c"
    break;

  case 31:
#line 388 "parser.y"
                                                { (yyval.statement) = make_statement_type_decl((yyvsp[-1].type)); }
#line 2923 "parser.tab.c"
    break;

  case 32:
#line 389 "parser.y"
                                                { (yyval.statement) = make_statement_declaration((yyvsp[-1].var)); }
#line 2929 "parser.tab.c"
    break;

  case 33:
#line 390 "parser.y"
                                                { (yyval.statement) = make_statement_import((yyvsp[0].str)); }
#line 2935 "parser.tab.c"
    break;

  case 34:
#line 391 "parser.y"
                                                { (yyval.statement) = (yyvsp[-1].statement); }
#line 2941 "parser.tab.c"
    break;

  case 35:
#line 392 "parser.y"
                                                { (yyval.statement) = make_statement_pragma((yyvsp[0].str)); }
#line 2947 "parser.tab.c"
    break;

  case 36:
#line 393 "parser.y"
                         { (yyval.statement) = NULL; }
#line 2953 "parser.tab.c"
    break;

  case 37:
#line 397 "parser.y"
                  {
                      int result;
                      (yyval.statement) = NULL;
                      result = do_warning((yyvsp[-3].str), (yyvsp[-1].warning_list));
                      if(!result)
                          error_loc("expected \"disable\" or \"enable\"\n");
                  }
#line 2965 "parser.tab.c"
    break;

  case 38:
#line 407 "parser.y"
               { (yyval.warning_list) = append_warning(NULL, (yyvsp[0].num)); }
#line 2971 "parser.tab.c"
    break;

  case 39:
#line 408 "parser.y"
                        { (yyval.warning_list) = append_warning((yyvsp[-1].warning_list), (yyvsp[0].num)); }
#line 2977 "parser.tab.c"
    break;

  case 41:
#line 413 "parser.y"
                                                { (yyval.type) = type_new_enum((yyvsp[0].str), current_namespace, FALSE, NULL); }
#line 2983 "parser.tab.c"
    break;

  case 43:
#line 415 "parser.y"
                                                { (yyval.type) = type_new_struct((yyvsp[0].str), current_namespace, FALSE, NULL); }
#line 2989 "parser.tab.c"
    break;

  case 45:
#line 417 "parser.y"
                                                { (yyval.type) = type_new_nonencapsulated_union((yyvsp[0].str), FALSE, NULL); }
#line 2995 "parser.tab.c"
    break;

  case 46:
#line 418 "parser.y"
                                                { (yyval.type) = (yyvsp[0].type); (yyval.type)->attrs = check_enum_attrs((yyvsp[-1].attr_list)); }
#line 3001 "parser.tab.c"
    break;

  case 47:
#line 419 "parser.y"
                                                { (yyval.type) = (yyvsp[0].type); (yyval.type)->attrs = check_struct_attrs((yyvsp[-1].attr_list)); }
#line 3007 "parser.tab.c"
    break;

  case 48:
#line 420 "parser.y"
                                                { (yyval.type) = (yyvsp[0].type); (yyval.type)->attrs = check_union_attrs((yyvsp[-1].attr_list)); }
#line 3013 "parser.tab.c"
    break;

  case 49:
#line 423 "parser.y"
                                                { (yyval.str) = (yyvsp[-1].str); }
#line 3019 "parser.tab.c"
    break;

  case 50:
#line 425 "parser.y"
                                                { assert(yychar == YYEMPTY);
						  (yyval.import) = xmalloc(sizeof(struct _import_t));
						  (yyval.import)->name = (yyvsp[-1].str);
						  (yyval.import)->import_performed = do_import((yyvsp[-1].str));
						  if (!(yyval.import)->import_performed) yychar = aEOF;
						}
#line 3030 "parser.tab.c"
    break;

  case 51:
#line 433 "parser.y"
                                                { (yyval.str) = (yyvsp[-2].import)->name;
						  if ((yyvsp[-2].import)->import_performed) pop_import();
						  free((yyvsp[-2].import));
						}
#line 3039 "parser.tab.c"
    break;

  case 52:
#line 441 "parser.y"
                                                { (yyval.str) = (yyvsp[-2].str); if(!parse_only) add_importlib((yyvsp[-2].str)); }
#line 3045 "parser.tab.c"
    break;

  case 53:
#line 447 "parser.y"
                                                { (yyval.str) = (yyvsp[0].str); }
#line 3051 "parser.tab.c"
    break;

  case 54:
#line 448 "parser.y"
                                                { (yyval.str) = (yyvsp[0].str); }
#line 3057 "parser.tab.c"
    break;

  case 55:
#line 450 "parser.y"
                                                { (yyval.typelib) = make_library((yyvsp[-1].str), check_library_attrs((yyvsp[-1].str), (yyvsp[-2].attr_list)));
/* ifdef __REACTOS__ */
						  if (!parse_only) start_typelib((yyval.typelib));
/* else
						  if (!parse_only && do_typelib) current_typelib = $$;
*/
						}
#line 3069 "parser.tab.c"
    break;

  case 56:
#line 460 "parser.y"
                                                { (yyval.typelib) = (yyvsp[-3].typelib);
						  (yyval.typelib)->stmts = (yyvsp[-2].stmt_list);
						  if (!parse_only) end_typelib();
						}
#line 3078 "parser.tab.c"
    break;

  case 57:
#line 469 "parser.y"
                                                { (yyval.var_list) = NULL; }
#line 3084 "parser.tab.c"
    break;

  case 59:
#line 473 "parser.y"
                                                { check_arg_attrs((yyvsp[0].var)); (yyval.var_list) = append_var( NULL, (yyvsp[0].var) ); }
#line 3090 "parser.tab.c"
    break;

  case 60:
#line 474 "parser.y"
                                                { check_arg_attrs((yyvsp[0].var)); (yyval.var_list) = append_var( (yyvsp[-2].var_list), (yyvsp[0].var) ); }
#line 3096 "parser.tab.c"
    break;

  case 62:
#line 478 "parser.y"
                                                { (yyval.var_list) = append_var( (yyvsp[-2].var_list), make_var(strdup("...")) ); }
#line 3102 "parser.tab.c"
    break;

  case 63:
#line 482 "parser.y"
                                                { if ((yyvsp[-1].declspec)->stgclass != STG_NONE && (yyvsp[-1].declspec)->stgclass != STG_REGISTER)
						    error_loc("invalid storage class for function parameter\n");
						  (yyval.var) = declare_var((yyvsp[-2].attr_list), (yyvsp[-1].declspec), (yyvsp[0].declarator), TRUE);
						  free((yyvsp[-1].declspec)); free((yyvsp[0].declarator));
						}
#line 3112 "parser.tab.c"
    break;

  case 64:
#line 487 "parser.y"
                                                { if ((yyvsp[-1].declspec)->stgclass != STG_NONE && (yyvsp[-1].declspec)->stgclass != STG_REGISTER)
						    error_loc("invalid storage class for function parameter\n");
						  (yyval.var) = declare_var(NULL, (yyvsp[-1].declspec), (yyvsp[0].declarator), TRUE);
						  free((yyvsp[-1].declspec)); free((yyvsp[0].declarator));
						}
#line 3122 "parser.tab.c"
    break;

  case 65:
#line 494 "parser.y"
                                                { (yyval.expr) = (yyvsp[-1].expr);
						  if (!(yyval.expr)->is_const || (yyval.expr)->cval <= 0)
						      error_loc("array dimension is not a positive integer constant\n");
						}
#line 3131 "parser.tab.c"
    break;

  case 66:
#line 498 "parser.y"
                                                { (yyval.expr) = make_expr(EXPR_VOID); }
#line 3137 "parser.tab.c"
    break;

  case 67:
#line 499 "parser.y"
                                                { (yyval.expr) = make_expr(EXPR_VOID); }
#line 3143 "parser.tab.c"
    break;

  case 68:
#line 502 "parser.y"
                                                { (yyval.attr_list) = NULL; }
#line 3149 "parser.tab.c"
    break;

  case 70:
#line 507 "parser.y"
                                                { (yyval.attr_list) = (yyvsp[-1].attr_list); }
#line 3155 "parser.tab.c"
    break;

  case 71:
#line 510 "parser.y"
                                                { (yyval.attr_list) = append_attr( NULL, (yyvsp[0].attr) ); }
#line 3161 "parser.tab.c"
    break;

  case 72:
#line 511 "parser.y"
                                                { (yyval.attr_list) = append_attr( (yyvsp[-2].attr_list), (yyvsp[0].attr) ); }
#line 3167 "parser.tab.c"
    break;

  case 73:
#line 512 "parser.y"
                                                { (yyval.attr_list) = append_attr( (yyvsp[-3].attr_list), (yyvsp[0].attr) ); }
#line 3173 "parser.tab.c"
    break;

  case 74:
#line 515 "parser.y"
                                                { (yyval.str_list) = append_str( NULL, (yyvsp[0].str) ); }
#line 3179 "parser.tab.c"
    break;

  case 75:
#line 516 "parser.y"
                                                { (yyval.str_list) = append_str( (yyvsp[-2].str_list), (yyvsp[0].str) ); }
#line 3185 "parser.tab.c"
    break;

  case 76:
#line 519 "parser.y"
                                                { (yyval.attr) = NULL; }
#line 3191 "parser.tab.c"
    break;

  case 77:
#line 520 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_AGGREGATABLE); }
#line 3197 "parser.tab.c"
    break;

  case 78:
#line 521 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_ANNOTATION, (yyvsp[-1].str)); }
#line 3203 "parser.tab.c"
    break;

  case 79:
#line 522 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_APPOBJECT); }
#line 3209 "parser.tab.c"
    break;

  case 80:
#line 523 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_ASYNC); }
#line 3215 "parser.tab.c"
    break;

  case 81:
#line 524 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_AUTO_HANDLE); }
#line 3221 "parser.tab.c"
    break;

  case 82:
#line 525 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_BINDABLE); }
#line 3227 "parser.tab.c"
    break;

  case 83:
#line 526 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_BROADCAST); }
#line 3233 "parser.tab.c"
    break;

  case 84:
#line 527 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_CALLAS, (yyvsp[-1].var)); }
#line 3239 "parser.tab.c"
    break;

  case 85:
#line 528 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_CASE, (yyvsp[-1].expr_list)); }
#line 3245 "parser.tab.c"
    break;

  case 86:
#line 529 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_CODE); }
#line 3251 "parser.tab.c"
    break;

  case 87:
#line 530 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_COMMSTATUS); }
#line 3257 "parser.tab.c"
    break;

  case 88:
#line 531 "parser.y"
                                                { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); }
#line 3263 "parser.tab.c"
    break;

  case 89:
#line 532 "parser.y"
                                                { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_DONT_SERIALIZE */ }
#line 3269 "parser.tab.c"
    break;

  case 90:
#line 533 "parser.y"
                                                { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_SERIALIZE */ }
#line 3275 "parser.tab.c"
    break;

  case 91:
#line 534 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_CONTROL); }
#line 3281 "parser.tab.c"
    break;

  case 92:
#line 535 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_DECODE); }
#line 3287 "parser.tab.c"
    break;

  case 93:
#line 536 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_DEFAULT); }
#line 3293 "parser.tab.c"
    break;

  case 94:
#line 537 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_DEFAULTBIND); }
#line 3299 "parser.tab.c"
    break;

  case 95:
#line 538 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_DEFAULTCOLLELEM); }
#line 3305 "parser.tab.c"
    break;

  case 96:
#line 539 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_DEFAULTVALUE, (yyvsp[-1].expr)); }
#line 3311 "parser.tab.c"
    break;

  case 97:
#line 540 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_DEFAULTVTABLE); }
#line 3317 "parser.tab.c"
    break;

  case 98:
#line 541 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_DISABLECONSISTENCYCHECK); }
#line 3323 "parser.tab.c"
    break;

  case 99:
#line 542 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_DISPLAYBIND); }
#line 3329 "parser.tab.c"
    break;

  case 100:
#line 543 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_DLLNAME, (yyvsp[-1].str)); }
#line 3335 "parser.tab.c"
    break;

  case 101:
#line 544 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_DUAL); }
#line 3341 "parser.tab.c"
    break;

  case 102:
#line 545 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_ENABLEALLOCATE); }
#line 3347 "parser.tab.c"
    break;

  case 103:
#line 546 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_ENCODE); }
#line 3353 "parser.tab.c"
    break;

  case 104:
#line 547 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_ENDPOINT, (yyvsp[-1].str_list)); }
#line 3359 "parser.tab.c"
    break;

  case 105:
#line 548 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_ENTRY, (yyvsp[-1].expr)); }
#line 3365 "parser.tab.c"
    break;

  case 106:
#line 549 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_EXPLICIT_HANDLE); }
#line 3371 "parser.tab.c"
    break;

  case 107:
#line 550 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_FAULTSTATUS); }
#line 3377 "parser.tab.c"
    break;

  case 108:
#line 551 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_FORCEALLOCATE); }
#line 3383 "parser.tab.c"
    break;

  case 109:
#line 552 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_HANDLE); }
#line 3389 "parser.tab.c"
    break;

  case 110:
#line 553 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_HELPCONTEXT, (yyvsp[-1].expr)); }
#line 3395 "parser.tab.c"
    break;

  case 111:
#line 554 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_HELPFILE, (yyvsp[-1].str)); }
#line 3401 "parser.tab.c"
    break;

  case 112:
#line 555 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_HELPSTRING, (yyvsp[-1].str)); }
#line 3407 "parser.tab.c"
    break;

  case 113:
#line 556 "parser.y"
                                                        { (yyval.attr) = make_attrp(ATTR_HELPSTRINGCONTEXT, (yyvsp[-1].expr)); }
#line 3413 "parser.tab.c"
    break;

  case 114:
#line 557 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_HELPSTRINGDLL, (yyvsp[-1].str)); }
#line 3419 "parser.tab.c"
    break;

  case 115:
#line 558 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_HIDDEN); }
#line 3425 "parser.tab.c"
    break;

  case 116:
#line 559 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_ID, (yyvsp[-1].expr)); }
#line 3431 "parser.tab.c"
    break;

  case 117:
#line 560 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_IDEMPOTENT); }
#line 3437 "parser.tab.c"
    break;

  case 118:
#line 561 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_IGNORE); }
#line 3443 "parser.tab.c"
    break;

  case 119:
#line 562 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_IIDIS, (yyvsp[-1].expr)); }
#line 3449 "parser.tab.c"
    break;

  case 120:
#line 563 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_IMMEDIATEBIND); }
#line 3455 "parser.tab.c"
    break;

  case 121:
#line 564 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_IMPLICIT_HANDLE, (yyvsp[-1].var)); }
#line 3461 "parser.tab.c"
    break;

  case 122:
#line 565 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_IN); }
#line 3467 "parser.tab.c"
    break;

  case 123:
#line 566 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_INPUTSYNC); }
#line 3473 "parser.tab.c"
    break;

  case 124:
#line 567 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_LENGTHIS, (yyvsp[-1].expr_list)); }
#line 3479 "parser.tab.c"
    break;

  case 125:
#line 568 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_LIBLCID, (yyvsp[-1].expr)); }
#line 3485 "parser.tab.c"
    break;

  case 126:
#line 569 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_PARAMLCID); }
#line 3491 "parser.tab.c"
    break;

  case 127:
#line 570 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_LICENSED); }
#line 3497 "parser.tab.c"
    break;

  case 128:
#line 571 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_LOCAL); }
#line 3503 "parser.tab.c"
    break;

  case 129:
#line 572 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_MAYBE); }
#line 3509 "parser.tab.c"
    break;

  case 130:
#line 573 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_MESSAGE); }
#line 3515 "parser.tab.c"
    break;

  case 131:
#line 574 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_NOCODE); }
#line 3521 "parser.tab.c"
    break;

  case 132:
#line 575 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_NONBROWSABLE); }
#line 3527 "parser.tab.c"
    break;

  case 133:
#line 576 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_NONCREATABLE); }
#line 3533 "parser.tab.c"
    break;

  case 134:
#line 577 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_NONEXTENSIBLE); }
#line 3539 "parser.tab.c"
    break;

  case 135:
#line 578 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_NOTIFY); }
#line 3545 "parser.tab.c"
    break;

  case 136:
#line 579 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_NOTIFYFLAG); }
#line 3551 "parser.tab.c"
    break;

  case 137:
#line 580 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_OBJECT); }
#line 3557 "parser.tab.c"
    break;

  case 138:
#line 581 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_ODL); }
#line 3563 "parser.tab.c"
    break;

  case 139:
#line 582 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_OLEAUTOMATION); }
#line 3569 "parser.tab.c"
    break;

  case 140:
#line 583 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_OPTIMIZE, (yyvsp[-1].str)); }
#line 3575 "parser.tab.c"
    break;

  case 141:
#line 584 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_OPTIONAL); }
#line 3581 "parser.tab.c"
    break;

  case 142:
#line 585 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_OUT); }
#line 3587 "parser.tab.c"
    break;

  case 143:
#line 586 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_PARTIALIGNORE); }
#line 3593 "parser.tab.c"
    break;

  case 144:
#line 587 "parser.y"
                                                { (yyval.attr) = make_attrv(ATTR_POINTERDEFAULT, (yyvsp[-1].num)); }
#line 3599 "parser.tab.c"
    break;

  case 145:
#line 588 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_PROGID, (yyvsp[-1].str)); }
#line 3605 "parser.tab.c"
    break;

  case 146:
#line 589 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_PROPGET); }
#line 3611 "parser.tab.c"
    break;

  case 147:
#line 590 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_PROPPUT); }
#line 3617 "parser.tab.c"
    break;

  case 148:
#line 591 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_PROPPUTREF); }
#line 3623 "parser.tab.c"
    break;

  case 149:
#line 592 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_PROXY); }
#line 3629 "parser.tab.c"
    break;

  case 150:
#line 593 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_PUBLIC); }
#line 3635 "parser.tab.c"
    break;

  case 151:
#line 595 "parser.y"
                                                { expr_list_t *list = append_expr( NULL, (yyvsp[-3].expr) );
						  list = append_expr( list, (yyvsp[-1].expr) );
						  (yyval.attr) = make_attrp(ATTR_RANGE, list); }
#line 3643 "parser.tab.c"
    break;

  case 152:
#line 598 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_READONLY); }
#line 3649 "parser.tab.c"
    break;

  case 153:
#line 599 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_REPRESENTAS, (yyvsp[-1].type)); }
#line 3655 "parser.tab.c"
    break;

  case 154:
#line 600 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_REQUESTEDIT); }
#line 3661 "parser.tab.c"
    break;

  case 155:
#line 601 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_RESTRICTED); }
#line 3667 "parser.tab.c"
    break;

  case 156:
#line 602 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_RETVAL); }
#line 3673 "parser.tab.c"
    break;

  case 157:
#line 603 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_SIZEIS, (yyvsp[-1].expr_list)); }
#line 3679 "parser.tab.c"
    break;

  case 158:
#line 604 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_SOURCE); }
#line 3685 "parser.tab.c"
    break;

  case 159:
#line 605 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_STRICTCONTEXTHANDLE); }
#line 3691 "parser.tab.c"
    break;

  case 160:
#line 606 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_STRING); }
#line 3697 "parser.tab.c"
    break;

  case 161:
#line 607 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_SWITCHIS, (yyvsp[-1].expr)); }
#line 3703 "parser.tab.c"
    break;

  case 162:
#line 608 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_SWITCHTYPE, (yyvsp[-1].type)); }
#line 3709 "parser.tab.c"
    break;

  case 163:
#line 609 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_TRANSMITAS, (yyvsp[-1].type)); }
#line 3715 "parser.tab.c"
    break;

  case 164:
#line 610 "parser.y"
                                                { (yyval.attr) = make_attrv(ATTR_THREADING, (yyvsp[-1].num)); }
#line 3721 "parser.tab.c"
    break;

  case 165:
#line 611 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_UIDEFAULT); }
#line 3727 "parser.tab.c"
    break;

  case 166:
#line 612 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_USESGETLASTERROR); }
#line 3733 "parser.tab.c"
    break;

  case 167:
#line 613 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_USERMARSHAL, (yyvsp[-1].type)); }
#line 3739 "parser.tab.c"
    break;

  case 168:
#line 614 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_UUID, (yyvsp[-1].uuid)); }
#line 3745 "parser.tab.c"
    break;

  case 169:
#line 615 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_ASYNCUUID, (yyvsp[-1].uuid)); }
#line 3751 "parser.tab.c"
    break;

  case 170:
#line 616 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_V1ENUM); }
#line 3757 "parser.tab.c"
    break;

  case 171:
#line 617 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_VARARG); }
#line 3763 "parser.tab.c"
    break;

  case 172:
#line 618 "parser.y"
                                                { (yyval.attr) = make_attrv(ATTR_VERSION, (yyvsp[-1].num)); }
#line 3769 "parser.tab.c"
    break;

  case 173:
#line 619 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_VIPROGID, (yyvsp[-1].str)); }
#line 3775 "parser.tab.c"
    break;

  case 174:
#line 620 "parser.y"
                                                { (yyval.attr) = make_attrp(ATTR_WIREMARSHAL, (yyvsp[-1].type)); }
#line 3781 "parser.tab.c"
    break;

  case 175:
#line 621 "parser.y"
                                                { (yyval.attr) = make_attrv(ATTR_POINTERTYPE, (yyvsp[0].num)); }
#line 3787 "parser.tab.c"
    break;

  case 177:
#line 626 "parser.y"
                                                { if (!is_valid_uuid((yyvsp[0].str)))
						    error_loc("invalid UUID: %s\n", (yyvsp[0].str));
						  (yyval.uuid) = parse_uuid((yyvsp[0].str)); }
#line 3795 "parser.tab.c"
    break;

  case 178:
#line 631 "parser.y"
                                                { (yyval.str) = xstrdup("__cdecl"); }
#line 3801 "parser.tab.c"
    break;

  case 179:
#line 632 "parser.y"
                                                { (yyval.str) = xstrdup("__fastcall"); }
#line 3807 "parser.tab.c"
    break;

  case 180:
#line 633 "parser.y"
                                                { (yyval.str) = xstrdup("__pascal"); }
#line 3813 "parser.tab.c"
    break;

  case 181:
#line 634 "parser.y"
                                                { (yyval.str) = xstrdup("__stdcall"); }
#line 3819 "parser.tab.c"
    break;

  case 182:
#line 637 "parser.y"
                                                { (yyval.var_list) = NULL; }
#line 3825 "parser.tab.c"
    break;

  case 183:
#line 638 "parser.y"
                                                { (yyval.var_list) = append_var( (yyvsp[-1].var_list), (yyvsp[0].var) ); }
#line 3831 "parser.tab.c"
    break;

  case 184:
#line 641 "parser.y"
                                                { attr_t *a = make_attrp(ATTR_CASE, append_expr( NULL, (yyvsp[-2].expr) ));
						  (yyval.var) = (yyvsp[0].var); if (!(yyval.var)) (yyval.var) = make_var(NULL);
						  (yyval.var)->attrs = append_attr( (yyval.var)->attrs, a );
						}
#line 3840 "parser.tab.c"
    break;

  case 185:
#line 645 "parser.y"
                                                { attr_t *a = make_attr(ATTR_DEFAULT);
						  (yyval.var) = (yyvsp[0].var); if (!(yyval.var)) (yyval.var) = make_var(NULL);
						  (yyval.var)->attrs = append_attr( (yyval.var)->attrs, a );
						}
#line 3849 "parser.tab.c"
    break;

  case 186:
#line 651 "parser.y"
                                                { (yyval.var_list) = NULL; }
#line 3855 "parser.tab.c"
    break;

  case 187:
#line 652 "parser.y"
                                                { (yyval.var_list) = (yyvsp[-1].var_list); }
#line 3861 "parser.tab.c"
    break;

  case 189:
#line 656 "parser.y"
                                                { if (!(yyvsp[0].var)->eval)
						    (yyvsp[0].var)->eval = make_exprl(EXPR_NUM, 0 /* default for first enum entry */);
                                                  (yyval.var_list) = append_var( NULL, (yyvsp[0].var) );
						}
#line 3870 "parser.tab.c"
    break;

  case 190:
#line 660 "parser.y"
                                                { if (!(yyvsp[0].var)->eval)
                                                  {
                                                    var_t *last = LIST_ENTRY( list_tail((yyval.var_list)), var_t, entry );
                                                    enum expr_type type = EXPR_NUM;
                                                    if (last->eval->type == EXPR_HEXNUM) type = EXPR_HEXNUM;
                                                    if (last->eval->cval + 1 < 0) type = EXPR_HEXNUM;
                                                    (yyvsp[0].var)->eval = make_exprl(type, last->eval->cval + 1);
                                                  }
                                                  (yyval.var_list) = append_var( (yyvsp[-2].var_list), (yyvsp[0].var) );
						}
#line 3885 "parser.tab.c"
    break;

  case 191:
#line 672 "parser.y"
                                                { (yyval.var) = reg_const((yyvsp[-2].var));
						  (yyval.var)->eval = (yyvsp[0].expr);
                                                  (yyval.var)->type = type_new_int(TYPE_BASIC_INT, 0);
						}
#line 3894 "parser.tab.c"
    break;

  case 192:
#line 676 "parser.y"
                                                { (yyval.var) = reg_const((yyvsp[0].var));
                                                  (yyval.var)->type = type_new_int(TYPE_BASIC_INT, 0);
						}
#line 3902 "parser.tab.c"
    break;

  case 193:
#line 681 "parser.y"
                                                { (yyval.type) = type_new_enum((yyvsp[-3].str), current_namespace, TRUE, (yyvsp[-1].var_list)); }
#line 3908 "parser.tab.c"
    break;

  case 194:
#line 684 "parser.y"
                                                { (yyval.expr_list) = append_expr( NULL, (yyvsp[0].expr) ); }
#line 3914 "parser.tab.c"
    break;

  case 195:
#line 685 "parser.y"
                                                { (yyval.expr_list) = append_expr( (yyvsp[-2].expr_list), (yyvsp[0].expr) ); }
#line 3920 "parser.tab.c"
    break;

  case 196:
#line 688 "parser.y"
                                                { (yyval.expr) = make_expr(EXPR_VOID); }
#line 3926 "parser.tab.c"
    break;

  case 198:
#line 692 "parser.y"
                                                { (yyval.expr) = make_exprl(EXPR_NUM, (yyvsp[0].num)); }
#line 3932 "parser.tab.c"
    break;

  case 199:
#line 693 "parser.y"
                                                { (yyval.expr) = make_exprl(EXPR_HEXNUM, (yyvsp[0].num)); }
#line 3938 "parser.tab.c"
    break;

  case 200:
#line 694 "parser.y"
                                                { (yyval.expr) = make_exprd(EXPR_DOUBLE, (yyvsp[0].dbl)); }
#line 3944 "parser.tab.c"
    break;

  case 201:
#line 695 "parser.y"
                                                { (yyval.expr) = make_exprl(EXPR_TRUEFALSE, 0); }
#line 3950 "parser.tab.c"
    break;

  case 202:
#line 696 "parser.y"
                                                { (yyval.expr) = make_exprl(EXPR_NUM, 0); }
#line 3956 "parser.tab.c"
    break;

  case 203:
#line 697 "parser.y"
                                                { (yyval.expr) = make_exprl(EXPR_TRUEFALSE, 1); }
#line 3962 "parser.tab.c"
    break;

  case 204:
#line 698 "parser.y"
                                                { (yyval.expr) = make_exprs(EXPR_STRLIT, (yyvsp[0].str)); }
#line 3968 "parser.tab.c"
    break;

  case 205:
#line 699 "parser.y"
                                                { (yyval.expr) = make_exprs(EXPR_WSTRLIT, (yyvsp[0].str)); }
#line 3974 "parser.tab.c"
    break;

  case 206:
#line 700 "parser.y"
                                                { (yyval.expr) = make_exprs(EXPR_CHARCONST, (yyvsp[0].str)); }
#line 3980 "parser.tab.c"
    break;

  case 207:
#line 701 "parser.y"
                                                { (yyval.expr) = make_exprs(EXPR_IDENTIFIER, (yyvsp[0].str)); }
#line 3986 "parser.tab.c"
    break;

  case 208:
#line 702 "parser.y"
                                                { (yyval.expr) = make_expr3(EXPR_COND, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3992 "parser.tab.c"
    break;

  case 209:
#line 703 "parser.y"
                                                { (yyval.expr) = make_expr2(EXPR_LOGOR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3998 "parser.tab.c"
    break;

  case 210:
#line 704 "parser.y"
                                                { (yyval.expr) = make_expr2(EXPR_LOGAND, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4004 "parser.tab.c"
    break;

  case 211:
#line 705 "parser.y"
                                                { (yyval.expr) = make_expr2(EXPR_OR , (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4010 "parser.tab.c"
    break;

  case 212:
#line 706 "parser.y"
                                                { (yyval.expr) = make_expr2(EXPR_XOR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4016 "parser.tab.c"
    break;

  case 213:
#line 707 "parser.y"
                                                { (yyval.expr) = make_expr2(EXPR_AND, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4022 "parser.tab.c"
    break;

  case 214:
#line 708 "parser.y"
                                                { (yyval.expr) = make_expr2(EXPR_EQUALITY, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4028 "parser.tab.c"
    break;

  case 215:
#line 709 "parser.y"
                                                { (yyval.expr) = make_expr2(EXPR_INEQUALITY, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4034 "parser.tab.c"
    break;

  case 216:
#line 710 "parser.y"
                                                { (yyval.expr) = make_expr2(EXPR_GTR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4040 "parser.tab.c"
    break;

  case 217:
#line 711 "parser.y"
                                                { (yyval.expr) = make_expr2(EXPR_LESS, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4046 "parser.tab.c"
    break;

  case 218:
#line 712 "parser.y"
                                                { (yyval.expr) = make_expr2(EXPR_GTREQL, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4052 "parser.tab.c"
    break;

  case 219:
#line 713 "parser.y"
                                                { (yyval.expr) = make_expr2(EXPR_LESSEQL, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4058 "parser.tab.c"
    break;

  case 220:
#line 714 "parser.y"
                                                { (yyval.expr) = make_expr2(EXPR_SHL, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4064 "parser.tab.c"
    break;

  case 221:
#line 715 "parser.y"
                                                { (yyval.expr) = make_expr2(EXPR_SHR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4070 "parser.tab.c"
    break;

  case 222:
#line 716 "parser.y"
                                                { (yyval.expr) = make_expr2(EXPR_ADD, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4076 "parser.tab.c"
    break;

  case 223:
#line 717 "parser.y"
                                                { (yyval.expr) = make_expr2(EXPR_SUB, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4082 "parser.tab.c"
    break;

  case 224:
#line 718 "parser.y"
                                                { (yyval.expr) = make_expr2(EXPR_MOD, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4088 "parser.tab.c"
    break;

  case 225:
#line 719 "parser.y"
                                                { (yyval.expr) = make_expr2(EXPR_MUL, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4094 "parser.tab.c"
    break;

  case 226:
#line 720 "parser.y"
                                                { (yyval.expr) = make_expr2(EXPR_DIV, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 4100 "parser.tab.c"
    break;

  case 227:
#line 721 "parser.y"
                                                { (yyval.expr) = make_expr1(EXPR_LOGNOT, (yyvsp[0].expr)); }
#line 4106 "parser.tab.c"
    break;

  case 228:
#line 722 "parser.y"
                                                { (yyval.expr) = make_expr1(EXPR_NOT, (yyvsp[0].expr)); }
#line 4112 "parser.tab.c"
    break;

  case 229:
#line 723 "parser.y"
                                                { (yyval.expr) = make_expr1(EXPR_POS, (yyvsp[0].expr)); }
#line 4118 "parser.tab.c"
    break;

  case 230:
#line 724 "parser.y"
                                                { (yyval.expr) = make_expr1(EXPR_NEG, (yyvsp[0].expr)); }
#line 4124 "parser.tab.c"
    break;

  case 231:
#line 725 "parser.y"
                                                { (yyval.expr) = make_expr1(EXPR_ADDRESSOF, (yyvsp[0].expr)); }
#line 4130 "parser.tab.c"
    break;

  case 232:
#line 726 "parser.y"
                                                { (yyval.expr) = make_expr1(EXPR_PPTR, (yyvsp[0].expr)); }
#line 4136 "parser.tab.c"
    break;

  case 233:
#line 727 "parser.y"
                                                { (yyval.expr) = make_expr2(EXPR_MEMBER, make_expr1(EXPR_PPTR, (yyvsp[-2].expr)), make_exprs(EXPR_IDENTIFIER, (yyvsp[0].str))); }
#line 4142 "parser.tab.c"
    break;

  case 234:
#line 728 "parser.y"
                                                { (yyval.expr) = make_expr2(EXPR_MEMBER, (yyvsp[-2].expr), make_exprs(EXPR_IDENTIFIER, (yyvsp[0].str))); }
#line 4148 "parser.tab.c"
    break;

  case 235:
#line 730 "parser.y"
                                                { (yyval.expr) = make_exprt(EXPR_CAST, declare_var(NULL, (yyvsp[-3].declspec), (yyvsp[-2].declarator), 0), (yyvsp[0].expr)); free((yyvsp[-3].declspec)); free((yyvsp[-2].declarator)); }
#line 4154 "parser.tab.c"
    break;

  case 236:
#line 732 "parser.y"
                                                { (yyval.expr) = make_exprt(EXPR_SIZEOF, declare_var(NULL, (yyvsp[-2].declspec), (yyvsp[-1].declarator), 0), NULL); free((yyvsp[-2].declspec)); free((yyvsp[-1].declarator)); }
#line 4160 "parser.tab.c"
    break;

  case 237:
#line 733 "parser.y"
                                                { (yyval.expr) = make_expr2(EXPR_ARRAY, (yyvsp[-3].expr), (yyvsp[-1].expr)); }
#line 4166 "parser.tab.c"
    break;

  case 238:
#line 734 "parser.y"
                                                { (yyval.expr) = (yyvsp[-1].expr); }
#line 4172 "parser.tab.c"
    break;

  case 239:
#line 737 "parser.y"
                                                { (yyval.expr_list) = append_expr( NULL, (yyvsp[0].expr) ); }
#line 4178 "parser.tab.c"
    break;

  case 240:
#line 738 "parser.y"
                                                        { (yyval.expr_list) = append_expr( (yyvsp[-2].expr_list), (yyvsp[0].expr) ); }
#line 4184 "parser.tab.c"
    break;

  case 241:
#line 741 "parser.y"
                                                { (yyval.expr) = (yyvsp[0].expr);
						  if (!(yyval.expr)->is_const)
						      error_loc("expression is not an integer constant\n");
						}
#line 4193 "parser.tab.c"
    break;

  case 242:
#line 747 "parser.y"
                                                { (yyval.expr) = (yyvsp[0].expr);
						  if (!(yyval.expr)->is_const && (yyval.expr)->type != EXPR_STRLIT && (yyval.expr)->type != EXPR_WSTRLIT)
						      error_loc("expression is not constant\n");
						}
#line 4202 "parser.tab.c"
    break;

  case 243:
#line 753 "parser.y"
                                                { (yyval.var_list) = NULL; }
#line 4208 "parser.tab.c"
    break;

  case 244:
#line 754 "parser.y"
                                                { (yyval.var_list) = append_var_list((yyvsp[-1].var_list), (yyvsp[0].var_list)); }
#line 4214 "parser.tab.c"
    break;

  case 245:
#line 758 "parser.y"
                                                { const char *first = LIST_ENTRY(list_head((yyvsp[-1].declarator_list)), declarator_t, entry)->var->name;
						  check_field_attrs(first, (yyvsp[-3].attr_list));
						  (yyval.var_list) = set_var_types((yyvsp[-3].attr_list), (yyvsp[-2].declspec), (yyvsp[-1].declarator_list));
						}
#line 4223 "parser.tab.c"
    break;

  case 246:
#line 762 "parser.y"
                                                { var_t *v = make_var(NULL);
						  v->type = (yyvsp[-1].type); v->attrs = (yyvsp[-2].attr_list);
						  (yyval.var_list) = append_var(NULL, v);
						}
#line 4232 "parser.tab.c"
    break;

  case 247:
#line 769 "parser.y"
                                                { (yyval.var) = (yyvsp[-1].var); }
#line 4238 "parser.tab.c"
    break;

  case 248:
#line 770 "parser.y"
                                                { (yyval.var) = make_var(NULL); (yyval.var)->attrs = (yyvsp[-1].attr_list); }
#line 4244 "parser.tab.c"
    break;

  case 249:
#line 773 "parser.y"
                                                { (yyval.var_list) = NULL; }
#line 4250 "parser.tab.c"
    break;

  case 250:
#line 774 "parser.y"
                                                { (yyval.var_list) = append_var( (yyvsp[-1].var_list), (yyvsp[0].var) ); }
#line 4256 "parser.tab.c"
    break;

  case 251:
#line 778 "parser.y"
                                                { (yyval.var) = (yyvsp[-1].var); }
#line 4262 "parser.tab.c"
    break;

  case 252:
#line 779 "parser.y"
                                                { (yyval.var) = NULL; }
#line 4268 "parser.tab.c"
    break;

  case 253:
#line 782 "parser.y"
                                                { (yyval.var) = declare_var(check_field_attrs((yyvsp[0].declarator)->var->name, (yyvsp[-2].attr_list)),
						                (yyvsp[-1].declspec), (yyvsp[0].declarator), FALSE);
						  free((yyvsp[0].declarator));
						}
#line 4277 "parser.tab.c"
    break;

  case 254:
#line 786 "parser.y"
                                                { var_t *v = make_var(NULL);
						  v->type = (yyvsp[0].type); v->attrs = (yyvsp[-1].attr_list);
						  (yyval.var) = v;
						}
#line 4286 "parser.tab.c"
    break;

  case 255:
#line 792 "parser.y"
                                                { (yyval.var) = (yyvsp[0].var);
						  if (type_get_type((yyval.var)->type) != TYPE_FUNCTION)
						    error_loc("only methods may be declared inside the methods section of a dispinterface\n");
						  check_function_attrs((yyval.var)->name, (yyval.var)->attrs);
						}
#line 4296 "parser.tab.c"
    break;

  case 256:
#line 801 "parser.y"
                                                { (yyval.var) = declare_var((yyvsp[-2].attr_list), (yyvsp[-1].declspec), (yyvsp[0].declarator), FALSE);
						  free((yyvsp[0].declarator));
						}
#line 4304 "parser.tab.c"
    break;

  case 257:
#line 804 "parser.y"
                                                { (yyval.var) = declare_var(NULL, (yyvsp[-1].declspec), (yyvsp[0].declarator), FALSE);
						  free((yyvsp[0].declarator));
						}
#line 4312 "parser.tab.c"
    break;

  case 258:
#line 809 "parser.y"
                                                { (yyval.var) = NULL; }
#line 4318 "parser.tab.c"
    break;

  case 260:
#line 813 "parser.y"
                                                { (yyval.str) = NULL; }
#line 4324 "parser.tab.c"
    break;

  case 261:
#line 814 "parser.y"
                                                { (yyval.str) = (yyvsp[0].str); }
#line 4330 "parser.tab.c"
    break;

  case 262:
#line 815 "parser.y"
                                                { (yyval.str) = (yyvsp[0].str); }
#line 4336 "parser.tab.c"
    break;

  case 263:
#line 818 "parser.y"
                                                { (yyval.var) = make_var((yyvsp[0].str)); }
#line 4342 "parser.tab.c"
    break;

  case 264:
#line 820 "parser.y"
                                                { (yyval.var) = make_var((yyvsp[0].str)); }
#line 4348 "parser.tab.c"
    break;

  case 265:
#line 823 "parser.y"
                                                { (yyval.type) = find_type_or_error((yyvsp[0].str), 0); }
#line 4354 "parser.tab.c"
    break;

  case 266:
#line 824 "parser.y"
                                                { (yyval.type) = find_type_or_error((yyvsp[0].str), 0); }
#line 4360 "parser.tab.c"
    break;

  case 268:
#line 826 "parser.y"
                                                { (yyval.type) = type_new_int(type_basic_get_type((yyvsp[0].type)), -1); }
#line 4366 "parser.tab.c"
    break;

  case 269:
#line 827 "parser.y"
                                                { (yyval.type) = type_new_int(type_basic_get_type((yyvsp[0].type)), 1); }
#line 4372 "parser.tab.c"
    break;

  case 270:
#line 828 "parser.y"
                                                { (yyval.type) = type_new_int(TYPE_BASIC_INT, 1); }
#line 4378 "parser.tab.c"
    break;

  case 271:
#line 829 "parser.y"
                                                { (yyval.type) = find_type_or_error((yyvsp[0].str), 0); }
#line 4384 "parser.tab.c"
    break;

  case 272:
#line 830 "parser.y"
                                                { (yyval.type) = find_type_or_error((yyvsp[0].str), 0); }
#line 4390 "parser.tab.c"
    break;

  case 273:
#line 831 "parser.y"
                                                { (yyval.type) = find_type_or_error((yyvsp[0].str), 0); }
#line 4396 "parser.tab.c"
    break;

  case 274:
#line 832 "parser.y"
                                                { (yyval.type) = find_type_or_error((yyvsp[0].str), 0); }
#line 4402 "parser.tab.c"
    break;

  case 275:
#line 833 "parser.y"
                                                { (yyval.type) = find_type_or_error((yyvsp[0].str), 0); }
#line 4408 "parser.tab.c"
    break;

  case 278:
#line 840 "parser.y"
                                                { (yyval.type) = type_new_int(TYPE_BASIC_INT, 0); }
#line 4414 "parser.tab.c"
    break;

  case 279:
#line 841 "parser.y"
                                                { (yyval.type) = type_new_int(TYPE_BASIC_INT16, 0); }
#line 4420 "parser.tab.c"
    break;

  case 280:
#line 842 "parser.y"
                                                { (yyval.type) = type_new_int(TYPE_BASIC_INT8, 0); }
#line 4426 "parser.tab.c"
    break;

  case 281:
#line 843 "parser.y"
                                                { (yyval.type) = type_new_int(TYPE_BASIC_LONG, 0); }
#line 4432 "parser.tab.c"
    break;

  case 282:
#line 844 "parser.y"
                                                { (yyval.type) = type_new_int(TYPE_BASIC_HYPER, 0); }
#line 4438 "parser.tab.c"
    break;

  case 283:
#line 845 "parser.y"
                                                { (yyval.type) = type_new_int(TYPE_BASIC_INT64, 0); }
#line 4444 "parser.tab.c"
    break;

  case 284:
#line 846 "parser.y"
                                                { (yyval.type) = type_new_int(TYPE_BASIC_CHAR, 0); }
#line 4450 "parser.tab.c"
    break;

  case 285:
#line 847 "parser.y"
                                                { (yyval.type) = type_new_int(TYPE_BASIC_INT32, 0); }
#line 4456 "parser.tab.c"
    break;

  case 286:
#line 848 "parser.y"
                                                { (yyval.type) = type_new_int(TYPE_BASIC_INT3264, 0); }
#line 4462 "parser.tab.c"
    break;

  case 287:
#line 851 "parser.y"
                                                { (yyval.type) = type_new_coclass((yyvsp[0].str)); }
#line 4468 "parser.tab.c"
    break;

  case 288:
#line 852 "parser.y"
                                                { (yyval.type) = find_type((yyvsp[0].str), NULL, 0);
						  if (type_get_type_detect_alias((yyval.type)) != TYPE_COCLASS)
						    error_loc("%s was not declared a coclass at %s:%d\n",
							      (yyvsp[0].str), (yyval.type)->loc_info.input_name,
							      (yyval.type)->loc_info.line_number);
						}
#line 4479 "parser.tab.c"
    break;

  case 289:
#line 860 "parser.y"
                                                { (yyval.type) = (yyvsp[0].type);
						  check_def((yyval.type));
						  (yyval.type)->attrs = check_coclass_attrs((yyvsp[0].type)->name, (yyvsp[-1].attr_list));
						}
#line 4488 "parser.tab.c"
    break;

  case 290:
#line 867 "parser.y"
                                                { (yyval.type) = type_coclass_define((yyvsp[-4].type), (yyvsp[-2].ifref_list)); }
#line 4494 "parser.tab.c"
    break;

  case 291:
#line 870 "parser.y"
                                                { (yyval.str) = (yyvsp[0].str); }
#line 4500 "parser.tab.c"
    break;

  case 292:
#line 873 "parser.y"
                                                { (yyval.ifref_list) = NULL; }
#line 4506 "parser.tab.c"
    break;

  case 293:
#line 874 "parser.y"
                                                { (yyval.ifref_list) = append_ifref( (yyvsp[-1].ifref_list), (yyvsp[0].ifref) ); }
#line 4512 "parser.tab.c"
    break;

  case 294:
#line 878 "parser.y"
                                                { (yyval.ifref) = make_ifref((yyvsp[0].type)); (yyval.ifref)->attrs = (yyvsp[-1].attr_list); }
#line 4518 "parser.tab.c"
    break;

  case 295:
#line 881 "parser.y"
                                                { (yyval.type) = get_type(TYPE_INTERFACE, (yyvsp[0].str), current_namespace, 0); }
#line 4524 "parser.tab.c"
    break;

  case 296:
#line 882 "parser.y"
                                                { (yyval.type) = get_type(TYPE_INTERFACE, (yyvsp[0].str), current_namespace, 0); }
#line 4530 "parser.tab.c"
    break;

  case 297:
#line 885 "parser.y"
                                                { attr_t *attrs;
						  (yyval.type) = (yyvsp[0].type);
						  check_def((yyval.type));
						  attrs = make_attr(ATTR_DISPINTERFACE);
						  (yyval.type)->attrs = append_attr( check_dispiface_attrs((yyvsp[0].type)->name, (yyvsp[-1].attr_list)), attrs );
						  (yyval.type)->defined = TRUE;
						}
#line 4542 "parser.tab.c"
    break;

  case 298:
#line 894 "parser.y"
                                                { (yyval.var_list) = NULL; }
#line 4548 "parser.tab.c"
    break;

  case 299:
#line 895 "parser.y"
                                                { (yyval.var_list) = append_var( (yyvsp[-2].var_list), (yyvsp[-1].var) ); }
#line 4554 "parser.tab.c"
    break;

  case 300:
#line 898 "parser.y"
                                                { (yyval.var_list) = NULL; }
#line 4560 "parser.tab.c"
    break;

  case 301:
#line 899 "parser.y"
                                                { (yyval.var_list) = append_var( (yyvsp[-2].var_list), (yyvsp[-1].var) ); }
#line 4566 "parser.tab.c"
    break;

  case 302:
#line 905 "parser.y"
                                                { (yyval.type) = (yyvsp[-4].type);
						  type_dispinterface_define((yyval.type), (yyvsp[-2].var_list), (yyvsp[-1].var_list));
						}
#line 4574 "parser.tab.c"
    break;

  case 303:
#line 909 "parser.y"
                                                { (yyval.type) = (yyvsp[-4].type);
						  type_dispinterface_define_from_iface((yyval.type), (yyvsp[-2].type));
						}
#line 4582 "parser.tab.c"
    break;

  case 304:
#line 914 "parser.y"
                                                { (yyval.type) = NULL; }
#line 4588 "parser.tab.c"
    break;

  case 305:
#line 915 "parser.y"
                                                { (yyval.type) = find_type_or_error2((yyvsp[0].str), 0); }
#line 4594 "parser.tab.c"
    break;

  case 306:
#line 918 "parser.y"
                                                { (yyval.type) = get_type(TYPE_INTERFACE, (yyvsp[0].str), current_namespace, 0); }
#line 4600 "parser.tab.c"
    break;

  case 307:
#line 919 "parser.y"
                                                { (yyval.type) = get_type(TYPE_INTERFACE, (yyvsp[0].str), current_namespace, 0); }
#line 4606 "parser.tab.c"
    break;

  case 308:
#line 922 "parser.y"
                                                { (yyval.ifinfo).interface = (yyvsp[0].type);
						  (yyval.ifinfo).old_pointer_default = pointer_default;
						  if (is_attr((yyvsp[-1].attr_list), ATTR_POINTERDEFAULT))
						    pointer_default = get_attrv((yyvsp[-1].attr_list), ATTR_POINTERDEFAULT);
						  check_def((yyvsp[0].type));
						  (yyvsp[0].type)->attrs = check_iface_attrs((yyvsp[0].type)->name, (yyvsp[-1].attr_list));
						  (yyvsp[0].type)->defined = TRUE;
						}
#line 4619 "parser.tab.c"
    break;

  case 309:
#line 933 "parser.y"
                                                { (yyval.type) = (yyvsp[-5].ifinfo).interface;
						  if((yyval.type) == (yyvsp[-4].type))
						    error_loc("Interface can't inherit from itself\n");
						  type_interface_define((yyval.type), (yyvsp[-4].type), (yyvsp[-2].stmt_list));
						  check_async_uuid((yyval.type));
						  pointer_default = (yyvsp[-5].ifinfo).old_pointer_default;
						}
#line 4631 "parser.tab.c"
    break;

  case 310:
#line 944 "parser.y"
                                                { (yyval.type) = (yyvsp[-7].ifinfo).interface;
						  type_interface_define((yyval.type), find_type_or_error2((yyvsp[-5].str), 0), (yyvsp[-2].stmt_list));
						  pointer_default = (yyvsp[-7].ifinfo).old_pointer_default;
						}
#line 4640 "parser.tab.c"
    break;

  case 311:
#line 948 "parser.y"
                                                { (yyval.type) = (yyvsp[-1].type); }
#line 4646 "parser.tab.c"
    break;

  case 312:
#line 952 "parser.y"
                                                { (yyval.type) = (yyvsp[-1].type); }
#line 4652 "parser.tab.c"
    break;

  case 313:
#line 953 "parser.y"
                                                { (yyval.type) = (yyvsp[-1].type); }
#line 4658 "parser.tab.c"
    break;

  case 314:
#line 956 "parser.y"
                                                { (yyval.type) = type_new_module((yyvsp[0].str)); }
#line 4664 "parser.tab.c"
    break;

  case 315:
#line 957 "parser.y"
                                                { (yyval.type) = type_new_module((yyvsp[0].str)); }
#line 4670 "parser.tab.c"
    break;

  case 316:
#line 960 "parser.y"
                                                { (yyval.type) = (yyvsp[0].type);
						  (yyval.type)->attrs = check_module_attrs((yyvsp[0].type)->name, (yyvsp[-1].attr_list));
						}
#line 4678 "parser.tab.c"
    break;

  case 317:
#line 966 "parser.y"
                                                { (yyval.type) = (yyvsp[-4].type);
                                                  type_module_define((yyval.type), (yyvsp[-2].stmt_list));
						}
#line 4686 "parser.tab.c"
    break;

  case 318:
#line 972 "parser.y"
                                                { (yyval.stgclass) = STG_EXTERN; }
#line 4692 "parser.tab.c"
    break;

  case 319:
#line 973 "parser.y"
                                                { (yyval.stgclass) = STG_STATIC; }
#line 4698 "parser.tab.c"
    break;

  case 320:
#line 974 "parser.y"
                                                { (yyval.stgclass) = STG_REGISTER; }
#line 4704 "parser.tab.c"
    break;

  case 321:
#line 978 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_INLINE); }
#line 4710 "parser.tab.c"
    break;

  case 322:
#line 982 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_CONST); }
#line 4716 "parser.tab.c"
    break;

  case 323:
#line 985 "parser.y"
                                                { (yyval.attr_list) = NULL; }
#line 4722 "parser.tab.c"
    break;

  case 324:
#line 986 "parser.y"
                                                { (yyval.attr_list) = append_attr((yyvsp[-1].attr_list), (yyvsp[0].attr)); }
#line 4728 "parser.tab.c"
    break;

  case 325:
#line 989 "parser.y"
                                                { (yyval.declspec) = make_decl_spec((yyvsp[-1].type), (yyvsp[0].declspec), NULL, NULL, STG_NONE); }
#line 4734 "parser.tab.c"
    break;

  case 326:
#line 991 "parser.y"
                                                { (yyval.declspec) = make_decl_spec((yyvsp[-1].type), (yyvsp[-2].declspec), (yyvsp[0].declspec), NULL, STG_NONE); }
#line 4740 "parser.tab.c"
    break;

  case 327:
#line 994 "parser.y"
                                                { (yyval.declspec) = NULL; }
#line 4746 "parser.tab.c"
    break;

  case 329:
#line 999 "parser.y"
                                                { (yyval.declspec) = make_decl_spec(NULL, (yyvsp[0].declspec), NULL, (yyvsp[-1].attr), STG_NONE); }
#line 4752 "parser.tab.c"
    break;

  case 330:
#line 1000 "parser.y"
                                                  { (yyval.declspec) = make_decl_spec(NULL, (yyvsp[0].declspec), NULL, (yyvsp[-1].attr), STG_NONE); }
#line 4758 "parser.tab.c"
    break;

  case 331:
#line 1001 "parser.y"
                                                { (yyval.declspec) = make_decl_spec(NULL, (yyvsp[0].declspec), NULL, NULL, (yyvsp[-1].stgclass)); }
#line 4764 "parser.tab.c"
    break;

  case 332:
#line 1006 "parser.y"
                                                { (yyval.declarator) = (yyvsp[0].declarator); (yyval.declarator)->type = append_chain_type((yyval.declarator)->type, type_new_pointer(pointer_default, NULL, (yyvsp[-1].attr_list))); }
#line 4770 "parser.tab.c"
    break;

  case 333:
#line 1007 "parser.y"
                                                { (yyval.declarator) = (yyvsp[0].declarator); if ((yyval.declarator)->func_type) (yyval.declarator)->func_type->attrs = append_attr((yyval.declarator)->func_type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[-1].str)));
						           else if ((yyval.declarator)->type) (yyval.declarator)->type->attrs = append_attr((yyval.declarator)->type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[-1].str))); }
#line 4777 "parser.tab.c"
    break;

  case 335:
#line 1013 "parser.y"
                                                { (yyval.declarator) = make_declarator((yyvsp[0].var)); }
#line 4783 "parser.tab.c"
    break;

  case 336:
#line 1014 "parser.y"
                                                { (yyval.declarator) = (yyvsp[-1].declarator); }
#line 4789 "parser.tab.c"
    break;

  case 337:
#line 1015 "parser.y"
                                                { (yyval.declarator) = (yyvsp[-1].declarator); (yyval.declarator)->type = append_array((yyval.declarator)->type, (yyvsp[0].expr)); }
#line 4795 "parser.tab.c"
    break;

  case 338:
#line 1016 "parser.y"
                                                { (yyval.declarator) = (yyvsp[-3].declarator);
						  (yyval.declarator)->func_type = append_chain_type((yyval.declarator)->type, type_new_function((yyvsp[-1].var_list)));
						  (yyval.declarator)->type = NULL;
						}
#line 4804 "parser.tab.c"
    break;

  case 339:
#line 1025 "parser.y"
                                                { (yyval.declarator) = (yyvsp[0].declarator); (yyval.declarator)->type = append_chain_type((yyval.declarator)->type, type_new_pointer(pointer_default, NULL, (yyvsp[-1].attr_list))); }
#line 4810 "parser.tab.c"
    break;

  case 340:
#line 1026 "parser.y"
                                                { (yyval.declarator) = (yyvsp[0].declarator); if ((yyval.declarator)->func_type) (yyval.declarator)->func_type->attrs = append_attr((yyval.declarator)->func_type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[-1].str)));
						           else if ((yyval.declarator)->type) (yyval.declarator)->type->attrs = append_attr((yyval.declarator)->type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[-1].str))); }
#line 4817 "parser.tab.c"
    break;

  case 342:
#line 1034 "parser.y"
                                                { (yyval.declarator) = (yyvsp[0].declarator); (yyval.declarator)->type = append_chain_type((yyval.declarator)->type, type_new_pointer(pointer_default, NULL, (yyvsp[-1].attr_list))); }
#line 4823 "parser.tab.c"
    break;

  case 343:
#line 1035 "parser.y"
                                                { (yyval.declarator) = (yyvsp[0].declarator); if ((yyval.declarator)->func_type) (yyval.declarator)->func_type->attrs = append_attr((yyval.declarator)->func_type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[-1].str)));
						           else if ((yyval.declarator)->type) (yyval.declarator)->type->attrs = append_attr((yyval.declarator)->type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[-1].str))); }
#line 4830 "parser.tab.c"
    break;

  case 344:
#line 1040 "parser.y"
                                                { (yyval.declarator) = make_declarator(NULL); }
#line 4836 "parser.tab.c"
    break;

  case 346:
#line 1046 "parser.y"
                                                { (yyval.declarator) = (yyvsp[-1].declarator); }
#line 4842 "parser.tab.c"
    break;

  case 347:
#line 1047 "parser.y"
                                                { (yyval.declarator) = (yyvsp[-1].declarator); (yyval.declarator)->type = append_array((yyval.declarator)->type, (yyvsp[0].expr)); }
#line 4848 "parser.tab.c"
    break;

  case 348:
#line 1048 "parser.y"
                                                { (yyval.declarator) = make_declarator(NULL); (yyval.declarator)->type = append_array((yyval.declarator)->type, (yyvsp[0].expr)); }
#line 4854 "parser.tab.c"
    break;

  case 349:
#line 1050 "parser.y"
                                                { (yyval.declarator) = make_declarator(NULL);
						  (yyval.declarator)->func_type = append_chain_type((yyval.declarator)->type, type_new_function((yyvsp[-1].var_list)));
						  (yyval.declarator)->type = NULL;
						}
#line 4863 "parser.tab.c"
    break;

  case 350:
#line 1055 "parser.y"
                                                { (yyval.declarator) = (yyvsp[-3].declarator);
						  (yyval.declarator)->func_type = append_chain_type((yyval.declarator)->type, type_new_function((yyvsp[-1].var_list)));
						  (yyval.declarator)->type = NULL;
						}
#line 4872 "parser.tab.c"
    break;

  case 351:
#line 1064 "parser.y"
                                                { (yyval.declarator) = (yyvsp[0].declarator); (yyval.declarator)->type = append_chain_type((yyval.declarator)->type, type_new_pointer(pointer_default, NULL, (yyvsp[-1].attr_list))); }
#line 4878 "parser.tab.c"
    break;

  case 352:
#line 1065 "parser.y"
                                                { (yyval.declarator) = (yyvsp[0].declarator); (yyval.declarator)->type->attrs = append_attr((yyval.declarator)->type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[-1].str))); }
#line 4884 "parser.tab.c"
    break;

  case 354:
#line 1072 "parser.y"
                                                { (yyval.declarator) = (yyvsp[0].declarator); (yyval.declarator)->type = append_chain_type((yyval.declarator)->type, type_new_pointer(pointer_default, NULL, (yyvsp[-1].attr_list))); }
#line 4890 "parser.tab.c"
    break;

  case 355:
#line 1073 "parser.y"
                                                { (yyval.declarator) = (yyvsp[0].declarator); (yyval.declarator)->type->attrs = append_attr((yyval.declarator)->type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[-1].str))); }
#line 4896 "parser.tab.c"
    break;

  case 356:
#line 1077 "parser.y"
                                                { (yyval.declarator) = make_declarator(NULL); }
#line 4902 "parser.tab.c"
    break;

  case 358:
#line 1085 "parser.y"
                                                { (yyval.declarator) = make_declarator((yyvsp[0].var)); }
#line 4908 "parser.tab.c"
    break;

  case 359:
#line 1086 "parser.y"
                                                { (yyval.declarator) = (yyvsp[-1].declarator); }
#line 4914 "parser.tab.c"
    break;

  case 360:
#line 1087 "parser.y"
                                                { (yyval.declarator) = (yyvsp[-1].declarator); (yyval.declarator)->type = append_array((yyval.declarator)->type, (yyvsp[0].expr)); }
#line 4920 "parser.tab.c"
    break;

  case 361:
#line 1088 "parser.y"
                                                { (yyval.declarator) = make_declarator(NULL); (yyval.declarator)->type = append_array((yyval.declarator)->type, (yyvsp[0].expr)); }
#line 4926 "parser.tab.c"
    break;

  case 362:
#line 1090 "parser.y"
                                                { (yyval.declarator) = make_declarator(NULL);
						  (yyval.declarator)->func_type = append_chain_type((yyval.declarator)->type, type_new_function((yyvsp[-1].var_list)));
						  (yyval.declarator)->type = NULL;
						}
#line 4935 "parser.tab.c"
    break;

  case 363:
#line 1095 "parser.y"
                                                { (yyval.declarator) = (yyvsp[-3].declarator);
						  (yyval.declarator)->func_type = append_chain_type((yyval.declarator)->type, type_new_function((yyvsp[-1].var_list)));
						  (yyval.declarator)->type = NULL;
						}
#line 4944 "parser.tab.c"
    break;

  case 364:
#line 1102 "parser.y"
                                                { (yyval.declarator_list) = append_declarator( NULL, (yyvsp[0].declarator) ); }
#line 4950 "parser.tab.c"
    break;

  case 365:
#line 1103 "parser.y"
                                                { (yyval.declarator_list) = append_declarator( (yyvsp[-2].declarator_list), (yyvsp[0].declarator) ); }
#line 4956 "parser.tab.c"
    break;

  case 366:
#line 1106 "parser.y"
                                                { (yyval.expr) = NULL; }
#line 4962 "parser.tab.c"
    break;

  case 367:
#line 1107 "parser.y"
                                                { (yyval.expr) = (yyvsp[0].expr); }
#line 4968 "parser.tab.c"
    break;

  case 368:
#line 1110 "parser.y"
                                                { (yyval.declarator) = (yyvsp[-1].declarator); (yyval.declarator)->bits = (yyvsp[0].expr);
						  if (!(yyval.declarator)->bits && !(yyval.declarator)->var->name)
						    error_loc("unnamed fields are not allowed\n");
						}
#line 4977 "parser.tab.c"
    break;

  case 369:
#line 1117 "parser.y"
                                                { (yyval.declarator_list) = append_declarator( NULL, (yyvsp[0].declarator) ); }
#line 4983 "parser.tab.c"
    break;

  case 370:
#line 1119 "parser.y"
                                                { (yyval.declarator_list) = append_declarator( (yyvsp[-2].declarator_list), (yyvsp[0].declarator) ); }
#line 4989 "parser.tab.c"
    break;

  case 371:
#line 1123 "parser.y"
                                                { (yyval.declarator) = (yyvsp[0].declarator); }
#line 4995 "parser.tab.c"
    break;

  case 372:
#line 1124 "parser.y"
                                                { (yyval.declarator) = (yyvsp[-2].declarator); (yyvsp[-2].declarator)->var->eval = (yyvsp[0].expr); }
#line 5001 "parser.tab.c"
    break;

  case 373:
#line 1128 "parser.y"
                                                { (yyval.num) = THREADING_APARTMENT; }
#line 5007 "parser.tab.c"
    break;

  case 374:
#line 1129 "parser.y"
                                                { (yyval.num) = THREADING_NEUTRAL; }
#line 5013 "parser.tab.c"
    break;

  case 375:
#line 1130 "parser.y"
                                                { (yyval.num) = THREADING_SINGLE; }
#line 5019 "parser.tab.c"
    break;

  case 376:
#line 1131 "parser.y"
                                                { (yyval.num) = THREADING_FREE; }
#line 5025 "parser.tab.c"
    break;

  case 377:
#line 1132 "parser.y"
                                                { (yyval.num) = THREADING_BOTH; }
#line 5031 "parser.tab.c"
    break;

  case 378:
#line 1136 "parser.y"
                                                { (yyval.num) = FC_RP; }
#line 5037 "parser.tab.c"
    break;

  case 379:
#line 1137 "parser.y"
                                                { (yyval.num) = FC_UP; }
#line 5043 "parser.tab.c"
    break;

  case 380:
#line 1138 "parser.y"
                                                { (yyval.num) = FC_FP; }
#line 5049 "parser.tab.c"
    break;

  case 381:
#line 1141 "parser.y"
                                                { (yyval.type) = type_new_struct((yyvsp[-3].str), current_namespace, TRUE, (yyvsp[-1].var_list)); }
#line 5055 "parser.tab.c"
    break;

  case 382:
#line 1144 "parser.y"
                                                { (yyval.type) = type_new_void(); }
#line 5061 "parser.tab.c"
    break;

  case 383:
#line 1145 "parser.y"
                                                { (yyval.type) = find_type_or_error((yyvsp[0].str), 0); }
#line 5067 "parser.tab.c"
    break;

  case 384:
#line 1146 "parser.y"
                                                { (yyval.type) = (yyvsp[0].type); }
#line 5073 "parser.tab.c"
    break;

  case 385:
#line 1147 "parser.y"
                                                { (yyval.type) = (yyvsp[0].type); }
#line 5079 "parser.tab.c"
    break;

  case 386:
#line 1148 "parser.y"
                                                { (yyval.type) = type_new_enum((yyvsp[0].str), current_namespace, FALSE, NULL); }
#line 5085 "parser.tab.c"
    break;

  case 387:
#line 1149 "parser.y"
                                                { (yyval.type) = (yyvsp[0].type); }
#line 5091 "parser.tab.c"
    break;

  case 388:
#line 1150 "parser.y"
                                                { (yyval.type) = type_new_struct((yyvsp[0].str), current_namespace, FALSE, NULL); }
#line 5097 "parser.tab.c"
    break;

  case 389:
#line 1151 "parser.y"
                                                { (yyval.type) = (yyvsp[0].type); }
#line 5103 "parser.tab.c"
    break;

  case 390:
#line 1152 "parser.y"
                                                { (yyval.type) = type_new_nonencapsulated_union((yyvsp[0].str), FALSE, NULL); }
#line 5109 "parser.tab.c"
    break;

  case 391:
#line 1153 "parser.y"
                                                { (yyval.type) = make_safearray((yyvsp[-1].type)); }
#line 5115 "parser.tab.c"
    break;

  case 392:
#line 1157 "parser.y"
                                                { (yyvsp[-4].attr_list) = append_attribs((yyvsp[-4].attr_list), (yyvsp[-2].attr_list));
						  reg_typedefs((yyvsp[-1].declspec), (yyvsp[0].declarator_list), check_typedef_attrs((yyvsp[-4].attr_list)));
						  (yyval.statement) = make_statement_typedef((yyvsp[0].declarator_list));
						}
#line 5124 "parser.tab.c"
    break;

  case 393:
#line 1164 "parser.y"
                                                { (yyval.type) = type_new_nonencapsulated_union((yyvsp[-3].str), TRUE, (yyvsp[-1].var_list)); }
#line 5130 "parser.tab.c"
    break;

  case 394:
#line 1167 "parser.y"
                                                { (yyval.type) = type_new_encapsulated_union((yyvsp[-8].str), (yyvsp[-5].var), (yyvsp[-3].var), (yyvsp[-1].var_list)); }
#line 5136 "parser.tab.c"
    break;

  case 395:
#line 1171 "parser.y"
                                                { (yyval.num) = MAKEVERSION((yyvsp[0].num), 0); }
#line 5142 "parser.tab.c"
    break;

  case 396:
#line 1172 "parser.y"
                                                { (yyval.num) = MAKEVERSION((yyvsp[-2].num), (yyvsp[0].num)); }
#line 5148 "parser.tab.c"
    break;

  case 397:
#line 1173 "parser.y"
                                                { (yyval.num) = (yyvsp[0].num); }
#line 5154 "parser.tab.c"
    break;

  case 402:
#line 1188 "parser.y"
                                                { type_t *type = find_type_or_error((yyvsp[-1].str), 0);
                                                  type->attrs = append_attr_list(type->attrs, (yyvsp[-2].attr_list));
                                                }
#line 5162 "parser.tab.c"
    break;

  case 403:
#line 1195 "parser.y"
                                                {  type_t *iface = find_type_or_error2((yyvsp[-3].str), 0);
                                                   if (type_get_type(iface) != TYPE_INTERFACE)
                                                       error_loc("%s is not an interface\n", iface->name);
                                                   iface->attrs = append_attr_list(iface->attrs, (yyvsp[-5].attr_list));
                                                }
#line 5172 "parser.tab.c"
    break;

  case 404:
#line 1203 "parser.y"
                                                { (yyval.attr_list) = NULL; }
#line 5178 "parser.tab.c"
    break;

  case 405:
#line 1204 "parser.y"
                                                { (yyval.attr_list) = (yyvsp[-1].attr_list); }
#line 5184 "parser.tab.c"
    break;

  case 406:
#line 1208 "parser.y"
                                                { (yyval.attr_list) = append_attr(NULL, (yyvsp[0].attr)); }
#line 5190 "parser.tab.c"
    break;

  case 407:
#line 1209 "parser.y"
                                                { (yyval.attr_list) = append_attr((yyvsp[-2].attr_list), (yyvsp[0].attr)); }
#line 5196 "parser.tab.c"
    break;

  case 408:
#line 1214 "parser.y"
                                                { (yyval.attr) = make_attrv(ATTR_ALLOCATE, (yyvsp[-1].num)); }
#line 5202 "parser.tab.c"
    break;

  case 409:
#line 1215 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_ENCODE); }
#line 5208 "parser.tab.c"
    break;

  case 410:
#line 1216 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_DECODE); }
#line 5214 "parser.tab.c"
    break;

  case 411:
#line 1217 "parser.y"
                                                { (yyval.attr) = make_attr(ATTR_EXPLICIT_HANDLE); }
#line 5220 "parser.tab.c"
    break;

  case 412:
#line 1221 "parser.y"
                                                { (yyval.num) = (yyvsp[0].num); }
#line 5226 "parser.tab.c"
    break;

  case 413:
#line 1223 "parser.y"
                                                { (yyval.num) = (yyvsp[-2].num) | (yyvsp[0].num); }
#line 5232 "parser.tab.c"
    break;

  case 414:
#line 1227 "parser.y"
                                                { (yyval.num) = FC_DONT_FREE; }
#line 5238 "parser.tab.c"
    break;

  case 415:
#line 1228 "parser.y"
                                                { (yyval.num) = 0; }
#line 5244 "parser.tab.c"
    break;

  case 416:
#line 1229 "parser.y"
                                                { (yyval.num) = FC_ALLOCATE_ALL_NODES; }
#line 5250 "parser.tab.c"
    break;

  case 417:
#line 1230 "parser.y"
                                                { (yyval.num) = 0; }
#line 5256 "parser.tab.c"
    break;


#line 5260 "parser.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *, YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
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

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

  /* Do not reclaim the symbols of the rule whose action triggered
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
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
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

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


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


#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif


/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[+*yyssp], yyvsp);
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
  return yyresult;
}
#line 1232 "parser.y"


static void decl_builtin_basic(const char *name, enum type_basic_type type)
{
  type_t *t = type_new_basic(type);
  reg_type(t, name, NULL, 0);
}

static void decl_builtin_alias(const char *name, type_t *t)
{
  reg_type(type_new_alias(t, name), name, NULL, 0);
}

void init_types(void)
{
  decl_builtin_basic("byte", TYPE_BASIC_BYTE);
  decl_builtin_basic("wchar_t", TYPE_BASIC_WCHAR);
  decl_builtin_basic("float", TYPE_BASIC_FLOAT);
  decl_builtin_basic("double", TYPE_BASIC_DOUBLE);
  decl_builtin_basic("error_status_t", TYPE_BASIC_ERROR_STATUS_T);
  decl_builtin_basic("handle_t", TYPE_BASIC_HANDLE);
  decl_builtin_alias("boolean", type_new_basic(TYPE_BASIC_BYTE));
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

typedef int (*map_attrs_filter_t)(attr_list_t*,const attr_t*);

static attr_list_t *map_attrs(const attr_list_t *list, map_attrs_filter_t filter)
{
  attr_list_t *new_list;
  const attr_t *attr;
  attr_t *new_attr;

  if (!list) return NULL;

  new_list = xmalloc( sizeof(*list) );
  list_init( new_list );
  LIST_FOR_EACH_ENTRY(attr, list, const attr_t, entry)
  {
    if (filter && !filter(new_list, attr)) continue;
    new_attr = xmalloc(sizeof(*new_attr));
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
    attrs = map_attrs(type->attrs, NULL);
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

static attr_t *make_attrv(enum attr_type type, unsigned int val)
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

static type_t *append_array(type_t *chain, expr_t *expr)
{
    type_t *array;

    if (!expr)
        return chain;

    /* An array is always a reference pointer unless explicitly marked otherwise
     * (regardless of what the default pointer attribute is). */
    array = type_new_array(NULL, NULL, FALSE, expr->is_const ? expr->cval : 0,
            expr->is_const ? NULL : expr, NULL, FC_RP);

    return append_chain_type(chain, array);
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

static void type_function_add_head_arg(type_t *type, var_t *arg)
{
    if (!type->details.function->args)
    {
        type->details.function->args = xmalloc( sizeof(*type->details.function->args) );
        list_init( type->details.function->args );
    }
    list_add_head( type->details.function->args, &arg->entry );
}

static int is_allowed_range_type(const type_t *type)
{
    switch (type_get_type(type))
    {
    case TYPE_ENUM:
        return TRUE;
    case TYPE_BASIC:
        switch (type_basic_get_type(type))
        {
        case TYPE_BASIC_INT8:
        case TYPE_BASIC_INT16:
        case TYPE_BASIC_INT32:
        case TYPE_BASIC_INT64:
        case TYPE_BASIC_INT:
        case TYPE_BASIC_INT3264:
        case TYPE_BASIC_LONG:
        case TYPE_BASIC_BYTE:
        case TYPE_BASIC_CHAR:
        case TYPE_BASIC_WCHAR:
        case TYPE_BASIC_HYPER:
            return TRUE;
        case TYPE_BASIC_FLOAT:
        case TYPE_BASIC_DOUBLE:
        case TYPE_BASIC_ERROR_STATUS_T:
        case TYPE_BASIC_HANDLE:
            return FALSE;
        }
        return FALSE;
    default:
        return FALSE;
    }
}

static type_t *get_array_or_ptr_ref(type_t *type)
{
    if (is_ptr(type))
        return type_pointer_get_ref(type);
    else if (is_array(type))
        return type_array_get_element(type);
    return NULL;
}

static type_t *append_chain_type(type_t *chain, type_t *type)
{
    type_t *chain_type;

    if (!chain)
        return type;
    for (chain_type = chain; get_array_or_ptr_ref(chain_type); chain_type = get_array_or_ptr_ref(chain_type))
        ;

    if (is_ptr(chain_type))
        chain_type->details.pointer.ref = type;
    else if (is_array(chain_type))
        chain_type->details.array.elem = type;
    else
        assert(0);

    return chain;
}

static warning_list_t *append_warning(warning_list_t *list, int num)
{
    warning_t *entry;

    if(!list)
    {
        list = xmalloc( sizeof(*list) );
        list_init( list );
    }
    entry = xmalloc( sizeof(*entry) );
    entry->num = num;
    list_add_tail( list, &entry->entry );
    return list;
}

static var_t *declare_var(attr_list_t *attrs, decl_spec_t *decl_spec, const declarator_t *decl,
                       int top)
{
  var_t *v = decl->var;
  expr_list_t *sizes = get_attrp(attrs, ATTR_SIZEIS);
  expr_list_t *lengs = get_attrp(attrs, ATTR_LENGTHIS);
  expr_t *dim;
  type_t **ptype;
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
  v->type = append_chain_type(decl ? decl->type : NULL, type);
  v->stgclass = decl_spec->stgclass;
  v->attrs = attrs;

  /* check for pointer attribute being applied to non-pointer, non-array
   * type */
  if (!is_array(v->type))
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
        ptr = type_alias_get_aliasee(ptr);
      else
        break;
    }
    if (is_ptr(ptr))
    {
      if (ptr_attr && ptr_attr != FC_UP &&
          type_get_type(type_pointer_get_ref(ptr)) == TYPE_INTERFACE)
          warning_loc_info(&v->loc_info,
                           "%s: pointer attribute applied to interface "
                           "pointer type has no effect\n", v->name);
      if (!ptr_attr && top && (*pt)->details.pointer.def_fc != FC_RP)
      {
        /* FIXME: this is a horrible hack to cope with the issue that we
         * store an offset to the typeformat string in the type object, but
         * two typeformat strings may be written depending on whether the
         * pointer is a toplevel parameter or not */
        *pt = duptype(*pt, 1);
      }
    }
    else if (ptr_attr)
       error_loc("%s: pointer attribute applied to non-pointer type\n", v->name);
  }

  if (is_attr(v->attrs, ATTR_STRING))
  {
    type_t *t = type;

    if (!is_ptr(v->type) && !is_array(v->type))
      error_loc("'%s': [string] attribute applied to non-pointer, non-array type\n",
                v->name);

    for (;;)
    {
        if (is_ptr(t))
            t = type_pointer_get_ref(t);
        else if (is_array(t))
            t = type_array_get_element(t);
        else
            break;
    }

    if (type_get_type(t) != TYPE_BASIC &&
        (get_basic_fc(t) != FC_CHAR &&
         get_basic_fc(t) != FC_BYTE &&
         get_basic_fc(t) != FC_WCHAR))
    {
      error_loc("'%s': [string] attribute is only valid on 'char', 'byte', or 'wchar_t' pointers and arrays\n",
                v->name);
    }
  }

  if (is_attr(v->attrs, ATTR_V1ENUM))
  {
    if (type_get_type_detect_alias(v->type) != TYPE_ENUM)
      error_loc("'%s': [v1_enum] attribute applied to non-enum type\n", v->name);
  }

  if (is_attr(v->attrs, ATTR_RANGE) && !is_allowed_range_type(v->type))
    error_loc("'%s': [range] attribute applied to non-integer type\n",
              v->name);

  ptype = &v->type;
  if (sizes) LIST_FOR_EACH_ENTRY(dim, sizes, expr_t, entry)
  {
    if (dim->type != EXPR_VOID)
    {
      if (is_array(*ptype))
      {
        if (!type_array_get_conformance(*ptype) ||
            type_array_get_conformance(*ptype)->type != EXPR_VOID)
          error_loc("%s: cannot specify size_is for an already sized array\n", v->name);
        else
          *ptype = type_new_array((*ptype)->name,
                                  type_array_get_element(*ptype), FALSE,
                                  0, dim, NULL, 0);
      }
      else if (is_ptr(*ptype))
        *ptype = type_new_array((*ptype)->name, type_pointer_get_ref(*ptype), TRUE,
                                0, dim, NULL, pointer_default);
      else
        error_loc("%s: size_is attribute applied to illegal type\n", v->name);
    }

    if (is_ptr(*ptype))
      ptype = &(*ptype)->details.pointer.ref;
    else if (is_array(*ptype))
      ptype = &(*ptype)->details.array.elem;
    else
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
                                type_array_is_decl_as_ptr(*ptype),
                                type_array_get_dim(*ptype),
                                type_array_get_conformance(*ptype),
                                dim, type_array_get_ptr_default_fc(*ptype));
      }
      else
        error_loc("%s: length_is attribute applied to illegal type\n", v->name);
    }

    if (is_ptr(*ptype))
      ptype = &(*ptype)->details.pointer.ref;
    else if (is_array(*ptype))
      ptype = &(*ptype)->details.array.elem;
    else
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
    assert(type_get_type_detect_alias(ft) == TYPE_FUNCTION);
    ft->details.function->retval = make_var(xstrdup("_RetVal"));
    ft->details.function->retval->type = return_type;
    /* move calling convention attribute, if present, from pointer nodes to
     * function node */
    for (t = v->type; is_ptr(t); t = type_pointer_get_ref(t))
      ft->attrs = move_attr(ft->attrs, t->attrs, ATTR_CALLCONV);
  }
  else
  {
    type_t *t;
    for (t = v->type; is_ptr(t); t = type_pointer_get_ref(t))
      if (is_attr(t->attrs, ATTR_CALLCONV))
        error_loc("calling convention applied to non-function-pointer type\n");
  }

  if (decl->bits)
    v->type = type_new_bitfield(v->type, decl->bits);

  return v;
}

static var_list_t *set_var_types(attr_list_t *attrs, decl_spec_t *decl_spec, declarator_list_t *decls)
{
  declarator_t *decl, *next;
  var_list_t *var_list = NULL;

  LIST_FOR_EACH_ENTRY_SAFE( decl, next, decls, declarator_t, entry )
  {
    var_t *var = declare_var(attrs, decl_spec, decl, 0);
    var_list = append_var(var_list, var);
    free(decl);
  }
  free(decl_spec);
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

var_list_t *append_var(var_list_t *list, var_t *var)
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

var_t *make_var(char *name)
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

static var_t *copy_var(var_t *src, char *name, map_attrs_filter_t attr_filter)
{
  var_t *v = xmalloc(sizeof(var_t));
  v->name = name;
  v->type = src->type;
  v->attrs = map_attrs(src->attrs, attr_filter);
  v->eval = src->eval;
  v->stgclass = src->stgclass;
  v->loc_info = src->loc_info;
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
  d->var = var ? var : make_var(NULL);
  d->type = NULL;
  d->func_type = NULL;
  d->bits = NULL;
  return d;
}

static type_t *make_safearray(type_t *type)
{
  return type_new_array(NULL, type_new_alias(type, "SAFEARRAY"), TRUE, 0,
                        NULL, NULL, FC_RP);
}

static typelib_t *make_library(const char *name, const attr_list_t *attrs)
{
    typelib_t *typelib = xmalloc(sizeof(*typelib));
    typelib->name = xstrdup(name);
    typelib->attrs = attrs;
    list_init( &typelib->importlibs );
    return typelib;
}

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

static struct namespace *find_sub_namespace(struct namespace *namespace, const char *name)
{
  struct namespace *cur;

  LIST_FOR_EACH_ENTRY(cur, &namespace->children, struct namespace, entry) {
    if(!strcmp(cur->name, name))
      return cur;
  }

  return NULL;
}

static void push_namespace(const char *name)
{
  struct namespace *namespace;

  namespace = find_sub_namespace(current_namespace, name);
  if(!namespace) {
    namespace = xmalloc(sizeof(*namespace));
    namespace->name = xstrdup(name);
    namespace->parent = current_namespace;
    list_add_tail(&current_namespace->children, &namespace->entry);
    list_init(&namespace->children);
    memset(namespace->type_hash, 0, sizeof(namespace->type_hash));
  }

  current_namespace = namespace;
}

static void pop_namespace(const char *name)
{
  assert(!strcmp(current_namespace->name, name) && current_namespace->parent);
  current_namespace = current_namespace->parent;
}

struct rtype {
  const char *name;
  type_t *type;
  int t;
  struct rtype *next;
};

type_t *reg_type(type_t *type, const char *name, struct namespace *namespace, int t)
{
  struct rtype *nt;
  int hash;
  if (!name) {
    error_loc("registering named type without name\n");
    return type;
  }
  if (!namespace)
    namespace = &global_namespace;
  hash = hash_ident(name);
  nt = xmalloc(sizeof(struct rtype));
  nt->name = name;
  if (is_global_namespace(namespace))
    type->c_name = name;
  else
    type->c_name = format_namespace(namespace, "__x_", "_C", name);
  nt->type = type;
  nt->t = t;
  nt->next = namespace->type_hash[hash];
  namespace->type_hash[hash] = nt;
  if ((t == tsSTRUCT || t == tsUNION))
    fix_incomplete_types(type);
  return type;
}

static int is_incomplete(const type_t *t)
{
  return !t->defined &&
    (type_get_type_detect_alias(t) == TYPE_STRUCT ||
     type_get_type_detect_alias(t) == TYPE_UNION ||
     type_get_type_detect_alias(t) == TYPE_ENCAPSULATED_UNION);
}

void add_incomplete(type_t *t)
{
  struct typenode *tn = xmalloc(sizeof *tn);
  tn->type = t;
  list_add_tail(&incomplete_types, &tn->entry);
}

static void fix_type(type_t *t)
{
  if (type_is_alias(t) && is_incomplete(t)) {
    type_t *ot = type_alias_get_aliasee(t);
    fix_type(ot);
    if (type_get_type_detect_alias(ot) == TYPE_STRUCT ||
        type_get_type_detect_alias(ot) == TYPE_UNION ||
        type_get_type_detect_alias(ot) == TYPE_ENCAPSULATED_UNION)
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
    if (type_is_equal(complete_type, tn->type))
    {
      tn->type->details.structure = complete_type->details.structure;
      list_remove(&tn->entry);
      free(tn);
    }
  }
}

static type_t *reg_typedefs(decl_spec_t *decl_spec, declarator_list_t *decls, attr_list_t *attrs)
{
  const declarator_t *decl;
  type_t *type = decl_spec->type;

  if (is_attr(attrs, ATTR_UUID) && !is_attr(attrs, ATTR_PUBLIC))
    attrs = append_attr( attrs, make_attr(ATTR_PUBLIC) );

  /* We must generate names for tagless enum, struct or union.
     Typedef-ing a tagless enum, struct or union means we want the typedef
     to be included in a library hence the public attribute.  */
  if (type_get_type_detect_alias(type) == TYPE_ENUM ||
      type_get_type_detect_alias(type) == TYPE_STRUCT ||
      type_get_type_detect_alias(type) == TYPE_UNION ||
      type_get_type_detect_alias(type) == TYPE_ENCAPSULATED_UNION)
  {
    if (!type->name)
      type->name = gen_name();

    /* replace existing attributes when generating a typelib */
    if (do_typelib)
        type->attrs = attrs;
  }

#ifdef __REACTOS__ /* r53187 / 5bf224e */
  /* Append the SWITCHTYPE attribute to a non-encapsulated union if it does not already have it.  */
  if (type_get_type_detect_alias(type) == TYPE_UNION &&
      is_attr(attrs, ATTR_SWITCHTYPE) &&
      !is_attr(type->attrs, ATTR_SWITCHTYPE))
    type->attrs = append_attr(type->attrs, make_attrp(ATTR_SWITCHTYPE, get_attrp(attrs, ATTR_SWITCHTYPE)));
#endif

  LIST_FOR_EACH_ENTRY( decl, decls, const declarator_t, entry )
  {

    if (decl->var->name) {
      type_t *cur;
      var_t *name;

      cur = find_type(decl->var->name, current_namespace, 0);

      /*
       * MIDL allows shadowing types that are declared in imported files.
       * We don't throw an error in this case and instead add a new type
       * (which is earlier on the list in hash table, so it will be used
       * instead of shadowed type).
       *
       * FIXME: We may consider string separated type tables for each input
       *        for cleaner solution.
       */
      if (cur && input_name == cur->loc_info.input_name)
          error_loc("%s: redefinition error; original definition was at %s:%d\n",
                    cur->name, cur->loc_info.input_name,
                    cur->loc_info.line_number);

      name = declare_var(attrs, decl_spec, decl, 0);
      cur = type_new_alias(name->type, name->name);
      cur->attrs = attrs;

      if (is_incomplete(cur))
        add_incomplete(cur);
      reg_type(cur, cur->name, current_namespace, 0);
    }
  }
  return type;
}

type_t *find_type(const char *name, struct namespace *namespace, int t)
{
  struct rtype *cur;

  if(namespace && namespace != &global_namespace) {
    for(cur = namespace->type_hash[hash_ident(name)]; cur; cur = cur->next) {
      if(cur->t == t && !strcmp(cur->name, name))
        return cur->type;
    }
  }
  for(cur = global_namespace.type_hash[hash_ident(name)]; cur; cur = cur->next) {
    if(cur->t == t && !strcmp(cur->name, name))
      return cur->type;
  }
  return NULL;
}

static type_t *find_type_or_error(const char *name, int t)
{
  type_t *type = find_type(name, NULL, t);
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
  return find_type(name, current_namespace, 0) != NULL;
}

type_t *get_type(enum type_type type, char *name, struct namespace *namespace, int t)
{
  type_t *tp;
  if (!namespace)
    namespace = &global_namespace;
  if (name) {
    tp = find_type(name, namespace, t);
    if (tp) {
      free(name);
      return tp;
    }
  }
  tp = make_type(type);
  tp->name = name;
  tp->namespace = namespace;
  if (!name) return tp;
  return reg_type(tp, name, namespace, t);
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
    char *dst = dup_basename(input_idl_name, ".idl");
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
    unsigned int on_struct : 2;
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
    /* ATTR_ALLOCATE */            { 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "allocate" },
    /* ATTR_ANNOTATION */          { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "annotation" },
    /* ATTR_APPOBJECT */           { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "appobject" },
    /* ATTR_ASYNC */               { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "async" },
    /* ATTR_ASYNCUUID */           { 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, "async_uuid" },
    /* ATTR_AUTO_HANDLE */         { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "auto_handle" },
    /* ATTR_BINDABLE */            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "bindable" },
    /* ATTR_BROADCAST */           { 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "broadcast" },
    /* ATTR_CALLAS */              { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "call_as" },
    /* ATTR_CALLCONV */            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL },
    /* ATTR_CASE */                { 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, "case" },
    /* ATTR_CODE */                { 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "code" },
    /* ATTR_COMMSTATUS */          { 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "comm_status" },
    /* ATTR_CONST */               { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "const" },
    /* ATTR_CONTEXTHANDLE */       { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "context_handle" },
    /* ATTR_CONTROL */             { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, "control" },
    /* ATTR_DECODE */              { 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "decode" },
    /* ATTR_DEFAULT */             { 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, "default" },
    /* ATTR_DEFAULTBIND */         { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "defaultbind" },
    /* ATTR_DEFAULTCOLLELEM */     { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "defaultcollelem" },
    /* ATTR_DEFAULTVALUE */        { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "defaultvalue" },
    /* ATTR_DEFAULTVTABLE */       { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "defaultvtable" },
 /* ATTR_DISABLECONSISTENCYCHECK */{ 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "disable_consistency_check" },
    /* ATTR_DISPINTERFACE */       { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL },
    /* ATTR_DISPLAYBIND */         { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "displaybind" },
    /* ATTR_DLLNAME */             { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, "dllname" },
    /* ATTR_DUAL */                { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "dual" },
    /* ATTR_ENABLEALLOCATE */      { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "enable_allocate" },
    /* ATTR_ENCODE */              { 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "encode" },
    /* ATTR_ENDPOINT */            { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "endpoint" },
    /* ATTR_ENTRY */               { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "entry" },
    /* ATTR_EXPLICIT_HANDLE */     { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "explicit_handle" },
    /* ATTR_FAULTSTATUS */         { 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "fault_status" },
    /* ATTR_FORCEALLOCATE */       { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "force_allocate" },
    /* ATTR_HANDLE */              { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "handle" },
    /* ATTR_HELPCONTEXT */         { 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, "helpcontext" },
    /* ATTR_HELPFILE */            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, "helpfile" },
    /* ATTR_HELPSTRING */          { 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, "helpstring" },
    /* ATTR_HELPSTRINGCONTEXT */   { 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, "helpstringcontext" },
    /* ATTR_HELPSTRINGDLL */       { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, "helpstringdll" },
    /* ATTR_HIDDEN */              { 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, "hidden" },
    /* ATTR_ID */                  { 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, "id" },
    /* ATTR_IDEMPOTENT */          { 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "idempotent" },
    /* ATTR_IGNORE */              { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, "ignore" },
    /* ATTR_IIDIS */               { 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, "iid_is" },
    /* ATTR_IMMEDIATEBIND */       { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "immediatebind" },
    /* ATTR_IMPLICIT_HANDLE */     { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "implicit_handle" },
    /* ATTR_IN */                  { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "in" },
    /* ATTR_INLINE */              { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "inline" },
    /* ATTR_INPUTSYNC */           { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "inputsync" },
    /* ATTR_LENGTHIS */            { 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, "length_is" },
    /* ATTR_LIBLCID */             { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, "lcid" },
    /* ATTR_LICENSED */            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "licensed" },
    /* ATTR_LOCAL */               { 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "local" },
    /* ATTR_MAYBE */               { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "maybe" },
    /* ATTR_MESSAGE */             { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "message" },
    /* ATTR_NOCODE */              { 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "nocode" },
    /* ATTR_NONBROWSABLE */        { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "nonbrowsable" },
    /* ATTR_NONCREATABLE */        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "noncreatable" },
    /* ATTR_NONEXTENSIBLE */       { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "nonextensible" },
    /* ATTR_NOTIFY */              { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "notify" },
    /* ATTR_NOTIFYFLAG */          { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "notify_flag" },
    /* ATTR_OBJECT */              { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "object" },
    /* ATTR_ODL */                 { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, "odl" },
    /* ATTR_OLEAUTOMATION */       { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "oleautomation" },
    /* ATTR_OPTIMIZE */            { 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "optimize" },
    /* ATTR_OPTIONAL */            { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "optional" },
    /* ATTR_OUT */                 { 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "out" },
    /* ATTR_PARAMLCID */           { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "lcid" },
    /* ATTR_PARTIALIGNORE */       { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "partial_ignore" },
    /* ATTR_POINTERDEFAULT */      { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "pointer_default" },
    /* ATTR_POINTERTYPE */         { 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, "ref, unique or ptr" },
    /* ATTR_PROGID */              { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "progid" },
    /* ATTR_PROPGET */             { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "propget" },
    /* ATTR_PROPPUT */             { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "propput" },
    /* ATTR_PROPPUTREF */          { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "propputref" },
    /* ATTR_PROXY */               { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "proxy" },
    /* ATTR_PUBLIC */              { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "public" },
    /* ATTR_RANGE */               { 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, "range" },
    /* ATTR_READONLY */            { 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, "readonly" },
    /* ATTR_REPRESENTAS */         { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "represent_as" },
    /* ATTR_REQUESTEDIT */         { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "requestedit" },
    /* ATTR_RESTRICTED */          { 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, "restricted" },
    /* ATTR_RETVAL */              { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "retval" },
    /* ATTR_SIZEIS */              { 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, "size_is" },
    /* ATTR_SOURCE */              { 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, "source" },
    /* ATTR_STRICTCONTEXTHANDLE */ { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "strict_context_handle" },
    /* ATTR_STRING */              { 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, "string" },
    /* ATTR_SWITCHIS */            { 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, "switch_is" },
    /* ATTR_SWITCHTYPE */          { 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, "switch_type" },
    /* ATTR_THREADING */           { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "threading" },
    /* ATTR_TRANSMITAS */          { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "transmit_as" },
    /* ATTR_UIDEFAULT */           { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "uidefault" },
    /* ATTR_USESGETLASTERROR */    { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "usesgetlasterror" },
    /* ATTR_USERMARSHAL */         { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "user_marshal" },
    /* ATTR_UUID */                { 1, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, "uuid" },
    /* ATTR_V1ENUM */              { 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, "v1_enum" },
    /* ATTR_VARARG */              { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "vararg" },
    /* ATTR_VERSION */             { 1, 0, 1, 0, 0, 1, 1, 2, 0, 0, 1, 0, 0, 1, "version" },
    /* ATTR_VIPROGID */            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "vi_progid" },
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
    if (attr->type == ATTR_IMPLICIT_HANDLE)
    {
        const var_t *var = attr->u.pval;
        if (type_get_type( var->type) == TYPE_BASIC &&
            type_basic_get_type( var->type ) == TYPE_BASIC_HANDLE)
            continue;
        if (is_aliaschain_attr( var->type, ATTR_HANDLE ))
            continue;
      error_loc("attribute %s requires a handle type in interface %s\n",
                allowed_attr[attr->type].display_name, name);
    }
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

static void check_arg_attrs(const var_t *arg)
{
  const attr_t *attr;

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
  int mask = winrt_mode ? 3 : 1;
  const attr_t *attr;
  if (!attrs) return attrs;
  LIST_FOR_EACH_ENTRY(attr, attrs, const attr_t, entry)
  {
    if (!(allowed_attr[attr->type].on_struct & mask))
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
    switch (type_get_type(type))
    {
    case TYPE_ENUM:
        return TRUE;
    case TYPE_BASIC:
        switch (type_basic_get_type(type))
        {
        case TYPE_BASIC_INT8:
        case TYPE_BASIC_INT16:
        case TYPE_BASIC_INT32:
        case TYPE_BASIC_INT64:
        case TYPE_BASIC_INT:
        case TYPE_BASIC_LONG:
        case TYPE_BASIC_CHAR:
        case TYPE_BASIC_HYPER:
        case TYPE_BASIC_BYTE:
        case TYPE_BASIC_WCHAR:
            return TRUE;
        default:
            return FALSE;
        }
    case TYPE_ALIAS:
        /* shouldn't get here because of type_get_type call above */
        assert(0);
        /* fall through */
    case TYPE_STRUCT:
    case TYPE_UNION:
    case TYPE_ENCAPSULATED_UNION:
    case TYPE_ARRAY:
    case TYPE_POINTER:
    case TYPE_VOID:
    case TYPE_MODULE:
    case TYPE_COCLASS:
    case TYPE_FUNCTION:
    case TYPE_INTERFACE:
    case TYPE_BITFIELD:
        return FALSE;
    }
    return FALSE;
}

static int is_ptr_guid_type(const type_t *type)
{
    /* first, make sure it is a pointer to something */
    if (!is_ptr(type)) return FALSE;

    /* second, make sure it is a pointer to something of size sizeof(GUID),
     * i.e. 16 bytes */
    return (type_memsize(type_pointer_get_ref(type)) == 16);
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
    int more_to_do;
    const char *container_type_name;
    const char *var_type;

    switch (type_get_type(container_type))
    {
    case TYPE_STRUCT:
        container_type_name = "struct";
        var_type = "field";
        break;
    case TYPE_UNION:
        container_type_name = "union";
        var_type = "arm";
        break;
    case TYPE_ENCAPSULATED_UNION:
        container_type_name = "encapsulated union";
        var_type = "arm";
        break;
    case TYPE_FUNCTION:
        container_type_name = "function";
        var_type = "parameter";
        break;
    default:
        /* should be no other container types */
        assert(0);
        return;
    }

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

    do
    {
        more_to_do = FALSE;

        switch (typegen_detect_type(type, arg->attrs, TDT_IGNORE_STRINGS))
        {
        case TGT_STRUCT:
        case TGT_UNION:
            check_remoting_fields(arg, type);
            break;
        case TGT_INVALID:
        {
            const char *reason = "is invalid";
            switch (type_get_type(type))
            {
            case TYPE_VOID:
                reason = "cannot derive from void *";
                break;
            case TYPE_FUNCTION:
                reason = "cannot be a function pointer";
                break;
            case TYPE_BITFIELD:
                reason = "cannot be a bit-field";
                break;
            case TYPE_COCLASS:
                reason = "cannot be a class";
                break;
            case TYPE_INTERFACE:
                reason = "cannot be a non-pointer to an interface";
                break;
            case TYPE_MODULE:
                reason = "cannot be a module";
                break;
            default:
                break;
            }
            error_loc_info(&arg->loc_info, "%s \'%s\' of %s \'%s\' %s\n",
                           var_type, arg->name, container_type_name, container_name, reason);
            break;
        }
        case TGT_CTXT_HANDLE:
        case TGT_CTXT_HANDLE_POINTER:
            if (type_get_type(container_type) != TYPE_FUNCTION)
                error_loc_info(&arg->loc_info,
                               "%s \'%s\' of %s \'%s\' cannot be a context handle\n",
                               var_type, arg->name, container_type_name,
                               container_name);
            break;
        case TGT_STRING:
        {
            const type_t *t = type;
            while (is_ptr(t))
                t = type_pointer_get_ref(t);
            if (is_aliaschain_attr(t, ATTR_RANGE))
                warning_loc_info(&arg->loc_info, "%s: range not verified for a string of ranged types\n", arg->name);
            break;
        }
        case TGT_POINTER:
            type = type_pointer_get_ref(type);
            more_to_do = TRUE;
            break;
        case TGT_ARRAY:
            type = type_array_get_element(type);
            more_to_do = TRUE;
            break;
        case TGT_USER_TYPE:
        case TGT_IFACE_POINTER:
        case TGT_BASIC:
        case TGT_ENUM:
        case TGT_RANGE:
            /* nothing to do */
            break;
        }
    } while (more_to_do);
}

static void check_remoting_fields(const var_t *var, type_t *type)
{
    const var_t *field;
    const var_list_t *fields = NULL;

    type = type_get_real_type(type);

    if (type->checked)
        return;

    type->checked = TRUE;

    if (type_get_type(type) == TYPE_STRUCT)
    {
        if (type_is_complete(type))
            fields = type_struct_get_fields(type);
        else
            error_loc_info(&var->loc_info, "undefined type declaration %s\n", type->name);
    }
    else if (type_get_type(type) == TYPE_UNION || type_get_type(type) == TYPE_ENCAPSULATED_UNION)
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
        const type_t *type = arg->type;

        /* check that [out] parameters have enough pointer levels */
        if (is_attr(arg->attrs, ATTR_OUT))
        {
            switch (typegen_detect_type(type, arg->attrs, TDT_ALL_TYPES))
            {
            case TGT_BASIC:
            case TGT_ENUM:
            case TGT_RANGE:
            case TGT_STRUCT:
            case TGT_UNION:
            case TGT_CTXT_HANDLE:
            case TGT_USER_TYPE:
                error_loc_info(&arg->loc_info, "out parameter \'%s\' of function \'%s\' is not a pointer\n", arg->name, funcname);
                break;
            case TGT_IFACE_POINTER:
                error_loc_info(&arg->loc_info, "out interface pointer \'%s\' of function \'%s\' is not a double pointer\n", arg->name, funcname);
                break;
            case TGT_STRING:
                if (is_array(type))
                {
                    /* needs conformance or fixed dimension */
                    if (type_array_has_conformance(type) &&
                        type_array_get_conformance(type)->type != EXPR_VOID) break;
                    if (!type_array_has_conformance(type) && type_array_get_dim(type)) break;
                }
                if (is_attr( arg->attrs, ATTR_IN )) break;
                error_loc_info(&arg->loc_info, "out parameter \'%s\' of function \'%s\' cannot be an unsized string\n", arg->name, funcname);
                break;
            case TGT_INVALID:
                /* already error'd before we get here */
            case TGT_CTXT_HANDLE_POINTER:
            case TGT_POINTER:
            case TGT_ARRAY:
                /* OK */
                break;
            }
        }

        check_field_common(func->type, funcname, arg);
    }

    if (type_get_type(type_function_get_rettype(func->type)) != TYPE_VOID)
    {
        var_t var;
        var = *func;
        var.type = type_function_get_rettype(func->type);
        var.name = xstrdup("return value");
        check_field_common(func->type, funcname, &var);
        free(var.name);
    }
}

static void add_explicit_handle_if_necessary(const type_t *iface, var_t *func)
{
    unsigned char explicit_fc, implicit_fc;

    /* check for a defined binding handle */
    if (!get_func_handle_var( iface, func, &explicit_fc, &implicit_fc ) || !explicit_fc)
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

static void check_functions(const type_t *iface, int is_inside_library)
{
    const statement_t *stmt;
    if (is_attr(iface->attrs, ATTR_EXPLICIT_HANDLE))
    {
        STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
        {
            var_t *func = stmt->u.var;
            add_explicit_handle_if_necessary(iface, func);
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

static char *concat_str(const char *prefix, const char *str)
{
    char *ret = xmalloc(strlen(prefix) + strlen(str) + 1);
    strcpy(ret, prefix);
    strcat(ret, str);
    return ret;
}

static int async_iface_attrs(attr_list_t *attrs, const attr_t *attr)
{
    switch(attr->type)
    {
    case ATTR_UUID:
        return 0;
    case ATTR_ASYNCUUID:
        append_attr(attrs, make_attrp(ATTR_UUID, attr->u.pval));
        return 0;
    default:
        return 1;
    }
}

static int arg_in_attrs(attr_list_t *attrs, const attr_t *attr)
{
    return attr->type != ATTR_OUT && attr->type != ATTR_RETVAL;
}

static int arg_out_attrs(attr_list_t *attrs, const attr_t *attr)
{
    return attr->type != ATTR_IN;
}

static void check_async_uuid(type_t *iface)
{
    statement_list_t *stmts = NULL;
    statement_t *stmt;
    type_t *async_iface;
    type_t *inherit;

    if (!is_attr(iface->attrs, ATTR_ASYNCUUID)) return;

    inherit = iface->details.iface->inherit;
    if (inherit && strcmp(inherit->name, "IUnknown"))
        inherit = inherit->details.iface->async_iface;
    if (!inherit)
        error_loc("async_uuid applied to an interface with incompatible parent\n");

    async_iface = get_type(TYPE_INTERFACE, concat_str("Async", iface->name), iface->namespace, 0);
    async_iface->attrs = map_attrs(iface->attrs, async_iface_attrs);

    STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
    {
        var_t *begin_func, *finish_func, *func = stmt->u.var, *arg;
        var_list_t *begin_args = NULL, *finish_args = NULL, *args;

        args = func->type->details.function->args;
        if (args) LIST_FOR_EACH_ENTRY(arg, args, var_t, entry)
        {
            if (is_attr(arg->attrs, ATTR_IN) || !is_attr(arg->attrs, ATTR_OUT))
                begin_args = append_var(begin_args, copy_var(arg, strdup(arg->name), arg_in_attrs));
            if (is_attr(arg->attrs, ATTR_OUT))
                finish_args = append_var(finish_args, copy_var(arg, strdup(arg->name), arg_out_attrs));
        }

        begin_func = copy_var(func, concat_str("Begin_", func->name), NULL);
        begin_func->type = type_new_function(begin_args);
        begin_func->type->attrs = func->attrs;
        begin_func->type->details.function->retval = func->type->details.function->retval;
        stmts = append_statement(stmts, make_statement_declaration(begin_func));

        finish_func = copy_var(func, concat_str("Finish_", func->name), NULL);
        finish_func->type = type_new_function(finish_args);
        finish_func->type->attrs = func->attrs;
        finish_func->type->details.function->retval = func->type->details.function->retval;
        stmts = append_statement(stmts, make_statement_declaration(finish_func));
    }

    type_interface_define(async_iface, inherit, stmts);
    iface->details.iface->async_iface = async_iface->details.iface->async_iface = async_iface;
}

static void check_statements(const statement_list_t *stmts, int is_inside_library)
{
    const statement_t *stmt;

    if (stmts) LIST_FOR_EACH_ENTRY(stmt, stmts, const statement_t, entry)
    {
        switch(stmt->type) {
        case STMT_LIBRARY:
            check_statements(stmt->u.lib->stmts, TRUE);
            break;
        case STMT_TYPE:
            switch(type_get_type(stmt->u.type)) {
            case TYPE_INTERFACE:
                check_functions(stmt->u.type, is_inside_library);
                break;
            case TYPE_COCLASS:
                if(winrt_mode)
                    error_loc("coclass is not allowed in Windows Runtime mode\n");
                break;
            default:
                break;
            }
            break;
        default:
            break;
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
    else if (stmt->type == STMT_TYPE && type_get_type(stmt->u.type) == TYPE_INTERFACE &&
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
    else if (type_get_type(var->type) == TYPE_FUNCTION)
        check_function_attrs(var->name, var->attrs);
    else if (var->stgclass == STG_NONE || var->stgclass == STG_REGISTER)
        error_loc("instantiation of data is illegal\n");
    return stmt;
}

static statement_t *make_statement_library(typelib_t *typelib)
{
    statement_t *stmt = make_statement(STMT_LIBRARY);
    stmt->u.lib = typelib;
    return stmt;
}

static statement_t *make_statement_pragma(const char *str)
{
    statement_t *stmt = make_statement(STMT_PRAGMA);
    stmt->u.str = str;
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

static statement_list_t *append_statements(statement_list_t *l1, statement_list_t *l2)
{
    if (!l2) return l1;
    if (!l1 || l1 == l2) return l2;
    list_move_tail (l1, l2);
    return l1;
}

static attr_list_t *append_attribs(attr_list_t *l1, attr_list_t *l2)
{
    if (!l2) return l1;
    if (!l1 || l1 == l2) return l2;
    list_move_tail (l1, l2);
    return l1;
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
