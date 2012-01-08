/* A Bison parser, made by GNU Bison 2.4.2.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2006, 2009-2010 Free Software
   Foundation, Inc.
   
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

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse         parser_parse
#define yylex           parser_lex
#define yyerror         parser_error
#define yylval          parser_lval
#define yychar          parser_char
#define yydebug         parser_debug
#define yynerrs         parser_nerrs


/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
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

static unsigned char pointer_default = RPC_FC_UP;

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
static array_dims_t *append_array(array_dims_t *list, expr_t *expr);
static var_t *declare_var(attr_list_t *attrs, decl_spec_t *decl_spec, const declarator_t *decl, int top);
static var_list_t *set_var_types(attr_list_t *attrs, decl_spec_t *decl_spec, declarator_list_t *decls);
static ifref_list_t *append_ifref(ifref_list_t *list, ifref_t *iface);
static ifref_t *make_ifref(type_t *iface);
static var_list_t *append_var_list(var_list_t *list, var_list_t *vars);
static declarator_list_t *append_declarator(declarator_list_t *list, declarator_t *p);
static declarator_t *make_declarator(var_t *var);
static type_t *make_safearray(type_t *type);
static typelib_t *make_library(const char *name, const attr_list_t *attrs);
static type_t *append_ptrchain_type(type_t *ptrchain, type_t *type);

static type_t *reg_typedefs(decl_spec_t *decl_spec, var_list_t *names, attr_list_t *attrs);
static type_t *find_type_or_error(const char *name, int t);
static type_t *find_type_or_error2(char *name, int t);

static var_t *reg_const(var_t *var);

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



/* Line 189 of yacc.c  */
#line 234 "parser.tab.c"

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
     aSQSTRING = 265,
     aUUID = 266,
     aEOF = 267,
     SHL = 268,
     SHR = 269,
     MEMBERPTR = 270,
     EQUALITY = 271,
     INEQUALITY = 272,
     GREATEREQUAL = 273,
     LESSEQUAL = 274,
     LOGICALOR = 275,
     LOGICALAND = 276,
     ELLIPSIS = 277,
     tAGGREGATABLE = 278,
     tALLOCATE = 279,
     tANNOTATION = 280,
     tAPPOBJECT = 281,
     tASYNC = 282,
     tASYNCUUID = 283,
     tAUTOHANDLE = 284,
     tBINDABLE = 285,
     tBOOLEAN = 286,
     tBROADCAST = 287,
     tBYTE = 288,
     tBYTECOUNT = 289,
     tCALLAS = 290,
     tCALLBACK = 291,
     tCASE = 292,
     tCDECL = 293,
     tCHAR = 294,
     tCOCLASS = 295,
     tCODE = 296,
     tCOMMSTATUS = 297,
     tCONST = 298,
     tCONTEXTHANDLE = 299,
     tCONTEXTHANDLENOSERIALIZE = 300,
     tCONTEXTHANDLESERIALIZE = 301,
     tCONTROL = 302,
     tCPPQUOTE = 303,
     tDECODE = 304,
     tDEFAULT = 305,
     tDEFAULTBIND = 306,
     tDEFAULTCOLLELEM = 307,
     tDEFAULTVALUE = 308,
     tDEFAULTVTABLE = 309,
     tDISABLECONSISTENCYCHECK = 310,
     tDISPLAYBIND = 311,
     tDISPINTERFACE = 312,
     tDLLNAME = 313,
     tDOUBLE = 314,
     tDUAL = 315,
     tENABLEALLOCATE = 316,
     tENCODE = 317,
     tENDPOINT = 318,
     tENTRY = 319,
     tENUM = 320,
     tERRORSTATUST = 321,
     tEXPLICITHANDLE = 322,
     tEXTERN = 323,
     tFALSE = 324,
     tFASTCALL = 325,
     tFAULTSTATUS = 326,
     tFLOAT = 327,
     tFORCEALLOCATE = 328,
     tHANDLE = 329,
     tHANDLET = 330,
     tHELPCONTEXT = 331,
     tHELPFILE = 332,
     tHELPSTRING = 333,
     tHELPSTRINGCONTEXT = 334,
     tHELPSTRINGDLL = 335,
     tHIDDEN = 336,
     tHYPER = 337,
     tID = 338,
     tIDEMPOTENT = 339,
     tIGNORE = 340,
     tIIDIS = 341,
     tIMMEDIATEBIND = 342,
     tIMPLICITHANDLE = 343,
     tIMPORT = 344,
     tIMPORTLIB = 345,
     tIN = 346,
     tIN_LINE = 347,
     tINLINE = 348,
     tINPUTSYNC = 349,
     tINT = 350,
     tINT3264 = 351,
     tINT64 = 352,
     tINTERFACE = 353,
     tLCID = 354,
     tLENGTHIS = 355,
     tLIBRARY = 356,
     tLICENSED = 357,
     tLOCAL = 358,
     tLONG = 359,
     tMAYBE = 360,
     tMESSAGE = 361,
     tMETHODS = 362,
     tMODULE = 363,
     tNOCODE = 364,
     tNONBROWSABLE = 365,
     tNONCREATABLE = 366,
     tNONEXTENSIBLE = 367,
     tNOTIFY = 368,
     tNOTIFYFLAG = 369,
     tNULL = 370,
     tOBJECT = 371,
     tODL = 372,
     tOLEAUTOMATION = 373,
     tOPTIMIZE = 374,
     tOPTIONAL = 375,
     tOUT = 376,
     tPARTIALIGNORE = 377,
     tPASCAL = 378,
     tPOINTERDEFAULT = 379,
     tPROGID = 380,
     tPROPERTIES = 381,
     tPROPGET = 382,
     tPROPPUT = 383,
     tPROPPUTREF = 384,
     tPROXY = 385,
     tPTR = 386,
     tPUBLIC = 387,
     tRANGE = 388,
     tREADONLY = 389,
     tREF = 390,
     tREGISTER = 391,
     tREPRESENTAS = 392,
     tREQUESTEDIT = 393,
     tRESTRICTED = 394,
     tRETVAL = 395,
     tSAFEARRAY = 396,
     tSHORT = 397,
     tSIGNED = 398,
     tSIZEIS = 399,
     tSIZEOF = 400,
     tSMALL = 401,
     tSOURCE = 402,
     tSTATIC = 403,
     tSTDCALL = 404,
     tSTRICTCONTEXTHANDLE = 405,
     tSTRING = 406,
     tSTRUCT = 407,
     tSWITCH = 408,
     tSWITCHIS = 409,
     tSWITCHTYPE = 410,
     tTHREADING = 411,
     tTRANSMITAS = 412,
     tTRUE = 413,
     tTYPEDEF = 414,
     tUIDEFAULT = 415,
     tUNION = 416,
     tUNIQUE = 417,
     tUNSIGNED = 418,
     tUSESGETLASTERROR = 419,
     tUSERMARSHAL = 420,
     tUUID = 421,
     tV1ENUM = 422,
     tVARARG = 423,
     tVERSION = 424,
     tVIPROGID = 425,
     tVOID = 426,
     tWCHAR = 427,
     tWIREMARSHAL = 428,
     tAPARTMENT = 429,
     tNEUTRAL = 430,
     tSINGLE = 431,
     tFREE = 432,
     tBOTH = 433,
     ADDRESSOF = 434,
     NEG = 435,
     POS = 436,
     PPTR = 437,
     CAST = 438
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 153 "parser.y"

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



/* Line 214 of yacc.c  */
#line 482 "parser.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 494 "parser.tab.c"

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
# if defined YYENABLE_NLS && YYENABLE_NLS
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
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
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
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
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
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2742

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  208
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  100
/* YYNRULES -- Number of rules.  */
#define YYNRULES  380
/* YYNRULES -- Number of states.  */
#define YYNSTATES  661

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   438

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   192,     2,     2,     2,   191,   184,     2,
     203,   204,   189,   188,   179,   187,   199,   190,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   181,   202,
     185,   207,   186,   180,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   200,     2,   201,   183,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   205,   182,   206,   193,     2,     2,     2,
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
     175,   176,   177,   178,   194,   195,   196,   197,   198
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     6,     9,    12,    16,    19,    22,
      25,    28,    29,    32,    35,    39,    42,    45,    48,    51,
      54,    55,    58,    59,    61,    63,    66,    69,    71,    74,
      76,    79,    81,    84,    86,    89,    92,    95,    98,   103,
     107,   111,   117,   120,   124,   129,   130,   132,   134,   138,
     140,   144,   148,   151,   155,   159,   162,   163,   165,   169,
     171,   175,   180,   182,   186,   187,   189,   194,   196,   198,
     200,   202,   204,   209,   214,   216,   218,   220,   222,   224,
     226,   228,   230,   232,   234,   239,   241,   243,   245,   250,
     252,   254,   256,   261,   266,   268,   270,   272,   274,   279,
     284,   289,   294,   299,   301,   306,   308,   310,   315,   317,
     322,   324,   326,   331,   336,   338,   340,   342,   344,   346,
     348,   350,   352,   354,   356,   358,   360,   362,   364,   369,
     371,   373,   375,   380,   385,   387,   389,   391,   393,   395,
     402,   404,   409,   411,   413,   415,   420,   422,   424,   426,
     431,   436,   441,   446,   448,   450,   455,   460,   462,   464,
     469,   474,   479,   481,   483,   485,   487,   489,   491,   493,
     494,   497,   502,   506,   507,   510,   512,   514,   518,   522,
     524,   530,   532,   536,   537,   539,   541,   543,   545,   547,
     549,   551,   553,   555,   557,   559,   565,   569,   573,   577,
     581,   585,   589,   593,   597,   601,   605,   609,   613,   617,
     621,   625,   629,   633,   637,   640,   643,   646,   649,   652,
     655,   659,   663,   669,   675,   680,   684,   686,   690,   692,
     694,   695,   698,   703,   707,   710,   713,   714,   717,   720,
     722,   726,   728,   732,   735,   736,   738,   739,   741,   743,
     745,   747,   749,   751,   753,   756,   759,   761,   763,   765,
     767,   769,   771,   772,   774,   776,   779,   781,   784,   787,
     789,   791,   793,   796,   799,   802,   808,   809,   812,   815,
     818,   821,   824,   827,   831,   834,   838,   844,   850,   851,
     854,   857,   860,   863,   870,   879,   882,   885,   888,   891,
     894,   897,   903,   905,   907,   909,   911,   913,   914,   917,
     920,   924,   925,   927,   930,   933,   936,   940,   943,   945,
     947,   951,   954,   959,   963,   966,   968,   972,   975,   976,
     978,   982,   985,   987,   991,   996,  1000,  1003,  1005,  1009,
    1012,  1013,  1015,  1017,  1021,  1024,  1026,  1030,  1035,  1037,
    1041,  1042,  1045,  1048,  1050,  1054,  1056,  1060,  1062,  1064,
    1066,  1068,  1070,  1072,  1074,  1076,  1082,  1084,  1086,  1088,
    1090,  1093,  1095,  1098,  1100,  1103,  1108,  1113,  1119,  1130,
    1132
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     209,     0,    -1,   210,    -1,    -1,   210,   275,    -1,   210,
     274,    -1,   210,   261,   202,    -1,   210,   263,    -1,   210,
     278,    -1,   210,   222,    -1,   210,   214,    -1,    -1,   211,
     275,    -1,   211,   274,    -1,   211,   261,   202,    -1,   211,
     263,    -1,   211,   278,    -1,   211,   214,    -1,   211,   219,
      -1,   211,   222,    -1,    -1,   212,   214,    -1,    -1,   202,
      -1,   216,    -1,   215,   202,    -1,   254,   202,    -1,   218,
      -1,   305,   202,    -1,   240,    -1,    65,     3,    -1,   303,
      -1,   152,     3,    -1,   306,    -1,   161,     3,    -1,   229,
     240,    -1,   229,   303,    -1,   229,   306,    -1,    48,   203,
       8,   204,    -1,    89,     8,   202,    -1,   217,   211,    12,
      -1,    90,   203,     8,   204,   213,    -1,   101,     3,    -1,
     229,   220,   205,    -1,   221,   211,   206,   213,    -1,    -1,
     225,    -1,   226,    -1,   224,   179,   226,    -1,   224,    -1,
     224,   179,    22,    -1,   229,   283,   294,    -1,   283,   294,
      -1,   200,   243,   201,    -1,   200,   189,   201,    -1,   200,
     201,    -1,    -1,   229,    -1,   200,   230,   201,    -1,   232,
      -1,   230,   179,   232,    -1,   230,   201,   200,   232,    -1,
       8,    -1,   231,   179,     8,    -1,    -1,    23,    -1,    25,
     203,     8,   204,    -1,    26,    -1,    27,    -1,    29,    -1,
      30,    -1,    32,    -1,    35,   203,   257,   204,    -1,    37,
     203,   244,   204,    -1,    41,    -1,    42,    -1,    44,    -1,
      45,    -1,    46,    -1,    47,    -1,    49,    -1,    50,    -1,
      51,    -1,    52,    -1,    53,   203,   246,   204,    -1,    54,
      -1,    55,    -1,    56,    -1,    58,   203,     8,   204,    -1,
      60,    -1,    61,    -1,    62,    -1,    63,   203,   231,   204,
      -1,    64,   203,   246,   204,    -1,    67,    -1,    71,    -1,
      73,    -1,    74,    -1,    76,   203,   245,   204,    -1,    77,
     203,     8,   204,    -1,    78,   203,     8,   204,    -1,    79,
     203,   245,   204,    -1,    80,   203,     8,   204,    -1,    81,
      -1,    83,   203,   245,   204,    -1,    84,    -1,    85,    -1,
      86,   203,   243,   204,    -1,    87,    -1,    88,   203,   226,
     204,    -1,    91,    -1,    94,    -1,   100,   203,   241,   204,
      -1,    99,   203,   245,   204,    -1,    99,    -1,   102,    -1,
     103,    -1,   105,    -1,   106,    -1,   109,    -1,   110,    -1,
     111,    -1,   112,    -1,   113,    -1,   114,    -1,   116,    -1,
     117,    -1,   118,    -1,   119,   203,     8,   204,    -1,   120,
      -1,   121,    -1,   122,    -1,   124,   203,   302,   204,    -1,
     125,   203,     8,   204,    -1,   127,    -1,   128,    -1,   129,
      -1,   130,    -1,   132,    -1,   133,   203,   245,   179,   245,
     204,    -1,   134,    -1,   137,   203,   304,   204,    -1,   138,
      -1,   139,    -1,   140,    -1,   144,   203,   241,   204,    -1,
     147,    -1,   150,    -1,   151,    -1,   154,   203,   243,   204,
      -1,   155,   203,   304,   204,    -1,   157,   203,   304,   204,
      -1,   156,   203,   301,   204,    -1,   160,    -1,   164,    -1,
     165,   203,   304,   204,    -1,   166,   203,   233,   204,    -1,
     167,    -1,   168,    -1,   169,   203,   307,   204,    -1,   170,
     203,     8,   204,    -1,   173,   203,   304,   204,    -1,   302,
      -1,    11,    -1,     8,    -1,    38,    -1,    70,    -1,   123,
      -1,   149,    -1,    -1,   235,   236,    -1,    37,   245,   181,
     251,    -1,    50,   181,   251,    -1,    -1,   238,   179,    -1,
     238,    -1,   239,    -1,   238,   179,   239,    -1,   257,   207,
     245,    -1,   257,    -1,    65,   256,   205,   237,   206,    -1,
     242,    -1,   241,   179,   242,    -1,    -1,   243,    -1,     5,
      -1,     6,    -1,     7,    -1,    69,    -1,   115,    -1,   158,
      -1,     8,    -1,     9,    -1,    10,    -1,     3,    -1,   243,
     180,   243,   181,   243,    -1,   243,    20,   243,    -1,   243,
      21,   243,    -1,   243,   182,   243,    -1,   243,   183,   243,
      -1,   243,   184,   243,    -1,   243,    16,   243,    -1,   243,
      17,   243,    -1,   243,   186,   243,    -1,   243,   185,   243,
      -1,   243,    18,   243,    -1,   243,    19,   243,    -1,   243,
      13,   243,    -1,   243,    14,   243,    -1,   243,   188,   243,
      -1,   243,   187,   243,    -1,   243,   191,   243,    -1,   243,
     189,   243,    -1,   243,   190,   243,    -1,   192,   243,    -1,
     193,   243,    -1,   188,   243,    -1,   187,   243,    -1,   184,
     243,    -1,   189,   243,    -1,   243,    15,     3,    -1,   243,
     199,     3,    -1,   203,   283,   290,   204,   243,    -1,   145,
     203,   283,   290,   204,    -1,   243,   200,   243,   201,    -1,
     203,   243,   204,    -1,   245,    -1,   244,   179,   245,    -1,
     243,    -1,   243,    -1,    -1,   247,   248,    -1,   228,   283,
     299,   202,    -1,   228,   306,   202,    -1,   252,   202,    -1,
     229,   202,    -1,    -1,   250,   249,    -1,   252,   202,    -1,
     202,    -1,   228,   283,   286,    -1,   254,    -1,   229,   283,
     300,    -1,   283,   300,    -1,    -1,   257,    -1,    -1,     3,
      -1,     4,    -1,     3,    -1,     4,    -1,    33,    -1,   172,
      -1,   260,    -1,   143,   260,    -1,   163,   260,    -1,   163,
      -1,    72,    -1,    59,    -1,    31,    -1,    66,    -1,    75,
      -1,    -1,    95,    -1,    95,    -1,   142,   259,    -1,   146,
      -1,   104,   259,    -1,    82,   259,    -1,    97,    -1,    39,
      -1,    96,    -1,    40,     3,    -1,    40,     4,    -1,   229,
     261,    -1,   262,   205,   264,   206,   213,    -1,    -1,   264,
     265,    -1,   228,   275,    -1,    57,     3,    -1,    57,     4,
      -1,   229,   266,    -1,   126,   181,    -1,   268,   252,   202,
      -1,   107,   181,    -1,   269,   253,   202,    -1,   267,   205,
     268,   269,   206,    -1,   267,   205,   272,   202,   206,    -1,
      -1,   181,     4,    -1,    98,     3,    -1,    98,     4,    -1,
     229,   272,    -1,   273,   271,   205,   212,   206,   213,    -1,
     273,   181,     3,   205,   218,   212,   206,   213,    -1,   270,
     213,    -1,   272,   202,    -1,   266,   202,    -1,   108,     3,
      -1,   108,     4,    -1,   229,   276,    -1,   277,   205,   212,
     206,   213,    -1,    68,    -1,   148,    -1,   136,    -1,    93,
      -1,    43,    -1,    -1,   282,   281,    -1,   304,   284,    -1,
     285,   304,   284,    -1,    -1,   285,    -1,   281,   284,    -1,
     280,   284,    -1,   279,   284,    -1,   189,   282,   286,    -1,
     234,   286,    -1,   287,    -1,   257,    -1,   203,   286,   204,
      -1,   287,   227,    -1,   287,   203,   223,   204,    -1,   189,
     282,   290,    -1,   234,   290,    -1,   291,    -1,   189,   282,
     294,    -1,   234,   294,    -1,    -1,   288,    -1,   203,   289,
     204,    -1,   291,   227,    -1,   227,    -1,   203,   223,   204,
      -1,   291,   203,   223,   204,    -1,   189,   282,   294,    -1,
     234,   294,    -1,   295,    -1,   189,   282,   294,    -1,   234,
     294,    -1,    -1,   292,    -1,   257,    -1,   203,   293,   204,
      -1,   295,   227,    -1,   227,    -1,   203,   223,   204,    -1,
     295,   203,   223,   204,    -1,   286,    -1,   296,   179,   286,
      -1,    -1,   181,   246,    -1,   292,   297,    -1,   298,    -1,
     299,   179,   298,    -1,   286,    -1,   286,   207,   246,    -1,
     174,    -1,   175,    -1,   176,    -1,   177,    -1,   178,    -1,
     135,    -1,   162,    -1,   131,    -1,   152,   256,   205,   247,
     206,    -1,   171,    -1,     4,    -1,   258,    -1,   240,    -1,
      65,     3,    -1,   303,    -1,   152,     3,    -1,   306,    -1,
     161,     3,    -1,   141,   203,   304,   204,    -1,   159,   228,
     283,   296,    -1,   161,   256,   205,   250,   206,    -1,   161,
     256,   153,   203,   252,   204,   255,   205,   235,   206,    -1,
       5,    -1,     5,   199,     5,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   328,   328,   342,   343,   344,   345,   348,   351,   352,
     353,   356,   357,   358,   359,   360,   363,   364,   365,   366,
     369,   370,   373,   374,   378,   379,   380,   381,   382,   386,
     387,   388,   389,   390,   391,   392,   393,   394,   397,   399,
     407,   413,   417,   419,   423,   430,   431,   434,   435,   438,
     439,   443,   448,   455,   459,   460,   463,   464,   468,   471,
     472,   473,   476,   477,   480,   481,   482,   483,   484,   485,
     486,   487,   488,   489,   490,   491,   492,   493,   494,   495,
     496,   497,   498,   499,   500,   501,   502,   503,   504,   505,
     506,   507,   508,   509,   510,   511,   512,   513,   514,   515,
     516,   517,   518,   519,   520,   521,   522,   523,   524,   525,
     526,   527,   528,   529,   530,   531,   532,   533,   534,   535,
     536,   537,   538,   539,   540,   541,   542,   543,   544,   545,
     546,   547,   548,   549,   550,   551,   552,   553,   554,   555,
     559,   560,   561,   562,   563,   564,   565,   566,   567,   568,
     569,   570,   571,   572,   573,   574,   575,   576,   577,   578,
     579,   580,   581,   585,   586,   591,   592,   593,   594,   597,
     598,   601,   605,   611,   612,   613,   616,   620,   629,   633,
     638,   641,   642,   645,   646,   649,   650,   651,   652,   653,
     654,   655,   656,   657,   658,   659,   660,   661,   662,   663,
     664,   665,   666,   667,   668,   669,   670,   671,   672,   673,
     674,   675,   676,   677,   678,   679,   680,   681,   682,   683,
     684,   685,   686,   688,   690,   691,   694,   695,   698,   704,
     710,   711,   714,   719,   726,   727,   730,   731,   735,   736,
     739,   745,   753,   757,   762,   763,   766,   767,   768,   771,
     773,   776,   777,   778,   779,   780,   781,   782,   783,   784,
     785,   786,   789,   790,   793,   794,   795,   796,   797,   798,
     799,   800,   803,   804,   812,   818,   822,   823,   827,   830,
     831,   834,   843,   844,   847,   848,   851,   857,   863,   864,
     867,   868,   871,   881,   888,   894,   898,   899,   902,   903,
     906,   911,   918,   919,   920,   924,   928,   931,   932,   935,
     936,   940,   941,   945,   946,   947,   951,   953,   955,   959,
     960,   961,   962,   970,   972,   974,   979,   981,   986,   987,
     992,   993,   994,   995,  1000,  1009,  1011,  1012,  1017,  1019,
    1023,  1024,  1031,  1032,  1033,  1034,  1035,  1040,  1048,  1049,
    1052,  1053,  1056,  1063,  1064,  1069,  1070,  1074,  1075,  1076,
    1077,  1078,  1082,  1083,  1084,  1087,  1090,  1091,  1092,  1093,
    1094,  1095,  1096,  1097,  1098,  1099,  1102,  1108,  1110,  1116,
    1117
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "aIDENTIFIER", "aKNOWNTYPE", "aNUM",
  "aHEXNUM", "aDOUBLE", "aSTRING", "aWSTRING", "aSQSTRING", "aUUID",
  "aEOF", "SHL", "SHR", "MEMBERPTR", "EQUALITY", "INEQUALITY",
  "GREATEREQUAL", "LESSEQUAL", "LOGICALOR", "LOGICALAND", "ELLIPSIS",
  "tAGGREGATABLE", "tALLOCATE", "tANNOTATION", "tAPPOBJECT", "tASYNC",
  "tASYNCUUID", "tAUTOHANDLE", "tBINDABLE", "tBOOLEAN", "tBROADCAST",
  "tBYTE", "tBYTECOUNT", "tCALLAS", "tCALLBACK", "tCASE", "tCDECL",
  "tCHAR", "tCOCLASS", "tCODE", "tCOMMSTATUS", "tCONST", "tCONTEXTHANDLE",
  "tCONTEXTHANDLENOSERIALIZE", "tCONTEXTHANDLESERIALIZE", "tCONTROL",
  "tCPPQUOTE", "tDECODE", "tDEFAULT", "tDEFAULTBIND", "tDEFAULTCOLLELEM",
  "tDEFAULTVALUE", "tDEFAULTVTABLE", "tDISABLECONSISTENCYCHECK",
  "tDISPLAYBIND", "tDISPINTERFACE", "tDLLNAME", "tDOUBLE", "tDUAL",
  "tENABLEALLOCATE", "tENCODE", "tENDPOINT", "tENTRY", "tENUM",
  "tERRORSTATUST", "tEXPLICITHANDLE", "tEXTERN", "tFALSE", "tFASTCALL",
  "tFAULTSTATUS", "tFLOAT", "tFORCEALLOCATE", "tHANDLE", "tHANDLET",
  "tHELPCONTEXT", "tHELPFILE", "tHELPSTRING", "tHELPSTRINGCONTEXT",
  "tHELPSTRINGDLL", "tHIDDEN", "tHYPER", "tID", "tIDEMPOTENT", "tIGNORE",
  "tIIDIS", "tIMMEDIATEBIND", "tIMPLICITHANDLE", "tIMPORT", "tIMPORTLIB",
  "tIN", "tIN_LINE", "tINLINE", "tINPUTSYNC", "tINT", "tINT3264", "tINT64",
  "tINTERFACE", "tLCID", "tLENGTHIS", "tLIBRARY", "tLICENSED", "tLOCAL",
  "tLONG", "tMAYBE", "tMESSAGE", "tMETHODS", "tMODULE", "tNOCODE",
  "tNONBROWSABLE", "tNONCREATABLE", "tNONEXTENSIBLE", "tNOTIFY",
  "tNOTIFYFLAG", "tNULL", "tOBJECT", "tODL", "tOLEAUTOMATION", "tOPTIMIZE",
  "tOPTIONAL", "tOUT", "tPARTIALIGNORE", "tPASCAL", "tPOINTERDEFAULT",
  "tPROGID", "tPROPERTIES", "tPROPGET", "tPROPPUT", "tPROPPUTREF",
  "tPROXY", "tPTR", "tPUBLIC", "tRANGE", "tREADONLY", "tREF", "tREGISTER",
  "tREPRESENTAS", "tREQUESTEDIT", "tRESTRICTED", "tRETVAL", "tSAFEARRAY",
  "tSHORT", "tSIGNED", "tSIZEIS", "tSIZEOF", "tSMALL", "tSOURCE",
  "tSTATIC", "tSTDCALL", "tSTRICTCONTEXTHANDLE", "tSTRING", "tSTRUCT",
  "tSWITCH", "tSWITCHIS", "tSWITCHTYPE", "tTHREADING", "tTRANSMITAS",
  "tTRUE", "tTYPEDEF", "tUIDEFAULT", "tUNION", "tUNIQUE", "tUNSIGNED",
  "tUSESGETLASTERROR", "tUSERMARSHAL", "tUUID", "tV1ENUM", "tVARARG",
  "tVERSION", "tVIPROGID", "tVOID", "tWCHAR", "tWIREMARSHAL", "tAPARTMENT",
  "tNEUTRAL", "tSINGLE", "tFREE", "tBOTH", "','", "'?'", "':'", "'|'",
  "'^'", "'&'", "'<'", "'>'", "'-'", "'+'", "'*'", "'/'", "'%'", "'!'",
  "'~'", "ADDRESSOF", "NEG", "POS", "PPTR", "CAST", "'.'", "'['", "']'",
  "';'", "'('", "')'", "'{'", "'}'", "'='", "$accept", "input",
  "gbl_statements", "imp_statements", "int_statements", "semicolon_opt",
  "statement", "typedecl", "cppquote", "import_start", "import",
  "importlib", "libraryhdr", "library_start", "librarydef", "m_args",
  "arg_list", "args", "arg", "array", "m_attributes", "attributes",
  "attrib_list", "str_list", "attribute", "uuid_string", "callconv",
  "cases", "case", "enums", "enum_list", "enum", "enumdef", "m_exprs",
  "m_expr", "expr", "expr_list_int_const", "expr_int_const", "expr_const",
  "fields", "field", "ne_union_field", "ne_union_fields", "union_field",
  "s_field", "funcdef", "declaration", "m_ident", "t_ident", "ident",
  "base_type", "m_int", "int_std", "coclass", "coclasshdr", "coclassdef",
  "coclass_ints", "coclass_int", "dispinterface", "dispinterfacehdr",
  "dispint_props", "dispint_meths", "dispinterfacedef", "inherit",
  "interface", "interfacehdr", "interfacedef", "interfacedec", "module",
  "modulehdr", "moduledef", "storage_cls_spec", "function_specifier",
  "type_qualifier", "m_type_qual_list", "decl_spec", "m_decl_spec_no_type",
  "decl_spec_no_type", "declarator", "direct_declarator",
  "abstract_declarator", "abstract_declarator_no_direct",
  "m_abstract_declarator", "abstract_direct_declarator", "any_declarator",
  "any_declarator_no_direct", "m_any_declarator", "any_direct_declarator",
  "declarator_list", "m_bitfield", "struct_declarator",
  "struct_declarator_list", "init_declarator", "threading_type",
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
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
     395,   396,   397,   398,   399,   400,   401,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   414,
     415,   416,   417,   418,   419,   420,   421,   422,   423,   424,
     425,   426,   427,   428,   429,   430,   431,   432,   433,    44,
      63,    58,   124,    94,    38,    60,    62,    45,    43,    42,
      47,    37,    33,   126,   434,   435,   436,   437,   438,    46,
      91,    93,    59,    40,    41,   123,   125,    61
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   208,   209,   210,   210,   210,   210,   210,   210,   210,
     210,   211,   211,   211,   211,   211,   211,   211,   211,   211,
     212,   212,   213,   213,   214,   214,   214,   214,   214,   215,
     215,   215,   215,   215,   215,   215,   215,   215,   216,   217,
     218,   219,   220,   221,   222,   223,   223,   224,   224,   225,
     225,   226,   226,   227,   227,   227,   228,   228,   229,   230,
     230,   230,   231,   231,   232,   232,   232,   232,   232,   232,
     232,   232,   232,   232,   232,   232,   232,   232,   232,   232,
     232,   232,   232,   232,   232,   232,   232,   232,   232,   232,
     232,   232,   232,   232,   232,   232,   232,   232,   232,   232,
     232,   232,   232,   232,   232,   232,   232,   232,   232,   232,
     232,   232,   232,   232,   232,   232,   232,   232,   232,   232,
     232,   232,   232,   232,   232,   232,   232,   232,   232,   232,
     232,   232,   232,   232,   232,   232,   232,   232,   232,   232,
     232,   232,   232,   232,   232,   232,   232,   232,   232,   232,
     232,   232,   232,   232,   232,   232,   232,   232,   232,   232,
     232,   232,   232,   233,   233,   234,   234,   234,   234,   235,
     235,   236,   236,   237,   237,   237,   238,   238,   239,   239,
     240,   241,   241,   242,   242,   243,   243,   243,   243,   243,
     243,   243,   243,   243,   243,   243,   243,   243,   243,   243,
     243,   243,   243,   243,   243,   243,   243,   243,   243,   243,
     243,   243,   243,   243,   243,   243,   243,   243,   243,   243,
     243,   243,   243,   243,   243,   243,   244,   244,   245,   246,
     247,   247,   248,   248,   249,   249,   250,   250,   251,   251,
     252,   253,   254,   254,   255,   255,   256,   256,   256,   257,
     257,   258,   258,   258,   258,   258,   258,   258,   258,   258,
     258,   258,   259,   259,   260,   260,   260,   260,   260,   260,
     260,   260,   261,   261,   262,   263,   264,   264,   265,   266,
     266,   267,   268,   268,   269,   269,   270,   270,   271,   271,
     272,   272,   273,   274,   274,   274,   275,   275,   276,   276,
     277,   278,   279,   279,   279,   280,   281,   282,   282,   283,
     283,   284,   284,   285,   285,   285,   286,   286,   286,   287,
     287,   287,   287,   288,   288,   288,   289,   289,   290,   290,
     291,   291,   291,   291,   291,   292,   292,   292,   293,   293,
     294,   294,   295,   295,   295,   295,   295,   295,   296,   296,
     297,   297,   298,   299,   299,   300,   300,   301,   301,   301,
     301,   301,   302,   302,   302,   303,   304,   304,   304,   304,
     304,   304,   304,   304,   304,   304,   305,   306,   306,   307,
     307
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     0,     2,     2,     3,     2,     2,     2,
       2,     0,     2,     2,     3,     2,     2,     2,     2,     2,
       0,     2,     0,     1,     1,     2,     2,     1,     2,     1,
       2,     1,     2,     1,     2,     2,     2,     2,     4,     3,
       3,     5,     2,     3,     4,     0,     1,     1,     3,     1,
       3,     3,     2,     3,     3,     2,     0,     1,     3,     1,
       3,     4,     1,     3,     0,     1,     4,     1,     1,     1,
       1,     1,     4,     4,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     4,     1,     1,     1,     4,     1,
       1,     1,     4,     4,     1,     1,     1,     1,     4,     4,
       4,     4,     4,     1,     4,     1,     1,     4,     1,     4,
       1,     1,     4,     4,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     4,     1,
       1,     1,     4,     4,     1,     1,     1,     1,     1,     6,
       1,     4,     1,     1,     1,     4,     1,     1,     1,     4,
       4,     4,     4,     1,     1,     4,     4,     1,     1,     4,
       4,     4,     1,     1,     1,     1,     1,     1,     1,     0,
       2,     4,     3,     0,     2,     1,     1,     3,     3,     1,
       5,     1,     3,     0,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     5,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     2,     2,     2,     2,     2,     2,
       3,     3,     5,     5,     4,     3,     1,     3,     1,     1,
       0,     2,     4,     3,     2,     2,     0,     2,     2,     1,
       3,     1,     3,     2,     0,     1,     0,     1,     1,     1,
       1,     1,     1,     1,     2,     2,     1,     1,     1,     1,
       1,     1,     0,     1,     1,     2,     1,     2,     2,     1,
       1,     1,     2,     2,     2,     5,     0,     2,     2,     2,
       2,     2,     2,     3,     2,     3,     5,     5,     0,     2,
       2,     2,     2,     6,     8,     2,     2,     2,     2,     2,
       2,     5,     1,     1,     1,     1,     1,     0,     2,     2,
       3,     0,     1,     2,     2,     2,     3,     2,     1,     1,
       3,     2,     4,     3,     2,     1,     3,     2,     0,     1,
       3,     2,     1,     3,     4,     3,     2,     1,     3,     2,
       0,     1,     1,     3,     2,     1,     3,     4,     1,     3,
       0,     2,     2,     1,     3,     1,     3,     1,     1,     1,
       1,     1,     1,     1,     1,     5,     1,     1,     1,     1,
       2,     1,     2,     1,     2,     4,     4,     5,    10,     1,
       3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       3,     0,     2,     1,   367,   259,   251,   270,     0,   306,
       0,     0,   258,   246,   260,   302,   257,   261,   262,     0,
     305,   264,   271,   269,     0,   262,   304,     0,   262,     0,
     266,   303,   246,    56,   246,   256,   366,   252,    64,    10,
       0,    24,    11,    27,    11,     9,     0,   369,     0,   368,
     253,     0,     0,     7,     0,     0,    22,     0,   288,     5,
       4,     0,     8,   311,   311,   311,     0,     0,   371,   311,
       0,   373,   272,   273,     0,   279,   280,   370,   248,     0,
     263,   268,     0,   290,   291,   267,     0,   265,   254,   372,
       0,     0,    57,   374,     0,   255,    65,     0,    67,    68,
      69,    70,    71,     0,     0,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,     0,    85,    86,    87,     0,
      89,    90,    91,     0,     0,    94,    95,    96,    97,     0,
       0,     0,     0,     0,   103,     0,   105,   106,     0,   108,
       0,   110,   111,   114,     0,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,     0,   129,
     130,   131,     0,     0,   134,   135,   136,   137,   364,   138,
       0,   140,   362,     0,   142,   143,   144,     0,   146,   147,
     148,     0,     0,     0,     0,   153,   363,   154,     0,     0,
     157,   158,     0,     0,     0,     0,    59,   162,    25,     0,
       0,   246,     0,     0,   246,   246,     0,   369,   274,   281,
     292,   300,     0,   371,   373,    26,     6,   276,   297,     0,
      23,   295,   296,     0,     0,    20,   315,   312,   314,   313,
     249,   250,   165,   166,   167,   168,   307,     0,     0,   319,
     355,   318,   243,   369,   371,   311,   373,   309,    28,     0,
     173,    39,     0,   230,     0,     0,   236,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   183,     0,     0,     0,     0,     0,   183,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    64,
      58,    40,     0,    17,    18,    19,     0,    15,    13,    12,
      16,    22,   370,    42,   298,   299,   372,   374,    43,   242,
      56,     0,    56,     0,     0,   289,    20,     0,     0,     0,
     317,     0,     0,    45,   321,   310,    38,     0,   175,   176,
     179,   375,    56,   348,   376,    56,    56,     0,     0,   194,
     185,   186,   187,   191,   192,   193,   188,   189,     0,   190,
       0,     0,     0,     0,     0,     0,     0,   228,     0,   226,
     229,     0,     0,    62,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   340,     0,     0,   181,   184,
       0,     0,     0,     0,     0,     0,     0,     0,   357,   358,
     359,   360,   361,     0,     0,     0,   164,   163,     0,   379,
       0,     0,     0,    60,    64,     0,    14,    44,    22,     0,
     277,   282,     0,     0,     0,     0,     0,     0,     0,    22,
      21,     0,   308,   316,   320,   356,     0,    55,     0,     0,
      49,    46,    47,   180,   174,     0,   365,     0,   231,     0,
       0,   377,    57,   237,     0,    66,    72,     0,   218,   217,
     216,   219,   214,   215,     0,   328,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    73,
      84,    88,     0,    92,    93,    98,    99,   100,   101,   102,
     104,   107,   109,   340,   307,    45,   345,   340,   342,   341,
      52,   337,   113,   183,   112,   128,   132,   133,     0,   141,
     145,   149,   150,   152,   151,   155,   156,     0,   159,   160,
     161,    61,     0,   275,   278,   284,     0,   283,   286,     0,
       0,   241,   287,    20,    22,   301,    54,    53,   322,     0,
     177,   178,     0,   373,   349,   244,   235,   234,   328,   225,
     307,    45,   332,   328,   329,     0,   325,   207,   208,   220,
     201,   202,   205,   206,   196,   197,     0,   198,   199,   200,
     204,   203,   210,   209,   212,   213,   211,   221,     0,   227,
      63,    51,   340,   307,     0,   340,     0,   336,    45,   344,
     182,     0,   380,    22,   240,   285,     0,   293,    50,    48,
     350,   353,     0,   233,     0,   245,     0,   328,   307,     0,
     340,     0,   324,     0,    45,   331,     0,   224,   335,   340,
     346,   339,   343,     0,   139,    41,    22,     0,   352,     0,
     232,   169,   223,   323,   340,   333,   327,   330,   222,     0,
     195,   338,   347,   294,   351,   354,     0,   326,   334,     0,
       0,   378,   170,     0,    56,    56,   239,   172,     0,   171,
     238
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     2,   199,   317,   221,   420,    40,    41,    42,
      43,   294,   206,    44,   295,   429,   430,   431,   432,   496,
     413,    92,   195,   364,   196,   398,   497,   646,   652,   327,
     328,   329,   243,   377,   378,   357,   358,   359,   361,   332,
     438,   443,   336,   657,   658,   530,    48,   604,    79,   498,
      49,    81,    50,   296,    52,   297,   310,   410,    54,    55,
     312,   415,    56,   224,    57,    58,   298,   299,   211,    61,
     300,    63,    64,    65,   318,    66,   226,    67,   240,   241,
     554,   611,   555,   556,   499,   586,   500,   501,   334,   628,
     601,   602,   242,   393,   197,   244,    69,    70,   246,   400
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -509
static const yytype_int16 yypact[] =
{
    -509,    85,  1435,  -509,  -509,  -509,  -509,  -509,    73,  -509,
    -105,    90,  -509,   205,  -509,  -509,  -509,  -509,    12,   110,
    -509,  -509,  -509,  -509,   258,    12,  -509,   -83,    12,   362,
    -509,  -509,   260,   -70,   262,   362,  -509,  -509,  2569,  -509,
     -58,  -509,  -509,  -509,  -509,  -509,  2253,   -42,   -40,  -509,
    -509,   -31,   -73,  -509,    -9,   -68,    11,    13,   -26,  -509,
    -509,   -17,  -509,    56,    56,    56,    93,  2419,    17,    56,
      21,    33,  -509,  -509,   170,  -509,  -509,   -60,  -509,    40,
    -509,  -509,    70,  -509,  -509,  -509,  2419,  -509,  -509,   -52,
      65,  2325,  -509,   -99,   -97,  -509,  -509,    76,  -509,  -509,
    -509,  -509,  -509,    78,   101,  -509,  -509,  -509,  -509,  -509,
    -509,  -509,  -509,  -509,  -509,   113,  -509,  -509,  -509,   117,
    -509,  -509,  -509,   123,   128,  -509,  -509,  -509,  -509,   130,
     133,   134,   135,   136,  -509,   137,  -509,  -509,   138,  -509,
     140,  -509,  -509,   141,   144,  -509,  -509,  -509,  -509,  -509,
    -509,  -509,  -509,  -509,  -509,  -509,  -509,  -509,   145,  -509,
    -509,  -509,   146,   147,  -509,  -509,  -509,  -509,  -509,  -509,
     148,  -509,  -509,   151,  -509,  -509,  -509,   169,  -509,  -509,
    -509,   173,   174,   175,   177,  -509,  -509,  -509,   179,   180,
    -509,  -509,   182,   185,   188,  -121,  -509,  -509,  -509,  1308,
     495,   274,   342,   285,   289,   303,   189,   191,  -509,  -509,
    -509,  -509,    93,   194,   195,  -509,  -509,  -509,  -509,   -59,
    -509,  -509,  -509,   305,   193,  -509,  -509,  -509,  -509,  -509,
    -509,  -509,  -509,  -509,  -509,  -509,  -509,    93,    93,  -509,
     139,   -57,  -509,  -509,  -509,    56,  -509,  -509,  -509,   196,
     325,  -509,   198,  -509,    93,   201,  -509,   399,   325,   821,
     821,   400,   401,   821,   821,   404,   406,   821,   409,   821,
     821,  1854,   821,   821,   411,   -78,   414,   821,  2419,   821,
     821,  2419,     9,  2419,  2419,   143,   420,   418,  2419,  2569,
     228,  -509,   226,  -509,  -509,  -509,   230,  -509,  -509,  -509,
    -509,    11,   229,  -509,  -509,  -509,   229,   -92,  -509,  -509,
    -128,   252,   -81,   233,   231,  -509,  -509,   596,    71,   234,
    -509,   821,  1110,  1854,  -509,  -509,  -509,   235,   263,  -509,
     232,  -509,   -84,  -509,   264,   -70,   -71,   241,   256,  -509,
    -509,  -509,  -509,  -509,  -509,  -509,  -509,  -509,   253,  -509,
     821,   821,   821,   821,   821,   821,  1039,  2065,  -134,  -509,
    2065,   261,   265,  -509,  -117,   266,   267,   268,   269,   270,
     271,   272,  1231,   273,  2325,    87,   276,   -93,  -509,  2065,
     277,   278,   280,   283,   281,   -87,  1852,   287,  -509,  -509,
    -509,  -509,  -509,   288,   290,   291,  -509,  -509,   292,   279,
     296,   297,   298,  -509,  2569,   459,  -509,  -509,    11,   -19,
    -509,  -509,   306,  2325,   301,   925,   299,   379,   710,    11,
    -509,  2325,  -509,  -509,  -509,  -509,  1143,  -509,   433,   302,
     310,  -509,  -509,  -509,   325,   821,  -509,  2325,  -509,    93,
     307,  -509,   308,  -509,   311,  -509,  -509,  2325,    22,    22,
      22,    22,    22,    22,  1924,   -21,   821,   821,   511,   821,
     821,   821,   821,   821,   821,   821,   821,   821,   821,   821,
     821,   821,   821,   821,   821,   821,   512,   821,   821,  -509,
    -509,  -509,   508,  -509,  -509,  -509,  -509,  -509,  -509,  -509,
    -509,  -509,  -509,    87,  -509,  1562,  -509,    87,  -509,  -509,
    -509,   -39,  -509,   821,  -509,  -509,  -509,  -509,   821,  -509,
    -509,  -509,  -509,  -509,  -509,  -509,  -509,   513,  -509,  -509,
    -509,  -509,   313,  -509,  -509,  -509,    93,  -509,  -509,  2325,
     318,  -509,  -509,  -509,    11,  -509,  -509,  -509,  -509,  1760,
    -509,  -509,    87,   319,  -509,   325,  -509,  -509,   -21,  -509,
    -509,  1688,  -509,   -21,  -509,   320,   -23,   112,   112,  -509,
     732,   732,    68,    68,  1970,  1994,  2044,  1076,  2082,  1018,
      68,    68,    95,    95,    22,    22,    22,  -509,  1949,  -509,
    -509,  -509,    25,  -509,   321,    87,   323,  -509,  1854,  -509,
    -509,   327,  -509,    11,  -509,  -509,   811,  -509,  -509,  -509,
     348,  -509,  -114,  -509,   317,  -509,   328,   341,  -509,   329,
      87,   332,  -509,   821,  1854,  -509,   821,  -509,  -509,    25,
    -509,  -509,  -509,   335,  -509,  -509,    11,   821,  -509,    87,
    -509,  -509,  -509,  -509,    25,  -509,  -509,  -509,    22,   336,
    2065,  -509,  -509,  -509,  -509,  -509,   -10,  -509,  -509,   821,
     364,  -509,  -509,   365,  -136,  -136,  -509,  -509,   345,  -509,
    -509
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -509,  -509,  -509,   504,  -304,  -296,    41,  -509,  -509,  -509,
     149,  -509,  -509,  -509,   548,  -462,  -509,  -509,  -264,  -221,
     -27,    -2,  -509,  -509,  -268,  -509,   -65,  -509,  -509,  -509,
    -509,   119,     2,   286,    48,  -104,  -509,  -254,  -252,  -509,
    -509,  -509,  -509,  -100,  -197,  -509,   153,  -509,    39,   -47,
    -509,   142,   105,    58,  -509,   555,  -509,  -509,   516,  -509,
    -509,  -509,  -509,  -509,   -24,  -509,   556,     1,  -509,  -509,
     557,  -509,  -509,  -309,  -450,   -32,   -14,   -33,  -213,  -509,
    -509,  -509,  -506,  -509,  -508,  -509,  -179,  -509,  -509,  -509,
     -56,  -509,   352,  -509,   294,     6,   -51,  -509,     0,  -509
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -248
static const yytype_int16 yytable[] =
{
      46,   238,    71,    60,    47,   407,    91,   373,    68,   422,
     366,   365,   418,   369,   212,   371,   245,   232,   376,   239,
     324,   403,   210,   383,   319,   320,   412,   649,   230,   231,
     227,   227,   227,   584,   600,   252,   227,   458,    11,    24,
     650,   333,   606,    39,   582,   478,   214,   612,   207,   233,
     228,   229,   213,   168,  -247,   247,   255,   172,   289,   254,
      51,  -247,   482,   232,    38,   629,   656,   311,     9,   425,
     479,    90,    38,    94,   230,   231,    72,    73,   408,    24,
     290,   456,   457,   458,   186,     3,   503,   483,   630,   609,
     230,   231,   503,    75,    76,   233,   230,   231,    74,     9,
     607,   633,   234,   -34,   208,   423,  -247,    80,   256,   232,
     458,   504,   523,  -247,     9,   414,    38,   510,    82,    38,
      86,   600,   436,   535,    15,   232,   623,   458,   235,    38,
      38,   232,   217,   619,    88,   441,   521,   219,   440,   444,
      95,   233,   -30,   322,   198,  -247,   323,   238,   234,    20,
     -32,   396,   639,  -247,   397,   223,   360,   233,   634,   360,
     -29,   322,   215,   233,   588,   239,   372,    85,   550,   379,
      87,   216,   238,   238,   235,   379,   386,   322,   249,   322,
     614,   541,   551,   388,   389,   390,   391,   392,   225,   238,
     239,   239,    26,   218,   234,   313,   651,    46,    46,    71,
      71,    47,    47,   330,    31,    68,    68,   239,    77,    78,
     234,   338,   227,   220,   494,   222,   234,   360,   428,   -31,
     235,   476,   477,   248,   579,   322,   544,   384,   495,   596,
     387,   325,   394,   395,   552,   -33,   235,   402,   597,   375,
     293,   293,   235,    90,    94,   250,   448,   449,   450,   451,
     452,   453,   454,   238,   591,   471,   472,   473,   474,   475,
     236,    83,    84,    89,    78,    93,    78,   476,   477,   374,
     253,   239,   251,   422,   237,   599,   494,   302,    78,   257,
     589,   258,   236,   409,   473,   474,   475,   322,   304,   305,
     495,   375,   306,    78,   476,   477,   237,   625,   422,   471,
     472,   473,   474,   475,   259,   437,   307,    78,   314,   315,
     422,   476,   477,   594,   581,   421,   260,    71,   587,    47,
     261,   374,   451,    68,   455,   422,   262,   552,   230,   231,
     643,   263,   552,   264,   442,   615,   265,   266,   267,   268,
     269,   270,   493,   271,   272,   303,   321,   273,   274,   275,
     276,   277,   557,   558,   278,   560,   561,   562,   563,   564,
     565,   566,   567,   568,   569,   570,   571,   572,   573,   574,
     575,   576,   279,   578,   238,   644,   280,   281,   282,   232,
     283,   526,   284,   285,     9,   286,   552,   330,   287,   212,
     553,   288,   239,   -35,   308,   653,   -36,   -37,   316,   379,
     326,     7,   331,   618,   335,   542,   621,   337,   362,   363,
     524,   233,   367,   529,   368,   548,   421,   370,    71,   380,
      47,   214,   382,   207,    68,   399,   401,   213,   404,   405,
     585,   636,   406,   411,  -247,   416,   417,   543,   424,   435,
     641,   433,   434,   439,    18,   445,   456,   457,   458,   459,
     460,   461,   462,   463,   464,   647,   447,    21,    22,    23,
     446,   238,   508,   375,   234,   480,    25,   522,    19,   481,
     484,   485,   486,   487,   488,   489,   490,   492,   517,   239,
     502,   505,   506,   553,   507,   509,   610,   525,   553,   539,
     235,   512,   513,   374,   514,   515,   516,   212,   605,     4,
     518,   519,   520,   527,    28,   532,   538,   375,    30,   638,
     546,   545,   640,   547,   559,   577,   580,   593,   592,   375,
     595,   603,   631,   360,   613,   620,     5,   622,     6,   627,
     550,   624,   632,   635,     7,     8,   637,   374,     9,   642,
     648,   322,   553,    10,   551,   654,   655,   660,   200,   374,
      45,   590,    11,   540,    12,   659,   375,    53,    59,    62,
      13,    14,   209,    15,   309,   385,   533,    16,   531,   381,
      17,     0,     0,   645,     0,     0,     0,    18,     0,     0,
       0,     0,   375,     0,    19,   292,   374,     0,    20,     0,
      21,    22,    23,    24,   421,     0,    71,     0,    47,    25,
       4,     0,    68,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   374,   465,     0,   466,   467,   468,   469,   470,
     471,   472,   473,   474,   475,     0,     0,     5,     0,     6,
       0,    26,   476,   477,   537,     7,    27,    28,    29,     9,
       0,    30,     0,    31,    10,     0,     0,    32,     0,     0,
       0,     0,     0,     0,    33,    12,    34,     0,    35,     0,
       0,    13,    14,     0,    15,     0,    36,    37,    16,     0,
       0,    17,     0,     0,     0,     0,     0,     0,    18,     0,
       0,     0,     0,     0,     0,    19,     0,     0,     0,    20,
       0,    21,    22,    23,     0,    38,     0,     0,     0,     0,
      25,   301,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     4,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    26,     0,     0,     0,     0,    27,    28,    29,
       0,     5,    30,     6,    31,   456,   457,   458,    32,     7,
     461,   462,     0,     9,     0,    33,     0,    34,    10,    35,
       0,     0,     0,     0,     0,     0,     0,    36,    37,    12,
       0,     0,     0,     0,     0,    13,    14,     0,    15,     0,
       0,     0,    16,     0,     0,    17,     0,     0,     0,     0,
       0,     0,    18,     0,     0,     0,    38,     0,     0,    19,
       0,     0,   419,    20,     0,    21,    22,    23,     0,     0,
       0,     0,     0,     0,    25,     4,     0,     0,     0,     0,
       0,     0,     0,     0,   339,     0,   340,   341,   342,   343,
     344,   345,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     5,     0,     6,     0,    26,     0,     0,     0,
       7,    27,    28,    29,     9,     0,    30,     0,    31,    10,
       0,     0,    32,     0,     0,     0,     0,     0,     0,    33,
      12,    34,     0,    35,     0,     0,    13,    14,     0,    15,
       0,    36,    37,    16,     0,     0,    17,     0,     0,     0,
     346,     0,     0,    18,     0,     0,     0,     0,     0,     0,
      19,     0,     0,     0,    20,     0,    21,    22,    23,     0,
      38,     0,     0,     0,     0,    25,   534,   469,   470,   471,
     472,   473,   474,   475,     0,     0,     0,     0,     0,     4,
       0,   476,   477,     0,     0,     0,   347,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    26,     0,     0,
       0,     0,    27,    28,    29,     0,     5,    30,     6,    31,
       0,     0,     0,    32,     7,     0,   348,     0,     9,     0,
      33,     0,    34,     0,    35,     0,     0,     0,     0,   349,
       0,     0,    36,    37,    12,     0,     0,     0,     0,     0,
     201,    14,     0,    15,     0,     0,     0,    16,     0,     0,
      17,     0,     0,     0,     0,   350,     0,    18,   351,   352,
     353,    38,     0,   354,   355,     0,     0,   626,    20,     0,
      21,    22,    23,     0,   356,     0,     0,     0,     0,    25,
       0,   456,   457,   458,   459,   460,   461,   462,     0,     0,
       0,     0,   339,     4,   340,   341,   342,   343,   344,   345,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    26,     0,     0,     0,     0,    27,    28,    29,     0,
       5,    30,     6,    31,     0,     0,     0,   204,     7,     0,
       0,     0,     9,     0,     0,     0,   205,     0,    35,   456,
     457,   458,   459,   460,   461,   462,    36,    37,    12,     0,
       0,     0,     0,     0,   201,    14,     0,    15,   346,     0,
       0,    16,     0,   339,    17,   340,   341,   342,   343,   344,
     345,    18,     0,     0,     0,    38,     0,     0,     0,     0,
       0,   528,    20,     0,    21,    22,    23,     0,     0,     0,
       0,     0,     0,    25,     0,     0,   339,     0,   340,   341,
     342,   343,   344,   345,   347,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    26,     0,     0,     0,   346,
      27,    28,    29,     0,   348,    30,     0,    31,     0,     0,
       0,   204,     0,     0,     0,     0,     0,   349,     0,     0,
     205,     0,    35,   469,   470,   471,   472,   473,   474,   475,
      36,    37,   346,     0,     0,     0,     0,   476,   477,     0,
       0,     0,     0,   350,     0,   347,   351,   352,   353,     0,
       0,   354,   355,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   356,     0,   456,   457,   458,   459,   460,   461,
     462,   463,   464,     0,     0,   348,     0,     0,   347,   467,
     468,   469,   470,   471,   472,   473,   474,   475,   349,     0,
       0,     0,     0,     0,     0,   476,   477,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   348,     0,
       0,     0,     0,     0,   350,     0,     0,   351,   352,   426,
       0,   349,   354,   355,     0,     0,     0,     0,     0,     0,
       0,   427,     4,   356,     0,     0,     0,     0,     0,     0,
     291,     0,     0,     0,     0,     0,     0,   350,     0,     0,
     351,   352,   353,     0,     0,   354,   355,     0,     0,     5,
       0,     6,     0,     0,   536,     0,   356,     7,     8,     0,
       0,     9,     0,     0,     0,     0,    10,     0,     0,     0,
       0,     0,     0,     0,     0,    11,     0,    12,     0,     0,
       0,     0,     0,    13,    14,     0,    15,     0,     0,     0,
      16,     0,     0,    17,     0,     0,     0,     0,     0,     0,
      18,     0,     0,     0,     0,     0,     0,    19,   292,     0,
       0,    20,     0,    21,    22,    23,    24,     0,     0,     0,
       0,   465,    25,   466,   467,   468,   469,   470,   471,   472,
     473,   474,   475,     0,     0,     0,     0,     0,     0,     0,
     476,   477,     0,     0,     0,   491,     0,     0,     0,     4,
       0,     0,     0,     0,    26,     0,     0,     0,     0,    27,
      28,    29,     0,     0,    30,     0,    31,     0,     0,     0,
      32,     0,     0,     0,     0,     0,     5,    33,     6,    34,
       0,    35,     0,     0,     7,     8,     0,     0,     9,    36,
      37,     0,     0,    10,     0,     0,     0,     0,     0,     0,
       0,     0,    11,     0,    12,     0,     0,     0,     0,     0,
      13,    14,     0,    15,     0,     0,     0,    16,    38,     0,
      17,     0,     0,     0,     0,     0,     0,    18,     0,     0,
       0,     0,     0,     0,    19,     0,     0,     0,    20,     0,
      21,    22,    23,    24,     0,     0,     0,     0,     0,    25,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     4,     0,     0,     0,
       0,    26,     0,     0,     0,     0,    27,    28,    29,     0,
       0,    30,     0,    31,     0,     0,     0,    32,     0,     0,
       0,     0,     0,     5,    33,     6,    34,     0,    35,     0,
     232,     7,     0,     0,     0,     9,    36,    37,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    12,     0,     0,     0,     0,     0,   201,    14,     0,
      15,     0,   233,     0,    16,    38,     0,    17,     0,     0,
       0,     0,     0,     0,    18,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    20,     0,    21,    22,    23,
       0,     0,     0,     0,     0,     0,    25,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   234,     0,     0,     0,     0,
       0,     0,     4,     0,     0,     0,     0,     0,    26,     0,
       0,     0,     0,    27,    28,    29,     0,     0,    30,     0,
      31,   235,     0,     0,   204,     0,     0,     0,     0,     5,
       0,     6,     0,   205,     0,    35,   232,     7,     0,     0,
       0,     9,     0,    36,    37,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    12,     0,     0,
       0,   583,     0,   201,    14,     0,    15,     0,   233,     0,
      16,     0,    38,    17,     4,     0,     0,     0,     0,     0,
      18,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    20,   598,    21,    22,    23,     0,     0,     0,     0,
       0,     5,    25,     6,     0,     0,     0,     0,     0,     7,
       0,     0,     0,     9,     0,     0,     0,     0,     0,     0,
       0,   234,     0,     0,     0,     0,     0,     0,     0,    12,
       0,     0,     0,     0,    26,   201,    14,     0,    15,    27,
      28,    29,    16,     0,    30,    17,    31,   235,     0,     0,
     204,     0,    18,     0,     0,     0,     0,     0,     0,   205,
       0,    35,     0,    20,     0,    21,    22,    23,     4,    36,
      37,     0,     0,     0,    25,   456,   457,   458,   459,   460,
     461,   462,   463,   464,     0,     0,     0,   608,     0,     0,
       0,     0,     0,     0,     0,     5,     0,     6,    38,     0,
       0,     0,     0,     7,     0,     0,    26,     9,     0,     0,
       0,    27,    28,    29,     0,     0,    30,     0,    31,     0,
       0,     0,   204,    12,     0,     0,     0,     0,     0,   201,
      14,   205,    15,    35,     0,     0,    16,     0,     0,    17,
       0,    36,    37,     0,     0,     0,    18,   456,   457,   458,
     459,   460,   461,   462,   463,   464,     0,    20,     0,    21,
      22,    23,     0,     0,     0,     0,     0,     0,    25,     0,
      38,     0,   456,   457,   458,   459,   460,   461,   462,   463,
     464,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   456,   457,   458,   459,   460,   461,   462,
      26,   464,     0,     0,     0,    27,    28,    29,     0,     0,
      30,     0,    31,     0,     0,     0,   204,   456,   457,   458,
     459,   460,   461,   462,     0,   205,     0,    35,     0,     0,
       0,     0,     0,     0,     0,    36,    37,     0,     0,     0,
       0,     0,   465,     0,   466,   467,   468,   469,   470,   471,
     472,   473,   474,   475,     0,     0,     0,     0,     0,     0,
       0,   476,   477,     0,    38,     0,   511,   456,   457,   458,
     459,   460,   461,   462,   463,   464,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   456,   457,
     458,   459,   460,   461,   462,   463,   464,     0,     0,     0,
       0,     0,     0,     0,     0,   456,   457,   458,   459,   460,
     461,   462,     0,     0,   465,     0,   466,   467,   468,   469,
     470,   471,   472,   473,   474,   475,     0,     0,     0,     0,
       0,     0,     0,   476,   477,     0,     0,     0,   549,   465,
       0,   466,   467,   468,   469,   470,   471,   472,   473,   474,
     475,     0,     0,     0,     0,     0,     0,     0,   476,   477,
     617,     0,   466,   467,   468,   469,   470,   471,   472,   473,
     474,   475,     0,     0,     0,     0,     0,     0,     0,   476,
     477,     0,     0,     0,     0,     0,   466,   467,   468,   469,
     470,   471,   472,   473,   474,   475,     0,     0,     0,     0,
       0,     0,     0,   476,   477,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   465,   616,   466,   467,   468,   469,
     470,   471,   472,   473,   474,   475,     0,     0,     0,     0,
       0,     0,     0,   476,   477,   465,     0,   466,   467,   468,
     469,   470,   471,   472,   473,   474,   475,     4,     0,     0,
       0,     0,     0,     0,   476,   477,   468,   469,   470,   471,
     472,   473,   474,   475,     0,     0,     0,     0,     0,     0,
       0,   476,   477,     0,     5,     0,     6,     0,     0,     0,
       0,     0,     7,     8,     0,     0,     9,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      11,     0,    12,     0,     0,     0,     0,     0,   201,    14,
       0,    15,     0,     0,     0,    16,     0,     0,    17,     4,
       0,     0,     0,     0,     0,    18,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    20,     0,    21,    22,
      23,    24,     0,     0,   202,     0,     5,    25,     6,     0,
       0,   203,     0,     0,     7,     0,     0,     0,     9,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    12,     0,     0,     0,     0,    26,
     201,    14,     0,    15,    27,    28,    29,    16,     0,    30,
      17,    31,     0,     0,     0,   204,     0,    18,     0,     0,
       0,     0,     0,     0,   205,     0,    35,     0,    20,     0,
      21,    22,    23,     4,    36,    37,     0,     0,     0,    25,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       5,     0,     6,     0,     0,     0,     0,     0,     7,     0,
       0,    26,     0,     0,     0,     0,    27,    28,    29,     0,
       0,    30,     0,    31,     0,     0,     0,   204,    12,     0,
       0,     0,     0,     0,   201,    14,   205,     0,    35,     0,
       0,    16,     0,     0,    17,     0,    36,    37,     0,     0,
       0,    18,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    21,    22,    23,     0,     0,     0,
       0,     0,     0,    25,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      27,    28,    29,     0,     0,    30,     0,     0,     0,     0,
       0,   204,     0,     0,     0,     0,     0,     0,     0,     0,
     205,     0,    35,     0,     0,     0,     0,     0,     0,     0,
      36,    37,    96,     0,    97,    98,    99,     0,   100,   101,
       0,   102,     0,     0,   103,     0,   104,     0,     0,     0,
     105,   106,     0,   107,   108,   109,   110,     0,   111,   112,
     113,   114,   115,   116,   117,   118,     0,   119,     0,   120,
     121,   122,   123,   124,     0,     0,   125,     0,     0,     0,
     126,     0,   127,   128,     0,   129,   130,   131,   132,   133,
     134,     0,   135,   136,   137,   138,   139,   140,     0,     0,
     141,     0,     0,   142,     0,     0,     0,     0,   143,   144,
       0,   145,   146,     0,   147,   148,     0,     0,   149,   150,
     151,   152,   153,   154,     0,   155,   156,   157,   158,   159,
     160,   161,     0,   162,   163,     0,   164,   165,   166,   167,
     168,   169,   170,   171,   172,     0,   173,   174,   175,   176,
       0,     0,     0,   177,     0,     0,   178,     0,     0,   179,
     180,     0,     0,   181,   182,   183,   184,     0,     0,   185,
       0,   186,     0,   187,   188,   189,   190,   191,   192,   193,
       0,     0,   194
};

static const yytype_int16 yycheck[] =
{
       2,    66,     2,     2,     2,   301,    33,   271,     2,   318,
     264,   263,   316,   267,    46,   269,    67,    38,   272,    66,
     241,   289,    46,   277,   237,   238,   107,    37,     3,     4,
      63,    64,    65,   495,   542,    86,    69,    15,    57,    98,
      50,   254,   548,     2,   494,   179,    46,   553,    46,    70,
      64,    65,    46,   131,   153,    69,   153,   135,   179,    91,
       2,   153,   179,    38,   200,   179,   202,   126,    43,   321,
     204,    32,   200,    34,     3,     4,     3,     4,   206,    98,
     201,    13,    14,    15,   162,     0,   179,   204,   202,   551,
       3,     4,   179,     3,     4,    70,     3,     4,   203,    43,
     550,   607,   123,   202,    46,   318,   205,    95,   205,    38,
      15,   204,   408,   205,    43,   312,   200,   204,     8,   200,
     203,   629,   206,   419,    68,    38,   588,    15,   149,   200,
     200,    38,   205,   583,    29,   206,   404,   205,   335,   336,
      35,    70,   202,   200,   202,   205,   203,   212,   123,    93,
     202,     8,   614,   205,    11,   181,   260,    70,   608,   263,
     202,   200,   202,    70,   203,   212,   270,    25,   189,   273,
      28,   202,   237,   238,   149,   279,   280,   200,     8,   200,
     203,   435,   203,   174,   175,   176,   177,   178,   205,   254,
     237,   238,   136,   202,   123,   219,   206,   199,   200,   199,
     200,   199,   200,   250,   148,   199,   200,   254,     3,     4,
     123,   258,   245,   202,   189,   202,   123,   321,   322,   202,
     149,   199,   200,   202,   478,   200,   439,   278,   203,   533,
     281,   245,   283,   284,   455,   202,   149,   288,   534,   271,
     199,   200,   149,   204,   205,   205,   350,   351,   352,   353,
     354,   355,   356,   318,   508,   187,   188,   189,   190,   191,
     189,     3,     4,     3,     4,     3,     4,   199,   200,   271,
     205,   318,   202,   582,   203,   539,   189,     3,     4,   203,
     501,   203,   189,   310,   189,   190,   191,   200,     3,     4,
     203,   323,     3,     4,   199,   200,   203,   593,   607,   187,
     188,   189,   190,   191,   203,   332,     3,     4,     3,     4,
     619,   199,   200,   526,   493,   317,   203,   317,   497,   317,
     203,   323,   426,   317,   356,   634,   203,   548,     3,     4,
     626,   203,   553,   203,   336,   556,   203,   203,   203,   203,
     203,   203,   374,   203,   203,     3,   207,   203,   203,   203,
     203,   203,   456,   457,   203,   459,   460,   461,   462,   463,
     464,   465,   466,   467,   468,   469,   470,   471,   472,   473,
     474,   475,   203,   477,   439,   627,   203,   203,   203,    38,
     203,   413,   203,   203,    43,   203,   607,   434,   203,   421,
     455,   203,   439,   202,   205,   649,   202,   202,   205,   503,
     204,    39,   204,   582,   203,   437,   585,     8,     8,     8,
     409,    70,     8,   415,     8,   447,   418,     8,   418,     8,
     418,   421,     8,   421,   418,     5,     8,   421,   200,   203,
     495,   610,   202,   181,   205,   202,   205,   437,   204,   207,
     619,   206,   179,   179,    82,   204,    13,    14,    15,    16,
      17,    18,    19,    20,    21,   634,   203,    95,    96,    97,
     204,   526,   179,   495,   123,   204,   104,     8,    89,   204,
     204,   204,   204,   204,   204,   204,   204,   204,   199,   526,
     204,   204,   204,   548,   204,   204,   551,   181,   553,   179,
     149,   204,   204,   495,   204,   204,   204,   529,   545,     4,
     204,   204,   204,   202,   142,   206,   204,   539,   146,   613,
     202,   204,   616,   202,     3,     3,     8,   204,     5,   551,
     202,   202,   205,   627,   204,   204,    31,   204,    33,   181,
     189,   204,   204,   204,    39,    40,   204,   539,    43,   204,
     204,   200,   607,    48,   203,   181,   181,   202,    44,   551,
       2,   503,    57,   434,    59,   655,   588,     2,     2,     2,
      65,    66,    46,    68,   212,   279,   417,    72,   415,   275,
      75,    -1,    -1,   629,    -1,    -1,    -1,    82,    -1,    -1,
      -1,    -1,   614,    -1,    89,    90,   588,    -1,    93,    -1,
      95,    96,    97,    98,   596,    -1,   596,    -1,   596,   104,
       4,    -1,   596,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   614,   180,    -1,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,    -1,    -1,    31,    -1,    33,
      -1,   136,   199,   200,   201,    39,   141,   142,   143,    43,
      -1,   146,    -1,   148,    48,    -1,    -1,   152,    -1,    -1,
      -1,    -1,    -1,    -1,   159,    59,   161,    -1,   163,    -1,
      -1,    65,    66,    -1,    68,    -1,   171,   172,    72,    -1,
      -1,    75,    -1,    -1,    -1,    -1,    -1,    -1,    82,    -1,
      -1,    -1,    -1,    -1,    -1,    89,    -1,    -1,    -1,    93,
      -1,    95,    96,    97,    -1,   200,    -1,    -1,    -1,    -1,
     104,   206,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     4,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   136,    -1,    -1,    -1,    -1,   141,   142,   143,
      -1,    31,   146,    33,   148,    13,    14,    15,   152,    39,
      18,    19,    -1,    43,    -1,   159,    -1,   161,    48,   163,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   171,   172,    59,
      -1,    -1,    -1,    -1,    -1,    65,    66,    -1,    68,    -1,
      -1,    -1,    72,    -1,    -1,    75,    -1,    -1,    -1,    -1,
      -1,    -1,    82,    -1,    -1,    -1,   200,    -1,    -1,    89,
      -1,    -1,   206,    93,    -1,    95,    96,    97,    -1,    -1,
      -1,    -1,    -1,    -1,   104,     4,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     3,    -1,     5,     6,     7,     8,
       9,    10,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    31,    -1,    33,    -1,   136,    -1,    -1,    -1,
      39,   141,   142,   143,    43,    -1,   146,    -1,   148,    48,
      -1,    -1,   152,    -1,    -1,    -1,    -1,    -1,    -1,   159,
      59,   161,    -1,   163,    -1,    -1,    65,    66,    -1,    68,
      -1,   171,   172,    72,    -1,    -1,    75,    -1,    -1,    -1,
      69,    -1,    -1,    82,    -1,    -1,    -1,    -1,    -1,    -1,
      89,    -1,    -1,    -1,    93,    -1,    95,    96,    97,    -1,
     200,    -1,    -1,    -1,    -1,   104,   206,   185,   186,   187,
     188,   189,   190,   191,    -1,    -1,    -1,    -1,    -1,     4,
      -1,   199,   200,    -1,    -1,    -1,   115,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   136,    -1,    -1,
      -1,    -1,   141,   142,   143,    -1,    31,   146,    33,   148,
      -1,    -1,    -1,   152,    39,    -1,   145,    -1,    43,    -1,
     159,    -1,   161,    -1,   163,    -1,    -1,    -1,    -1,   158,
      -1,    -1,   171,   172,    59,    -1,    -1,    -1,    -1,    -1,
      65,    66,    -1,    68,    -1,    -1,    -1,    72,    -1,    -1,
      75,    -1,    -1,    -1,    -1,   184,    -1,    82,   187,   188,
     189,   200,    -1,   192,   193,    -1,    -1,   206,    93,    -1,
      95,    96,    97,    -1,   203,    -1,    -1,    -1,    -1,   104,
      -1,    13,    14,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,     3,     4,     5,     6,     7,     8,     9,    10,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   136,    -1,    -1,    -1,    -1,   141,   142,   143,    -1,
      31,   146,    33,   148,    -1,    -1,    -1,   152,    39,    -1,
      -1,    -1,    43,    -1,    -1,    -1,   161,    -1,   163,    13,
      14,    15,    16,    17,    18,    19,   171,   172,    59,    -1,
      -1,    -1,    -1,    -1,    65,    66,    -1,    68,    69,    -1,
      -1,    72,    -1,     3,    75,     5,     6,     7,     8,     9,
      10,    82,    -1,    -1,    -1,   200,    -1,    -1,    -1,    -1,
      -1,   206,    93,    -1,    95,    96,    97,    -1,    -1,    -1,
      -1,    -1,    -1,   104,    -1,    -1,     3,    -1,     5,     6,
       7,     8,     9,    10,   115,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   136,    -1,    -1,    -1,    69,
     141,   142,   143,    -1,   145,   146,    -1,   148,    -1,    -1,
      -1,   152,    -1,    -1,    -1,    -1,    -1,   158,    -1,    -1,
     161,    -1,   163,   185,   186,   187,   188,   189,   190,   191,
     171,   172,    69,    -1,    -1,    -1,    -1,   199,   200,    -1,
      -1,    -1,    -1,   184,    -1,   115,   187,   188,   189,    -1,
      -1,   192,   193,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   203,    -1,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    -1,    -1,   145,    -1,    -1,   115,   183,
     184,   185,   186,   187,   188,   189,   190,   191,   158,    -1,
      -1,    -1,    -1,    -1,    -1,   199,   200,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   145,    -1,
      -1,    -1,    -1,    -1,   184,    -1,    -1,   187,   188,   189,
      -1,   158,   192,   193,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   201,     4,   203,    -1,    -1,    -1,    -1,    -1,    -1,
      12,    -1,    -1,    -1,    -1,    -1,    -1,   184,    -1,    -1,
     187,   188,   189,    -1,    -1,   192,   193,    -1,    -1,    31,
      -1,    33,    -1,    -1,   201,    -1,   203,    39,    40,    -1,
      -1,    43,    -1,    -1,    -1,    -1,    48,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    57,    -1,    59,    -1,    -1,
      -1,    -1,    -1,    65,    66,    -1,    68,    -1,    -1,    -1,
      72,    -1,    -1,    75,    -1,    -1,    -1,    -1,    -1,    -1,
      82,    -1,    -1,    -1,    -1,    -1,    -1,    89,    90,    -1,
      -1,    93,    -1,    95,    96,    97,    98,    -1,    -1,    -1,
      -1,   180,   104,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     199,   200,    -1,    -1,    -1,   204,    -1,    -1,    -1,     4,
      -1,    -1,    -1,    -1,   136,    -1,    -1,    -1,    -1,   141,
     142,   143,    -1,    -1,   146,    -1,   148,    -1,    -1,    -1,
     152,    -1,    -1,    -1,    -1,    -1,    31,   159,    33,   161,
      -1,   163,    -1,    -1,    39,    40,    -1,    -1,    43,   171,
     172,    -1,    -1,    48,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    57,    -1,    59,    -1,    -1,    -1,    -1,    -1,
      65,    66,    -1,    68,    -1,    -1,    -1,    72,   200,    -1,
      75,    -1,    -1,    -1,    -1,    -1,    -1,    82,    -1,    -1,
      -1,    -1,    -1,    -1,    89,    -1,    -1,    -1,    93,    -1,
      95,    96,    97,    98,    -1,    -1,    -1,    -1,    -1,   104,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     4,    -1,    -1,    -1,
      -1,   136,    -1,    -1,    -1,    -1,   141,   142,   143,    -1,
      -1,   146,    -1,   148,    -1,    -1,    -1,   152,    -1,    -1,
      -1,    -1,    -1,    31,   159,    33,   161,    -1,   163,    -1,
      38,    39,    -1,    -1,    -1,    43,   171,   172,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    59,    -1,    -1,    -1,    -1,    -1,    65,    66,    -1,
      68,    -1,    70,    -1,    72,   200,    -1,    75,    -1,    -1,
      -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    93,    -1,    95,    96,    97,
      -1,    -1,    -1,    -1,    -1,    -1,   104,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   123,    -1,    -1,    -1,    -1,
      -1,    -1,     4,    -1,    -1,    -1,    -1,    -1,   136,    -1,
      -1,    -1,    -1,   141,   142,   143,    -1,    -1,   146,    -1,
     148,   149,    -1,    -1,   152,    -1,    -1,    -1,    -1,    31,
      -1,    33,    -1,   161,    -1,   163,    38,    39,    -1,    -1,
      -1,    43,    -1,   171,   172,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    59,    -1,    -1,
      -1,   189,    -1,    65,    66,    -1,    68,    -1,    70,    -1,
      72,    -1,   200,    75,     4,    -1,    -1,    -1,    -1,    -1,
      82,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    93,    22,    95,    96,    97,    -1,    -1,    -1,    -1,
      -1,    31,   104,    33,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    43,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   123,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    59,
      -1,    -1,    -1,    -1,   136,    65,    66,    -1,    68,   141,
     142,   143,    72,    -1,   146,    75,   148,   149,    -1,    -1,
     152,    -1,    82,    -1,    -1,    -1,    -1,    -1,    -1,   161,
      -1,   163,    -1,    93,    -1,    95,    96,    97,     4,   171,
     172,    -1,    -1,    -1,   104,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    -1,    -1,    -1,   189,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    31,    -1,    33,   200,    -1,
      -1,    -1,    -1,    39,    -1,    -1,   136,    43,    -1,    -1,
      -1,   141,   142,   143,    -1,    -1,   146,    -1,   148,    -1,
      -1,    -1,   152,    59,    -1,    -1,    -1,    -1,    -1,    65,
      66,   161,    68,   163,    -1,    -1,    72,    -1,    -1,    75,
      -1,   171,   172,    -1,    -1,    -1,    82,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    -1,    93,    -1,    95,
      96,    97,    -1,    -1,    -1,    -1,    -1,    -1,   104,    -1,
     200,    -1,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    13,    14,    15,    16,    17,    18,    19,
     136,    21,    -1,    -1,    -1,   141,   142,   143,    -1,    -1,
     146,    -1,   148,    -1,    -1,    -1,   152,    13,    14,    15,
      16,    17,    18,    19,    -1,   161,    -1,   163,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   171,   172,    -1,    -1,    -1,
      -1,    -1,   180,    -1,   182,   183,   184,   185,   186,   187,
     188,   189,   190,   191,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   199,   200,    -1,   200,    -1,   204,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    13,    14,    15,    16,    17,
      18,    19,    -1,    -1,   180,    -1,   182,   183,   184,   185,
     186,   187,   188,   189,   190,   191,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   199,   200,    -1,    -1,    -1,   204,   180,
      -1,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   199,   200,
     201,    -1,   182,   183,   184,   185,   186,   187,   188,   189,
     190,   191,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   199,
     200,    -1,    -1,    -1,    -1,    -1,   182,   183,   184,   185,
     186,   187,   188,   189,   190,   191,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   180,   181,   182,   183,   184,   185,
     186,   187,   188,   189,   190,   191,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   199,   200,   180,    -1,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,     4,    -1,    -1,
      -1,    -1,    -1,    -1,   199,   200,   184,   185,   186,   187,
     188,   189,   190,   191,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   199,   200,    -1,    31,    -1,    33,    -1,    -1,    -1,
      -1,    -1,    39,    40,    -1,    -1,    43,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      57,    -1,    59,    -1,    -1,    -1,    -1,    -1,    65,    66,
      -1,    68,    -1,    -1,    -1,    72,    -1,    -1,    75,     4,
      -1,    -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    93,    -1,    95,    96,
      97,    98,    -1,    -1,   101,    -1,    31,   104,    33,    -1,
      -1,   108,    -1,    -1,    39,    -1,    -1,    -1,    43,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    59,    -1,    -1,    -1,    -1,   136,
      65,    66,    -1,    68,   141,   142,   143,    72,    -1,   146,
      75,   148,    -1,    -1,    -1,   152,    -1,    82,    -1,    -1,
      -1,    -1,    -1,    -1,   161,    -1,   163,    -1,    93,    -1,
      95,    96,    97,     4,   171,   172,    -1,    -1,    -1,   104,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      31,    -1,    33,    -1,    -1,    -1,    -1,    -1,    39,    -1,
      -1,   136,    -1,    -1,    -1,    -1,   141,   142,   143,    -1,
      -1,   146,    -1,   148,    -1,    -1,    -1,   152,    59,    -1,
      -1,    -1,    -1,    -1,    65,    66,   161,    -1,   163,    -1,
      -1,    72,    -1,    -1,    75,    -1,   171,   172,    -1,    -1,
      -1,    82,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    95,    96,    97,    -1,    -1,    -1,
      -1,    -1,    -1,   104,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     141,   142,   143,    -1,    -1,   146,    -1,    -1,    -1,    -1,
      -1,   152,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     161,    -1,   163,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     171,   172,    23,    -1,    25,    26,    27,    -1,    29,    30,
      -1,    32,    -1,    -1,    35,    -1,    37,    -1,    -1,    -1,
      41,    42,    -1,    44,    45,    46,    47,    -1,    49,    50,
      51,    52,    53,    54,    55,    56,    -1,    58,    -1,    60,
      61,    62,    63,    64,    -1,    -1,    67,    -1,    -1,    -1,
      71,    -1,    73,    74,    -1,    76,    77,    78,    79,    80,
      81,    -1,    83,    84,    85,    86,    87,    88,    -1,    -1,
      91,    -1,    -1,    94,    -1,    -1,    -1,    -1,    99,   100,
      -1,   102,   103,    -1,   105,   106,    -1,    -1,   109,   110,
     111,   112,   113,   114,    -1,   116,   117,   118,   119,   120,
     121,   122,    -1,   124,   125,    -1,   127,   128,   129,   130,
     131,   132,   133,   134,   135,    -1,   137,   138,   139,   140,
      -1,    -1,    -1,   144,    -1,    -1,   147,    -1,    -1,   150,
     151,    -1,    -1,   154,   155,   156,   157,    -1,    -1,   160,
      -1,   162,    -1,   164,   165,   166,   167,   168,   169,   170,
      -1,    -1,   173
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   209,   210,     0,     4,    31,    33,    39,    40,    43,
      48,    57,    59,    65,    66,    68,    72,    75,    82,    89,
      93,    95,    96,    97,    98,   104,   136,   141,   142,   143,
     146,   148,   152,   159,   161,   163,   171,   172,   200,   214,
     215,   216,   217,   218,   221,   222,   229,   240,   254,   258,
     260,   261,   262,   263,   266,   267,   270,   272,   273,   274,
     275,   277,   278,   279,   280,   281,   283,   285,   303,   304,
     305,   306,     3,     4,   203,     3,     4,     3,     4,   256,
      95,   259,     8,     3,     4,   259,   203,   259,   260,     3,
     256,   228,   229,     3,   256,   260,    23,    25,    26,    27,
      29,    30,    32,    35,    37,    41,    42,    44,    45,    46,
      47,    49,    50,    51,    52,    53,    54,    55,    56,    58,
      60,    61,    62,    63,    64,    67,    71,    73,    74,    76,
      77,    78,    79,    80,    81,    83,    84,    85,    86,    87,
      88,    91,    94,    99,   100,   102,   103,   105,   106,   109,
     110,   111,   112,   113,   114,   116,   117,   118,   119,   120,
     121,   122,   124,   125,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   137,   138,   139,   140,   144,   147,   150,
     151,   154,   155,   156,   157,   160,   162,   164,   165,   166,
     167,   168,   169,   170,   173,   230,   232,   302,   202,   211,
     211,    65,   101,   108,   152,   161,   220,   240,   261,   266,
     272,   276,   283,   303,   306,   202,   202,   205,   202,   205,
     202,   213,   202,   181,   271,   205,   284,   285,   284,   284,
       3,     4,    38,    70,   123,   149,   189,   203,   234,   257,
     286,   287,   300,   240,   303,   304,   306,   284,   202,     8,
     205,   202,   304,   205,   283,   153,   205,   203,   203,   203,
     203,   203,   203,   203,   203,   203,   203,   203,   203,   203,
     203,   203,   203,   203,   203,   203,   203,   203,   203,   203,
     203,   203,   203,   203,   203,   203,   203,   203,   203,   179,
     201,    12,    90,   214,   219,   222,   261,   263,   274,   275,
     278,   206,     3,     3,     3,     4,     3,     3,   205,   300,
     264,   126,   268,   272,     3,     4,   205,   212,   282,   286,
     286,   207,   200,   203,   227,   284,   204,   237,   238,   239,
     257,   204,   247,   286,   296,   203,   250,     8,   257,     3,
       5,     6,     7,     8,     9,    10,    69,   115,   145,   158,
     184,   187,   188,   189,   192,   193,   203,   243,   244,   245,
     243,   246,     8,     8,   231,   246,   245,     8,     8,   245,
       8,   245,   243,   226,   229,   283,   245,   241,   242,   243,
       8,   302,     8,   245,   304,   241,   243,   304,   174,   175,
     176,   177,   178,   301,   304,   304,     8,    11,   233,     5,
     307,     8,   304,   232,   200,   203,   202,   213,   206,   228,
     265,   181,   107,   228,   252,   269,   202,   205,   212,   206,
     214,   229,   281,   286,   204,   246,   189,   201,   243,   223,
     224,   225,   226,   206,   179,   207,   206,   228,   248,   179,
     252,   206,   229,   249,   252,   204,   204,   203,   243,   243,
     243,   243,   243,   243,   243,   283,    13,    14,    15,    16,
      17,    18,    19,    20,    21,   180,   182,   183,   184,   185,
     186,   187,   188,   189,   190,   191,   199,   200,   179,   204,
     204,   204,   179,   204,   204,   204,   204,   204,   204,   204,
     204,   204,   204,   283,   189,   203,   227,   234,   257,   292,
     294,   295,   204,   179,   204,   204,   204,   204,   179,   204,
     204,   204,   204,   204,   204,   204,   204,   199,   204,   204,
     204,   232,     8,   213,   275,   181,   283,   202,   206,   229,
     253,   254,   206,   218,   206,   213,   201,   201,   204,   179,
     239,   245,   283,   306,   286,   204,   202,   202,   283,   204,
     189,   203,   227,   234,   288,   290,   291,   243,   243,     3,
     243,   243,   243,   243,   243,   243,   243,   243,   243,   243,
     243,   243,   243,   243,   243,   243,   243,     3,   243,   245,
       8,   294,   282,   189,   223,   234,   293,   294,   203,   227,
     242,   245,     5,   204,   286,   202,   212,   213,    22,   226,
     292,   298,   299,   202,   255,   257,   290,   282,   189,   223,
     234,   289,   290,   204,   203,   227,   181,   201,   294,   282,
     204,   294,   204,   223,   204,   213,   206,   181,   297,   179,
     202,   205,   204,   290,   282,   204,   294,   204,   243,   223,
     243,   294,   204,   213,   246,   298,   235,   294,   204,    37,
      50,   206,   236,   245,   181,   181,   202,   251,   252,   251,
     202
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
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

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
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
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
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
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
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
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


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

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
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

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
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
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

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
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

/* Line 1464 of yacc.c  */
#line 328 "parser.y"
    { fix_incomplete();
						  check_statements((yyvsp[(1) - (1)].stmt_list), FALSE);
						  check_all_user_types((yyvsp[(1) - (1)].stmt_list));
						  write_header((yyvsp[(1) - (1)].stmt_list));
						  write_id_data((yyvsp[(1) - (1)].stmt_list));
						  write_proxies((yyvsp[(1) - (1)].stmt_list));
						  write_client((yyvsp[(1) - (1)].stmt_list));
						  write_server((yyvsp[(1) - (1)].stmt_list));
						  write_regscript((yyvsp[(1) - (1)].stmt_list));
						  write_dlldata((yyvsp[(1) - (1)].stmt_list));
						  write_local_stubs((yyvsp[(1) - (1)].stmt_list));
						;}
    break;

  case 3:

/* Line 1464 of yacc.c  */
#line 342 "parser.y"
    { (yyval.stmt_list) = NULL; ;}
    break;

  case 4:

/* Line 1464 of yacc.c  */
#line 343 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_reference((yyvsp[(2) - (2)].type))); ;}
    break;

  case 5:

/* Line 1464 of yacc.c  */
#line 344 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_type_decl((yyvsp[(2) - (2)].type))); ;}
    break;

  case 6:

/* Line 1464 of yacc.c  */
#line 345 "parser.y"
    { (yyval.stmt_list) = (yyvsp[(1) - (3)].stmt_list);
						  reg_type((yyvsp[(2) - (3)].type), (yyvsp[(2) - (3)].type)->name, 0);
						;}
    break;

  case 7:

/* Line 1464 of yacc.c  */
#line 348 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_type_decl((yyvsp[(2) - (2)].type)));
						  reg_type((yyvsp[(2) - (2)].type), (yyvsp[(2) - (2)].type)->name, 0);
						;}
    break;

  case 8:

/* Line 1464 of yacc.c  */
#line 351 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_module((yyvsp[(2) - (2)].type))); ;}
    break;

  case 9:

/* Line 1464 of yacc.c  */
#line 352 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_library((yyvsp[(2) - (2)].typelib))); ;}
    break;

  case 10:

/* Line 1464 of yacc.c  */
#line 353 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), (yyvsp[(2) - (2)].statement)); ;}
    break;

  case 11:

/* Line 1464 of yacc.c  */
#line 356 "parser.y"
    { (yyval.stmt_list) = NULL; ;}
    break;

  case 12:

/* Line 1464 of yacc.c  */
#line 357 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_reference((yyvsp[(2) - (2)].type))); ;}
    break;

  case 13:

/* Line 1464 of yacc.c  */
#line 358 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_type_decl((yyvsp[(2) - (2)].type))); ;}
    break;

  case 14:

/* Line 1464 of yacc.c  */
#line 359 "parser.y"
    { (yyval.stmt_list) = (yyvsp[(1) - (3)].stmt_list); reg_type((yyvsp[(2) - (3)].type), (yyvsp[(2) - (3)].type)->name, 0); ;}
    break;

  case 15:

/* Line 1464 of yacc.c  */
#line 360 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_type_decl((yyvsp[(2) - (2)].type)));
						  reg_type((yyvsp[(2) - (2)].type), (yyvsp[(2) - (2)].type)->name, 0);
						;}
    break;

  case 16:

/* Line 1464 of yacc.c  */
#line 363 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_module((yyvsp[(2) - (2)].type))); ;}
    break;

  case 17:

/* Line 1464 of yacc.c  */
#line 364 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), (yyvsp[(2) - (2)].statement)); ;}
    break;

  case 18:

/* Line 1464 of yacc.c  */
#line 365 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_importlib((yyvsp[(2) - (2)].str))); ;}
    break;

  case 19:

/* Line 1464 of yacc.c  */
#line 366 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_library((yyvsp[(2) - (2)].typelib))); ;}
    break;

  case 20:

/* Line 1464 of yacc.c  */
#line 369 "parser.y"
    { (yyval.stmt_list) = NULL; ;}
    break;

  case 21:

/* Line 1464 of yacc.c  */
#line 370 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), (yyvsp[(2) - (2)].statement)); ;}
    break;

  case 24:

/* Line 1464 of yacc.c  */
#line 378 "parser.y"
    { (yyval.statement) = make_statement_cppquote((yyvsp[(1) - (1)].str)); ;}
    break;

  case 25:

/* Line 1464 of yacc.c  */
#line 379 "parser.y"
    { (yyval.statement) = make_statement_type_decl((yyvsp[(1) - (2)].type)); ;}
    break;

  case 26:

/* Line 1464 of yacc.c  */
#line 380 "parser.y"
    { (yyval.statement) = make_statement_declaration((yyvsp[(1) - (2)].var)); ;}
    break;

  case 27:

/* Line 1464 of yacc.c  */
#line 381 "parser.y"
    { (yyval.statement) = make_statement_import((yyvsp[(1) - (1)].str)); ;}
    break;

  case 28:

/* Line 1464 of yacc.c  */
#line 382 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (2)].statement); ;}
    break;

  case 30:

/* Line 1464 of yacc.c  */
#line 387 "parser.y"
    { (yyval.type) = type_new_enum((yyvsp[(2) - (2)].str), FALSE, NULL); ;}
    break;

  case 32:

/* Line 1464 of yacc.c  */
#line 389 "parser.y"
    { (yyval.type) = type_new_struct((yyvsp[(2) - (2)].str), FALSE, NULL); ;}
    break;

  case 34:

/* Line 1464 of yacc.c  */
#line 391 "parser.y"
    { (yyval.type) = type_new_nonencapsulated_union((yyvsp[(2) - (2)].str), FALSE, NULL); ;}
    break;

  case 35:

/* Line 1464 of yacc.c  */
#line 392 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type); (yyval.type)->attrs = check_enum_attrs((yyvsp[(1) - (2)].attr_list)); ;}
    break;

  case 36:

/* Line 1464 of yacc.c  */
#line 393 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type); (yyval.type)->attrs = check_struct_attrs((yyvsp[(1) - (2)].attr_list)); ;}
    break;

  case 37:

/* Line 1464 of yacc.c  */
#line 394 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type); (yyval.type)->attrs = check_union_attrs((yyvsp[(1) - (2)].attr_list)); ;}
    break;

  case 38:

/* Line 1464 of yacc.c  */
#line 397 "parser.y"
    { (yyval.str) = (yyvsp[(3) - (4)].str); ;}
    break;

  case 39:

/* Line 1464 of yacc.c  */
#line 399 "parser.y"
    { assert(yychar == YYEMPTY);
						  (yyval.import) = xmalloc(sizeof(struct _import_t));
						  (yyval.import)->name = (yyvsp[(2) - (3)].str);
						  (yyval.import)->import_performed = do_import((yyvsp[(2) - (3)].str));
						  if (!(yyval.import)->import_performed) yychar = aEOF;
						;}
    break;

  case 40:

/* Line 1464 of yacc.c  */
#line 407 "parser.y"
    { (yyval.str) = (yyvsp[(1) - (3)].import)->name;
						  if ((yyvsp[(1) - (3)].import)->import_performed) pop_import();
						  free((yyvsp[(1) - (3)].import));
						;}
    break;

  case 41:

/* Line 1464 of yacc.c  */
#line 414 "parser.y"
    { (yyval.str) = (yyvsp[(3) - (5)].str); if(!parse_only) add_importlib((yyvsp[(3) - (5)].str)); ;}
    break;

  case 42:

/* Line 1464 of yacc.c  */
#line 417 "parser.y"
    { (yyval.str) = (yyvsp[(2) - (2)].str); ;}
    break;

  case 43:

/* Line 1464 of yacc.c  */
#line 419 "parser.y"
    { (yyval.typelib) = make_library((yyvsp[(2) - (3)].str), check_library_attrs((yyvsp[(2) - (3)].str), (yyvsp[(1) - (3)].attr_list)));
						  if (!parse_only) start_typelib((yyval.typelib));
						;}
    break;

  case 44:

/* Line 1464 of yacc.c  */
#line 424 "parser.y"
    { (yyval.typelib) = (yyvsp[(1) - (4)].typelib);
						  (yyval.typelib)->stmts = (yyvsp[(2) - (4)].stmt_list);
						  if (!parse_only) end_typelib();
						;}
    break;

  case 45:

/* Line 1464 of yacc.c  */
#line 430 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 47:

/* Line 1464 of yacc.c  */
#line 434 "parser.y"
    { check_arg_attrs((yyvsp[(1) - (1)].var)); (yyval.var_list) = append_var( NULL, (yyvsp[(1) - (1)].var) ); ;}
    break;

  case 48:

/* Line 1464 of yacc.c  */
#line 435 "parser.y"
    { check_arg_attrs((yyvsp[(3) - (3)].var)); (yyval.var_list) = append_var( (yyvsp[(1) - (3)].var_list), (yyvsp[(3) - (3)].var) ); ;}
    break;

  case 50:

/* Line 1464 of yacc.c  */
#line 439 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[(1) - (3)].var_list), make_var(strdup("...")) ); ;}
    break;

  case 51:

/* Line 1464 of yacc.c  */
#line 443 "parser.y"
    { if ((yyvsp[(2) - (3)].declspec)->stgclass != STG_NONE && (yyvsp[(2) - (3)].declspec)->stgclass != STG_REGISTER)
						    error_loc("invalid storage class for function parameter\n");
						  (yyval.var) = declare_var((yyvsp[(1) - (3)].attr_list), (yyvsp[(2) - (3)].declspec), (yyvsp[(3) - (3)].declarator), TRUE);
						  free((yyvsp[(2) - (3)].declspec)); free((yyvsp[(3) - (3)].declarator));
						;}
    break;

  case 52:

/* Line 1464 of yacc.c  */
#line 448 "parser.y"
    { if ((yyvsp[(1) - (2)].declspec)->stgclass != STG_NONE && (yyvsp[(1) - (2)].declspec)->stgclass != STG_REGISTER)
						    error_loc("invalid storage class for function parameter\n");
						  (yyval.var) = declare_var(NULL, (yyvsp[(1) - (2)].declspec), (yyvsp[(2) - (2)].declarator), TRUE);
						  free((yyvsp[(1) - (2)].declspec)); free((yyvsp[(2) - (2)].declarator));
						;}
    break;

  case 53:

/* Line 1464 of yacc.c  */
#line 455 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (3)].expr);
						  if (!(yyval.expr)->is_const)
						      error_loc("array dimension is not an integer constant\n");
						;}
    break;

  case 54:

/* Line 1464 of yacc.c  */
#line 459 "parser.y"
    { (yyval.expr) = make_expr(EXPR_VOID); ;}
    break;

  case 55:

/* Line 1464 of yacc.c  */
#line 460 "parser.y"
    { (yyval.expr) = make_expr(EXPR_VOID); ;}
    break;

  case 56:

/* Line 1464 of yacc.c  */
#line 463 "parser.y"
    { (yyval.attr_list) = NULL; ;}
    break;

  case 58:

/* Line 1464 of yacc.c  */
#line 468 "parser.y"
    { (yyval.attr_list) = (yyvsp[(2) - (3)].attr_list); ;}
    break;

  case 59:

/* Line 1464 of yacc.c  */
#line 471 "parser.y"
    { (yyval.attr_list) = append_attr( NULL, (yyvsp[(1) - (1)].attr) ); ;}
    break;

  case 60:

/* Line 1464 of yacc.c  */
#line 472 "parser.y"
    { (yyval.attr_list) = append_attr( (yyvsp[(1) - (3)].attr_list), (yyvsp[(3) - (3)].attr) ); ;}
    break;

  case 61:

/* Line 1464 of yacc.c  */
#line 473 "parser.y"
    { (yyval.attr_list) = append_attr( (yyvsp[(1) - (4)].attr_list), (yyvsp[(4) - (4)].attr) ); ;}
    break;

  case 62:

/* Line 1464 of yacc.c  */
#line 476 "parser.y"
    { (yyval.str_list) = append_str( NULL, (yyvsp[(1) - (1)].str) ); ;}
    break;

  case 63:

/* Line 1464 of yacc.c  */
#line 477 "parser.y"
    { (yyval.str_list) = append_str( (yyvsp[(1) - (3)].str_list), (yyvsp[(3) - (3)].str) ); ;}
    break;

  case 64:

/* Line 1464 of yacc.c  */
#line 480 "parser.y"
    { (yyval.attr) = NULL; ;}
    break;

  case 65:

/* Line 1464 of yacc.c  */
#line 481 "parser.y"
    { (yyval.attr) = make_attr(ATTR_AGGREGATABLE); ;}
    break;

  case 66:

/* Line 1464 of yacc.c  */
#line 482 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ANNOTATION, (yyvsp[(3) - (4)].str)); ;}
    break;

  case 67:

/* Line 1464 of yacc.c  */
#line 483 "parser.y"
    { (yyval.attr) = make_attr(ATTR_APPOBJECT); ;}
    break;

  case 68:

/* Line 1464 of yacc.c  */
#line 484 "parser.y"
    { (yyval.attr) = make_attr(ATTR_ASYNC); ;}
    break;

  case 69:

/* Line 1464 of yacc.c  */
#line 485 "parser.y"
    { (yyval.attr) = make_attr(ATTR_AUTO_HANDLE); ;}
    break;

  case 70:

/* Line 1464 of yacc.c  */
#line 486 "parser.y"
    { (yyval.attr) = make_attr(ATTR_BINDABLE); ;}
    break;

  case 71:

/* Line 1464 of yacc.c  */
#line 487 "parser.y"
    { (yyval.attr) = make_attr(ATTR_BROADCAST); ;}
    break;

  case 72:

/* Line 1464 of yacc.c  */
#line 488 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_CALLAS, (yyvsp[(3) - (4)].var)); ;}
    break;

  case 73:

/* Line 1464 of yacc.c  */
#line 489 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_CASE, (yyvsp[(3) - (4)].expr_list)); ;}
    break;

  case 74:

/* Line 1464 of yacc.c  */
#line 490 "parser.y"
    { (yyval.attr) = make_attr(ATTR_CODE); ;}
    break;

  case 75:

/* Line 1464 of yacc.c  */
#line 491 "parser.y"
    { (yyval.attr) = make_attr(ATTR_COMMSTATUS); ;}
    break;

  case 76:

/* Line 1464 of yacc.c  */
#line 492 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); ;}
    break;

  case 77:

/* Line 1464 of yacc.c  */
#line 493 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_DONT_SERIALIZE */ ;}
    break;

  case 78:

/* Line 1464 of yacc.c  */
#line 494 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_SERIALIZE */ ;}
    break;

  case 79:

/* Line 1464 of yacc.c  */
#line 495 "parser.y"
    { (yyval.attr) = make_attr(ATTR_CONTROL); ;}
    break;

  case 80:

/* Line 1464 of yacc.c  */
#line 496 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DECODE); ;}
    break;

  case 81:

/* Line 1464 of yacc.c  */
#line 497 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULT); ;}
    break;

  case 82:

/* Line 1464 of yacc.c  */
#line 498 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULTBIND); ;}
    break;

  case 83:

/* Line 1464 of yacc.c  */
#line 499 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULTCOLLELEM); ;}
    break;

  case 84:

/* Line 1464 of yacc.c  */
#line 500 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_DEFAULTVALUE, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 85:

/* Line 1464 of yacc.c  */
#line 501 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULTVTABLE); ;}
    break;

  case 86:

/* Line 1464 of yacc.c  */
#line 502 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DISABLECONSISTENCYCHECK); ;}
    break;

  case 87:

/* Line 1464 of yacc.c  */
#line 503 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DISPLAYBIND); ;}
    break;

  case 88:

/* Line 1464 of yacc.c  */
#line 504 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_DLLNAME, (yyvsp[(3) - (4)].str)); ;}
    break;

  case 89:

/* Line 1464 of yacc.c  */
#line 505 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DUAL); ;}
    break;

  case 90:

/* Line 1464 of yacc.c  */
#line 506 "parser.y"
    { (yyval.attr) = make_attr(ATTR_ENABLEALLOCATE); ;}
    break;

  case 91:

/* Line 1464 of yacc.c  */
#line 507 "parser.y"
    { (yyval.attr) = make_attr(ATTR_ENCODE); ;}
    break;

  case 92:

/* Line 1464 of yacc.c  */
#line 508 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ENDPOINT, (yyvsp[(3) - (4)].str_list)); ;}
    break;

  case 93:

/* Line 1464 of yacc.c  */
#line 509 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ENTRY, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 94:

/* Line 1464 of yacc.c  */
#line 510 "parser.y"
    { (yyval.attr) = make_attr(ATTR_EXPLICIT_HANDLE); ;}
    break;

  case 95:

/* Line 1464 of yacc.c  */
#line 511 "parser.y"
    { (yyval.attr) = make_attr(ATTR_FAULTSTATUS); ;}
    break;

  case 96:

/* Line 1464 of yacc.c  */
#line 512 "parser.y"
    { (yyval.attr) = make_attr(ATTR_FORCEALLOCATE); ;}
    break;

  case 97:

/* Line 1464 of yacc.c  */
#line 513 "parser.y"
    { (yyval.attr) = make_attr(ATTR_HANDLE); ;}
    break;

  case 98:

/* Line 1464 of yacc.c  */
#line 514 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPCONTEXT, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 99:

/* Line 1464 of yacc.c  */
#line 515 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPFILE, (yyvsp[(3) - (4)].str)); ;}
    break;

  case 100:

/* Line 1464 of yacc.c  */
#line 516 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRING, (yyvsp[(3) - (4)].str)); ;}
    break;

  case 101:

/* Line 1464 of yacc.c  */
#line 517 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRINGCONTEXT, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 102:

/* Line 1464 of yacc.c  */
#line 518 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRINGDLL, (yyvsp[(3) - (4)].str)); ;}
    break;

  case 103:

/* Line 1464 of yacc.c  */
#line 519 "parser.y"
    { (yyval.attr) = make_attr(ATTR_HIDDEN); ;}
    break;

  case 104:

/* Line 1464 of yacc.c  */
#line 520 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ID, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 105:

/* Line 1464 of yacc.c  */
#line 521 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IDEMPOTENT); ;}
    break;

  case 106:

/* Line 1464 of yacc.c  */
#line 522 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IGNORE); ;}
    break;

  case 107:

/* Line 1464 of yacc.c  */
#line 523 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_IIDIS, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 108:

/* Line 1464 of yacc.c  */
#line 524 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IMMEDIATEBIND); ;}
    break;

  case 109:

/* Line 1464 of yacc.c  */
#line 525 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_IMPLICIT_HANDLE, (yyvsp[(3) - (4)].var)); ;}
    break;

  case 110:

/* Line 1464 of yacc.c  */
#line 526 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IN); ;}
    break;

  case 111:

/* Line 1464 of yacc.c  */
#line 527 "parser.y"
    { (yyval.attr) = make_attr(ATTR_INPUTSYNC); ;}
    break;

  case 112:

/* Line 1464 of yacc.c  */
#line 528 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_LENGTHIS, (yyvsp[(3) - (4)].expr_list)); ;}
    break;

  case 113:

/* Line 1464 of yacc.c  */
#line 529 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_LIBLCID, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 114:

/* Line 1464 of yacc.c  */
#line 530 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PARAMLCID); ;}
    break;

  case 115:

/* Line 1464 of yacc.c  */
#line 531 "parser.y"
    { (yyval.attr) = make_attr(ATTR_LICENSED); ;}
    break;

  case 116:

/* Line 1464 of yacc.c  */
#line 532 "parser.y"
    { (yyval.attr) = make_attr(ATTR_LOCAL); ;}
    break;

  case 117:

/* Line 1464 of yacc.c  */
#line 533 "parser.y"
    { (yyval.attr) = make_attr(ATTR_MAYBE); ;}
    break;

  case 118:

/* Line 1464 of yacc.c  */
#line 534 "parser.y"
    { (yyval.attr) = make_attr(ATTR_MESSAGE); ;}
    break;

  case 119:

/* Line 1464 of yacc.c  */
#line 535 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NOCODE); ;}
    break;

  case 120:

/* Line 1464 of yacc.c  */
#line 536 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONBROWSABLE); ;}
    break;

  case 121:

/* Line 1464 of yacc.c  */
#line 537 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONCREATABLE); ;}
    break;

  case 122:

/* Line 1464 of yacc.c  */
#line 538 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONEXTENSIBLE); ;}
    break;

  case 123:

/* Line 1464 of yacc.c  */
#line 539 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NOTIFY); ;}
    break;

  case 124:

/* Line 1464 of yacc.c  */
#line 540 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NOTIFYFLAG); ;}
    break;

  case 125:

/* Line 1464 of yacc.c  */
#line 541 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OBJECT); ;}
    break;

  case 126:

/* Line 1464 of yacc.c  */
#line 542 "parser.y"
    { (yyval.attr) = make_attr(ATTR_ODL); ;}
    break;

  case 127:

/* Line 1464 of yacc.c  */
#line 543 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OLEAUTOMATION); ;}
    break;

  case 128:

/* Line 1464 of yacc.c  */
#line 544 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_OPTIMIZE, (yyvsp[(3) - (4)].str)); ;}
    break;

  case 129:

/* Line 1464 of yacc.c  */
#line 545 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OPTIONAL); ;}
    break;

  case 130:

/* Line 1464 of yacc.c  */
#line 546 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OUT); ;}
    break;

  case 131:

/* Line 1464 of yacc.c  */
#line 547 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PARTIALIGNORE); ;}
    break;

  case 132:

/* Line 1464 of yacc.c  */
#line 548 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_POINTERDEFAULT, (yyvsp[(3) - (4)].num)); ;}
    break;

  case 133:

/* Line 1464 of yacc.c  */
#line 549 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_PROGID, (yyvsp[(3) - (4)].str)); ;}
    break;

  case 134:

/* Line 1464 of yacc.c  */
#line 550 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPGET); ;}
    break;

  case 135:

/* Line 1464 of yacc.c  */
#line 551 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPPUT); ;}
    break;

  case 136:

/* Line 1464 of yacc.c  */
#line 552 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPPUTREF); ;}
    break;

  case 137:

/* Line 1464 of yacc.c  */
#line 553 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROXY); ;}
    break;

  case 138:

/* Line 1464 of yacc.c  */
#line 554 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PUBLIC); ;}
    break;

  case 139:

/* Line 1464 of yacc.c  */
#line 556 "parser.y"
    { expr_list_t *list = append_expr( NULL, (yyvsp[(3) - (6)].expr) );
						  list = append_expr( list, (yyvsp[(5) - (6)].expr) );
						  (yyval.attr) = make_attrp(ATTR_RANGE, list); ;}
    break;

  case 140:

/* Line 1464 of yacc.c  */
#line 559 "parser.y"
    { (yyval.attr) = make_attr(ATTR_READONLY); ;}
    break;

  case 141:

/* Line 1464 of yacc.c  */
#line 560 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_REPRESENTAS, (yyvsp[(3) - (4)].type)); ;}
    break;

  case 142:

/* Line 1464 of yacc.c  */
#line 561 "parser.y"
    { (yyval.attr) = make_attr(ATTR_REQUESTEDIT); ;}
    break;

  case 143:

/* Line 1464 of yacc.c  */
#line 562 "parser.y"
    { (yyval.attr) = make_attr(ATTR_RESTRICTED); ;}
    break;

  case 144:

/* Line 1464 of yacc.c  */
#line 563 "parser.y"
    { (yyval.attr) = make_attr(ATTR_RETVAL); ;}
    break;

  case 145:

/* Line 1464 of yacc.c  */
#line 564 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SIZEIS, (yyvsp[(3) - (4)].expr_list)); ;}
    break;

  case 146:

/* Line 1464 of yacc.c  */
#line 565 "parser.y"
    { (yyval.attr) = make_attr(ATTR_SOURCE); ;}
    break;

  case 147:

/* Line 1464 of yacc.c  */
#line 566 "parser.y"
    { (yyval.attr) = make_attr(ATTR_STRICTCONTEXTHANDLE); ;}
    break;

  case 148:

/* Line 1464 of yacc.c  */
#line 567 "parser.y"
    { (yyval.attr) = make_attr(ATTR_STRING); ;}
    break;

  case 149:

/* Line 1464 of yacc.c  */
#line 568 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SWITCHIS, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 150:

/* Line 1464 of yacc.c  */
#line 569 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SWITCHTYPE, (yyvsp[(3) - (4)].type)); ;}
    break;

  case 151:

/* Line 1464 of yacc.c  */
#line 570 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_TRANSMITAS, (yyvsp[(3) - (4)].type)); ;}
    break;

  case 152:

/* Line 1464 of yacc.c  */
#line 571 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_THREADING, (yyvsp[(3) - (4)].num)); ;}
    break;

  case 153:

/* Line 1464 of yacc.c  */
#line 572 "parser.y"
    { (yyval.attr) = make_attr(ATTR_UIDEFAULT); ;}
    break;

  case 154:

/* Line 1464 of yacc.c  */
#line 573 "parser.y"
    { (yyval.attr) = make_attr(ATTR_USESGETLASTERROR); ;}
    break;

  case 155:

/* Line 1464 of yacc.c  */
#line 574 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_USERMARSHAL, (yyvsp[(3) - (4)].type)); ;}
    break;

  case 156:

/* Line 1464 of yacc.c  */
#line 575 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_UUID, (yyvsp[(3) - (4)].uuid)); ;}
    break;

  case 157:

/* Line 1464 of yacc.c  */
#line 576 "parser.y"
    { (yyval.attr) = make_attr(ATTR_V1ENUM); ;}
    break;

  case 158:

/* Line 1464 of yacc.c  */
#line 577 "parser.y"
    { (yyval.attr) = make_attr(ATTR_VARARG); ;}
    break;

  case 159:

/* Line 1464 of yacc.c  */
#line 578 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_VERSION, (yyvsp[(3) - (4)].num)); ;}
    break;

  case 160:

/* Line 1464 of yacc.c  */
#line 579 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_VIPROGID, (yyvsp[(3) - (4)].str)); ;}
    break;

  case 161:

/* Line 1464 of yacc.c  */
#line 580 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_WIREMARSHAL, (yyvsp[(3) - (4)].type)); ;}
    break;

  case 162:

/* Line 1464 of yacc.c  */
#line 581 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_POINTERTYPE, (yyvsp[(1) - (1)].num)); ;}
    break;

  case 164:

/* Line 1464 of yacc.c  */
#line 586 "parser.y"
    { if (!is_valid_uuid((yyvsp[(1) - (1)].str)))
						    error_loc("invalid UUID: %s\n", (yyvsp[(1) - (1)].str));
						  (yyval.uuid) = parse_uuid((yyvsp[(1) - (1)].str)); ;}
    break;

  case 165:

/* Line 1464 of yacc.c  */
#line 591 "parser.y"
    { (yyval.str) = xstrdup("__cdecl"); ;}
    break;

  case 166:

/* Line 1464 of yacc.c  */
#line 592 "parser.y"
    { (yyval.str) = xstrdup("__fastcall"); ;}
    break;

  case 167:

/* Line 1464 of yacc.c  */
#line 593 "parser.y"
    { (yyval.str) = xstrdup("__pascal"); ;}
    break;

  case 168:

/* Line 1464 of yacc.c  */
#line 594 "parser.y"
    { (yyval.str) = xstrdup("__stdcall"); ;}
    break;

  case 169:

/* Line 1464 of yacc.c  */
#line 597 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 170:

/* Line 1464 of yacc.c  */
#line 598 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[(1) - (2)].var_list), (yyvsp[(2) - (2)].var) ); ;}
    break;

  case 171:

/* Line 1464 of yacc.c  */
#line 601 "parser.y"
    { attr_t *a = make_attrp(ATTR_CASE, append_expr( NULL, (yyvsp[(2) - (4)].expr) ));
						  (yyval.var) = (yyvsp[(4) - (4)].var); if (!(yyval.var)) (yyval.var) = make_var(NULL);
						  (yyval.var)->attrs = append_attr( (yyval.var)->attrs, a );
						;}
    break;

  case 172:

/* Line 1464 of yacc.c  */
#line 605 "parser.y"
    { attr_t *a = make_attr(ATTR_DEFAULT);
						  (yyval.var) = (yyvsp[(3) - (3)].var); if (!(yyval.var)) (yyval.var) = make_var(NULL);
						  (yyval.var)->attrs = append_attr( (yyval.var)->attrs, a );
						;}
    break;

  case 173:

/* Line 1464 of yacc.c  */
#line 611 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 174:

/* Line 1464 of yacc.c  */
#line 612 "parser.y"
    { (yyval.var_list) = (yyvsp[(1) - (2)].var_list); ;}
    break;

  case 176:

/* Line 1464 of yacc.c  */
#line 616 "parser.y"
    { if (!(yyvsp[(1) - (1)].var)->eval)
						    (yyvsp[(1) - (1)].var)->eval = make_exprl(EXPR_NUM, 0 /* default for first enum entry */);
                                                  (yyval.var_list) = append_var( NULL, (yyvsp[(1) - (1)].var) );
						;}
    break;

  case 177:

/* Line 1464 of yacc.c  */
#line 620 "parser.y"
    { if (!(yyvsp[(3) - (3)].var)->eval)
                                                  {
                                                    var_t *last = LIST_ENTRY( list_tail((yyval.var_list)), var_t, entry );
                                                    (yyvsp[(3) - (3)].var)->eval = make_exprl(EXPR_NUM, last->eval->cval + 1);
                                                  }
                                                  (yyval.var_list) = append_var( (yyvsp[(1) - (3)].var_list), (yyvsp[(3) - (3)].var) );
						;}
    break;

  case 178:

/* Line 1464 of yacc.c  */
#line 629 "parser.y"
    { (yyval.var) = reg_const((yyvsp[(1) - (3)].var));
						  (yyval.var)->eval = (yyvsp[(3) - (3)].expr);
                                                  (yyval.var)->type = type_new_int(TYPE_BASIC_INT, 0);
						;}
    break;

  case 179:

/* Line 1464 of yacc.c  */
#line 633 "parser.y"
    { (yyval.var) = reg_const((yyvsp[(1) - (1)].var));
                                                  (yyval.var)->type = type_new_int(TYPE_BASIC_INT, 0);
						;}
    break;

  case 180:

/* Line 1464 of yacc.c  */
#line 638 "parser.y"
    { (yyval.type) = type_new_enum((yyvsp[(2) - (5)].str), TRUE, (yyvsp[(4) - (5)].var_list)); ;}
    break;

  case 181:

/* Line 1464 of yacc.c  */
#line 641 "parser.y"
    { (yyval.expr_list) = append_expr( NULL, (yyvsp[(1) - (1)].expr) ); ;}
    break;

  case 182:

/* Line 1464 of yacc.c  */
#line 642 "parser.y"
    { (yyval.expr_list) = append_expr( (yyvsp[(1) - (3)].expr_list), (yyvsp[(3) - (3)].expr) ); ;}
    break;

  case 183:

/* Line 1464 of yacc.c  */
#line 645 "parser.y"
    { (yyval.expr) = make_expr(EXPR_VOID); ;}
    break;

  case 185:

/* Line 1464 of yacc.c  */
#line 649 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_NUM, (yyvsp[(1) - (1)].num)); ;}
    break;

  case 186:

/* Line 1464 of yacc.c  */
#line 650 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_HEXNUM, (yyvsp[(1) - (1)].num)); ;}
    break;

  case 187:

/* Line 1464 of yacc.c  */
#line 651 "parser.y"
    { (yyval.expr) = make_exprd(EXPR_DOUBLE, (yyvsp[(1) - (1)].dbl)); ;}
    break;

  case 188:

/* Line 1464 of yacc.c  */
#line 652 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_TRUEFALSE, 0); ;}
    break;

  case 189:

/* Line 1464 of yacc.c  */
#line 653 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_NUM, 0); ;}
    break;

  case 190:

/* Line 1464 of yacc.c  */
#line 654 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_TRUEFALSE, 1); ;}
    break;

  case 191:

/* Line 1464 of yacc.c  */
#line 655 "parser.y"
    { (yyval.expr) = make_exprs(EXPR_STRLIT, (yyvsp[(1) - (1)].str)); ;}
    break;

  case 192:

/* Line 1464 of yacc.c  */
#line 656 "parser.y"
    { (yyval.expr) = make_exprs(EXPR_WSTRLIT, (yyvsp[(1) - (1)].str)); ;}
    break;

  case 193:

/* Line 1464 of yacc.c  */
#line 657 "parser.y"
    { (yyval.expr) = make_exprs(EXPR_CHARCONST, (yyvsp[(1) - (1)].str)); ;}
    break;

  case 194:

/* Line 1464 of yacc.c  */
#line 658 "parser.y"
    { (yyval.expr) = make_exprs(EXPR_IDENTIFIER, (yyvsp[(1) - (1)].str)); ;}
    break;

  case 195:

/* Line 1464 of yacc.c  */
#line 659 "parser.y"
    { (yyval.expr) = make_expr3(EXPR_COND, (yyvsp[(1) - (5)].expr), (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].expr)); ;}
    break;

  case 196:

/* Line 1464 of yacc.c  */
#line 660 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_LOGOR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 197:

/* Line 1464 of yacc.c  */
#line 661 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_LOGAND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 198:

/* Line 1464 of yacc.c  */
#line 662 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_OR , (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 199:

/* Line 1464 of yacc.c  */
#line 663 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_XOR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 200:

/* Line 1464 of yacc.c  */
#line 664 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_AND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 201:

/* Line 1464 of yacc.c  */
#line 665 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_EQUALITY, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 202:

/* Line 1464 of yacc.c  */
#line 666 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_INEQUALITY, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 203:

/* Line 1464 of yacc.c  */
#line 667 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_GTR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 204:

/* Line 1464 of yacc.c  */
#line 668 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_LESS, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 205:

/* Line 1464 of yacc.c  */
#line 669 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_GTREQL, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 206:

/* Line 1464 of yacc.c  */
#line 670 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_LESSEQL, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 207:

/* Line 1464 of yacc.c  */
#line 671 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SHL, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 208:

/* Line 1464 of yacc.c  */
#line 672 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SHR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 209:

/* Line 1464 of yacc.c  */
#line 673 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_ADD, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 210:

/* Line 1464 of yacc.c  */
#line 674 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SUB, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 211:

/* Line 1464 of yacc.c  */
#line 675 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MOD, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 212:

/* Line 1464 of yacc.c  */
#line 676 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MUL, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 213:

/* Line 1464 of yacc.c  */
#line 677 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_DIV, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 214:

/* Line 1464 of yacc.c  */
#line 678 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_LOGNOT, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 215:

/* Line 1464 of yacc.c  */
#line 679 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_NOT, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 216:

/* Line 1464 of yacc.c  */
#line 680 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_POS, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 217:

/* Line 1464 of yacc.c  */
#line 681 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_NEG, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 218:

/* Line 1464 of yacc.c  */
#line 682 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_ADDRESSOF, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 219:

/* Line 1464 of yacc.c  */
#line 683 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_PPTR, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 220:

/* Line 1464 of yacc.c  */
#line 684 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MEMBER, make_expr1(EXPR_PPTR, (yyvsp[(1) - (3)].expr)), make_exprs(EXPR_IDENTIFIER, (yyvsp[(3) - (3)].str))); ;}
    break;

  case 221:

/* Line 1464 of yacc.c  */
#line 685 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MEMBER, (yyvsp[(1) - (3)].expr), make_exprs(EXPR_IDENTIFIER, (yyvsp[(3) - (3)].str))); ;}
    break;

  case 222:

/* Line 1464 of yacc.c  */
#line 687 "parser.y"
    { (yyval.expr) = make_exprt(EXPR_CAST, declare_var(NULL, (yyvsp[(2) - (5)].declspec), (yyvsp[(3) - (5)].declarator), 0), (yyvsp[(5) - (5)].expr)); free((yyvsp[(2) - (5)].declspec)); free((yyvsp[(3) - (5)].declarator)); ;}
    break;

  case 223:

/* Line 1464 of yacc.c  */
#line 689 "parser.y"
    { (yyval.expr) = make_exprt(EXPR_SIZEOF, declare_var(NULL, (yyvsp[(3) - (5)].declspec), (yyvsp[(4) - (5)].declarator), 0), NULL); free((yyvsp[(3) - (5)].declspec)); free((yyvsp[(4) - (5)].declarator)); ;}
    break;

  case 224:

/* Line 1464 of yacc.c  */
#line 690 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_ARRAY, (yyvsp[(1) - (4)].expr), (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 225:

/* Line 1464 of yacc.c  */
#line 691 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (3)].expr); ;}
    break;

  case 226:

/* Line 1464 of yacc.c  */
#line 694 "parser.y"
    { (yyval.expr_list) = append_expr( NULL, (yyvsp[(1) - (1)].expr) ); ;}
    break;

  case 227:

/* Line 1464 of yacc.c  */
#line 695 "parser.y"
    { (yyval.expr_list) = append_expr( (yyvsp[(1) - (3)].expr_list), (yyvsp[(3) - (3)].expr) ); ;}
    break;

  case 228:

/* Line 1464 of yacc.c  */
#line 698 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr);
						  if (!(yyval.expr)->is_const)
						      error_loc("expression is not an integer constant\n");
						;}
    break;

  case 229:

/* Line 1464 of yacc.c  */
#line 704 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr);
						  if (!(yyval.expr)->is_const && (yyval.expr)->type != EXPR_STRLIT && (yyval.expr)->type != EXPR_WSTRLIT)
						      error_loc("expression is not constant\n");
						;}
    break;

  case 230:

/* Line 1464 of yacc.c  */
#line 710 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 231:

/* Line 1464 of yacc.c  */
#line 711 "parser.y"
    { (yyval.var_list) = append_var_list((yyvsp[(1) - (2)].var_list), (yyvsp[(2) - (2)].var_list)); ;}
    break;

  case 232:

/* Line 1464 of yacc.c  */
#line 715 "parser.y"
    { const char *first = LIST_ENTRY(list_head((yyvsp[(3) - (4)].declarator_list)), declarator_t, entry)->var->name;
						  check_field_attrs(first, (yyvsp[(1) - (4)].attr_list));
						  (yyval.var_list) = set_var_types((yyvsp[(1) - (4)].attr_list), (yyvsp[(2) - (4)].declspec), (yyvsp[(3) - (4)].declarator_list));
						;}
    break;

  case 233:

/* Line 1464 of yacc.c  */
#line 719 "parser.y"
    { var_t *v = make_var(NULL);
						  v->type = (yyvsp[(2) - (3)].type); v->attrs = (yyvsp[(1) - (3)].attr_list);
						  (yyval.var_list) = append_var(NULL, v);
						;}
    break;

  case 234:

/* Line 1464 of yacc.c  */
#line 726 "parser.y"
    { (yyval.var) = (yyvsp[(1) - (2)].var); ;}
    break;

  case 235:

/* Line 1464 of yacc.c  */
#line 727 "parser.y"
    { (yyval.var) = make_var(NULL); (yyval.var)->attrs = (yyvsp[(1) - (2)].attr_list); ;}
    break;

  case 236:

/* Line 1464 of yacc.c  */
#line 730 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 237:

/* Line 1464 of yacc.c  */
#line 731 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[(1) - (2)].var_list), (yyvsp[(2) - (2)].var) ); ;}
    break;

  case 238:

/* Line 1464 of yacc.c  */
#line 735 "parser.y"
    { (yyval.var) = (yyvsp[(1) - (2)].var); ;}
    break;

  case 239:

/* Line 1464 of yacc.c  */
#line 736 "parser.y"
    { (yyval.var) = NULL; ;}
    break;

  case 240:

/* Line 1464 of yacc.c  */
#line 739 "parser.y"
    { (yyval.var) = declare_var(check_field_attrs((yyvsp[(3) - (3)].declarator)->var->name, (yyvsp[(1) - (3)].attr_list)),
						                (yyvsp[(2) - (3)].declspec), (yyvsp[(3) - (3)].declarator), FALSE);
						  free((yyvsp[(3) - (3)].declarator));
						;}
    break;

  case 241:

/* Line 1464 of yacc.c  */
#line 745 "parser.y"
    { (yyval.var) = (yyvsp[(1) - (1)].var);
						  if (type_get_type((yyval.var)->type) != TYPE_FUNCTION)
						    error_loc("only methods may be declared inside the methods section of a dispinterface\n");
						  check_function_attrs((yyval.var)->name, (yyval.var)->attrs);
						;}
    break;

  case 242:

/* Line 1464 of yacc.c  */
#line 754 "parser.y"
    { (yyval.var) = declare_var((yyvsp[(1) - (3)].attr_list), (yyvsp[(2) - (3)].declspec), (yyvsp[(3) - (3)].declarator), FALSE);
						  free((yyvsp[(3) - (3)].declarator));
						;}
    break;

  case 243:

/* Line 1464 of yacc.c  */
#line 757 "parser.y"
    { (yyval.var) = declare_var(NULL, (yyvsp[(1) - (2)].declspec), (yyvsp[(2) - (2)].declarator), FALSE);
						  free((yyvsp[(2) - (2)].declarator));
						;}
    break;

  case 244:

/* Line 1464 of yacc.c  */
#line 762 "parser.y"
    { (yyval.var) = NULL; ;}
    break;

  case 246:

/* Line 1464 of yacc.c  */
#line 766 "parser.y"
    { (yyval.str) = NULL; ;}
    break;

  case 247:

/* Line 1464 of yacc.c  */
#line 767 "parser.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); ;}
    break;

  case 248:

/* Line 1464 of yacc.c  */
#line 768 "parser.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); ;}
    break;

  case 249:

/* Line 1464 of yacc.c  */
#line 771 "parser.y"
    { (yyval.var) = make_var((yyvsp[(1) - (1)].str)); ;}
    break;

  case 250:

/* Line 1464 of yacc.c  */
#line 773 "parser.y"
    { (yyval.var) = make_var((yyvsp[(1) - (1)].str)); ;}
    break;

  case 251:

/* Line 1464 of yacc.c  */
#line 776 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 252:

/* Line 1464 of yacc.c  */
#line 777 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 254:

/* Line 1464 of yacc.c  */
#line 779 "parser.y"
    { (yyval.type) = type_new_int(type_basic_get_type((yyvsp[(2) - (2)].type)), -1); ;}
    break;

  case 255:

/* Line 1464 of yacc.c  */
#line 780 "parser.y"
    { (yyval.type) = type_new_int(type_basic_get_type((yyvsp[(2) - (2)].type)), 1); ;}
    break;

  case 256:

/* Line 1464 of yacc.c  */
#line 781 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT, 1); ;}
    break;

  case 257:

/* Line 1464 of yacc.c  */
#line 782 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 258:

/* Line 1464 of yacc.c  */
#line 783 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 259:

/* Line 1464 of yacc.c  */
#line 784 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 260:

/* Line 1464 of yacc.c  */
#line 785 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 261:

/* Line 1464 of yacc.c  */
#line 786 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 264:

/* Line 1464 of yacc.c  */
#line 793 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT, 0); ;}
    break;

  case 265:

/* Line 1464 of yacc.c  */
#line 794 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT16, 0); ;}
    break;

  case 266:

/* Line 1464 of yacc.c  */
#line 795 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT8, 0); ;}
    break;

  case 267:

/* Line 1464 of yacc.c  */
#line 796 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT32, 0); ;}
    break;

  case 268:

/* Line 1464 of yacc.c  */
#line 797 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_HYPER, 0); ;}
    break;

  case 269:

/* Line 1464 of yacc.c  */
#line 798 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT64, 0); ;}
    break;

  case 270:

/* Line 1464 of yacc.c  */
#line 799 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_CHAR, 0); ;}
    break;

  case 271:

/* Line 1464 of yacc.c  */
#line 800 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT3264, 0); ;}
    break;

  case 272:

/* Line 1464 of yacc.c  */
#line 803 "parser.y"
    { (yyval.type) = type_new_coclass((yyvsp[(2) - (2)].str)); ;}
    break;

  case 273:

/* Line 1464 of yacc.c  */
#line 804 "parser.y"
    { (yyval.type) = find_type((yyvsp[(2) - (2)].str), 0);
						  if (type_get_type_detect_alias((yyval.type)) != TYPE_COCLASS)
						    error_loc("%s was not declared a coclass at %s:%d\n",
							      (yyvsp[(2) - (2)].str), (yyval.type)->loc_info.input_name,
							      (yyval.type)->loc_info.line_number);
						;}
    break;

  case 274:

/* Line 1464 of yacc.c  */
#line 812 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type);
						  check_def((yyval.type));
						  (yyval.type)->attrs = check_coclass_attrs((yyvsp[(2) - (2)].type)->name, (yyvsp[(1) - (2)].attr_list));
						;}
    break;

  case 275:

/* Line 1464 of yacc.c  */
#line 819 "parser.y"
    { (yyval.type) = type_coclass_define((yyvsp[(1) - (5)].type), (yyvsp[(3) - (5)].ifref_list)); ;}
    break;

  case 276:

/* Line 1464 of yacc.c  */
#line 822 "parser.y"
    { (yyval.ifref_list) = NULL; ;}
    break;

  case 277:

/* Line 1464 of yacc.c  */
#line 823 "parser.y"
    { (yyval.ifref_list) = append_ifref( (yyvsp[(1) - (2)].ifref_list), (yyvsp[(2) - (2)].ifref) ); ;}
    break;

  case 278:

/* Line 1464 of yacc.c  */
#line 827 "parser.y"
    { (yyval.ifref) = make_ifref((yyvsp[(2) - (2)].type)); (yyval.ifref)->attrs = (yyvsp[(1) - (2)].attr_list); ;}
    break;

  case 279:

/* Line 1464 of yacc.c  */
#line 830 "parser.y"
    { (yyval.type) = get_type(TYPE_INTERFACE, (yyvsp[(2) - (2)].str), 0); ;}
    break;

  case 280:

/* Line 1464 of yacc.c  */
#line 831 "parser.y"
    { (yyval.type) = get_type(TYPE_INTERFACE, (yyvsp[(2) - (2)].str), 0); ;}
    break;

  case 281:

/* Line 1464 of yacc.c  */
#line 834 "parser.y"
    { attr_t *attrs;
						  (yyval.type) = (yyvsp[(2) - (2)].type);
						  check_def((yyval.type));
						  attrs = make_attr(ATTR_DISPINTERFACE);
						  (yyval.type)->attrs = append_attr( check_dispiface_attrs((yyvsp[(2) - (2)].type)->name, (yyvsp[(1) - (2)].attr_list)), attrs );
						  (yyval.type)->defined = TRUE;
						;}
    break;

  case 282:

/* Line 1464 of yacc.c  */
#line 843 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 283:

/* Line 1464 of yacc.c  */
#line 844 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[(1) - (3)].var_list), (yyvsp[(2) - (3)].var) ); ;}
    break;

  case 284:

/* Line 1464 of yacc.c  */
#line 847 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 285:

/* Line 1464 of yacc.c  */
#line 848 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[(1) - (3)].var_list), (yyvsp[(2) - (3)].var) ); ;}
    break;

  case 286:

/* Line 1464 of yacc.c  */
#line 854 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (5)].type);
						  type_dispinterface_define((yyval.type), (yyvsp[(3) - (5)].var_list), (yyvsp[(4) - (5)].var_list));
						;}
    break;

  case 287:

/* Line 1464 of yacc.c  */
#line 858 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (5)].type);
						  type_dispinterface_define_from_iface((yyval.type), (yyvsp[(3) - (5)].type));
						;}
    break;

  case 288:

/* Line 1464 of yacc.c  */
#line 863 "parser.y"
    { (yyval.type) = NULL; ;}
    break;

  case 289:

/* Line 1464 of yacc.c  */
#line 864 "parser.y"
    { (yyval.type) = find_type_or_error2((yyvsp[(2) - (2)].str), 0); ;}
    break;

  case 290:

/* Line 1464 of yacc.c  */
#line 867 "parser.y"
    { (yyval.type) = get_type(TYPE_INTERFACE, (yyvsp[(2) - (2)].str), 0); ;}
    break;

  case 291:

/* Line 1464 of yacc.c  */
#line 868 "parser.y"
    { (yyval.type) = get_type(TYPE_INTERFACE, (yyvsp[(2) - (2)].str), 0); ;}
    break;

  case 292:

/* Line 1464 of yacc.c  */
#line 871 "parser.y"
    { (yyval.ifinfo).interface = (yyvsp[(2) - (2)].type);
						  (yyval.ifinfo).old_pointer_default = pointer_default;
						  if (is_attr((yyvsp[(1) - (2)].attr_list), ATTR_POINTERDEFAULT))
						    pointer_default = get_attrv((yyvsp[(1) - (2)].attr_list), ATTR_POINTERDEFAULT);
						  check_def((yyvsp[(2) - (2)].type));
						  (yyvsp[(2) - (2)].type)->attrs = check_iface_attrs((yyvsp[(2) - (2)].type)->name, (yyvsp[(1) - (2)].attr_list));
						  (yyvsp[(2) - (2)].type)->defined = TRUE;
						;}
    break;

  case 293:

/* Line 1464 of yacc.c  */
#line 882 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (6)].ifinfo).interface;
						  type_interface_define((yyval.type), (yyvsp[(2) - (6)].type), (yyvsp[(4) - (6)].stmt_list));
						  pointer_default = (yyvsp[(1) - (6)].ifinfo).old_pointer_default;
						;}
    break;

  case 294:

/* Line 1464 of yacc.c  */
#line 890 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (8)].ifinfo).interface;
						  type_interface_define((yyval.type), find_type_or_error2((yyvsp[(3) - (8)].str), 0), (yyvsp[(6) - (8)].stmt_list));
						  pointer_default = (yyvsp[(1) - (8)].ifinfo).old_pointer_default;
						;}
    break;

  case 295:

/* Line 1464 of yacc.c  */
#line 894 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (2)].type); ;}
    break;

  case 296:

/* Line 1464 of yacc.c  */
#line 898 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (2)].type); ;}
    break;

  case 297:

/* Line 1464 of yacc.c  */
#line 899 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (2)].type); ;}
    break;

  case 298:

/* Line 1464 of yacc.c  */
#line 902 "parser.y"
    { (yyval.type) = type_new_module((yyvsp[(2) - (2)].str)); ;}
    break;

  case 299:

/* Line 1464 of yacc.c  */
#line 903 "parser.y"
    { (yyval.type) = type_new_module((yyvsp[(2) - (2)].str)); ;}
    break;

  case 300:

/* Line 1464 of yacc.c  */
#line 906 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type);
						  (yyval.type)->attrs = check_module_attrs((yyvsp[(2) - (2)].type)->name, (yyvsp[(1) - (2)].attr_list));
						;}
    break;

  case 301:

/* Line 1464 of yacc.c  */
#line 912 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (5)].type);
                                                  type_module_define((yyval.type), (yyvsp[(3) - (5)].stmt_list));
						;}
    break;

  case 302:

/* Line 1464 of yacc.c  */
#line 918 "parser.y"
    { (yyval.stgclass) = STG_EXTERN; ;}
    break;

  case 303:

/* Line 1464 of yacc.c  */
#line 919 "parser.y"
    { (yyval.stgclass) = STG_STATIC; ;}
    break;

  case 304:

/* Line 1464 of yacc.c  */
#line 920 "parser.y"
    { (yyval.stgclass) = STG_REGISTER; ;}
    break;

  case 305:

/* Line 1464 of yacc.c  */
#line 924 "parser.y"
    { (yyval.attr) = make_attr(ATTR_INLINE); ;}
    break;

  case 306:

/* Line 1464 of yacc.c  */
#line 928 "parser.y"
    { (yyval.attr) = make_attr(ATTR_CONST); ;}
    break;

  case 307:

/* Line 1464 of yacc.c  */
#line 931 "parser.y"
    { (yyval.attr_list) = NULL; ;}
    break;

  case 308:

/* Line 1464 of yacc.c  */
#line 932 "parser.y"
    { (yyval.attr_list) = append_attr((yyvsp[(1) - (2)].attr_list), (yyvsp[(2) - (2)].attr)); ;}
    break;

  case 309:

/* Line 1464 of yacc.c  */
#line 935 "parser.y"
    { (yyval.declspec) = make_decl_spec((yyvsp[(1) - (2)].type), (yyvsp[(2) - (2)].declspec), NULL, NULL, STG_NONE); ;}
    break;

  case 310:

/* Line 1464 of yacc.c  */
#line 937 "parser.y"
    { (yyval.declspec) = make_decl_spec((yyvsp[(2) - (3)].type), (yyvsp[(1) - (3)].declspec), (yyvsp[(3) - (3)].declspec), NULL, STG_NONE); ;}
    break;

  case 311:

/* Line 1464 of yacc.c  */
#line 940 "parser.y"
    { (yyval.declspec) = NULL; ;}
    break;

  case 313:

/* Line 1464 of yacc.c  */
#line 945 "parser.y"
    { (yyval.declspec) = make_decl_spec(NULL, (yyvsp[(2) - (2)].declspec), NULL, (yyvsp[(1) - (2)].attr), STG_NONE); ;}
    break;

  case 314:

/* Line 1464 of yacc.c  */
#line 946 "parser.y"
    { (yyval.declspec) = make_decl_spec(NULL, (yyvsp[(2) - (2)].declspec), NULL, (yyvsp[(1) - (2)].attr), STG_NONE); ;}
    break;

  case 315:

/* Line 1464 of yacc.c  */
#line 947 "parser.y"
    { (yyval.declspec) = make_decl_spec(NULL, (yyvsp[(2) - (2)].declspec), NULL, NULL, (yyvsp[(1) - (2)].stgclass)); ;}
    break;

  case 316:

/* Line 1464 of yacc.c  */
#line 952 "parser.y"
    { (yyval.declarator) = (yyvsp[(3) - (3)].declarator); (yyval.declarator)->type = append_ptrchain_type((yyval.declarator)->type, type_new_pointer(pointer_default, NULL, (yyvsp[(2) - (3)].attr_list))); ;}
    break;

  case 317:

/* Line 1464 of yacc.c  */
#line 953 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (2)].declarator); if ((yyval.declarator)->func_type) (yyval.declarator)->func_type->attrs = append_attr((yyval.declarator)->func_type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[(1) - (2)].str)));
						           else if ((yyval.declarator)->type) (yyval.declarator)->type->attrs = append_attr((yyval.declarator)->type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[(1) - (2)].str))); ;}
    break;

  case 319:

/* Line 1464 of yacc.c  */
#line 959 "parser.y"
    { (yyval.declarator) = make_declarator((yyvsp[(1) - (1)].var)); ;}
    break;

  case 320:

/* Line 1464 of yacc.c  */
#line 960 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (3)].declarator); ;}
    break;

  case 321:

/* Line 1464 of yacc.c  */
#line 961 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (2)].declarator); (yyval.declarator)->array = append_array((yyval.declarator)->array, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 322:

/* Line 1464 of yacc.c  */
#line 962 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (4)].declarator);
						  (yyval.declarator)->func_type = append_ptrchain_type((yyval.declarator)->type, type_new_function((yyvsp[(3) - (4)].var_list)));
						  (yyval.declarator)->type = NULL;
						;}
    break;

  case 323:

/* Line 1464 of yacc.c  */
#line 971 "parser.y"
    { (yyval.declarator) = (yyvsp[(3) - (3)].declarator); (yyval.declarator)->type = append_ptrchain_type((yyval.declarator)->type, type_new_pointer(pointer_default, NULL, (yyvsp[(2) - (3)].attr_list))); ;}
    break;

  case 324:

/* Line 1464 of yacc.c  */
#line 972 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (2)].declarator); if ((yyval.declarator)->func_type) (yyval.declarator)->func_type->attrs = append_attr((yyval.declarator)->func_type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[(1) - (2)].str)));
						           else if ((yyval.declarator)->type) (yyval.declarator)->type->attrs = append_attr((yyval.declarator)->type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[(1) - (2)].str))); ;}
    break;

  case 326:

/* Line 1464 of yacc.c  */
#line 980 "parser.y"
    { (yyval.declarator) = (yyvsp[(3) - (3)].declarator); (yyval.declarator)->type = append_ptrchain_type((yyval.declarator)->type, type_new_pointer(pointer_default, NULL, (yyvsp[(2) - (3)].attr_list))); ;}
    break;

  case 327:

/* Line 1464 of yacc.c  */
#line 981 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (2)].declarator); if ((yyval.declarator)->func_type) (yyval.declarator)->func_type->attrs = append_attr((yyval.declarator)->func_type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[(1) - (2)].str)));
						           else if ((yyval.declarator)->type) (yyval.declarator)->type->attrs = append_attr((yyval.declarator)->type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[(1) - (2)].str))); ;}
    break;

  case 328:

/* Line 1464 of yacc.c  */
#line 986 "parser.y"
    { (yyval.declarator) = make_declarator(NULL); ;}
    break;

  case 330:

/* Line 1464 of yacc.c  */
#line 992 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (3)].declarator); ;}
    break;

  case 331:

/* Line 1464 of yacc.c  */
#line 993 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (2)].declarator); (yyval.declarator)->array = append_array((yyval.declarator)->array, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 332:

/* Line 1464 of yacc.c  */
#line 994 "parser.y"
    { (yyval.declarator) = make_declarator(NULL); (yyval.declarator)->array = append_array((yyval.declarator)->array, (yyvsp[(1) - (1)].expr)); ;}
    break;

  case 333:

/* Line 1464 of yacc.c  */
#line 996 "parser.y"
    { (yyval.declarator) = make_declarator(NULL);
						  (yyval.declarator)->func_type = append_ptrchain_type((yyval.declarator)->type, type_new_function((yyvsp[(2) - (3)].var_list)));
						  (yyval.declarator)->type = NULL;
						;}
    break;

  case 334:

/* Line 1464 of yacc.c  */
#line 1001 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (4)].declarator);
						  (yyval.declarator)->func_type = append_ptrchain_type((yyval.declarator)->type, type_new_function((yyvsp[(3) - (4)].var_list)));
						  (yyval.declarator)->type = NULL;
						;}
    break;

  case 335:

/* Line 1464 of yacc.c  */
#line 1010 "parser.y"
    { (yyval.declarator) = (yyvsp[(3) - (3)].declarator); (yyval.declarator)->type = append_ptrchain_type((yyval.declarator)->type, type_new_pointer(pointer_default, NULL, (yyvsp[(2) - (3)].attr_list))); ;}
    break;

  case 336:

/* Line 1464 of yacc.c  */
#line 1011 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (2)].declarator); (yyval.declarator)->type->attrs = append_attr((yyval.declarator)->type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[(1) - (2)].str))); ;}
    break;

  case 338:

/* Line 1464 of yacc.c  */
#line 1018 "parser.y"
    { (yyval.declarator) = (yyvsp[(3) - (3)].declarator); (yyval.declarator)->type = append_ptrchain_type((yyval.declarator)->type, type_new_pointer(pointer_default, NULL, (yyvsp[(2) - (3)].attr_list))); ;}
    break;

  case 339:

/* Line 1464 of yacc.c  */
#line 1019 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (2)].declarator); (yyval.declarator)->type->attrs = append_attr((yyval.declarator)->type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[(1) - (2)].str))); ;}
    break;

  case 340:

/* Line 1464 of yacc.c  */
#line 1023 "parser.y"
    { (yyval.declarator) = make_declarator(NULL); ;}
    break;

  case 342:

/* Line 1464 of yacc.c  */
#line 1031 "parser.y"
    { (yyval.declarator) = make_declarator((yyvsp[(1) - (1)].var)); ;}
    break;

  case 343:

/* Line 1464 of yacc.c  */
#line 1032 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (3)].declarator); ;}
    break;

  case 344:

/* Line 1464 of yacc.c  */
#line 1033 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (2)].declarator); (yyval.declarator)->array = append_array((yyval.declarator)->array, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 345:

/* Line 1464 of yacc.c  */
#line 1034 "parser.y"
    { (yyval.declarator) = make_declarator(NULL); (yyval.declarator)->array = append_array((yyval.declarator)->array, (yyvsp[(1) - (1)].expr)); ;}
    break;

  case 346:

/* Line 1464 of yacc.c  */
#line 1036 "parser.y"
    { (yyval.declarator) = make_declarator(NULL);
						  (yyval.declarator)->func_type = append_ptrchain_type((yyval.declarator)->type, type_new_function((yyvsp[(2) - (3)].var_list)));
						  (yyval.declarator)->type = NULL;
						;}
    break;

  case 347:

/* Line 1464 of yacc.c  */
#line 1041 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (4)].declarator);
						  (yyval.declarator)->func_type = append_ptrchain_type((yyval.declarator)->type, type_new_function((yyvsp[(3) - (4)].var_list)));
						  (yyval.declarator)->type = NULL;
						;}
    break;

  case 348:

/* Line 1464 of yacc.c  */
#line 1048 "parser.y"
    { (yyval.declarator_list) = append_declarator( NULL, (yyvsp[(1) - (1)].declarator) ); ;}
    break;

  case 349:

/* Line 1464 of yacc.c  */
#line 1049 "parser.y"
    { (yyval.declarator_list) = append_declarator( (yyvsp[(1) - (3)].declarator_list), (yyvsp[(3) - (3)].declarator) ); ;}
    break;

  case 350:

/* Line 1464 of yacc.c  */
#line 1052 "parser.y"
    { (yyval.expr) = NULL; ;}
    break;

  case 351:

/* Line 1464 of yacc.c  */
#line 1053 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (2)].expr); ;}
    break;

  case 352:

/* Line 1464 of yacc.c  */
#line 1056 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (2)].declarator); (yyval.declarator)->bits = (yyvsp[(2) - (2)].expr);
						  if (!(yyval.declarator)->bits && !(yyval.declarator)->var->name)
						    error_loc("unnamed fields are not allowed\n");
						;}
    break;

  case 353:

/* Line 1464 of yacc.c  */
#line 1063 "parser.y"
    { (yyval.declarator_list) = append_declarator( NULL, (yyvsp[(1) - (1)].declarator) ); ;}
    break;

  case 354:

/* Line 1464 of yacc.c  */
#line 1065 "parser.y"
    { (yyval.declarator_list) = append_declarator( (yyvsp[(1) - (3)].declarator_list), (yyvsp[(3) - (3)].declarator) ); ;}
    break;

  case 355:

/* Line 1464 of yacc.c  */
#line 1069 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (1)].declarator); ;}
    break;

  case 356:

/* Line 1464 of yacc.c  */
#line 1070 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (3)].declarator); (yyvsp[(1) - (3)].declarator)->var->eval = (yyvsp[(3) - (3)].expr); ;}
    break;

  case 357:

/* Line 1464 of yacc.c  */
#line 1074 "parser.y"
    { (yyval.num) = THREADING_APARTMENT; ;}
    break;

  case 358:

/* Line 1464 of yacc.c  */
#line 1075 "parser.y"
    { (yyval.num) = THREADING_NEUTRAL; ;}
    break;

  case 359:

/* Line 1464 of yacc.c  */
#line 1076 "parser.y"
    { (yyval.num) = THREADING_SINGLE; ;}
    break;

  case 360:

/* Line 1464 of yacc.c  */
#line 1077 "parser.y"
    { (yyval.num) = THREADING_FREE; ;}
    break;

  case 361:

/* Line 1464 of yacc.c  */
#line 1078 "parser.y"
    { (yyval.num) = THREADING_BOTH; ;}
    break;

  case 362:

/* Line 1464 of yacc.c  */
#line 1082 "parser.y"
    { (yyval.num) = RPC_FC_RP; ;}
    break;

  case 363:

/* Line 1464 of yacc.c  */
#line 1083 "parser.y"
    { (yyval.num) = RPC_FC_UP; ;}
    break;

  case 364:

/* Line 1464 of yacc.c  */
#line 1084 "parser.y"
    { (yyval.num) = RPC_FC_FP; ;}
    break;

  case 365:

/* Line 1464 of yacc.c  */
#line 1087 "parser.y"
    { (yyval.type) = type_new_struct((yyvsp[(2) - (5)].str), TRUE, (yyvsp[(4) - (5)].var_list)); ;}
    break;

  case 366:

/* Line 1464 of yacc.c  */
#line 1090 "parser.y"
    { (yyval.type) = type_new_void(); ;}
    break;

  case 367:

/* Line 1464 of yacc.c  */
#line 1091 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 368:

/* Line 1464 of yacc.c  */
#line 1092 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (1)].type); ;}
    break;

  case 369:

/* Line 1464 of yacc.c  */
#line 1093 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (1)].type); ;}
    break;

  case 370:

/* Line 1464 of yacc.c  */
#line 1094 "parser.y"
    { (yyval.type) = type_new_enum((yyvsp[(2) - (2)].str), FALSE, NULL); ;}
    break;

  case 371:

/* Line 1464 of yacc.c  */
#line 1095 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (1)].type); ;}
    break;

  case 372:

/* Line 1464 of yacc.c  */
#line 1096 "parser.y"
    { (yyval.type) = type_new_struct((yyvsp[(2) - (2)].str), FALSE, NULL); ;}
    break;

  case 373:

/* Line 1464 of yacc.c  */
#line 1097 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (1)].type); ;}
    break;

  case 374:

/* Line 1464 of yacc.c  */
#line 1098 "parser.y"
    { (yyval.type) = type_new_nonencapsulated_union((yyvsp[(2) - (2)].str), FALSE, NULL); ;}
    break;

  case 375:

/* Line 1464 of yacc.c  */
#line 1099 "parser.y"
    { (yyval.type) = make_safearray((yyvsp[(3) - (4)].type)); ;}
    break;

  case 376:

/* Line 1464 of yacc.c  */
#line 1103 "parser.y"
    { reg_typedefs((yyvsp[(3) - (4)].declspec), (yyvsp[(4) - (4)].declarator_list), check_typedef_attrs((yyvsp[(2) - (4)].attr_list)));
						  (yyval.statement) = make_statement_typedef((yyvsp[(4) - (4)].declarator_list));
						;}
    break;

  case 377:

/* Line 1464 of yacc.c  */
#line 1109 "parser.y"
    { (yyval.type) = type_new_nonencapsulated_union((yyvsp[(2) - (5)].str), TRUE, (yyvsp[(4) - (5)].var_list)); ;}
    break;

  case 378:

/* Line 1464 of yacc.c  */
#line 1112 "parser.y"
    { (yyval.type) = type_new_encapsulated_union((yyvsp[(2) - (10)].str), (yyvsp[(5) - (10)].var), (yyvsp[(7) - (10)].var), (yyvsp[(9) - (10)].var_list)); ;}
    break;

  case 379:

/* Line 1464 of yacc.c  */
#line 1116 "parser.y"
    { (yyval.num) = MAKEVERSION((yyvsp[(1) - (1)].num), 0); ;}
    break;

  case 380:

/* Line 1464 of yacc.c  */
#line 1117 "parser.y"
    { (yyval.num) = MAKEVERSION((yyvsp[(1) - (3)].num), (yyvsp[(3) - (3)].num)); ;}
    break;



/* Line 1464 of yacc.c  */
#line 5456 "parser.tab.c"
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

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
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



/* Line 1684 of yacc.c  */
#line 1120 "parser.y"


static void decl_builtin_basic(const char *name, enum type_basic_type type)
{
  type_t *t = type_new_basic(type);
  reg_type(t, name, 0);
}

static void decl_builtin_alias(const char *name, type_t *t)
{
  reg_type(type_new_alias(t, name), name, 0);
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

static type_t *append_ptrchain_type(type_t *ptrchain, type_t *type)
{
  type_t *ptrchain_type;
  if (!ptrchain)
    return type;
  for (ptrchain_type = ptrchain; type_pointer_get_ref(ptrchain_type); ptrchain_type = type_pointer_get_ref(ptrchain_type))
    ;
  assert(ptrchain_type->type_type == TYPE_POINTER);
  ptrchain_type->details.pointer.ref = type;
  return ptrchain;
}

static var_t *declare_var(attr_list_t *attrs, decl_spec_t *decl_spec, const declarator_t *decl,
                       int top)
{
  var_t *v = decl->var;
  expr_list_t *sizes = get_attrp(attrs, ATTR_SIZEIS);
  expr_list_t *lengs = get_attrp(attrs, ATTR_LENGTHIS);
  int sizeless;
  expr_t *dim;
  type_t **ptype;
  array_dims_t *arr = decl ? decl->array : NULL;
  type_t *func_type = decl ? decl->func_type : NULL;
  type_t *type = decl_spec->type;
  
  /* In case of a ranged type, duplicate it */
  if(is_attr(attrs, ATTR_RANGE))
    type = duptype(type, 1);

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
  v->attrs = attrs;

  /* check for pointer attribute being applied to non-pointer, non-array
   * type */
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
        ptr = type_alias_get_aliasee(ptr);
      else
        break;
    }
    if (is_ptr(ptr))
    {
      if (ptr_attr && ptr_attr != RPC_FC_UP &&
          type_get_type(type_pointer_get_ref(ptr)) == TYPE_INTERFACE)
          warning_loc_info(&v->loc_info,
                           "%s: pointer attribute applied to interface "
                           "pointer type has no effect\n", v->name);
      if (!ptr_attr && top && (*pt)->details.pointer.def_fc != RPC_FC_RP)
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

    if (!is_ptr(v->type) && !arr)
      error_loc("'%s': [string] attribute applied to non-pointer, non-array type\n",
                v->name);

    while (is_ptr(t))
      t = type_pointer_get_ref(t);

    if (type_get_type(t) != TYPE_BASIC &&
        (get_basic_fc(t) != RPC_FC_CHAR &&
         get_basic_fc(t) != RPC_FC_BYTE &&
         get_basic_fc(t) != RPC_FC_WCHAR))
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
        unsigned int size = type_memsize(v->type);

        if (0xffffffffu / size < dim->cval)
          error_loc("%s: total array size is too large\n", v->name);
      }
    }
    else
      sizeless = TRUE;

    *ptype = type_new_array(NULL, *ptype, FALSE,
                            dim->is_const ? dim->cval : 0,
                            dim->is_const ? NULL : dim, NULL,
                            pointer_default);
  }

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
    ft->details.function->rettype = return_type;
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
  d->array = NULL;
  d->bits = NULL;
  return d;
}

static type_t *make_safearray(type_t *type)
{
  return type_new_array(NULL, type_new_alias(type, "SAFEARRAY"), TRUE, 0,
                        NULL, NULL, RPC_FC_RP);
}

static typelib_t *make_library(const char *name, const attr_list_t *attrs)
{
    typelib_t *typelib = xmalloc(sizeof(*typelib));
    typelib->name = xstrdup(name);
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

type_t *reg_type(type_t *type, const char *name, int t)
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

  /* We must generate names for tagless enum, struct or union.
     Typedef-ing a tagless enum, struct or union means we want the typedef
     to be included in a library hence the public attribute.  */
  if ((type_get_type_detect_alias(type) == TYPE_ENUM ||
       type_get_type_detect_alias(type) == TYPE_STRUCT ||
       type_get_type_detect_alias(type) == TYPE_UNION ||
       type_get_type_detect_alias(type) == TYPE_ENCAPSULATED_UNION) &&
      !type->name && !parse_only)
  {
    if (! is_attr(attrs, ATTR_PUBLIC))
      attrs = append_attr( attrs, make_attr(ATTR_PUBLIC) );
    type->name = gen_name();
  }
  else if (is_attr(attrs, ATTR_UUID) && !is_attr(attrs, ATTR_PUBLIC))
    attrs = append_attr( attrs, make_attr(ATTR_PUBLIC) );

  /* Append the SWITCHTYPE attribute to a non-encapsulated union if it does not already have it.  */
  if (type_get_type_detect_alias(type) == TYPE_UNION &&
      is_attr(attrs, ATTR_SWITCHTYPE) &&
      !is_attr(type->attrs, ATTR_SWITCHTYPE))
    type->attrs = append_attr(type->attrs, make_attrp(ATTR_SWITCHTYPE, get_attrp(attrs, ATTR_SWITCHTYPE)));

  LIST_FOR_EACH_ENTRY( decl, decls, const declarator_t, entry )
  {

    if (decl->var->name) {
      type_t *cur;
      var_t *name;

      cur = find_type(decl->var->name, 0);
      if (cur)
          error_loc("%s: redefinition error; original definition was at %s:%d\n",
                    cur->name, cur->loc_info.input_name,
                    cur->loc_info.line_number);

      name = declare_var(attrs, decl_spec, decl, 0);
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

type_t *get_type(enum type_type type, char *name, int t)
{
  type_t *tp;
  if (name) {
    tp = find_type(name, t);
    if (tp) {
      free(name);
      return tp;
    }
  }
  tp = make_type(type);
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
    /* ATTR_ANNOTATION */          { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "annotation" },
    /* ATTR_APPOBJECT */           { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "appobject" },
    /* ATTR_ASYNC */               { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "async" },
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
    /* ATTR_HIDDEN */              { 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, "hidden" },
    /* ATTR_ID */                  { 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, "id" },
    /* ATTR_IDEMPOTENT */          { 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "idempotent" },
    /* ATTR_IGNORE */              { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, "ignore" },
    /* ATTR_IIDIS */               { 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, "iid_is" },
    /* ATTR_IMMEDIATEBIND */       { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "immediatebind" },
    /* ATTR_IMPLICIT_HANDLE */     { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "implicit_handle" },
    /* ATTR_IN */                  { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "in" },
    /* ATTR_INLINE */              { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "inline" },
    /* ATTR_INPUTSYNC */           { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "inputsync" },
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
    /* ATTR_UUID */                { 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, "uuid" },
    /* ATTR_V1ENUM */              { 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, "v1_enum" },
    /* ATTR_VARARG */              { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "vararg" },
    /* ATTR_VERSION */             { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, "version" },
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

static void check_statements(const statement_list_t *stmts, int is_inside_library)
{
    const statement_t *stmt;

    if (stmts) LIST_FOR_EACH_ENTRY(stmt, stmts, const statement_t, entry)
    {
      if (stmt->type == STMT_LIBRARY)
          check_statements(stmt->u.lib->stmts, TRUE);
      else if (stmt->type == STMT_TYPE && type_get_type(stmt->u.type) == TYPE_INTERFACE)
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

