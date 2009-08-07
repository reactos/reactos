
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
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
#define YYBISON_VERSION "2.4.1"

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
static var_list_t *append_var_list(var_list_t *list, var_list_t *vars);
static declarator_list_t *append_declarator(declarator_list_t *list, declarator_t *p);
static declarator_t *make_declarator(var_t *var);
static func_list_t *append_func(func_list_t *list, func_t *func);
static func_t *make_func(var_t *def);
static type_t *make_safearray(type_t *type);
static typelib_t *make_library(const char *name, const attr_list_t *attrs);
static type_t *append_ptrchain_type(type_t *ptrchain, type_t *type);

static type_t *reg_typedefs(decl_spec_t *decl_spec, var_list_t *names, attr_list_t *attrs);
static type_t *find_type_or_error(const char *name, int t);
static type_t *find_type_or_error2(char *name, int t);

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



/* Line 189 of yacc.c  */
#line 238 "parser.tab.c"

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
     tSIZEIS = 376,
     tSIZEOF = 377,
     tSMALL = 378,
     tSOURCE = 379,
     tSTATIC = 380,
     tSTDCALL = 381,
     tSTRICTCONTEXTHANDLE = 382,
     tSTRING = 383,
     tSTRUCT = 384,
     tSWITCH = 385,
     tSWITCHIS = 386,
     tSWITCHTYPE = 387,
     tTRANSMITAS = 388,
     tTRUE = 389,
     tTYPEDEF = 390,
     tUNION = 391,
     tUNIQUE = 392,
     tUNSIGNED = 393,
     tUUID = 394,
     tV1ENUM = 395,
     tVARARG = 396,
     tVERSION = 397,
     tVOID = 398,
     tWCHAR = 399,
     tWIREMARSHAL = 400,
     ADDRESSOF = 401,
     NEG = 402,
     POS = 403,
     PPTR = 404,
     CAST = 405
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 156 "parser.y"

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



/* Line 214 of yacc.c  */
#line 455 "parser.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 467 "parser.tab.c"

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
#define YYLAST   1996

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  175
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  88
/* YYNRULES -- Number of rules.  */
#define YYNRULES  311
/* YYNRULES -- Number of states.  */
#define YYNSTATES  547

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   405

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   159,     2,     2,     2,   158,   151,     2,
     170,   171,   156,   155,   146,   154,   166,   157,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   148,   169,
     152,   174,   153,   147,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   167,     2,   168,   150,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   172,   149,   173,   160,     2,     2,     2,
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
     145,   161,   162,   163,   164,   165
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
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
     680,   682,   684,   687,   689,   692,   695,   697,   699,   702,
     705,   708,   714,   715,   718,   721,   724,   727,   730,   733,
     737,   740,   744,   750,   756,   757,   760,   763,   766,   769,
     776,   785,   788,   791,   794,   797,   800,   803,   809,   811,
     813,   815,   817,   819,   820,   823,   826,   830,   831,   833,
     836,   839,   842,   846,   849,   851,   853,   857,   860,   865,
     867,   871,   873,   877,   879,   881,   883,   889,   891,   893,
     895,   897,   900,   902,   905,   907,   910,   915,   920,   926,
     937,   939
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     176,     0,    -1,   177,    -1,    -1,   177,   242,    -1,   177,
     241,    -1,   177,   228,   169,    -1,   177,   230,    -1,   177,
     245,    -1,   177,   189,    -1,   177,   181,    -1,    -1,   178,
     242,    -1,   178,   241,    -1,   178,   228,   169,    -1,   178,
     230,    -1,   178,   245,    -1,   178,   181,    -1,   178,   186,
      -1,   178,   189,    -1,    -1,   179,   181,    -1,    -1,   169,
      -1,   183,    -1,   182,   169,    -1,   221,   169,    -1,   185,
      -1,   260,   169,    -1,   207,    -1,   258,    -1,   261,    -1,
     196,   207,    -1,   196,   258,    -1,   196,   261,    -1,    45,
     170,     8,   171,    -1,    78,     8,   169,    -1,   184,   178,
      11,    -1,    79,   170,     8,   171,   180,    -1,    89,     3,
      -1,   196,   187,   172,    -1,   188,   178,   173,   180,    -1,
      -1,   192,    -1,   143,    -1,   193,    -1,   192,   146,   193,
      -1,   191,    -1,   196,   250,   253,    -1,   250,   253,    -1,
     167,   209,   168,    -1,   167,   156,   168,    -1,    -1,   196,
      -1,   167,   197,   168,    -1,   199,    -1,   197,   146,   199,
      -1,   197,   168,   167,   199,    -1,     8,    -1,   198,   146,
       8,    -1,    -1,    21,    -1,    23,    -1,    24,    -1,    26,
      -1,    27,    -1,    29,    -1,    32,   170,   224,   171,    -1,
      34,   170,   211,   171,    -1,    41,    -1,    42,    -1,    43,
      -1,    44,    -1,    46,    -1,    47,    -1,    48,   170,   213,
     171,    -1,    49,    -1,    50,    -1,    52,   170,     8,   171,
      -1,    54,    -1,    55,   170,   198,   171,    -1,    56,   170,
     213,   171,    -1,    59,    -1,    64,    -1,    66,   170,   212,
     171,    -1,    67,   170,     8,   171,    -1,    68,   170,     8,
     171,    -1,    69,   170,   212,   171,    -1,    70,   170,     8,
     171,    -1,    71,    -1,    73,   170,   212,   171,    -1,    74,
      -1,    75,   170,   210,   171,    -1,    76,    -1,    77,   170,
      65,     3,   171,    -1,    80,    -1,    83,    -1,    88,   170,
     208,   171,    -1,    87,   170,   212,   171,    -1,    90,    -1,
      94,    -1,    95,    -1,    96,    -1,    98,    -1,    99,    -1,
     100,    -1,   101,    -1,   102,    -1,   104,   170,   257,   171,
      -1,   106,    -1,   107,    -1,   108,    -1,   110,    -1,   111,
     170,   212,   146,   212,   171,    -1,   112,    -1,   115,    -1,
     116,    -1,   117,    -1,   121,   170,   208,   171,    -1,   124,
      -1,   127,    -1,   128,    -1,   131,   170,   210,   171,    -1,
     132,   170,   259,   171,    -1,   133,   170,   259,   171,    -1,
     139,   170,   200,   171,    -1,   140,    -1,   141,    -1,   142,
     170,   262,   171,    -1,   145,   170,   259,   171,    -1,   257,
      -1,    10,    -1,     8,    -1,    35,    -1,    62,    -1,   103,
      -1,   126,    -1,    -1,   202,   203,    -1,    34,   212,   148,
     218,    -1,    46,   148,   218,    -1,    -1,   205,   146,    -1,
     205,    -1,   206,    -1,   205,   146,   206,    -1,   224,   174,
     212,    -1,   224,    -1,    57,   223,   172,   204,   173,    -1,
     209,    -1,   208,   146,   209,    -1,    -1,   210,    -1,     5,
      -1,     6,    -1,     7,    -1,    61,    -1,    97,    -1,   134,
      -1,     8,    -1,     9,    -1,     3,    -1,   210,   147,   210,
     148,   210,    -1,   210,    19,   210,    -1,   210,    20,   210,
      -1,   210,   149,   210,    -1,   210,   150,   210,    -1,   210,
     151,   210,    -1,   210,    15,   210,    -1,   210,    16,   210,
      -1,   210,   153,   210,    -1,   210,   152,   210,    -1,   210,
      17,   210,    -1,   210,    18,   210,    -1,   210,    12,   210,
      -1,   210,    13,   210,    -1,   210,   155,   210,    -1,   210,
     154,   210,    -1,   210,   158,   210,    -1,   210,   156,   210,
      -1,   210,   157,   210,    -1,   159,   210,    -1,   160,   210,
      -1,   155,   210,    -1,   154,   210,    -1,   151,   210,    -1,
     156,   210,    -1,   210,    14,     3,    -1,   210,   166,     3,
      -1,   170,   259,   171,   210,    -1,   122,   170,   259,   171,
      -1,   210,   167,   210,   168,    -1,   170,   210,   171,    -1,
     212,    -1,   211,   146,   212,    -1,   210,    -1,   210,    -1,
      -1,   214,   215,    -1,   195,   250,   255,   169,    -1,   195,
     261,   169,    -1,   219,   169,    -1,   196,   169,    -1,    -1,
     217,   216,    -1,   219,   169,    -1,   169,    -1,   195,   250,
     253,    -1,   195,   250,   253,    -1,   196,   250,   256,    -1,
     250,   256,    -1,    -1,   224,    -1,    -1,     3,    -1,     4,
      -1,     3,    -1,     4,    -1,    30,    -1,   144,    -1,   227,
      -1,   120,   227,    -1,   138,   227,    -1,   138,    -1,    63,
      -1,    53,    -1,    28,    -1,    58,    -1,    65,    -1,    -1,
      84,    -1,    84,    -1,   119,   226,    -1,   123,    -1,    91,
     226,    -1,    72,   226,    -1,    85,    -1,    36,    -1,    37,
       3,    -1,    37,     4,    -1,   196,   228,    -1,   229,   172,
     231,   173,   180,    -1,    -1,   231,   232,    -1,   195,   242,
      -1,    51,     3,    -1,    51,     4,    -1,   196,   233,    -1,
     105,   148,    -1,   235,   219,   169,    -1,    92,   148,    -1,
     236,   220,   169,    -1,   234,   172,   235,   236,   173,    -1,
     234,   172,   239,   169,   173,    -1,    -1,   148,     4,    -1,
      86,     3,    -1,    86,     4,    -1,   196,   239,    -1,   240,
     238,   172,   179,   173,   180,    -1,   240,   148,     3,   172,
     185,   179,   173,   180,    -1,   237,   180,    -1,   239,   169,
      -1,   233,   169,    -1,    93,     3,    -1,    93,     4,    -1,
     196,   243,    -1,   244,   172,   179,   173,   180,    -1,    60,
      -1,   125,    -1,   114,    -1,    82,    -1,    40,    -1,    -1,
     249,   248,    -1,   259,   251,    -1,   252,   259,   251,    -1,
      -1,   252,    -1,   248,   251,    -1,   247,   251,    -1,   246,
     251,    -1,   156,   249,   253,    -1,   201,   253,    -1,   254,
      -1,   224,    -1,   170,   253,   171,    -1,   254,   194,    -1,
     254,   170,   190,   171,    -1,   253,    -1,   255,   146,   253,
      -1,   253,    -1,   253,   174,   213,    -1,   113,    -1,   137,
      -1,   109,    -1,   129,   223,   172,   214,   173,    -1,   143,
      -1,     4,    -1,   225,    -1,   207,    -1,    57,     3,    -1,
     258,    -1,   129,     3,    -1,   261,    -1,   136,     3,    -1,
     118,   170,   259,   171,    -1,   135,   195,   250,   255,    -1,
     136,   223,   172,   217,   173,    -1,   136,   223,   130,   170,
     219,   171,   222,   172,   202,   173,    -1,     5,    -1,     5,
     166,     5,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   326,   326,   339,   340,   341,   342,   345,   348,   349,
     350,   353,   354,   355,   356,   357,   360,   361,   362,   363,
     366,   367,   370,   371,   375,   376,   377,   378,   379,   383,
     384,   385,   386,   387,   388,   391,   393,   401,   407,   411,
     413,   417,   424,   425,   428,   431,   432,   433,   437,   444,
     452,   453,   456,   457,   461,   464,   465,   466,   469,   470,
     473,   474,   475,   476,   477,   478,   479,   480,   481,   482,
     483,   484,   485,   486,   487,   488,   489,   490,   491,   492,
     493,   494,   495,   496,   497,   498,   499,   500,   501,   502,
     503,   504,   505,   506,   507,   508,   509,   510,   511,   512,
     513,   514,   515,   516,   517,   518,   519,   520,   521,   522,
     523,   524,   525,   526,   530,   531,   532,   533,   534,   535,
     536,   537,   538,   539,   540,   541,   542,   543,   544,   545,
     546,   550,   551,   556,   557,   558,   559,   562,   563,   566,
     570,   576,   577,   578,   581,   585,   594,   598,   603,   606,
     607,   620,   621,   624,   625,   626,   627,   628,   629,   630,
     631,   632,   633,   634,   635,   636,   637,   638,   639,   640,
     641,   642,   643,   644,   645,   646,   647,   648,   649,   650,
     651,   652,   653,   654,   655,   656,   657,   658,   659,   660,
     661,   662,   663,   666,   667,   670,   676,   682,   683,   686,
     691,   698,   699,   702,   703,   707,   708,   711,   719,   728,
     734,   740,   741,   744,   745,   746,   749,   751,   754,   755,
     756,   757,   758,   759,   760,   761,   762,   763,   764,   767,
     768,   771,   772,   773,   774,   775,   776,   777,   780,   781,
     789,   795,   799,   800,   804,   807,   808,   811,   821,   822,
     825,   826,   829,   835,   841,   842,   845,   846,   849,   860,
     867,   873,   877,   878,   881,   882,   885,   890,   897,   898,
     899,   903,   907,   910,   911,   914,   915,   919,   920,   924,
     925,   926,   930,   932,   933,   937,   938,   939,   940,   947,
     948,   952,   953,   957,   958,   959,   962,   965,   966,   967,
     968,   969,   970,   971,   972,   973,   974,   977,   983,   985,
     991,   992
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
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
  "tRETVAL", "tSAFEARRAY", "tSHORT", "tSIGNED", "tSIZEIS", "tSIZEOF",
  "tSMALL", "tSOURCE", "tSTATIC", "tSTDCALL", "tSTRICTCONTEXTHANDLE",
  "tSTRING", "tSTRUCT", "tSWITCH", "tSWITCHIS", "tSWITCHTYPE",
  "tTRANSMITAS", "tTRUE", "tTYPEDEF", "tUNION", "tUNIQUE", "tUNSIGNED",
  "tUUID", "tV1ENUM", "tVARARG", "tVERSION", "tVOID", "tWCHAR",
  "tWIREMARSHAL", "','", "'?'", "':'", "'|'", "'^'", "'&'", "'<'", "'>'",
  "'-'", "'+'", "'*'", "'/'", "'%'", "'!'", "'~'", "ADDRESSOF", "NEG",
  "POS", "PPTR", "CAST", "'.'", "'['", "']'", "';'", "'('", "')'", "'{'",
  "'}'", "'='", "$accept", "input", "gbl_statements", "imp_statements",
  "int_statements", "semicolon_opt", "statement", "typedecl", "cppquote",
  "import_start", "import", "importlib", "libraryhdr", "library_start",
  "librarydef", "m_args", "no_args", "args", "arg", "array",
  "m_attributes", "attributes", "attrib_list", "str_list", "attribute",
  "uuid_string", "callconv", "cases", "case", "enums", "enum_list", "enum",
  "enumdef", "m_exprs", "m_expr", "expr", "expr_list_int_const",
  "expr_int_const", "expr_const", "fields", "field", "ne_union_field",
  "ne_union_fields", "union_field", "s_field", "funcdef", "declaration",
  "m_ident", "t_ident", "ident", "base_type", "m_int", "int_std",
  "coclass", "coclasshdr", "coclassdef", "coclass_ints", "coclass_int",
  "dispinterface", "dispinterfacehdr", "dispint_props", "dispint_meths",
  "dispinterfacedef", "inherit", "interface", "interfacehdr",
  "interfacedef", "interfacedec", "module", "modulehdr", "moduledef",
  "storage_cls_spec", "function_specifier", "type_qualifier",
  "m_type_qual_list", "decl_spec", "m_decl_spec_no_type",
  "decl_spec_no_type", "declarator", "direct_declarator",
  "declarator_list", "init_declarator", "pointer_type", "structdef",
  "type", "typedef", "uniondef", "version", 0
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
     395,   396,   397,   398,   399,   400,    44,    63,    58,   124,
      94,    38,    60,    62,    45,    43,    42,    47,    37,    33,
     126,   401,   402,   403,   404,   405,    46,    91,    93,    59,
      40,    41,   123,   125,    61
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   175,   176,   177,   177,   177,   177,   177,   177,   177,
     177,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     179,   179,   180,   180,   181,   181,   181,   181,   181,   182,
     182,   182,   182,   182,   182,   183,   184,   185,   186,   187,
     188,   189,   190,   190,   191,   192,   192,   192,   193,   193,
     194,   194,   195,   195,   196,   197,   197,   197,   198,   198,
     199,   199,   199,   199,   199,   199,   199,   199,   199,   199,
     199,   199,   199,   199,   199,   199,   199,   199,   199,   199,
     199,   199,   199,   199,   199,   199,   199,   199,   199,   199,
     199,   199,   199,   199,   199,   199,   199,   199,   199,   199,
     199,   199,   199,   199,   199,   199,   199,   199,   199,   199,
     199,   199,   199,   199,   199,   199,   199,   199,   199,   199,
     199,   199,   199,   199,   199,   199,   199,   199,   199,   199,
     199,   200,   200,   201,   201,   201,   201,   202,   202,   203,
     203,   204,   204,   204,   205,   205,   206,   206,   207,   208,
     208,   209,   209,   210,   210,   210,   210,   210,   210,   210,
     210,   210,   210,   210,   210,   210,   210,   210,   210,   210,
     210,   210,   210,   210,   210,   210,   210,   210,   210,   210,
     210,   210,   210,   210,   210,   210,   210,   210,   210,   210,
     210,   210,   210,   211,   211,   212,   213,   214,   214,   215,
     215,   216,   216,   217,   217,   218,   218,   219,   220,   221,
     221,   222,   222,   223,   223,   223,   224,   224,   225,   225,
     225,   225,   225,   225,   225,   225,   225,   225,   225,   226,
     226,   227,   227,   227,   227,   227,   227,   227,   228,   228,
     229,   230,   231,   231,   232,   233,   233,   234,   235,   235,
     236,   236,   237,   237,   238,   238,   239,   239,   240,   241,
     241,   241,   242,   242,   243,   243,   244,   245,   246,   246,
     246,   247,   248,   249,   249,   250,   250,   251,   251,   252,
     252,   252,   253,   253,   253,   254,   254,   254,   254,   255,
     255,   256,   256,   257,   257,   257,   258,   259,   259,   259,
     259,   259,   259,   259,   259,   259,   259,   260,   261,   261,
     262,   262
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
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
       1,     2,     2,     1,     1,     1,     1,     1,     1,     0,
       1,     1,     2,     1,     2,     2,     1,     1,     2,     2,
       2,     5,     0,     2,     2,     2,     2,     2,     2,     3,
       2,     3,     5,     5,     0,     2,     2,     2,     2,     6,
       8,     2,     2,     2,     2,     2,     2,     5,     1,     1,
       1,     1,     1,     0,     2,     2,     3,     0,     1,     2,
       2,     2,     3,     2,     1,     1,     3,     2,     4,     1,
       3,     1,     3,     1,     1,     1,     5,     1,     1,     1,
       1,     2,     1,     2,     1,     2,     4,     4,     5,    10,
       1,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       3,     0,     2,     1,   298,   226,   218,   237,     0,   272,
       0,     0,   225,   213,   227,   268,   224,   228,   229,     0,
     271,   231,   236,     0,   229,   270,     0,   229,     0,   233,
     269,   213,    52,   213,   223,   297,   219,    60,    10,     0,
      24,    11,    27,    11,     9,     0,   300,     0,   299,   220,
       0,     0,     7,     0,     0,    22,     0,   254,     5,     4,
       0,     8,   277,   277,   277,     0,     0,   302,   277,     0,
     304,   238,   239,     0,   245,   246,   301,   215,     0,   230,
     235,     0,   256,   257,   234,     0,   232,   221,   303,     0,
       0,    53,   305,     0,   222,    61,    62,    63,    64,    65,
      66,     0,     0,    69,    70,    71,    72,    73,    74,     0,
      76,    77,     0,    79,     0,     0,    82,    83,     0,     0,
       0,     0,     0,    89,     0,    91,     0,    93,     0,    95,
      96,     0,     0,    99,   100,   101,   102,   103,   104,   105,
     106,   107,     0,   109,   110,   111,   295,   112,     0,   114,
     293,   115,   116,   117,     0,   119,   120,   121,     0,     0,
       0,   294,     0,   126,   127,     0,     0,     0,    55,   130,
      25,     0,     0,     0,     0,     0,   300,   240,   247,   258,
     266,     0,   302,   304,    26,     6,   242,   263,     0,    23,
     261,   262,     0,     0,    20,   281,   278,   280,   279,   216,
     217,   133,   134,   135,   136,   273,     0,     0,   285,   291,
     284,   210,   300,   302,   277,   304,   275,    28,     0,   141,
      36,     0,   197,     0,     0,   203,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   151,     0,     0,   151,     0,     0,     0,     0,     0,
       0,    60,    54,    37,     0,    17,    18,    19,     0,    15,
      13,    12,    16,    22,    39,   264,   265,    40,   209,    52,
       0,    52,     0,     0,   255,    20,     0,     0,     0,   283,
       0,   151,    42,   287,   276,    35,     0,   143,   144,   147,
     306,    52,   289,   307,    52,    52,     0,   161,   153,   154,
     155,   159,   160,   156,   157,     0,   158,     0,     0,     0,
       0,     0,     0,     0,   195,     0,   193,   196,     0,     0,
      58,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   149,   152,     0,     0,     0,     0,     0,
       0,   132,   131,     0,   310,     0,     0,    56,    60,     0,
      14,    41,    22,     0,   243,   248,     0,     0,     0,    52,
       0,     0,     0,    22,    21,     0,   274,   282,   286,   292,
       0,     0,   297,     0,    47,    43,    45,     0,     0,   148,
     142,     0,   296,     0,   198,     0,     0,   308,    53,   204,
       0,    67,     0,   185,   184,   183,   186,   181,   182,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    68,    75,    78,     0,    80,    81,
      84,    85,    86,    87,    88,    90,    92,     0,    98,   151,
      97,   108,     0,   118,   122,   123,   124,   125,     0,   128,
     129,    57,     0,   241,   244,   250,     0,   249,   252,     0,
       0,   253,    20,    22,   267,    51,    50,   288,     0,     0,
      49,   145,   146,     0,   304,   290,   211,   202,   201,     0,
     192,     0,   174,   175,   187,   168,   169,   172,   173,   163,
     164,     0,   165,   166,   167,   171,   170,   177,   176,   179,
     180,   178,   188,     0,   194,    59,    94,   150,     0,   311,
      22,   207,     0,   251,     0,   259,    46,    48,     0,   200,
       0,   212,   190,   189,     0,   191,   113,    38,   208,    22,
     199,   137,   162,   260,     0,     0,     0,   309,   138,     0,
      52,    52,   206,   140,     0,   139,   205
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     2,   171,   276,   190,   364,    39,    40,    41,
      42,   256,   175,    43,   257,   373,   374,   375,   376,   283,
     357,    91,   167,   321,   168,   343,   207,   534,   538,   286,
     287,   288,   212,   332,   333,   314,   315,   316,   318,   291,
     384,   389,   295,   543,   544,   460,    47,   520,    78,   208,
      48,    80,    49,   258,    51,   259,   269,   354,    53,    54,
     271,   359,    55,   193,    56,    57,   260,   261,   180,    60,
     262,    62,    63,    64,   277,    65,   195,    66,   209,   210,
     293,   211,   169,   213,    68,    69,   215,   345
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -269
static const yytype_int16 yypact[] =
{
    -269,    42,  1015,  -269,  -269,  -269,  -269,  -269,    82,  -269,
    -118,   125,  -269,   127,  -269,  -269,  -269,  -269,   -14,    88,
    -269,  -269,  -269,   129,   -14,  -269,   -70,   -14,   253,  -269,
    -269,   131,   -64,   134,   253,  -269,  -269,  1851,  -269,   -22,
    -269,  -269,  -269,  -269,  -269,  1607,   -21,   -20,  -269,  -269,
     -19,   -58,  -269,   -17,   -18,   -10,    -8,     5,  -269,  -269,
      -4,  -269,    -5,    -5,    -5,   255,  1727,    -7,    -5,    28,
      30,  -269,  -269,   147,  -269,  -269,    20,  -269,    26,  -269,
    -269,    43,  -269,  -269,  -269,  1727,  -269,  -269,    20,    31,
    1684,  -269,   -96,   -94,  -269,  -269,  -269,  -269,  -269,  -269,
    -269,    41,    46,  -269,  -269,  -269,  -269,  -269,  -269,    50,
    -269,  -269,    53,  -269,    56,    59,  -269,  -269,    63,    64,
      65,    67,    73,  -269,    76,  -269,    77,  -269,    84,  -269,
    -269,    95,    96,  -269,  -269,  -269,  -269,  -269,  -269,  -269,
    -269,  -269,    99,  -269,  -269,  -269,  -269,  -269,   100,  -269,
    -269,  -269,  -269,  -269,   103,  -269,  -269,  -269,   111,   113,
     114,  -269,   115,  -269,  -269,   116,   117,   -93,  -269,  -269,
    -269,   921,   404,   161,   136,    58,   119,  -269,  -269,  -269,
    -269,   255,   122,   123,  -269,  -269,  -269,  -269,    -6,  -269,
    -269,  -269,   139,    91,  -269,  -269,  -269,  -269,  -269,  -269,
    -269,  -269,  -269,  -269,  -269,  -269,   255,   255,  -269,   128,
    -129,  -269,  -269,  -269,    -5,  -269,  -269,  -269,   -15,   141,
    -269,   130,  -269,   255,   133,  -269,   141,    85,    85,   296,
     297,    85,    85,   298,   304,    85,   307,    85,    85,   251,
      85,    85,   -55,    85,    85,    85,  1727,  1727,   102,   317,
    1727,  1851,   156,  -269,   154,  -269,  -269,  -269,   157,  -269,
    -269,  -269,  -269,   -10,  -269,  -269,  -269,  -269,  -269,   -66,
     179,   -69,   159,   158,  -269,  -269,   498,   101,   160,  -269,
      85,   867,  1109,  -269,  -269,  -269,   163,   183,  -269,   166,
    -269,   -56,  -269,   188,   -64,   -54,   170,  -269,  -269,  -269,
    -269,  -269,  -269,  -269,  -269,   172,  -269,    85,    85,    85,
      85,    85,    85,   780,  1402,  -106,  -269,  1402,   174,   175,
    -269,  -102,   176,   177,   178,   180,   181,   182,   184,  1189,
     340,   185,   -90,  -269,  1402,   190,   204,   -82,  1242,   192,
     195,  -269,  -269,   198,   193,   199,   202,  -269,  1851,   366,
    -269,  -269,   -10,   -12,  -269,  -269,   227,  1684,   208,   -51,
     205,   301,   592,   -10,  -269,  1684,  -269,  -269,  -269,  -269,
      54,   212,   -74,   211,  -269,   238,  -269,  1684,   255,  -269,
     141,    85,  -269,  1684,  -269,   255,   214,  -269,   217,  -269,
     218,  -269,  1727,    34,    34,    34,    34,    34,    34,  1292,
     219,    85,    85,   407,    85,    85,    85,    85,    85,    85,
      85,    85,    85,    85,    85,    85,    85,    85,    85,    85,
      85,   409,    85,    85,  -269,  -269,  -269,   405,  -269,  -269,
    -269,  -269,  -269,  -269,  -269,  -269,  -269,   243,  -269,    85,
    -269,  -269,    85,  -269,  -269,  -269,  -269,  -269,   411,  -269,
    -269,  -269,   246,  -269,  -269,  -269,   255,  -269,  -269,  1684,
     249,  -269,  -269,   -10,  -269,  -269,  -269,  -269,  1159,   255,
    -269,  -269,  -269,   255,   250,  -269,   141,  -269,  -269,   252,
    -269,    85,    94,    94,  -269,  1114,  1114,   153,   153,  1368,
    1439,  1349,  1420,  1462,  1496,   153,   153,    29,    29,    34,
      34,    34,  -269,  1315,  -269,  -269,  -269,  -269,   256,  -269,
     -10,  -269,   255,  -269,   686,  -269,  -269,  -269,   -67,  -269,
     248,  -269,  -269,    34,    85,  -269,  -269,  -269,  -269,   -10,
    -269,  -269,  1402,  -269,   -16,    85,   273,  -269,  -269,   274,
     -43,   -43,  -269,  -269,   259,  -269,  -269
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -269,  -269,  -269,   386,  -268,  -257,     8,  -269,  -269,  -269,
      69,  -269,  -269,  -269,   429,  -269,  -269,  -269,   -35,  -269,
     -27,    -2,  -269,  -269,  -230,  -269,  -269,  -269,  -269,  -269,
    -269,    55,     2,   194,  -264,   -13,  -269,  -221,  -207,  -269,
    -269,  -269,  -269,  -105,  -245,  -269,  -269,  -269,    92,  -199,
    -269,    60,    93,    23,  -269,   435,  -269,  -269,   394,  -269,
    -269,  -269,  -269,  -269,   -30,  -269,   440,     1,  -269,  -269,
     441,  -269,  -269,   168,  -269,   -44,     3,   -31,  -194,  -269,
     -26,   265,   206,     6,   -57,  -269,     0,  -269
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -215
static const yytype_int16 yytable[] =
{
      45,   181,    70,    59,    46,    90,   351,   362,    67,   214,
      38,   323,   278,   279,   326,   179,   328,   371,   535,   331,
     289,   347,   336,   356,   322,    50,   358,   296,   221,   292,
     536,   196,   196,   196,  -214,     9,   224,   196,   281,    11,
     423,   282,     3,   403,   427,   183,   223,   176,   403,   386,
     390,   182,    73,   251,   146,    15,   439,   297,   150,   298,
     299,   300,   301,   302,   439,   424,   197,   198,   177,   428,
      79,   216,   -44,   369,    23,   252,  -214,    20,   225,   385,
      23,   440,   161,   367,    84,    71,    72,    86,   297,   443,
     298,   299,   300,   301,   302,   453,    81,   -44,    37,   270,
      85,    37,   530,    37,   199,   200,   464,   352,   403,    25,
     341,    37,   342,    37,   186,   303,    37,   382,   451,   387,
      30,    87,   458,    89,    37,    93,   542,    94,    74,    75,
      76,    77,    82,    83,    88,    77,   201,    92,    77,   265,
     266,     9,   273,   274,   199,   200,   303,   170,   -29,   184,
     185,   304,   187,   192,   188,   218,   285,   537,   272,   189,
     472,   191,   -30,   202,   264,   401,   402,   403,   194,    45,
      45,    70,    70,    46,    46,   507,   305,    67,    67,   255,
     255,   289,   304,   196,   470,   418,   419,   420,   306,   339,
     340,   475,  -214,   346,   514,   421,   422,   217,   219,   -31,
     421,   422,   504,   222,   203,   307,   515,   305,   308,   309,
     310,   226,   220,   311,   312,   317,   227,   284,   317,   306,
     228,   508,   465,   229,   313,   329,   230,   204,   334,   231,
     267,   334,   338,   232,   233,   234,   307,   235,   378,   308,
     309,   310,   353,   236,   311,   312,   237,   238,   416,   417,
     418,   419,   420,   527,   239,   313,   400,   205,   199,   200,
     421,   422,   511,   275,   383,   240,   241,   317,   334,   242,
     243,   206,   533,   244,   365,   517,    70,   521,    46,   292,
     377,   245,    67,   246,   247,   248,   249,   250,   -32,     7,
     201,   -33,   -34,   388,   393,   394,   395,   396,   397,   398,
     399,   290,   280,   294,   319,   320,   324,   416,   417,   418,
     419,   420,   325,   456,   539,   327,   330,   202,   528,   421,
     422,   181,   344,   348,   349,    18,   350,   355,   360,   380,
     361,   368,   459,   469,   385,   479,   379,    21,    22,   473,
     381,   391,   392,   437,    24,   425,   426,   429,   430,   431,
     442,   432,   433,   434,   454,   435,   438,   396,   203,   448,
     365,   441,    70,   445,    46,   183,   446,   176,    67,   447,
     449,   182,    27,   450,   452,   455,    29,   457,   461,    19,
     466,   204,   467,   474,   468,   476,   477,   478,   482,   483,
     481,   485,   486,   487,   488,   489,   490,   491,   492,   493,
     494,   495,   496,   497,   498,   499,   500,   501,     4,   503,
     484,   205,   502,   505,   506,   512,   509,   510,   513,   519,
     531,   540,   541,   522,   378,   206,   334,   526,   546,   172,
     462,    44,     5,   516,     6,   471,   545,    52,   337,   178,
       7,     8,    58,    61,     9,   366,   268,   518,   335,    10,
       0,     0,     0,     0,     0,    11,     0,    12,     0,     0,
       0,    13,    14,     0,    15,     0,   377,    16,   523,    17,
       0,     0,     0,     0,     0,     0,    18,     0,     0,     0,
       0,     0,    19,   254,     0,     0,    20,     0,    21,    22,
      23,     0,     0,     0,     0,    24,     0,     0,     0,     0,
       0,     0,     4,     0,     0,     0,     0,     0,     0,     0,
       0,   532,   365,     0,    70,     0,    46,     0,    25,     0,
      67,     0,    26,    27,    28,     0,     5,    29,     6,    30,
       0,     0,     0,    31,     7,     0,     0,     0,     9,    32,
      33,     0,    34,    10,     0,     0,     0,    35,    36,     0,
       0,    12,     0,     0,     0,    13,    14,     0,    15,     0,
       0,    16,     0,    17,     0,     0,     0,     0,     0,     0,
      18,    37,     0,     0,     0,     0,    19,   263,     0,     0,
      20,     0,    21,    22,     0,     0,     0,     0,     0,    24,
       0,     0,     0,     0,     0,     0,     4,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    25,     0,     0,     0,    26,    27,    28,     0,
       5,    29,     6,    30,     0,     0,     0,    31,     7,     0,
       0,     0,     9,    32,    33,     0,    34,    10,     0,     0,
       0,    35,    36,     0,     0,    12,     0,     0,     0,    13,
      14,     0,    15,     0,     0,    16,     0,    17,     0,     0,
       0,     0,     0,     0,    18,    37,     0,     0,     0,     0,
      19,   363,     0,     0,    20,     0,    21,    22,     0,     0,
       0,     0,     0,    24,     0,     0,     0,     0,     0,     0,
       4,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    25,     0,     0,     0,
      26,    27,    28,     0,     5,    29,     6,    30,     0,     0,
       0,    31,     7,     0,     0,     0,     9,    32,    33,     0,
      34,    10,     0,     0,     0,    35,    36,     0,     0,    12,
       0,     0,     0,    13,    14,     0,    15,     0,     0,    16,
       0,    17,     0,     0,     0,     0,     0,     0,    18,    37,
       0,     0,     0,     0,    19,   463,     0,     0,    20,     0,
      21,    22,     0,     0,     0,     0,     0,    24,     0,     0,
       0,     0,     0,   297,     4,   298,   299,   300,   301,   302,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      25,     0,     0,     0,    26,    27,    28,     0,     5,    29,
       6,    30,     0,     0,     0,    31,     7,     0,     0,     0,
       0,    32,    33,     0,    34,     0,     0,     0,     0,    35,
      36,     0,     0,    12,     0,     0,     0,    13,    14,     0,
       0,   303,     0,    16,     0,    17,     0,     0,     0,     0,
       0,     0,    18,    37,     0,     0,     0,     0,     0,   529,
       0,     0,     0,     0,    21,    22,     0,     0,     0,     0,
     297,    24,   298,   299,   300,   301,   302,   304,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    26,    27,
      28,     0,   305,    29,     0,     0,     0,     0,     0,    31,
       0,     0,     0,     0,   306,     0,    33,     0,    34,     0,
       0,     0,     0,    35,    36,     4,     0,     0,   303,     0,
       0,   307,   253,     0,   308,   309,   310,     0,     0,   311,
     312,     0,     0,     0,     0,     0,     0,     0,     0,     5,
     313,     6,     0,     0,     0,     0,     0,     7,     8,     0,
       0,     9,     0,     0,   304,     0,    10,     0,     0,     0,
       0,     0,    11,     0,    12,     0,     0,     0,    13,    14,
       0,    15,     0,     0,    16,     0,    17,     0,     0,   305,
       0,     0,     0,    18,     0,     0,     0,     0,     0,    19,
     254,   306,     0,    20,     0,    21,    22,    23,     0,     0,
       0,     0,    24,     0,     0,     0,     0,     0,   307,     4,
       0,   308,   309,   370,     0,     0,   311,   312,     0,     0,
       0,     0,     0,     0,     0,    25,     0,   313,     0,    26,
      27,    28,     0,     5,    29,     6,    30,     0,     0,     0,
      31,     7,     8,     0,     0,     9,    32,    33,     0,    34,
      10,     0,     0,     0,    35,    36,    11,     0,    12,     0,
       0,     0,    13,    14,     0,    15,     0,     0,    16,     0,
      17,     0,     0,     0,     0,     0,     0,    18,    37,     0,
       0,     0,     0,    19,     0,     0,     0,    20,     0,    21,
      22,    23,     0,     0,     0,     0,    24,     0,     0,     0,
       0,     0,     0,     4,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   401,   402,   403,    25,
       0,   406,   407,    26,    27,    28,     0,     5,    29,     6,
      30,     0,     0,     0,    31,     7,     0,     0,     0,     9,
      32,    33,     0,    34,     0,     0,     0,     0,    35,    36,
       0,     0,    12,     4,     0,     0,    13,    14,     0,    15,
       0,     0,    16,     0,    17,     0,     0,     0,     0,     0,
       0,    18,    37,     0,     0,     0,     0,     5,     0,     6,
       0,    20,     0,    21,    22,     7,     0,     0,     0,     9,
      24,   401,   402,   403,   404,   405,   406,   407,   408,   409,
       0,     0,    12,     0,     0,     0,    13,    14,     0,    15,
       0,     0,    16,    25,    17,     0,     0,    26,    27,    28,
       0,    18,    29,     0,    30,     0,     0,     0,    31,     0,
       0,    20,     0,    21,    22,    33,     0,    34,     0,     0,
      24,     0,   372,    36,   401,   402,   403,   404,   405,   406,
     407,   408,   409,     0,     0,     0,   414,   415,   416,   417,
     418,   419,   420,    25,     0,     0,    37,    26,    27,    28,
     421,   422,    29,     0,    30,     0,     0,     0,    31,     0,
       0,     0,     0,     0,     0,    33,     0,    34,     0,     0,
       0,     0,    35,    36,   401,   402,   403,   404,   405,   406,
     407,   408,   409,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    37,   401,   402,   403,
     404,   405,   406,   407,   408,   409,   410,     0,   411,   412,
     413,   414,   415,   416,   417,   418,   419,   420,     0,     0,
       0,     0,     0,     0,     0,   421,   422,     0,     0,     0,
     436,   401,   402,   403,   404,   405,   406,   407,   408,   409,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     401,   402,   403,   404,   405,   406,   407,     0,   409,   410,
       0,   411,   412,   413,   414,   415,   416,   417,   418,   419,
     420,     0,     0,     0,     0,     0,     0,     0,   421,   422,
       0,     0,     0,   444,   401,   402,   403,   404,   405,   406,
     407,   408,   409,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   401,   402,   403,   404,   405,   406,   407,   410,
       0,   411,   412,   413,   414,   415,   416,   417,   418,   419,
     420,   401,   402,   403,   404,   405,   406,   407,   421,   422,
       0,     0,   410,   480,   411,   412,   413,   414,   415,   416,
     417,   418,   419,   420,   401,   402,   403,   404,   405,   406,
     407,   421,   422,   525,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   410,   524,   411,   412,
     413,   414,   415,   416,   417,   418,   419,   420,   401,   402,
     403,   404,   405,   406,   407,   421,   422,   411,   412,   413,
     414,   415,   416,   417,   418,   419,   420,     0,     0,     0,
       0,     0,     0,     0,   421,   422,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   410,
       0,   411,   412,   413,   414,   415,   416,   417,   418,   419,
     420,     0,     0,     0,     0,     0,     0,     0,   421,   422,
     412,   413,   414,   415,   416,   417,   418,   419,   420,     0,
       0,     0,     0,     0,     0,     0,   421,   422,   411,   412,
     413,   414,   415,   416,   417,   418,   419,   420,     0,     0,
       0,     0,     0,     0,     0,   421,   422,     0,     0,     0,
       0,     4,     0,   413,   414,   415,   416,   417,   418,   419,
     420,     0,     0,     0,     0,     0,     0,     0,   421,   422,
       0,     0,     0,     0,     0,     5,     0,     6,     0,     0,
       0,     0,     0,     7,     8,     0,     0,     9,   414,   415,
     416,   417,   418,   419,   420,     0,     0,     0,    11,     0,
      12,     0,   421,   422,    13,    14,     0,    15,     0,     0,
      16,     0,    17,     0,     0,     0,     0,     0,     0,    18,
       0,     0,     0,     0,     0,     0,     0,     0,     4,    20,
       0,    21,    22,    23,     0,     0,   173,     0,    24,     0,
     174,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     5,     0,     6,     0,     0,     0,     0,     0,
       7,    25,     0,     0,     9,    26,    27,    28,     0,     0,
      29,     4,    30,     0,     0,     0,    31,    12,     0,     0,
       0,    13,    14,    33,    15,    34,     0,    16,     0,    17,
      35,    36,     0,     0,     0,     5,    18,     6,     0,     0,
       0,     0,     0,     7,     0,     0,    20,     0,    21,    22,
       0,     0,     0,     0,     0,    24,     0,     0,     0,     0,
      12,     0,     0,     0,    13,    14,     0,     0,     0,     0,
      16,     0,    17,     0,     0,     0,     0,     0,    25,    18,
       0,     0,    26,    27,    28,     0,     0,    29,     0,    30,
       0,    21,    22,    31,     0,     0,     0,     0,    24,     0,
      33,     0,    34,     0,     0,     0,     0,    35,    36,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    26,    27,    28,     0,     0,
      29,     0,     0,     0,     0,     0,    31,     0,     0,     0,
       0,     0,     0,    33,     0,    34,     0,     0,     0,     0,
      35,    36,    95,     0,    96,    97,     0,    98,    99,     0,
     100,     0,     0,   101,     0,   102,     0,     0,     0,     0,
       0,     0,   103,   104,   105,   106,     0,   107,   108,   109,
     110,   111,     0,   112,     0,   113,   114,   115,     0,     0,
     116,     0,     0,     0,     0,   117,     0,   118,   119,   120,
     121,   122,   123,     0,   124,   125,   126,   127,   128,     0,
       0,   129,     0,     0,   130,     0,     0,     0,   131,   132,
       0,   133,     0,     0,     0,   134,   135,   136,     0,   137,
     138,   139,   140,   141,     0,   142,     0,   143,   144,   145,
     146,   147,   148,   149,   150,     0,   151,   152,   153,     0,
       0,     0,   154,     0,     0,   155,     0,     0,   156,   157,
       0,     0,   158,   159,   160,     0,     0,     0,   161,     0,
     162,   163,   164,   165,     0,     0,   166
};

static const yytype_int16 yycheck[] =
{
       2,    45,     2,     2,     2,    32,   263,   275,     2,    66,
       2,   232,   206,   207,   235,    45,   237,   281,    34,   240,
     219,   251,   243,    92,   231,     2,   271,   226,    85,   223,
      46,    62,    63,    64,   130,    40,   130,    68,   167,    51,
     146,   170,     0,    14,   146,    45,    90,    45,    14,   294,
     295,    45,   170,   146,   109,    60,   146,     3,   113,     5,
       6,     7,     8,     9,   146,   171,    63,    64,    45,   171,
      84,    68,   146,   280,    86,   168,   172,    82,   172,   146,
      86,   171,   137,   277,    24,     3,     4,    27,     3,   171,
       5,     6,     7,     8,     9,   352,     8,   171,   167,   105,
     170,   167,   169,   167,     3,     4,   363,   173,    14,   114,
       8,   167,    10,   167,   172,    61,   167,   173,   348,   173,
     125,    28,   173,    31,   167,    33,   169,    34,     3,     4,
       3,     4,     3,     4,     3,     4,    35,     3,     4,     3,
       4,    40,     3,     4,     3,     4,    61,   169,   169,   169,
     169,    97,   169,   148,   172,     8,   171,   173,   188,   169,
     381,   169,   169,    62,     3,    12,    13,    14,   172,   171,
     172,   171,   172,   171,   172,   439,   122,   171,   172,   171,
     172,   380,    97,   214,   378,   156,   157,   158,   134,   246,
     247,   385,   172,   250,   462,   166,   167,   169,   172,   169,
     166,   167,   423,   172,   103,   151,   463,   122,   154,   155,
     156,   170,   169,   159,   160,   228,   170,   214,   231,   134,
     170,   442,   168,   170,   170,   238,   170,   126,   241,   170,
     172,   244,   245,   170,   170,   170,   151,   170,   282,   154,
     155,   156,   269,   170,   159,   160,   170,   170,   154,   155,
     156,   157,   158,   510,   170,   170,   313,   156,     3,     4,
     166,   167,   456,   172,   291,   170,   170,   280,   281,   170,
     170,   170,   529,   170,   276,   469,   276,   476,   276,   473,
     282,   170,   276,   170,   170,   170,   170,   170,   169,    36,
      35,   169,   169,   295,   307,   308,   309,   310,   311,   312,
     313,   171,   174,   170,     8,     8,     8,   154,   155,   156,
     157,   158,     8,   357,   535,     8,    65,    62,   512,   166,
     167,   365,     5,   167,   170,    72,   169,   148,   169,   146,
     172,   171,   359,   377,   146,   392,   173,    84,    85,   383,
     174,   171,   170,     3,    91,   171,   171,   171,   171,   171,
     146,   171,   171,   171,   353,   171,   171,   370,   103,   166,
     362,   171,   362,   171,   362,   365,   171,   365,   362,   171,
     171,   365,   119,   171,     8,   148,   123,   169,   173,    78,
     168,   126,   171,   383,   146,   171,   169,   169,   401,   402,
     171,   404,   405,   406,   407,   408,   409,   410,   411,   412,
     413,   414,   415,   416,   417,   418,   419,   420,     4,   422,
       3,   156,     3,     8,   171,   459,     5,   171,   169,   169,
     172,   148,   148,   171,   468,   170,   439,   171,   169,    43,
     361,     2,    28,   468,    30,   380,   541,     2,   244,    45,
      36,    37,     2,     2,    40,   277,   181,   473,   242,    45,
      -1,    -1,    -1,    -1,    -1,    51,    -1,    53,    -1,    -1,
      -1,    57,    58,    -1,    60,    -1,   468,    63,   481,    65,
      -1,    -1,    -1,    -1,    -1,    -1,    72,    -1,    -1,    -1,
      -1,    -1,    78,    79,    -1,    -1,    82,    -1,    84,    85,
      86,    -1,    -1,    -1,    -1,    91,    -1,    -1,    -1,    -1,
      -1,    -1,     4,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   524,   514,    -1,   514,    -1,   514,    -1,   114,    -1,
     514,    -1,   118,   119,   120,    -1,    28,   123,    30,   125,
      -1,    -1,    -1,   129,    36,    -1,    -1,    -1,    40,   135,
     136,    -1,   138,    45,    -1,    -1,    -1,   143,   144,    -1,
      -1,    53,    -1,    -1,    -1,    57,    58,    -1,    60,    -1,
      -1,    63,    -1,    65,    -1,    -1,    -1,    -1,    -1,    -1,
      72,   167,    -1,    -1,    -1,    -1,    78,   173,    -1,    -1,
      82,    -1,    84,    85,    -1,    -1,    -1,    -1,    -1,    91,
      -1,    -1,    -1,    -1,    -1,    -1,     4,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   114,    -1,    -1,    -1,   118,   119,   120,    -1,
      28,   123,    30,   125,    -1,    -1,    -1,   129,    36,    -1,
      -1,    -1,    40,   135,   136,    -1,   138,    45,    -1,    -1,
      -1,   143,   144,    -1,    -1,    53,    -1,    -1,    -1,    57,
      58,    -1,    60,    -1,    -1,    63,    -1,    65,    -1,    -1,
      -1,    -1,    -1,    -1,    72,   167,    -1,    -1,    -1,    -1,
      78,   173,    -1,    -1,    82,    -1,    84,    85,    -1,    -1,
      -1,    -1,    -1,    91,    -1,    -1,    -1,    -1,    -1,    -1,
       4,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   114,    -1,    -1,    -1,
     118,   119,   120,    -1,    28,   123,    30,   125,    -1,    -1,
      -1,   129,    36,    -1,    -1,    -1,    40,   135,   136,    -1,
     138,    45,    -1,    -1,    -1,   143,   144,    -1,    -1,    53,
      -1,    -1,    -1,    57,    58,    -1,    60,    -1,    -1,    63,
      -1,    65,    -1,    -1,    -1,    -1,    -1,    -1,    72,   167,
      -1,    -1,    -1,    -1,    78,   173,    -1,    -1,    82,    -1,
      84,    85,    -1,    -1,    -1,    -1,    -1,    91,    -1,    -1,
      -1,    -1,    -1,     3,     4,     5,     6,     7,     8,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     114,    -1,    -1,    -1,   118,   119,   120,    -1,    28,   123,
      30,   125,    -1,    -1,    -1,   129,    36,    -1,    -1,    -1,
      -1,   135,   136,    -1,   138,    -1,    -1,    -1,    -1,   143,
     144,    -1,    -1,    53,    -1,    -1,    -1,    57,    58,    -1,
      -1,    61,    -1,    63,    -1,    65,    -1,    -1,    -1,    -1,
      -1,    -1,    72,   167,    -1,    -1,    -1,    -1,    -1,   173,
      -1,    -1,    -1,    -1,    84,    85,    -1,    -1,    -1,    -1,
       3,    91,     5,     6,     7,     8,     9,    97,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,
     120,    -1,   122,   123,    -1,    -1,    -1,    -1,    -1,   129,
      -1,    -1,    -1,    -1,   134,    -1,   136,    -1,   138,    -1,
      -1,    -1,    -1,   143,   144,     4,    -1,    -1,    61,    -1,
      -1,   151,    11,    -1,   154,   155,   156,    -1,    -1,   159,
     160,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    28,
     170,    30,    -1,    -1,    -1,    -1,    -1,    36,    37,    -1,
      -1,    40,    -1,    -1,    97,    -1,    45,    -1,    -1,    -1,
      -1,    -1,    51,    -1,    53,    -1,    -1,    -1,    57,    58,
      -1,    60,    -1,    -1,    63,    -1,    65,    -1,    -1,   122,
      -1,    -1,    -1,    72,    -1,    -1,    -1,    -1,    -1,    78,
      79,   134,    -1,    82,    -1,    84,    85,    86,    -1,    -1,
      -1,    -1,    91,    -1,    -1,    -1,    -1,    -1,   151,     4,
      -1,   154,   155,   156,    -1,    -1,   159,   160,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   114,    -1,   170,    -1,   118,
     119,   120,    -1,    28,   123,    30,   125,    -1,    -1,    -1,
     129,    36,    37,    -1,    -1,    40,   135,   136,    -1,   138,
      45,    -1,    -1,    -1,   143,   144,    51,    -1,    53,    -1,
      -1,    -1,    57,    58,    -1,    60,    -1,    -1,    63,    -1,
      65,    -1,    -1,    -1,    -1,    -1,    -1,    72,   167,    -1,
      -1,    -1,    -1,    78,    -1,    -1,    -1,    82,    -1,    84,
      85,    86,    -1,    -1,    -1,    -1,    91,    -1,    -1,    -1,
      -1,    -1,    -1,     4,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    12,    13,    14,   114,
      -1,    17,    18,   118,   119,   120,    -1,    28,   123,    30,
     125,    -1,    -1,    -1,   129,    36,    -1,    -1,    -1,    40,
     135,   136,    -1,   138,    -1,    -1,    -1,    -1,   143,   144,
      -1,    -1,    53,     4,    -1,    -1,    57,    58,    -1,    60,
      -1,    -1,    63,    -1,    65,    -1,    -1,    -1,    -1,    -1,
      -1,    72,   167,    -1,    -1,    -1,    -1,    28,    -1,    30,
      -1,    82,    -1,    84,    85,    36,    -1,    -1,    -1,    40,
      91,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      -1,    -1,    53,    -1,    -1,    -1,    57,    58,    -1,    60,
      -1,    -1,    63,   114,    65,    -1,    -1,   118,   119,   120,
      -1,    72,   123,    -1,   125,    -1,    -1,    -1,   129,    -1,
      -1,    82,    -1,    84,    85,   136,    -1,   138,    -1,    -1,
      91,    -1,   143,   144,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    -1,    -1,    -1,   152,   153,   154,   155,
     156,   157,   158,   114,    -1,    -1,   167,   118,   119,   120,
     166,   167,   123,    -1,   125,    -1,    -1,    -1,   129,    -1,
      -1,    -1,    -1,    -1,    -1,   136,    -1,   138,    -1,    -1,
      -1,    -1,   143,   144,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   167,    12,    13,    14,
      15,    16,    17,    18,    19,    20,   147,    -1,   149,   150,
     151,   152,   153,   154,   155,   156,   157,   158,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   166,   167,    -1,    -1,    -1,
     171,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      12,    13,    14,    15,    16,    17,    18,    -1,    20,   147,
      -1,   149,   150,   151,   152,   153,   154,   155,   156,   157,
     158,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   166,   167,
      -1,    -1,    -1,   171,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    12,    13,    14,    15,    16,    17,    18,   147,
      -1,   149,   150,   151,   152,   153,   154,   155,   156,   157,
     158,    12,    13,    14,    15,    16,    17,    18,   166,   167,
      -1,    -1,   147,   171,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,    12,    13,    14,    15,    16,    17,
      18,   166,   167,   168,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,   158,    12,    13,
      14,    15,    16,    17,    18,   166,   167,   149,   150,   151,
     152,   153,   154,   155,   156,   157,   158,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   166,   167,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   147,
      -1,   149,   150,   151,   152,   153,   154,   155,   156,   157,
     158,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   166,   167,
     150,   151,   152,   153,   154,   155,   156,   157,   158,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   166,   167,   149,   150,
     151,   152,   153,   154,   155,   156,   157,   158,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   166,   167,    -1,    -1,    -1,
      -1,     4,    -1,   151,   152,   153,   154,   155,   156,   157,
     158,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   166,   167,
      -1,    -1,    -1,    -1,    -1,    28,    -1,    30,    -1,    -1,
      -1,    -1,    -1,    36,    37,    -1,    -1,    40,   152,   153,
     154,   155,   156,   157,   158,    -1,    -1,    -1,    51,    -1,
      53,    -1,   166,   167,    57,    58,    -1,    60,    -1,    -1,
      63,    -1,    65,    -1,    -1,    -1,    -1,    -1,    -1,    72,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     4,    82,
      -1,    84,    85,    86,    -1,    -1,    89,    -1,    91,    -1,
      93,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    28,    -1,    30,    -1,    -1,    -1,    -1,    -1,
      36,   114,    -1,    -1,    40,   118,   119,   120,    -1,    -1,
     123,     4,   125,    -1,    -1,    -1,   129,    53,    -1,    -1,
      -1,    57,    58,   136,    60,   138,    -1,    63,    -1,    65,
     143,   144,    -1,    -1,    -1,    28,    72,    30,    -1,    -1,
      -1,    -1,    -1,    36,    -1,    -1,    82,    -1,    84,    85,
      -1,    -1,    -1,    -1,    -1,    91,    -1,    -1,    -1,    -1,
      53,    -1,    -1,    -1,    57,    58,    -1,    -1,    -1,    -1,
      63,    -1,    65,    -1,    -1,    -1,    -1,    -1,   114,    72,
      -1,    -1,   118,   119,   120,    -1,    -1,   123,    -1,   125,
      -1,    84,    85,   129,    -1,    -1,    -1,    -1,    91,    -1,
     136,    -1,   138,    -1,    -1,    -1,    -1,   143,   144,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   118,   119,   120,    -1,    -1,
     123,    -1,    -1,    -1,    -1,    -1,   129,    -1,    -1,    -1,
      -1,    -1,    -1,   136,    -1,   138,    -1,    -1,    -1,    -1,
     143,   144,    21,    -1,    23,    24,    -1,    26,    27,    -1,
      29,    -1,    -1,    32,    -1,    34,    -1,    -1,    -1,    -1,
      -1,    -1,    41,    42,    43,    44,    -1,    46,    47,    48,
      49,    50,    -1,    52,    -1,    54,    55,    56,    -1,    -1,
      59,    -1,    -1,    -1,    -1,    64,    -1,    66,    67,    68,
      69,    70,    71,    -1,    73,    74,    75,    76,    77,    -1,
      -1,    80,    -1,    -1,    83,    -1,    -1,    -1,    87,    88,
      -1,    90,    -1,    -1,    -1,    94,    95,    96,    -1,    98,
      99,   100,   101,   102,    -1,   104,    -1,   106,   107,   108,
     109,   110,   111,   112,   113,    -1,   115,   116,   117,    -1,
      -1,    -1,   121,    -1,    -1,   124,    -1,    -1,   127,   128,
      -1,    -1,   131,   132,   133,    -1,    -1,    -1,   137,    -1,
     139,   140,   141,   142,    -1,    -1,   145
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   176,   177,     0,     4,    28,    30,    36,    37,    40,
      45,    51,    53,    57,    58,    60,    63,    65,    72,    78,
      82,    84,    85,    86,    91,   114,   118,   119,   120,   123,
     125,   129,   135,   136,   138,   143,   144,   167,   181,   182,
     183,   184,   185,   188,   189,   196,   207,   221,   225,   227,
     228,   229,   230,   233,   234,   237,   239,   240,   241,   242,
     244,   245,   246,   247,   248,   250,   252,   258,   259,   260,
     261,     3,     4,   170,     3,     4,     3,     4,   223,    84,
     226,     8,     3,     4,   226,   170,   226,   227,     3,   223,
     195,   196,     3,   223,   227,    21,    23,    24,    26,    27,
      29,    32,    34,    41,    42,    43,    44,    46,    47,    48,
      49,    50,    52,    54,    55,    56,    59,    64,    66,    67,
      68,    69,    70,    71,    73,    74,    75,    76,    77,    80,
      83,    87,    88,    90,    94,    95,    96,    98,    99,   100,
     101,   102,   104,   106,   107,   108,   109,   110,   111,   112,
     113,   115,   116,   117,   121,   124,   127,   128,   131,   132,
     133,   137,   139,   140,   141,   142,   145,   197,   199,   257,
     169,   178,   178,    89,    93,   187,   207,   228,   233,   239,
     243,   250,   258,   261,   169,   169,   172,   169,   172,   169,
     180,   169,   148,   238,   172,   251,   252,   251,   251,     3,
       4,    35,    62,   103,   126,   156,   170,   201,   224,   253,
     254,   256,   207,   258,   259,   261,   251,   169,     8,   172,
     169,   259,   172,   250,   130,   172,   170,   170,   170,   170,
     170,   170,   170,   170,   170,   170,   170,   170,   170,   170,
     170,   170,   170,   170,   170,   170,   170,   170,   170,   170,
     170,   146,   168,    11,    79,   181,   186,   189,   228,   230,
     241,   242,   245,   173,     3,     3,     4,   172,   256,   231,
     105,   235,   239,     3,     4,   172,   179,   249,   253,   253,
     174,   167,   170,   194,   251,   171,   204,   205,   206,   224,
     171,   214,   253,   255,   170,   217,   224,     3,     5,     6,
       7,     8,     9,    61,    97,   122,   134,   151,   154,   155,
     156,   159,   160,   170,   210,   211,   212,   210,   213,     8,
       8,   198,   213,   212,     8,     8,   212,     8,   212,   210,
      65,   212,   208,   209,   210,   257,   212,   208,   210,   259,
     259,     8,    10,   200,     5,   262,   259,   199,   167,   170,
     169,   180,   173,   195,   232,   148,    92,   195,   219,   236,
     169,   172,   179,   173,   181,   196,   248,   253,   171,   213,
     156,   209,   143,   190,   191,   192,   193,   196,   250,   173,
     146,   174,   173,   195,   215,   146,   219,   173,   196,   216,
     219,   171,   170,   210,   210,   210,   210,   210,   210,   210,
     259,    12,    13,    14,    15,    16,    17,    18,    19,    20,
     147,   149,   150,   151,   152,   153,   154,   155,   156,   157,
     158,   166,   167,   146,   171,   171,   171,   146,   171,   171,
     171,   171,   171,   171,   171,   171,   171,     3,   171,   146,
     171,   171,   146,   171,   171,   171,   171,   171,   166,   171,
     171,   199,     8,   180,   242,   148,   250,   169,   173,   195,
     220,   173,   185,   173,   180,   168,   168,   171,   146,   250,
     253,   206,   212,   250,   261,   253,   171,   169,   169,   259,
     171,   171,   210,   210,     3,   210,   210,   210,   210,   210,
     210,   210,   210,   210,   210,   210,   210,   210,   210,   210,
     210,   210,     3,   210,   212,     8,   171,   209,   212,     5,
     171,   253,   250,   169,   179,   180,   193,   253,   255,   169,
     222,   224,   171,   210,   148,   168,   171,   180,   253,   173,
     169,   172,   210,   180,   202,    34,    46,   173,   203,   212,
     148,   148,   169,   218,   219,   218,   169
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

/* Line 1455 of yacc.c  */
#line 326 "parser.y"
    { fix_incomplete();
						  check_statements((yyvsp[(1) - (1)].stmt_list), FALSE);
						  check_all_user_types((yyvsp[(1) - (1)].stmt_list));
						  write_header((yyvsp[(1) - (1)].stmt_list));
						  write_id_data((yyvsp[(1) - (1)].stmt_list));
						  write_proxies((yyvsp[(1) - (1)].stmt_list));
						  write_client((yyvsp[(1) - (1)].stmt_list));
						  write_server((yyvsp[(1) - (1)].stmt_list));
						  write_dlldata((yyvsp[(1) - (1)].stmt_list));
						  write_local_stubs((yyvsp[(1) - (1)].stmt_list));
						;}
    break;

  case 3:

/* Line 1455 of yacc.c  */
#line 339 "parser.y"
    { (yyval.stmt_list) = NULL; ;}
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 340 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_reference((yyvsp[(2) - (2)].type))); ;}
    break;

  case 5:

/* Line 1455 of yacc.c  */
#line 341 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_type_decl((yyvsp[(2) - (2)].type))); ;}
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 342 "parser.y"
    { (yyval.stmt_list) = (yyvsp[(1) - (3)].stmt_list);
						  reg_type((yyvsp[(2) - (3)].type), (yyvsp[(2) - (3)].type)->name, 0);
						;}
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 345 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_type_decl((yyvsp[(2) - (2)].type)));
						  reg_type((yyvsp[(2) - (2)].type), (yyvsp[(2) - (2)].type)->name, 0);
						;}
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 348 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_module((yyvsp[(2) - (2)].type))); ;}
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 349 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_library((yyvsp[(2) - (2)].typelib))); ;}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 350 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), (yyvsp[(2) - (2)].statement)); ;}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 353 "parser.y"
    { (yyval.stmt_list) = NULL; ;}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 354 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_reference((yyvsp[(2) - (2)].type))); ;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 355 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_type_decl((yyvsp[(2) - (2)].type))); ;}
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 356 "parser.y"
    { (yyval.stmt_list) = (yyvsp[(1) - (3)].stmt_list); reg_type((yyvsp[(2) - (3)].type), (yyvsp[(2) - (3)].type)->name, 0); ;}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 357 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_type_decl((yyvsp[(2) - (2)].type)));
						  reg_type((yyvsp[(2) - (2)].type), (yyvsp[(2) - (2)].type)->name, 0);
						;}
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 360 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_module((yyvsp[(2) - (2)].type))); ;}
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 361 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), (yyvsp[(2) - (2)].statement)); ;}
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 362 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_importlib((yyvsp[(2) - (2)].str))); ;}
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 363 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_library((yyvsp[(2) - (2)].typelib))); ;}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 366 "parser.y"
    { (yyval.stmt_list) = NULL; ;}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 367 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), (yyvsp[(2) - (2)].statement)); ;}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 375 "parser.y"
    { (yyval.statement) = make_statement_cppquote((yyvsp[(1) - (1)].str)); ;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 376 "parser.y"
    { (yyval.statement) = make_statement_type_decl((yyvsp[(1) - (2)].type)); ;}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 377 "parser.y"
    { (yyval.statement) = make_statement_declaration((yyvsp[(1) - (2)].var)); ;}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 378 "parser.y"
    { (yyval.statement) = make_statement_import((yyvsp[(1) - (1)].str)); ;}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 379 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (2)].statement); ;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 386 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type); (yyval.type)->attrs = check_enum_attrs((yyvsp[(1) - (2)].attr_list)); ;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 387 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type); (yyval.type)->attrs = check_struct_attrs((yyvsp[(1) - (2)].attr_list)); ;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 388 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type); (yyval.type)->attrs = check_union_attrs((yyvsp[(1) - (2)].attr_list)); ;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 391 "parser.y"
    { (yyval.str) = (yyvsp[(3) - (4)].str); ;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 393 "parser.y"
    { assert(yychar == YYEMPTY);
						  (yyval.import) = xmalloc(sizeof(struct _import_t));
						  (yyval.import)->name = (yyvsp[(2) - (3)].str);
						  (yyval.import)->import_performed = do_import((yyvsp[(2) - (3)].str));
						  if (!(yyval.import)->import_performed) yychar = aEOF;
						;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 401 "parser.y"
    { (yyval.str) = (yyvsp[(1) - (3)].import)->name;
						  if ((yyvsp[(1) - (3)].import)->import_performed) pop_import();
						  free((yyvsp[(1) - (3)].import));
						;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 408 "parser.y"
    { (yyval.str) = (yyvsp[(3) - (5)].str); if(!parse_only) add_importlib((yyvsp[(3) - (5)].str)); ;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 411 "parser.y"
    { (yyval.str) = (yyvsp[(2) - (2)].str); ;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 413 "parser.y"
    { (yyval.typelib) = make_library((yyvsp[(2) - (3)].str), check_library_attrs((yyvsp[(2) - (3)].str), (yyvsp[(1) - (3)].attr_list)));
						  if (!parse_only) start_typelib((yyval.typelib));
						;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 418 "parser.y"
    { (yyval.typelib) = (yyvsp[(1) - (4)].typelib);
						  (yyval.typelib)->stmts = (yyvsp[(2) - (4)].stmt_list);
						  if (!parse_only) end_typelib();
						;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 424 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 428 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 431 "parser.y"
    { check_arg((yyvsp[(1) - (1)].var)); (yyval.var_list) = append_var( NULL, (yyvsp[(1) - (1)].var) ); ;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 432 "parser.y"
    { check_arg((yyvsp[(3) - (3)].var)); (yyval.var_list) = append_var( (yyvsp[(1) - (3)].var_list), (yyvsp[(3) - (3)].var)); ;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 437 "parser.y"
    { (yyval.var) = (yyvsp[(3) - (3)].declarator)->var;
						  (yyval.var)->attrs = (yyvsp[(1) - (3)].attr_list);
						  if ((yyvsp[(2) - (3)].declspec)->stgclass != STG_NONE && (yyvsp[(2) - (3)].declspec)->stgclass != STG_REGISTER)
						    error_loc("invalid storage class for function parameter\n");
						  set_type((yyval.var), (yyvsp[(2) - (3)].declspec), (yyvsp[(3) - (3)].declarator), TRUE);
						  free((yyvsp[(3) - (3)].declarator));
						;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 444 "parser.y"
    { (yyval.var) = (yyvsp[(2) - (2)].declarator)->var;
						  if ((yyvsp[(1) - (2)].declspec)->stgclass != STG_NONE && (yyvsp[(1) - (2)].declspec)->stgclass != STG_REGISTER)
						    error_loc("invalid storage class for function parameter\n");
						  set_type((yyval.var), (yyvsp[(1) - (2)].declspec), (yyvsp[(2) - (2)].declarator), TRUE);
						  free((yyvsp[(2) - (2)].declarator));
						;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 452 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (3)].expr); ;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 453 "parser.y"
    { (yyval.expr) = make_expr(EXPR_VOID); ;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 456 "parser.y"
    { (yyval.attr_list) = NULL; ;}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 461 "parser.y"
    { (yyval.attr_list) = (yyvsp[(2) - (3)].attr_list); ;}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 464 "parser.y"
    { (yyval.attr_list) = append_attr( NULL, (yyvsp[(1) - (1)].attr) ); ;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 465 "parser.y"
    { (yyval.attr_list) = append_attr( (yyvsp[(1) - (3)].attr_list), (yyvsp[(3) - (3)].attr) ); ;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 466 "parser.y"
    { (yyval.attr_list) = append_attr( (yyvsp[(1) - (4)].attr_list), (yyvsp[(4) - (4)].attr) ); ;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 469 "parser.y"
    { (yyval.str_list) = append_str( NULL, (yyvsp[(1) - (1)].str) ); ;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 470 "parser.y"
    { (yyval.str_list) = append_str( (yyvsp[(1) - (3)].str_list), (yyvsp[(3) - (3)].str) ); ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 473 "parser.y"
    { (yyval.attr) = NULL; ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 474 "parser.y"
    { (yyval.attr) = make_attr(ATTR_AGGREGATABLE); ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 475 "parser.y"
    { (yyval.attr) = make_attr(ATTR_APPOBJECT); ;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 476 "parser.y"
    { (yyval.attr) = make_attr(ATTR_ASYNC); ;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 477 "parser.y"
    { (yyval.attr) = make_attr(ATTR_AUTO_HANDLE); ;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 478 "parser.y"
    { (yyval.attr) = make_attr(ATTR_BINDABLE); ;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 479 "parser.y"
    { (yyval.attr) = make_attr(ATTR_BROADCAST); ;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 480 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_CALLAS, (yyvsp[(3) - (4)].var)); ;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 481 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_CASE, (yyvsp[(3) - (4)].expr_list)); ;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 482 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); ;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 483 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_DONT_SERIALIZE */ ;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 484 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_SERIALIZE */ ;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 485 "parser.y"
    { (yyval.attr) = make_attr(ATTR_CONTROL); ;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 486 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULT); ;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 487 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULTCOLLELEM); ;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 488 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_DEFAULTVALUE, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 489 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULTVTABLE); ;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 490 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DISPLAYBIND); ;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 491 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_DLLNAME, (yyvsp[(3) - (4)].str)); ;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 492 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DUAL); ;}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 493 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ENDPOINT, (yyvsp[(3) - (4)].str_list)); ;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 494 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ENTRY, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 495 "parser.y"
    { (yyval.attr) = make_attr(ATTR_EXPLICIT_HANDLE); ;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 496 "parser.y"
    { (yyval.attr) = make_attr(ATTR_HANDLE); ;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 497 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPCONTEXT, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 498 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPFILE, (yyvsp[(3) - (4)].str)); ;}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 499 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRING, (yyvsp[(3) - (4)].str)); ;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 500 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRINGCONTEXT, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 501 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRINGDLL, (yyvsp[(3) - (4)].str)); ;}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 502 "parser.y"
    { (yyval.attr) = make_attr(ATTR_HIDDEN); ;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 503 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ID, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 504 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IDEMPOTENT); ;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 505 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_IIDIS, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 506 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IMMEDIATEBIND); ;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 507 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_IMPLICIT_HANDLE, (yyvsp[(4) - (5)].str)); ;}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 508 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IN); ;}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 509 "parser.y"
    { (yyval.attr) = make_attr(ATTR_INPUTSYNC); ;}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 510 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_LENGTHIS, (yyvsp[(3) - (4)].expr_list)); ;}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 511 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_LIBLCID, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 512 "parser.y"
    { (yyval.attr) = make_attr(ATTR_LOCAL); ;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 513 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONBROWSABLE); ;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 514 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONCREATABLE); ;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 515 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONEXTENSIBLE); ;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 516 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OBJECT); ;}
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 517 "parser.y"
    { (yyval.attr) = make_attr(ATTR_ODL); ;}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 518 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OLEAUTOMATION); ;}
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 519 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OPTIONAL); ;}
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 520 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OUT); ;}
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 521 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_POINTERDEFAULT, (yyvsp[(3) - (4)].num)); ;}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 522 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPGET); ;}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 523 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPPUT); ;}
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 524 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPPUTREF); ;}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 525 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PUBLIC); ;}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 527 "parser.y"
    { expr_list_t *list = append_expr( NULL, (yyvsp[(3) - (6)].expr) );
						  list = append_expr( list, (yyvsp[(5) - (6)].expr) );
						  (yyval.attr) = make_attrp(ATTR_RANGE, list); ;}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 530 "parser.y"
    { (yyval.attr) = make_attr(ATTR_READONLY); ;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 531 "parser.y"
    { (yyval.attr) = make_attr(ATTR_REQUESTEDIT); ;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 532 "parser.y"
    { (yyval.attr) = make_attr(ATTR_RESTRICTED); ;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 533 "parser.y"
    { (yyval.attr) = make_attr(ATTR_RETVAL); ;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 534 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SIZEIS, (yyvsp[(3) - (4)].expr_list)); ;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 535 "parser.y"
    { (yyval.attr) = make_attr(ATTR_SOURCE); ;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 536 "parser.y"
    { (yyval.attr) = make_attr(ATTR_STRICTCONTEXTHANDLE); ;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 537 "parser.y"
    { (yyval.attr) = make_attr(ATTR_STRING); ;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 538 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SWITCHIS, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 539 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SWITCHTYPE, (yyvsp[(3) - (4)].type)); ;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 540 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_TRANSMITAS, (yyvsp[(3) - (4)].type)); ;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 541 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_UUID, (yyvsp[(3) - (4)].uuid)); ;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 542 "parser.y"
    { (yyval.attr) = make_attr(ATTR_V1ENUM); ;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 543 "parser.y"
    { (yyval.attr) = make_attr(ATTR_VARARG); ;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 544 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_VERSION, (yyvsp[(3) - (4)].num)); ;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 545 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_WIREMARSHAL, (yyvsp[(3) - (4)].type)); ;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 546 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_POINTERTYPE, (yyvsp[(1) - (1)].num)); ;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 551 "parser.y"
    { if (!is_valid_uuid((yyvsp[(1) - (1)].str)))
						    error_loc("invalid UUID: %s\n", (yyvsp[(1) - (1)].str));
						  (yyval.uuid) = parse_uuid((yyvsp[(1) - (1)].str)); ;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 556 "parser.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); ;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 557 "parser.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); ;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 558 "parser.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); ;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 559 "parser.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); ;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 562 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 563 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[(1) - (2)].var_list), (yyvsp[(2) - (2)].var) ); ;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 566 "parser.y"
    { attr_t *a = make_attrp(ATTR_CASE, append_expr( NULL, (yyvsp[(2) - (4)].expr) ));
						  (yyval.var) = (yyvsp[(4) - (4)].var); if (!(yyval.var)) (yyval.var) = make_var(NULL);
						  (yyval.var)->attrs = append_attr( (yyval.var)->attrs, a );
						;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 570 "parser.y"
    { attr_t *a = make_attr(ATTR_DEFAULT);
						  (yyval.var) = (yyvsp[(3) - (3)].var); if (!(yyval.var)) (yyval.var) = make_var(NULL);
						  (yyval.var)->attrs = append_attr( (yyval.var)->attrs, a );
						;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 576 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 577 "parser.y"
    { (yyval.var_list) = (yyvsp[(1) - (2)].var_list); ;}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 581 "parser.y"
    { if (!(yyvsp[(1) - (1)].var)->eval)
						    (yyvsp[(1) - (1)].var)->eval = make_exprl(EXPR_NUM, 0 /* default for first enum entry */);
                                                  (yyval.var_list) = append_var( NULL, (yyvsp[(1) - (1)].var) );
						;}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 585 "parser.y"
    { if (!(yyvsp[(3) - (3)].var)->eval)
                                                  {
                                                    var_t *last = LIST_ENTRY( list_tail((yyval.var_list)), var_t, entry );
                                                    (yyvsp[(3) - (3)].var)->eval = make_exprl(EXPR_NUM, last->eval->cval + 1);
                                                  }
                                                  (yyval.var_list) = append_var( (yyvsp[(1) - (3)].var_list), (yyvsp[(3) - (3)].var) );
						;}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 594 "parser.y"
    { (yyval.var) = reg_const((yyvsp[(1) - (3)].var));
						  (yyval.var)->eval = (yyvsp[(3) - (3)].expr);
                                                  (yyval.var)->type = type_new_int(TYPE_BASIC_INT, 0);
						;}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 598 "parser.y"
    { (yyval.var) = reg_const((yyvsp[(1) - (1)].var));
                                                  (yyval.var)->type = type_new_int(TYPE_BASIC_INT, 0);
						;}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 603 "parser.y"
    { (yyval.type) = type_new_enum((yyvsp[(2) - (5)].str), TRUE, (yyvsp[(4) - (5)].var_list)); ;}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 606 "parser.y"
    { (yyval.expr_list) = append_expr( NULL, (yyvsp[(1) - (1)].expr) ); ;}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 607 "parser.y"
    { (yyval.expr_list) = append_expr( (yyvsp[(1) - (3)].expr_list), (yyvsp[(3) - (3)].expr) ); ;}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 620 "parser.y"
    { (yyval.expr) = make_expr(EXPR_VOID); ;}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 624 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_NUM, (yyvsp[(1) - (1)].num)); ;}
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 625 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_HEXNUM, (yyvsp[(1) - (1)].num)); ;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 626 "parser.y"
    { (yyval.expr) = make_exprd(EXPR_DOUBLE, (yyvsp[(1) - (1)].dbl)); ;}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 627 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_TRUEFALSE, 0); ;}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 628 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_NUM, 0); ;}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 629 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_TRUEFALSE, 1); ;}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 630 "parser.y"
    { (yyval.expr) = make_exprs(EXPR_STRLIT, (yyvsp[(1) - (1)].str)); ;}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 631 "parser.y"
    { (yyval.expr) = make_exprs(EXPR_WSTRLIT, (yyvsp[(1) - (1)].str)); ;}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 632 "parser.y"
    { (yyval.expr) = make_exprs(EXPR_IDENTIFIER, (yyvsp[(1) - (1)].str)); ;}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 633 "parser.y"
    { (yyval.expr) = make_expr3(EXPR_COND, (yyvsp[(1) - (5)].expr), (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].expr)); ;}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 634 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_LOGOR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 635 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_LOGAND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 636 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_OR , (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 637 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_XOR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 638 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_AND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 639 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_EQUALITY, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 640 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_INEQUALITY, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 641 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_GTR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 642 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_LESS, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 643 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_GTREQL, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 644 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_LESSEQL, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 645 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SHL, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 646 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SHR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 647 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_ADD, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 648 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SUB, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 649 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MOD, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 650 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MUL, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 651 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_DIV, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 652 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_LOGNOT, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 653 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_NOT, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 654 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_POS, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 655 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_NEG, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 656 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_ADDRESSOF, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 657 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_PPTR, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 658 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MEMBER, make_expr1(EXPR_PPTR, (yyvsp[(1) - (3)].expr)), make_exprs(EXPR_IDENTIFIER, (yyvsp[(3) - (3)].str))); ;}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 659 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MEMBER, (yyvsp[(1) - (3)].expr), make_exprs(EXPR_IDENTIFIER, (yyvsp[(3) - (3)].str))); ;}
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 660 "parser.y"
    { (yyval.expr) = make_exprt(EXPR_CAST, (yyvsp[(2) - (4)].type), (yyvsp[(4) - (4)].expr)); ;}
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 661 "parser.y"
    { (yyval.expr) = make_exprt(EXPR_SIZEOF, (yyvsp[(3) - (4)].type), NULL); ;}
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 662 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_ARRAY, (yyvsp[(1) - (4)].expr), (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 663 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (3)].expr); ;}
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 666 "parser.y"
    { (yyval.expr_list) = append_expr( NULL, (yyvsp[(1) - (1)].expr) ); ;}
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 667 "parser.y"
    { (yyval.expr_list) = append_expr( (yyvsp[(1) - (3)].expr_list), (yyvsp[(3) - (3)].expr) ); ;}
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 670 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr);
						  if (!(yyval.expr)->is_const)
						      error_loc("expression is not an integer constant\n");
						;}
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 676 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr);
						  if (!(yyval.expr)->is_const && (yyval.expr)->type != EXPR_STRLIT && (yyval.expr)->type != EXPR_WSTRLIT)
						      error_loc("expression is not constant\n");
						;}
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 682 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 683 "parser.y"
    { (yyval.var_list) = append_var_list((yyvsp[(1) - (2)].var_list), (yyvsp[(2) - (2)].var_list)); ;}
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 687 "parser.y"
    { const char *first = LIST_ENTRY(list_head((yyvsp[(3) - (4)].declarator_list)), declarator_t, entry)->var->name;
						  check_field_attrs(first, (yyvsp[(1) - (4)].attr_list));
						  (yyval.var_list) = set_var_types((yyvsp[(1) - (4)].attr_list), (yyvsp[(2) - (4)].declspec), (yyvsp[(3) - (4)].declarator_list));
						;}
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 691 "parser.y"
    { var_t *v = make_var(NULL);
						  v->type = (yyvsp[(2) - (3)].type); v->attrs = (yyvsp[(1) - (3)].attr_list);
						  (yyval.var_list) = append_var(NULL, v);
						;}
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 698 "parser.y"
    { (yyval.var) = (yyvsp[(1) - (2)].var); ;}
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 699 "parser.y"
    { (yyval.var) = make_var(NULL); (yyval.var)->attrs = (yyvsp[(1) - (2)].attr_list); ;}
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 702 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 703 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[(1) - (2)].var_list), (yyvsp[(2) - (2)].var) ); ;}
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 707 "parser.y"
    { (yyval.var) = (yyvsp[(1) - (2)].var); ;}
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 708 "parser.y"
    { (yyval.var) = NULL; ;}
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 711 "parser.y"
    { (yyval.var) = (yyvsp[(3) - (3)].declarator)->var;
						  (yyval.var)->attrs = check_field_attrs((yyval.var)->name, (yyvsp[(1) - (3)].attr_list));
						  set_type((yyval.var), (yyvsp[(2) - (3)].declspec), (yyvsp[(3) - (3)].declarator), FALSE);
						  free((yyvsp[(3) - (3)].declarator));
						;}
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 719 "parser.y"
    { var_t *v = (yyvsp[(3) - (3)].declarator)->var;
						  v->attrs = check_function_attrs(v->name, (yyvsp[(1) - (3)].attr_list));
						  set_type(v, (yyvsp[(2) - (3)].declspec), (yyvsp[(3) - (3)].declarator), FALSE);
						  free((yyvsp[(3) - (3)].declarator));
						  (yyval.func) = make_func(v);
						;}
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 729 "parser.y"
    { (yyval.var) = (yyvsp[(3) - (3)].declarator)->var;
						  (yyval.var)->attrs = (yyvsp[(1) - (3)].attr_list);
						  set_type((yyval.var), (yyvsp[(2) - (3)].declspec), (yyvsp[(3) - (3)].declarator), FALSE);
						  free((yyvsp[(3) - (3)].declarator));
						;}
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 734 "parser.y"
    { (yyval.var) = (yyvsp[(2) - (2)].declarator)->var;
						  set_type((yyval.var), (yyvsp[(1) - (2)].declspec), (yyvsp[(2) - (2)].declarator), FALSE);
						  free((yyvsp[(2) - (2)].declarator));
						;}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 740 "parser.y"
    { (yyval.var) = NULL; ;}
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 744 "parser.y"
    { (yyval.str) = NULL; ;}
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 745 "parser.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); ;}
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 746 "parser.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); ;}
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 749 "parser.y"
    { (yyval.var) = make_var((yyvsp[(1) - (1)].str)); ;}
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 751 "parser.y"
    { (yyval.var) = make_var((yyvsp[(1) - (1)].str)); ;}
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 754 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 755 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 757 "parser.y"
    { (yyval.type) = type_new_int(type_basic_get_type((yyvsp[(2) - (2)].type)), -1); ;}
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 758 "parser.y"
    { (yyval.type) = type_new_int(type_basic_get_type((yyvsp[(2) - (2)].type)), 1); ;}
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 759 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT, 1); ;}
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 760 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 761 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 762 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 763 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 764 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 771 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT, 0); ;}
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 772 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT16, 0); ;}
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 773 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT8, 0); ;}
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 774 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT32, 0); ;}
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 775 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_HYPER, 0); ;}
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 776 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT64, 0); ;}
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 777 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_CHAR, 0); ;}
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 780 "parser.y"
    { (yyval.type) = type_new_coclass((yyvsp[(2) - (2)].str)); ;}
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 781 "parser.y"
    { (yyval.type) = find_type((yyvsp[(2) - (2)].str), 0);
						  if (type_get_type_detect_alias((yyval.type)) != TYPE_COCLASS)
						    error_loc("%s was not declared a coclass at %s:%d\n",
							      (yyvsp[(2) - (2)].str), (yyval.type)->loc_info.input_name,
							      (yyval.type)->loc_info.line_number);
						;}
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 789 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type);
						  check_def((yyval.type));
						  (yyval.type)->attrs = check_coclass_attrs((yyvsp[(2) - (2)].type)->name, (yyvsp[(1) - (2)].attr_list));
						;}
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 796 "parser.y"
    { (yyval.type) = type_coclass_define((yyvsp[(1) - (5)].type), (yyvsp[(3) - (5)].ifref_list)); ;}
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 799 "parser.y"
    { (yyval.ifref_list) = NULL; ;}
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 800 "parser.y"
    { (yyval.ifref_list) = append_ifref( (yyvsp[(1) - (2)].ifref_list), (yyvsp[(2) - (2)].ifref) ); ;}
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 804 "parser.y"
    { (yyval.ifref) = make_ifref((yyvsp[(2) - (2)].type)); (yyval.ifref)->attrs = (yyvsp[(1) - (2)].attr_list); ;}
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 807 "parser.y"
    { (yyval.type) = get_type(TYPE_INTERFACE, (yyvsp[(2) - (2)].str), 0); ;}
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 808 "parser.y"
    { (yyval.type) = get_type(TYPE_INTERFACE, (yyvsp[(2) - (2)].str), 0); ;}
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 811 "parser.y"
    { attr_t *attrs;
						  is_object_interface = TRUE;
						  (yyval.type) = (yyvsp[(2) - (2)].type);
						  check_def((yyval.type));
						  attrs = make_attr(ATTR_DISPINTERFACE);
						  (yyval.type)->attrs = append_attr( check_dispiface_attrs((yyvsp[(2) - (2)].type)->name, (yyvsp[(1) - (2)].attr_list)), attrs );
						  (yyval.type)->defined = TRUE;
						;}
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 821 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 822 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[(1) - (3)].var_list), (yyvsp[(2) - (3)].var) ); ;}
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 825 "parser.y"
    { (yyval.stmt_list) = NULL; ;}
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 826 "parser.y"
    { (yyval.stmt_list) = append_func( (yyvsp[(1) - (3)].stmt_list), (yyvsp[(2) - (3)].func) ); ;}
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 832 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (5)].type);
						  type_dispinterface_define((yyval.type), (yyvsp[(3) - (5)].var_list), (yyvsp[(4) - (5)].stmt_list));
						;}
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 836 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (5)].type);
						  type_dispinterface_define_from_iface((yyval.type), (yyvsp[(3) - (5)].type));
						;}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 841 "parser.y"
    { (yyval.type) = NULL; ;}
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 842 "parser.y"
    { (yyval.type) = find_type_or_error2((yyvsp[(2) - (2)].str), 0); ;}
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 845 "parser.y"
    { (yyval.type) = get_type(TYPE_INTERFACE, (yyvsp[(2) - (2)].str), 0); ;}
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 846 "parser.y"
    { (yyval.type) = get_type(TYPE_INTERFACE, (yyvsp[(2) - (2)].str), 0); ;}
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 849 "parser.y"
    { (yyval.ifinfo).interface = (yyvsp[(2) - (2)].type);
						  (yyval.ifinfo).old_pointer_default = pointer_default;
						  if (is_attr((yyvsp[(1) - (2)].attr_list), ATTR_POINTERDEFAULT))
						    pointer_default = get_attrv((yyvsp[(1) - (2)].attr_list), ATTR_POINTERDEFAULT);
						  is_object_interface = is_object((yyvsp[(1) - (2)].attr_list));
						  check_def((yyvsp[(2) - (2)].type));
						  (yyvsp[(2) - (2)].type)->attrs = check_iface_attrs((yyvsp[(2) - (2)].type)->name, (yyvsp[(1) - (2)].attr_list));
						  (yyvsp[(2) - (2)].type)->defined = TRUE;
						;}
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 861 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (6)].ifinfo).interface;
						  type_interface_define((yyval.type), (yyvsp[(2) - (6)].type), (yyvsp[(4) - (6)].stmt_list));
						  pointer_default = (yyvsp[(1) - (6)].ifinfo).old_pointer_default;
						;}
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 869 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (8)].ifinfo).interface;
						  type_interface_define((yyval.type), find_type_or_error2((yyvsp[(3) - (8)].str), 0), (yyvsp[(6) - (8)].stmt_list));
						  pointer_default = (yyvsp[(1) - (8)].ifinfo).old_pointer_default;
						;}
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 873 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (2)].type); ;}
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 877 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (2)].type); ;}
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 878 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (2)].type); ;}
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 881 "parser.y"
    { (yyval.type) = type_new_module((yyvsp[(2) - (2)].str)); ;}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 882 "parser.y"
    { (yyval.type) = type_new_module((yyvsp[(2) - (2)].str)); ;}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 885 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type);
						  (yyval.type)->attrs = check_module_attrs((yyvsp[(2) - (2)].type)->name, (yyvsp[(1) - (2)].attr_list));
						;}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 891 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (5)].type);
                                                  type_module_define((yyval.type), (yyvsp[(3) - (5)].stmt_list));
						;}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 897 "parser.y"
    { (yyval.stgclass) = STG_EXTERN; ;}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 898 "parser.y"
    { (yyval.stgclass) = STG_STATIC; ;}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 899 "parser.y"
    { (yyval.stgclass) = STG_REGISTER; ;}
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 903 "parser.y"
    { (yyval.attr) = make_attr(ATTR_INLINE); ;}
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 907 "parser.y"
    { (yyval.attr) = make_attr(ATTR_CONST); ;}
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 910 "parser.y"
    { (yyval.attr_list) = NULL; ;}
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 911 "parser.y"
    { (yyval.attr_list) = append_attr((yyvsp[(1) - (2)].attr_list), (yyvsp[(2) - (2)].attr)); ;}
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 914 "parser.y"
    { (yyval.declspec) = make_decl_spec((yyvsp[(1) - (2)].type), (yyvsp[(2) - (2)].declspec), NULL, NULL, STG_NONE); ;}
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 916 "parser.y"
    { (yyval.declspec) = make_decl_spec((yyvsp[(2) - (3)].type), (yyvsp[(1) - (3)].declspec), (yyvsp[(3) - (3)].declspec), NULL, STG_NONE); ;}
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 919 "parser.y"
    { (yyval.declspec) = NULL; ;}
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 924 "parser.y"
    { (yyval.declspec) = make_decl_spec(NULL, (yyvsp[(2) - (2)].declspec), NULL, (yyvsp[(1) - (2)].attr), STG_NONE); ;}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 925 "parser.y"
    { (yyval.declspec) = make_decl_spec(NULL, (yyvsp[(2) - (2)].declspec), NULL, (yyvsp[(1) - (2)].attr), STG_NONE); ;}
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 926 "parser.y"
    { (yyval.declspec) = make_decl_spec(NULL, (yyvsp[(2) - (2)].declspec), NULL, NULL, (yyvsp[(1) - (2)].stgclass)); ;}
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 931 "parser.y"
    { (yyval.declarator) = (yyvsp[(3) - (3)].declarator); (yyval.declarator)->type = append_ptrchain_type((yyval.declarator)->type, type_new_pointer(pointer_default, NULL, (yyvsp[(2) - (3)].attr_list))); ;}
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 932 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (2)].declarator); (yyval.declarator)->type->attrs = append_attr((yyval.declarator)->type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[(1) - (2)].str))); ;}
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 937 "parser.y"
    { (yyval.declarator) = make_declarator((yyvsp[(1) - (1)].var)); ;}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 938 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (3)].declarator); ;}
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 939 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (2)].declarator); (yyval.declarator)->array = append_array((yyval.declarator)->array, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 940 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (4)].declarator);
						  (yyval.declarator)->func_type = append_ptrchain_type((yyval.declarator)->type, type_new_function((yyvsp[(3) - (4)].var_list)));
						  (yyval.declarator)->type = NULL;
						;}
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 947 "parser.y"
    { (yyval.declarator_list) = append_declarator( NULL, (yyvsp[(1) - (1)].declarator) ); ;}
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 948 "parser.y"
    { (yyval.declarator_list) = append_declarator( (yyvsp[(1) - (3)].declarator_list), (yyvsp[(3) - (3)].declarator) ); ;}
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 952 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (1)].declarator); ;}
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 953 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (3)].declarator); (yyvsp[(1) - (3)].declarator)->var->eval = (yyvsp[(3) - (3)].expr); ;}
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 957 "parser.y"
    { (yyval.num) = RPC_FC_RP; ;}
    break;

  case 294:

/* Line 1455 of yacc.c  */
#line 958 "parser.y"
    { (yyval.num) = RPC_FC_UP; ;}
    break;

  case 295:

/* Line 1455 of yacc.c  */
#line 959 "parser.y"
    { (yyval.num) = RPC_FC_FP; ;}
    break;

  case 296:

/* Line 1455 of yacc.c  */
#line 962 "parser.y"
    { (yyval.type) = type_new_struct((yyvsp[(2) - (5)].str), TRUE, (yyvsp[(4) - (5)].var_list)); ;}
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 965 "parser.y"
    { (yyval.type) = type_new_void(); ;}
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 966 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 967 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (1)].type); ;}
    break;

  case 300:

/* Line 1455 of yacc.c  */
#line 968 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (1)].type); ;}
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 969 "parser.y"
    { (yyval.type) = type_new_enum((yyvsp[(2) - (2)].str), FALSE, NULL); ;}
    break;

  case 302:

/* Line 1455 of yacc.c  */
#line 970 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (1)].type); ;}
    break;

  case 303:

/* Line 1455 of yacc.c  */
#line 971 "parser.y"
    { (yyval.type) = type_new_struct((yyvsp[(2) - (2)].str), FALSE, NULL); ;}
    break;

  case 304:

/* Line 1455 of yacc.c  */
#line 972 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (1)].type); ;}
    break;

  case 305:

/* Line 1455 of yacc.c  */
#line 973 "parser.y"
    { (yyval.type) = type_new_nonencapsulated_union((yyvsp[(2) - (2)].str), FALSE, NULL); ;}
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 974 "parser.y"
    { (yyval.type) = make_safearray((yyvsp[(3) - (4)].type)); ;}
    break;

  case 307:

/* Line 1455 of yacc.c  */
#line 978 "parser.y"
    { reg_typedefs((yyvsp[(3) - (4)].declspec), (yyvsp[(4) - (4)].declarator_list), check_typedef_attrs((yyvsp[(2) - (4)].attr_list)));
						  (yyval.statement) = make_statement_typedef((yyvsp[(4) - (4)].declarator_list));
						;}
    break;

  case 308:

/* Line 1455 of yacc.c  */
#line 984 "parser.y"
    { (yyval.type) = type_new_nonencapsulated_union((yyvsp[(2) - (5)].str), TRUE, (yyvsp[(4) - (5)].var_list)); ;}
    break;

  case 309:

/* Line 1455 of yacc.c  */
#line 987 "parser.y"
    { (yyval.type) = type_new_encapsulated_union((yyvsp[(2) - (10)].str), (yyvsp[(5) - (10)].var), (yyvsp[(7) - (10)].var), (yyvsp[(9) - (10)].var_list)); ;}
    break;

  case 310:

/* Line 1455 of yacc.c  */
#line 991 "parser.y"
    { (yyval.num) = MAKEVERSION((yyvsp[(1) - (1)].num), 0); ;}
    break;

  case 311:

/* Line 1455 of yacc.c  */
#line 992 "parser.y"
    { (yyval.num) = MAKEVERSION((yyvsp[(1) - (3)].num), (yyvsp[(3) - (3)].num)); ;}
    break;



/* Line 1455 of yacc.c  */
#line 4702 "parser.tab.c"
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



/* Line 1675 of yacc.c  */
#line 995 "parser.y"


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
  for (ptrchain_type = ptrchain; type_pointer_get_ref(ptrchain_type); ptrchain_type = type_pointer_get_ref(ptrchain_type))
    ;
  assert(ptrchain_type->type_type == TYPE_POINTER);
  ptrchain_type->details.pointer.ref = type;
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

  if (is_attr(v->attrs, ATTR_STRING) && !is_ptr(v->type) && !arr)
    error_loc("'%s': [string] attribute applied to non-pointer, non-array type\n",
              v->name);

  if (is_attr(v->attrs, ATTR_V1ENUM))
  {
    if (type_get_type_detect_alias(v->type) != TYPE_ENUM)
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
        unsigned int size = type_memsize(v->type, &align);

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
        if (type_array_get_conformance(*ptype)->is_const)
          error_loc("%s: cannot specify size_is for a fixed sized array\n", v->name);
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

static type_t *make_safearray(type_t *type)
{
  return type_new_array(NULL, type_new_alias(type, "SAFEARRAY"), TRUE, 0,
                        NULL, NULL, RPC_FC_RP);
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
  int is_str = is_attr(attrs, ATTR_STRING);
  type_t *type = decl_spec->type;

  if (is_str)
  {
    type_t *t = decl_spec->type;

    while (is_ptr(t))
      t = type_pointer_get_ref(t);

    if (type_get_type(t) != TYPE_BASIC &&
        (get_basic_fc(t) != RPC_FC_CHAR &&
         get_basic_fc(t) != RPC_FC_BYTE &&
         get_basic_fc(t) != RPC_FC_WCHAR))
    {
      decl = LIST_ENTRY( list_head( decls ), const declarator_t, entry );
      error_loc("'%s': [string] attribute is only valid on 'char', 'byte', or 'wchar_t' pointers and arrays\n",
              decl->var->name);
    }
  }

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

  if (type_get_type(t) == TYPE_VOID)
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
        case TYPE_BASIC_ERROR_STATUS_T:
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
        return FALSE;
    }
    return FALSE;
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
    int more_to_do;
    const char *container_type_name = NULL;

    switch (type_get_type_detect_alias(type))
    {
    case TYPE_STRUCT:
        container_type_name = "struct";
        break;
    case TYPE_UNION:
        container_type_name = "union";
        break;
    case TYPE_ENCAPSULATED_UNION:
        container_type_name = "encapsulated union";
        break;
    case TYPE_FUNCTION:
        container_type_name = "function";
        break;
    default:
        break;
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
            switch (type_get_type(type))
            {
            case TYPE_VOID:
                error_loc_info(&arg->loc_info, "parameter \'%s\' of %s \'%s\' cannot derive from void *\n",
                               arg->name, container_type_name, container_name);
                break;
            case TYPE_FUNCTION:
                error_loc_info(&arg->loc_info, "parameter \'%s\' of %s \'%s\' cannot be a function pointer\n",
                               arg->name, container_type_name, container_name);
                break;
            case TYPE_COCLASS:
            case TYPE_INTERFACE:
            case TYPE_MODULE:
                /* FIXME */
                break;
            default:
                break;
            }
        case TGT_CTXT_HANDLE:
        case TGT_CTXT_HANDLE_POINTER:
            /* FIXME */
            break;
        case TGT_POINTER:
            type = type_pointer_get_ref(type);
            more_to_do = TRUE;
            break;
        case TGT_ARRAY:
            type = type_array_get_element(type);
            more_to_do = TRUE;
            break;
        case TGT_USER_TYPE:
        case TGT_STRING:
        case TGT_IFACE_POINTER:
        case TGT_BASIC:
        case TGT_ENUM:
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
                if (!is_array(type))
                {
                    /* FIXME */
                }
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

