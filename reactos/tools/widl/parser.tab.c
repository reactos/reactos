
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
static var_t *declare_var(attr_list_t *attrs, decl_spec_t *decl_spec, const declarator_t *decl, int top);
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
     tDEFAULT = 304,
     tDEFAULTCOLLELEM = 305,
     tDEFAULTVALUE = 306,
     tDEFAULTVTABLE = 307,
     tDISPLAYBIND = 308,
     tDISPINTERFACE = 309,
     tDLLNAME = 310,
     tDOUBLE = 311,
     tDUAL = 312,
     tENDPOINT = 313,
     tENTRY = 314,
     tENUM = 315,
     tERRORSTATUST = 316,
     tEXPLICITHANDLE = 317,
     tEXTERN = 318,
     tFALSE = 319,
     tFASTCALL = 320,
     tFLOAT = 321,
     tHANDLE = 322,
     tHANDLET = 323,
     tHELPCONTEXT = 324,
     tHELPFILE = 325,
     tHELPSTRING = 326,
     tHELPSTRINGCONTEXT = 327,
     tHELPSTRINGDLL = 328,
     tHIDDEN = 329,
     tHYPER = 330,
     tID = 331,
     tIDEMPOTENT = 332,
     tIIDIS = 333,
     tIMMEDIATEBIND = 334,
     tIMPLICITHANDLE = 335,
     tIMPORT = 336,
     tIMPORTLIB = 337,
     tIN = 338,
     tIN_LINE = 339,
     tINLINE = 340,
     tINPUTSYNC = 341,
     tINT = 342,
     tINT3264 = 343,
     tINT64 = 344,
     tINTERFACE = 345,
     tLCID = 346,
     tLENGTHIS = 347,
     tLIBRARY = 348,
     tLOCAL = 349,
     tLONG = 350,
     tMETHODS = 351,
     tMODULE = 352,
     tNONBROWSABLE = 353,
     tNONCREATABLE = 354,
     tNONEXTENSIBLE = 355,
     tNULL = 356,
     tOBJECT = 357,
     tODL = 358,
     tOLEAUTOMATION = 359,
     tOPTIONAL = 360,
     tOUT = 361,
     tPASCAL = 362,
     tPOINTERDEFAULT = 363,
     tPROPERTIES = 364,
     tPROPGET = 365,
     tPROPPUT = 366,
     tPROPPUTREF = 367,
     tPTR = 368,
     tPUBLIC = 369,
     tRANGE = 370,
     tREADONLY = 371,
     tREF = 372,
     tREGISTER = 373,
     tREQUESTEDIT = 374,
     tRESTRICTED = 375,
     tRETVAL = 376,
     tSAFEARRAY = 377,
     tSHORT = 378,
     tSIGNED = 379,
     tSIZEIS = 380,
     tSIZEOF = 381,
     tSMALL = 382,
     tSOURCE = 383,
     tSTATIC = 384,
     tSTDCALL = 385,
     tSTRICTCONTEXTHANDLE = 386,
     tSTRING = 387,
     tSTRUCT = 388,
     tSWITCH = 389,
     tSWITCHIS = 390,
     tSWITCHTYPE = 391,
     tTRANSMITAS = 392,
     tTRUE = 393,
     tTYPEDEF = 394,
     tUNION = 395,
     tUNIQUE = 396,
     tUNSIGNED = 397,
     tUUID = 398,
     tV1ENUM = 399,
     tVARARG = 400,
     tVERSION = 401,
     tVOID = 402,
     tWCHAR = 403,
     tWIREMARSHAL = 404,
     ADDRESSOF = 405,
     NEG = 406,
     POS = 407,
     PPTR = 408,
     CAST = 409
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
#line 459 "parser.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 471 "parser.tab.c"

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
#define YYLAST   2343

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  179
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  99
/* YYNRULES -- Number of rules.  */
#define YYNRULES  345
/* YYNRULES -- Number of states.  */
#define YYNSTATES  607

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   409

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   163,     2,     2,     2,   162,   155,     2,
     174,   175,   160,   159,   150,   158,   170,   161,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   152,   173,
     156,   178,   157,   151,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   171,     2,   172,   154,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   176,   153,   177,   164,     2,     2,     2,
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
     145,   146,   147,   148,   149,   165,   166,   167,   168,   169
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
     111,   115,   120,   121,   123,   125,   129,   131,   135,   139,
     142,   146,   150,   151,   153,   157,   159,   163,   168,   170,
     174,   175,   177,   182,   184,   186,   188,   190,   192,   197,
     202,   204,   206,   208,   210,   212,   214,   219,   221,   223,
     228,   230,   235,   240,   242,   244,   249,   254,   259,   264,
     269,   271,   276,   278,   283,   285,   291,   293,   295,   300,
     305,   307,   309,   311,   313,   315,   317,   319,   321,   323,
     325,   330,   332,   334,   336,   338,   345,   347,   349,   351,
     353,   358,   360,   362,   364,   369,   374,   379,   384,   386,
     388,   393,   398,   400,   402,   404,   406,   408,   410,   412,
     413,   416,   421,   425,   426,   429,   431,   433,   437,   441,
     443,   449,   451,   455,   456,   458,   460,   462,   464,   466,
     468,   470,   472,   474,   476,   478,   484,   488,   492,   496,
     500,   504,   508,   512,   516,   520,   524,   528,   532,   536,
     540,   544,   548,   552,   556,   559,   562,   565,   568,   571,
     574,   578,   582,   588,   594,   599,   603,   605,   609,   611,
     613,   614,   617,   622,   626,   629,   632,   633,   636,   639,
     641,   645,   649,   653,   656,   657,   659,   660,   662,   664,
     666,   668,   670,   672,   674,   677,   680,   682,   684,   686,
     688,   690,   692,   693,   695,   697,   700,   702,   705,   708,
     710,   712,   714,   717,   720,   723,   729,   730,   733,   736,
     739,   742,   745,   748,   752,   755,   759,   765,   771,   772,
     775,   778,   781,   784,   791,   800,   803,   806,   809,   812,
     815,   818,   824,   826,   828,   830,   832,   834,   835,   838,
     841,   845,   846,   848,   851,   854,   857,   861,   864,   866,
     868,   872,   875,   880,   884,   887,   889,   893,   896,   897,
     899,   903,   906,   908,   912,   917,   921,   924,   926,   930,
     933,   934,   936,   938,   942,   945,   947,   951,   956,   958,
     962,   963,   966,   969,   971,   975,   977,   981,   983,   985,
     987,   993,   995,   997,   999,  1001,  1004,  1006,  1009,  1011,
    1014,  1019,  1024,  1030,  1041,  1043
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     180,     0,    -1,   181,    -1,    -1,   181,   246,    -1,   181,
     245,    -1,   181,   232,   173,    -1,   181,   234,    -1,   181,
     249,    -1,   181,   193,    -1,   181,   185,    -1,    -1,   182,
     246,    -1,   182,   245,    -1,   182,   232,   173,    -1,   182,
     234,    -1,   182,   249,    -1,   182,   185,    -1,   182,   190,
      -1,   182,   193,    -1,    -1,   183,   185,    -1,    -1,   173,
      -1,   187,    -1,   186,   173,    -1,   225,   173,    -1,   189,
      -1,   275,   173,    -1,   211,    -1,   273,    -1,   276,    -1,
     200,   211,    -1,   200,   273,    -1,   200,   276,    -1,    48,
     174,     8,   175,    -1,    81,     8,   173,    -1,   188,   182,
      12,    -1,    82,   174,     8,   175,   184,    -1,    93,     3,
      -1,   200,   191,   176,    -1,   192,   182,   177,   184,    -1,
      -1,   196,    -1,   197,    -1,   195,   150,   197,    -1,   195,
      -1,   195,   150,    22,    -1,   200,   254,   265,    -1,   254,
     265,    -1,   171,   213,   172,    -1,   171,   160,   172,    -1,
      -1,   200,    -1,   171,   201,   172,    -1,   203,    -1,   201,
     150,   203,    -1,   201,   172,   171,   203,    -1,     8,    -1,
     202,   150,     8,    -1,    -1,    23,    -1,    25,   174,     8,
     175,    -1,    26,    -1,    27,    -1,    29,    -1,    30,    -1,
      32,    -1,    35,   174,   228,   175,    -1,    37,   174,   215,
     175,    -1,    44,    -1,    45,    -1,    46,    -1,    47,    -1,
      49,    -1,    50,    -1,    51,   174,   217,   175,    -1,    52,
      -1,    53,    -1,    55,   174,     8,   175,    -1,    57,    -1,
      58,   174,   202,   175,    -1,    59,   174,   217,   175,    -1,
      62,    -1,    67,    -1,    69,   174,   216,   175,    -1,    70,
     174,     8,   175,    -1,    71,   174,     8,   175,    -1,    72,
     174,   216,   175,    -1,    73,   174,     8,   175,    -1,    74,
      -1,    76,   174,   216,   175,    -1,    77,    -1,    78,   174,
     214,   175,    -1,    79,    -1,    80,   174,    68,     3,   175,
      -1,    83,    -1,    86,    -1,    92,   174,   212,   175,    -1,
      91,   174,   216,   175,    -1,    91,    -1,    94,    -1,    98,
      -1,    99,    -1,   100,    -1,   102,    -1,   103,    -1,   104,
      -1,   105,    -1,   106,    -1,   108,   174,   272,   175,    -1,
     110,    -1,   111,    -1,   112,    -1,   114,    -1,   115,   174,
     216,   150,   216,   175,    -1,   116,    -1,   119,    -1,   120,
      -1,   121,    -1,   125,   174,   212,   175,    -1,   128,    -1,
     131,    -1,   132,    -1,   135,   174,   214,   175,    -1,   136,
     174,   274,   175,    -1,   137,   174,   274,   175,    -1,   143,
     174,   204,   175,    -1,   144,    -1,   145,    -1,   146,   174,
     277,   175,    -1,   149,   174,   274,   175,    -1,   272,    -1,
      11,    -1,     8,    -1,    38,    -1,    65,    -1,   107,    -1,
     130,    -1,    -1,   206,   207,    -1,    37,   216,   152,   222,
      -1,    49,   152,   222,    -1,    -1,   209,   150,    -1,   209,
      -1,   210,    -1,   209,   150,   210,    -1,   228,   178,   216,
      -1,   228,    -1,    60,   227,   176,   208,   177,    -1,   213,
      -1,   212,   150,   213,    -1,    -1,   214,    -1,     5,    -1,
       6,    -1,     7,    -1,    64,    -1,   101,    -1,   138,    -1,
       8,    -1,     9,    -1,    10,    -1,     3,    -1,   214,   151,
     214,   152,   214,    -1,   214,    20,   214,    -1,   214,    21,
     214,    -1,   214,   153,   214,    -1,   214,   154,   214,    -1,
     214,   155,   214,    -1,   214,    16,   214,    -1,   214,    17,
     214,    -1,   214,   157,   214,    -1,   214,   156,   214,    -1,
     214,    18,   214,    -1,   214,    19,   214,    -1,   214,    13,
     214,    -1,   214,    14,   214,    -1,   214,   159,   214,    -1,
     214,   158,   214,    -1,   214,   162,   214,    -1,   214,   160,
     214,    -1,   214,   161,   214,    -1,   163,   214,    -1,   164,
     214,    -1,   159,   214,    -1,   158,   214,    -1,   155,   214,
      -1,   160,   214,    -1,   214,    15,     3,    -1,   214,   170,
       3,    -1,   174,   254,   261,   175,   214,    -1,   126,   174,
     254,   261,   175,    -1,   214,   171,   214,   172,    -1,   174,
     214,   175,    -1,   216,    -1,   215,   150,   216,    -1,   214,
      -1,   214,    -1,    -1,   218,   219,    -1,   199,   254,   270,
     173,    -1,   199,   276,   173,    -1,   223,   173,    -1,   200,
     173,    -1,    -1,   221,   220,    -1,   223,   173,    -1,   173,
      -1,   199,   254,   257,    -1,   199,   254,   257,    -1,   200,
     254,   271,    -1,   254,   271,    -1,    -1,   228,    -1,    -1,
       3,    -1,     4,    -1,     3,    -1,     4,    -1,    33,    -1,
     148,    -1,   231,    -1,   124,   231,    -1,   142,   231,    -1,
     142,    -1,    66,    -1,    56,    -1,    31,    -1,    61,    -1,
      68,    -1,    -1,    87,    -1,    87,    -1,   123,   230,    -1,
     127,    -1,    95,   230,    -1,    75,   230,    -1,    89,    -1,
      39,    -1,    88,    -1,    40,     3,    -1,    40,     4,    -1,
     200,   232,    -1,   233,   176,   235,   177,   184,    -1,    -1,
     235,   236,    -1,   199,   246,    -1,    54,     3,    -1,    54,
       4,    -1,   200,   237,    -1,   109,   152,    -1,   239,   223,
     173,    -1,    96,   152,    -1,   240,   224,   173,    -1,   238,
     176,   239,   240,   177,    -1,   238,   176,   243,   173,   177,
      -1,    -1,   152,     4,    -1,    90,     3,    -1,    90,     4,
      -1,   200,   243,    -1,   244,   242,   176,   183,   177,   184,
      -1,   244,   152,     3,   176,   189,   183,   177,   184,    -1,
     241,   184,    -1,   243,   173,    -1,   237,   173,    -1,    97,
       3,    -1,    97,     4,    -1,   200,   247,    -1,   248,   176,
     183,   177,   184,    -1,    63,    -1,   129,    -1,   118,    -1,
      85,    -1,    43,    -1,    -1,   253,   252,    -1,   274,   255,
      -1,   256,   274,   255,    -1,    -1,   256,    -1,   252,   255,
      -1,   251,   255,    -1,   250,   255,    -1,   160,   253,   257,
      -1,   205,   257,    -1,   258,    -1,   228,    -1,   174,   257,
     175,    -1,   258,   198,    -1,   258,   174,   194,   175,    -1,
     160,   253,   261,    -1,   205,   261,    -1,   262,    -1,   160,
     253,   265,    -1,   205,   265,    -1,    -1,   259,    -1,   174,
     260,   175,    -1,   262,   198,    -1,   198,    -1,   174,   194,
     175,    -1,   262,   174,   194,   175,    -1,   160,   253,   265,
      -1,   205,   265,    -1,   266,    -1,   160,   253,   265,    -1,
     205,   265,    -1,    -1,   263,    -1,   228,    -1,   174,   264,
     175,    -1,   266,   198,    -1,   198,    -1,   174,   194,   175,
      -1,   266,   174,   194,   175,    -1,   257,    -1,   267,   150,
     257,    -1,    -1,   152,   217,    -1,   263,   268,    -1,   269,
      -1,   270,   150,   269,    -1,   257,    -1,   257,   178,   217,
      -1,   117,    -1,   141,    -1,   113,    -1,   133,   227,   176,
     218,   177,    -1,   147,    -1,     4,    -1,   229,    -1,   211,
      -1,    60,     3,    -1,   273,    -1,   133,     3,    -1,   276,
      -1,   140,     3,    -1,   122,   174,   274,   175,    -1,   139,
     199,   254,   267,    -1,   140,   227,   176,   221,   177,    -1,
     140,   227,   134,   174,   223,   175,   226,   176,   206,   177,
      -1,     5,    -1,     5,   170,     5,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   330,   330,   343,   344,   345,   346,   349,   352,   353,
     354,   357,   358,   359,   360,   361,   364,   365,   366,   367,
     370,   371,   374,   375,   379,   380,   381,   382,   383,   387,
     388,   389,   390,   391,   392,   395,   397,   405,   411,   415,
     417,   421,   428,   429,   432,   433,   436,   437,   441,   446,
     453,   454,   457,   458,   462,   465,   466,   467,   470,   471,
     474,   475,   476,   477,   478,   479,   480,   481,   482,   483,
     484,   485,   486,   487,   488,   489,   490,   491,   492,   493,
     494,   495,   496,   497,   498,   499,   500,   501,   502,   503,
     504,   505,   506,   507,   508,   509,   510,   511,   512,   513,
     514,   515,   516,   517,   518,   519,   520,   521,   522,   523,
     524,   525,   526,   527,   528,   529,   533,   534,   535,   536,
     537,   538,   539,   540,   541,   542,   543,   544,   545,   546,
     547,   548,   549,   553,   554,   559,   560,   561,   562,   565,
     566,   569,   573,   579,   580,   581,   584,   588,   597,   601,
     606,   609,   610,   623,   624,   627,   628,   629,   630,   631,
     632,   633,   634,   635,   636,   637,   638,   639,   640,   641,
     642,   643,   644,   645,   646,   647,   648,   649,   650,   651,
     652,   653,   654,   655,   656,   657,   658,   659,   660,   661,
     662,   663,   664,   666,   668,   669,   672,   673,   676,   682,
     688,   689,   692,   697,   704,   705,   708,   709,   713,   714,
     717,   724,   733,   737,   742,   743,   746,   747,   748,   751,
     753,   756,   757,   758,   759,   760,   761,   762,   763,   764,
     765,   766,   769,   770,   773,   774,   775,   776,   777,   778,
     779,   780,   783,   784,   792,   798,   802,   803,   807,   810,
     811,   814,   824,   825,   828,   829,   832,   838,   844,   845,
     848,   849,   852,   863,   870,   876,   880,   881,   884,   885,
     888,   893,   900,   901,   902,   906,   910,   913,   914,   917,
     918,   922,   923,   927,   928,   929,   933,   935,   936,   940,
     941,   942,   943,   951,   953,   954,   959,   961,   965,   966,
     971,   972,   973,   974,   979,   988,   990,   991,   996,   998,
    1002,  1003,  1010,  1011,  1012,  1013,  1014,  1019,  1027,  1028,
    1031,  1032,  1035,  1042,  1043,  1048,  1049,  1053,  1054,  1055,
    1058,  1061,  1062,  1063,  1064,  1065,  1066,  1067,  1068,  1069,
    1070,  1073,  1079,  1081,  1087,  1088
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
  "tCPPQUOTE", "tDEFAULT", "tDEFAULTCOLLELEM", "tDEFAULTVALUE",
  "tDEFAULTVTABLE", "tDISPLAYBIND", "tDISPINTERFACE", "tDLLNAME",
  "tDOUBLE", "tDUAL", "tENDPOINT", "tENTRY", "tENUM", "tERRORSTATUST",
  "tEXPLICITHANDLE", "tEXTERN", "tFALSE", "tFASTCALL", "tFLOAT", "tHANDLE",
  "tHANDLET", "tHELPCONTEXT", "tHELPFILE", "tHELPSTRING",
  "tHELPSTRINGCONTEXT", "tHELPSTRINGDLL", "tHIDDEN", "tHYPER", "tID",
  "tIDEMPOTENT", "tIIDIS", "tIMMEDIATEBIND", "tIMPLICITHANDLE", "tIMPORT",
  "tIMPORTLIB", "tIN", "tIN_LINE", "tINLINE", "tINPUTSYNC", "tINT",
  "tINT3264", "tINT64", "tINTERFACE", "tLCID", "tLENGTHIS", "tLIBRARY",
  "tLOCAL", "tLONG", "tMETHODS", "tMODULE", "tNONBROWSABLE",
  "tNONCREATABLE", "tNONEXTENSIBLE", "tNULL", "tOBJECT", "tODL",
  "tOLEAUTOMATION", "tOPTIONAL", "tOUT", "tPASCAL", "tPOINTERDEFAULT",
  "tPROPERTIES", "tPROPGET", "tPROPPUT", "tPROPPUTREF", "tPTR", "tPUBLIC",
  "tRANGE", "tREADONLY", "tREF", "tREGISTER", "tREQUESTEDIT",
  "tRESTRICTED", "tRETVAL", "tSAFEARRAY", "tSHORT", "tSIGNED", "tSIZEIS",
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
  "libraryhdr", "library_start", "librarydef", "m_args", "arg_list",
  "args", "arg", "array", "m_attributes", "attributes", "attrib_list",
  "str_list", "attribute", "uuid_string", "callconv", "cases", "case",
  "enums", "enum_list", "enum", "enumdef", "m_exprs", "m_expr", "expr",
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
  "abstract_declarator", "abstract_declarator_no_direct",
  "m_abstract_declarator", "abstract_direct_declarator", "any_declarator",
  "any_declarator_no_direct", "m_any_declarator", "any_direct_declarator",
  "declarator_list", "m_bitfield", "struct_declarator",
  "struct_declarator_list", "init_declarator", "pointer_type", "structdef",
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
     395,   396,   397,   398,   399,   400,   401,   402,   403,   404,
      44,    63,    58,   124,    94,    38,    60,    62,    45,    43,
      42,    47,    37,    33,   126,   405,   406,   407,   408,   409,
      46,    91,    93,    59,    40,    41,   123,   125,    61
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   179,   180,   181,   181,   181,   181,   181,   181,   181,
     181,   182,   182,   182,   182,   182,   182,   182,   182,   182,
     183,   183,   184,   184,   185,   185,   185,   185,   185,   186,
     186,   186,   186,   186,   186,   187,   188,   189,   190,   191,
     192,   193,   194,   194,   195,   195,   196,   196,   197,   197,
     198,   198,   199,   199,   200,   201,   201,   201,   202,   202,
     203,   203,   203,   203,   203,   203,   203,   203,   203,   203,
     203,   203,   203,   203,   203,   203,   203,   203,   203,   203,
     203,   203,   203,   203,   203,   203,   203,   203,   203,   203,
     203,   203,   203,   203,   203,   203,   203,   203,   203,   203,
     203,   203,   203,   203,   203,   203,   203,   203,   203,   203,
     203,   203,   203,   203,   203,   203,   203,   203,   203,   203,
     203,   203,   203,   203,   203,   203,   203,   203,   203,   203,
     203,   203,   203,   204,   204,   205,   205,   205,   205,   206,
     206,   207,   207,   208,   208,   208,   209,   209,   210,   210,
     211,   212,   212,   213,   213,   214,   214,   214,   214,   214,
     214,   214,   214,   214,   214,   214,   214,   214,   214,   214,
     214,   214,   214,   214,   214,   214,   214,   214,   214,   214,
     214,   214,   214,   214,   214,   214,   214,   214,   214,   214,
     214,   214,   214,   214,   214,   214,   215,   215,   216,   217,
     218,   218,   219,   219,   220,   220,   221,   221,   222,   222,
     223,   224,   225,   225,   226,   226,   227,   227,   227,   228,
     228,   229,   229,   229,   229,   229,   229,   229,   229,   229,
     229,   229,   230,   230,   231,   231,   231,   231,   231,   231,
     231,   231,   232,   232,   233,   234,   235,   235,   236,   237,
     237,   238,   239,   239,   240,   240,   241,   241,   242,   242,
     243,   243,   244,   245,   245,   245,   246,   246,   247,   247,
     248,   249,   250,   250,   250,   251,   252,   253,   253,   254,
     254,   255,   255,   256,   256,   256,   257,   257,   257,   258,
     258,   258,   258,   259,   259,   259,   260,   260,   261,   261,
     262,   262,   262,   262,   262,   263,   263,   263,   264,   264,
     265,   265,   266,   266,   266,   266,   266,   266,   267,   267,
     268,   268,   269,   270,   270,   271,   271,   272,   272,   272,
     273,   274,   274,   274,   274,   274,   274,   274,   274,   274,
     274,   275,   276,   276,   277,   277
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     0,     2,     2,     3,     2,     2,     2,
       2,     0,     2,     2,     3,     2,     2,     2,     2,     2,
       0,     2,     0,     1,     1,     2,     2,     1,     2,     1,
       1,     1,     2,     2,     2,     4,     3,     3,     5,     2,
       3,     4,     0,     1,     1,     3,     1,     3,     3,     2,
       3,     3,     0,     1,     3,     1,     3,     4,     1,     3,
       0,     1,     4,     1,     1,     1,     1,     1,     4,     4,
       1,     1,     1,     1,     1,     1,     4,     1,     1,     4,
       1,     4,     4,     1,     1,     4,     4,     4,     4,     4,
       1,     4,     1,     4,     1,     5,     1,     1,     4,     4,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       4,     1,     1,     1,     1,     6,     1,     1,     1,     1,
       4,     1,     1,     1,     4,     4,     4,     4,     1,     1,
       4,     4,     1,     1,     1,     1,     1,     1,     1,     0,
       2,     4,     3,     0,     2,     1,     1,     3,     3,     1,
       5,     1,     3,     0,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     5,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     2,     2,     2,     2,     2,     2,
       3,     3,     5,     5,     4,     3,     1,     3,     1,     1,
       0,     2,     4,     3,     2,     2,     0,     2,     2,     1,
       3,     3,     3,     2,     0,     1,     0,     1,     1,     1,
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
       5,     1,     1,     1,     1,     2,     1,     2,     1,     2,
       4,     4,     5,    10,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       3,     0,     2,     1,   332,   229,   221,   240,     0,   276,
       0,     0,   228,   216,   230,   272,   227,   231,   232,     0,
     275,   234,   241,   239,     0,   232,   274,     0,   232,     0,
     236,   273,   216,    52,   216,   226,   331,   222,    60,    10,
       0,    24,    11,    27,    11,     9,     0,   334,     0,   333,
     223,     0,     0,     7,     0,     0,    22,     0,   258,     5,
       4,     0,     8,   281,   281,   281,     0,     0,   336,   281,
       0,   338,   242,   243,     0,   249,   250,   335,   218,     0,
     233,   238,     0,   260,   261,   237,     0,   235,   224,   337,
       0,     0,    53,   339,     0,   225,    61,     0,    63,    64,
      65,    66,    67,     0,     0,    70,    71,    72,    73,    74,
      75,     0,    77,    78,     0,    80,     0,     0,    83,    84,
       0,     0,     0,     0,     0,    90,     0,    92,     0,    94,
       0,    96,    97,   100,     0,   101,   102,   103,   104,   105,
     106,   107,   108,   109,     0,   111,   112,   113,   329,   114,
       0,   116,   327,   117,   118,   119,     0,   121,   122,   123,
       0,     0,     0,   328,     0,   128,   129,     0,     0,     0,
      55,   132,    25,     0,     0,     0,     0,     0,   334,   244,
     251,   262,   270,     0,   336,   338,    26,     6,   246,   267,
       0,    23,   265,   266,     0,     0,    20,   285,   282,   284,
     283,   219,   220,   135,   136,   137,   138,   277,     0,     0,
     289,   325,   288,   213,   334,   336,   281,   338,   279,    28,
       0,   143,    36,     0,   200,     0,     0,   206,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   153,     0,     0,   153,     0,     0,
       0,     0,     0,     0,    60,    54,    37,     0,    17,    18,
      19,     0,    15,    13,    12,    16,    22,    39,   268,   269,
      40,   212,    52,     0,    52,     0,     0,   259,    20,     0,
       0,     0,   287,     0,   153,    42,   291,   280,    35,     0,
     145,   146,   149,   340,    52,   318,   341,    52,    52,     0,
       0,   164,   155,   156,   157,   161,   162,   163,   158,   159,
       0,   160,     0,     0,     0,     0,     0,     0,     0,   198,
       0,   196,   199,     0,     0,    58,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   151,   154,
       0,     0,     0,     0,     0,     0,   134,   133,     0,   344,
       0,     0,    56,    60,     0,    14,    41,    22,     0,   247,
     252,     0,     0,     0,    52,     0,     0,     0,    22,    21,
       0,   278,   286,   290,   326,     0,     0,     0,    46,    43,
      44,     0,   310,   150,   144,     0,   330,     0,   201,     0,
       0,   342,    53,   207,     0,    62,    68,     0,   188,   187,
     186,   189,   184,   185,     0,   298,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    69,
      76,    79,     0,    81,    82,    85,    86,    87,    88,    89,
      91,    93,     0,    99,   153,    98,   110,     0,   120,   124,
     125,   126,   127,     0,   130,   131,    57,     0,   245,   248,
     254,     0,   253,   256,     0,     0,   257,    20,    22,   271,
      51,    50,   292,     0,   310,   277,    42,   315,   310,   312,
     311,    49,   307,   147,   148,     0,   338,   319,   214,   205,
     204,   298,   195,   277,    42,   302,   298,   299,     0,   295,
     177,   178,   190,   171,   172,   175,   176,   166,   167,     0,
     168,   169,   170,   174,   173,   180,   179,   182,   183,   181,
     191,     0,   197,    59,    95,   152,     0,   345,    22,   210,
       0,   255,     0,   263,    47,    45,    48,   310,   277,     0,
     310,     0,   306,    42,   314,   320,   323,     0,   203,     0,
     215,     0,   298,   277,     0,   310,     0,   294,     0,    42,
     301,     0,   194,   115,    38,   211,    22,   305,   310,   316,
     309,   313,     0,     0,   322,     0,   202,   139,   193,   293,
     310,   303,   297,   300,   192,     0,   165,   264,   308,   317,
     321,   324,     0,   296,   304,     0,     0,   343,   140,     0,
      52,    52,   209,   142,     0,   141,   208
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     2,   173,   279,   192,   369,    40,    41,    42,
      43,   259,   177,    44,   260,   377,   378,   379,   380,   477,
     362,    92,   169,   326,   170,   348,   478,   592,   598,   289,
     290,   291,   214,   337,   338,   319,   320,   321,   323,   294,
     388,   393,   298,   603,   604,   465,    48,   549,    79,   479,
      49,    81,    50,   261,    52,   262,   272,   359,    54,    55,
     274,   364,    56,   195,    57,    58,   263,   264,   182,    61,
     265,    63,    64,    65,   280,    66,   197,    67,   211,   212,
     497,   556,   498,   499,   480,   541,   481,   482,   296,   574,
     546,   547,   213,   171,   215,    69,    70,   217,   350
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -459
static const yytype_int16 yypact[] =
{
    -459,    58,  1214,  -459,  -459,  -459,  -459,  -459,   161,  -459,
    -102,   256,  -459,   264,  -459,  -459,  -459,  -459,    -1,    88,
    -459,  -459,  -459,  -459,   289,    -1,  -459,   -67,    -1,   484,
    -459,  -459,   295,   -19,   297,   484,  -459,  -459,  2194,  -459,
     -16,  -459,  -459,  -459,  -459,  -459,  2007,     5,    13,  -459,
    -459,    39,    25,  -459,    46,    72,    54,    89,   117,  -459,
    -459,    98,  -459,   -14,   -14,   -14,   287,   992,   103,   -14,
     109,   113,  -459,  -459,   276,  -459,  -459,   112,  -459,   119,
    -459,  -459,   129,  -459,  -459,  -459,   992,  -459,  -459,   112,
     128,  2085,  -459,  -103,  -101,  -459,  -459,   134,  -459,  -459,
    -459,  -459,  -459,   140,   144,  -459,  -459,  -459,  -459,  -459,
    -459,   146,  -459,  -459,   149,  -459,   153,   154,  -459,  -459,
     163,   164,   168,   171,   174,  -459,   175,  -459,   177,  -459,
     182,  -459,  -459,   184,   185,  -459,  -459,  -459,  -459,  -459,
    -459,  -459,  -459,  -459,   186,  -459,  -459,  -459,  -459,  -459,
     192,  -459,  -459,  -459,  -459,  -459,   194,  -459,  -459,  -459,
     200,   201,   204,  -459,   205,  -459,  -459,   207,   208,   -93,
    -459,  -459,  -459,  1119,   500,   359,   303,   209,   210,  -459,
    -459,  -459,  -459,   287,   211,   216,  -459,  -459,  -459,  -459,
      33,  -459,  -459,  -459,   307,   212,  -459,  -459,  -459,  -459,
    -459,  -459,  -459,  -459,  -459,  -459,  -459,  -459,   287,   287,
    -459,   213,   -87,  -459,  -459,  -459,   -14,  -459,  -459,  -459,
     218,   309,  -459,   220,  -459,   287,   223,  -459,   391,   309,
     944,   944,   392,   398,   944,   944,   399,   400,   944,   402,
     944,   944,   344,   944,   944,   -72,   944,   944,   944,   992,
     992,   111,   410,   992,  2194,   245,  -459,   244,  -459,  -459,
    -459,   246,  -459,  -459,  -459,  -459,    54,  -459,  -459,  -459,
    -459,  -459,   -50,   268,   -32,   248,   247,  -459,  -459,   595,
      40,   253,  -459,   944,   983,  1577,  -459,  -459,  -459,   255,
     272,  -459,   252,  -459,   -18,  -459,   284,   -19,    -8,   260,
     262,  -459,  -459,  -459,  -459,  -459,  -459,  -459,  -459,  -459,
     266,  -459,   944,   944,   944,   944,   944,   944,   880,  1826,
     -80,  -459,  1826,   263,   267,  -459,   -69,   269,   270,   271,
     273,   274,   280,   281,  1582,   438,   285,   -62,  -459,  1826,
     291,   293,   -24,  1640,   294,   298,  -459,  -459,   300,   302,
     301,   304,  -459,  2194,   443,  -459,  -459,    54,     7,  -459,
    -459,   316,  2085,   305,    14,   306,   389,   690,    54,  -459,
    2085,  -459,  -459,  -459,  -459,   326,   308,   313,   327,  -459,
    -459,  2085,    90,  -459,   309,   944,  -459,  2085,  -459,   287,
     318,  -459,   321,  -459,   322,  -459,  -459,  2085,    11,    11,
      11,    11,    11,    11,  1663,   279,   944,   944,   479,   944,
     944,   944,   944,   944,   944,   944,   944,   944,   944,   944,
     944,   944,   944,   944,   944,   944,   488,   944,   944,  -459,
    -459,  -459,   489,  -459,  -459,  -459,  -459,  -459,  -459,  -459,
    -459,  -459,   324,  -459,   944,  -459,  -459,   944,  -459,  -459,
    -459,  -459,  -459,   491,  -459,  -459,  -459,   330,  -459,  -459,
    -459,   287,  -459,  -459,  2085,   328,  -459,  -459,    54,  -459,
    -459,  -459,  -459,  1499,    90,  -459,  1326,  -459,    90,  -459,
    -459,  -459,    16,  -459,  -459,    90,   329,  -459,   309,  -459,
    -459,   279,  -459,  -459,  1404,  -459,   279,  -459,   331,    31,
     243,   243,  -459,   444,   444,    95,    95,  1713,  1790,  1767,
    1844,  1863,  1895,    95,    95,   193,   193,    11,    11,    11,
    -459,  1745,  -459,  -459,  -459,  -459,   332,  -459,    54,  -459,
     287,  -459,   785,  -459,  -459,  -459,  -459,   145,  -459,   333,
      90,   334,  -459,  1577,  -459,   358,  -459,   -74,  -459,   335,
    -459,   337,    47,  -459,   338,    90,   339,  -459,   944,  1577,
    -459,   944,  -459,  -459,  -459,  -459,    54,  -459,   145,  -459,
    -459,  -459,   340,   944,  -459,    90,  -459,  -459,  -459,  -459,
     145,  -459,  -459,  -459,    11,   341,  1826,  -459,  -459,  -459,
    -459,  -459,    -9,  -459,  -459,   944,   365,  -459,  -459,   366,
      51,    51,  -459,  -459,   347,  -459,  -459
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -459,  -459,  -459,   477,  -258,  -257,    19,  -459,  -459,  -459,
     156,  -459,  -459,  -459,   522,  -429,  -459,  -459,    52,  -202,
     -21,    -2,  -459,  -459,  -229,  -459,   -65,  -459,  -459,  -459,
    -459,   142,     2,   282,  -260,  -181,  -459,  -224,  -227,  -459,
    -459,  -459,  -459,   -73,  -206,  -459,  -459,  -459,   215,   -63,
    -459,   188,   121,    28,  -459,   525,  -459,  -459,   490,  -459,
    -459,  -459,  -459,  -459,   -23,  -459,   533,     3,  -459,  -459,
     535,  -459,  -459,  -265,  -413,   -40,   -10,   -27,  -191,  -459,
    -459,  -459,  -414,  -459,  -458,  -459,  -439,  -459,  -459,  -459,
     -33,  -459,   361,   310,     6,   -54,  -459,     0,  -459
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -218
static const yytype_int16 yytable[] =
{
      46,   209,    71,   210,    47,    60,   183,   327,    68,   356,
     286,   328,    91,   216,   331,   371,   333,   281,   282,   336,
     367,    39,   341,   181,   376,   352,   408,   545,   595,     9,
      51,  -217,   223,   226,   295,   536,   198,   198,   198,   542,
     596,   148,   198,   201,   202,   152,   185,   539,   178,    15,
     322,   225,   184,   322,   199,   200,   374,   254,     3,   218,
     334,    11,   537,   339,   361,   554,   339,   343,   363,   163,
     428,    20,    74,  -217,   179,   227,   575,   551,   203,   255,
     552,   432,   557,     9,   284,   203,    80,   285,   444,   372,
       9,   390,   394,   201,   202,   429,    82,    24,   567,   576,
     458,   570,   322,   339,    26,   204,   433,    86,   406,   407,
     408,   469,   204,   445,   572,    31,   582,   545,   209,   346,
     210,    38,   347,    24,   456,   568,   444,   357,   203,   588,
     585,   398,   399,   400,   401,   402,   403,   404,   579,    38,
     580,   593,   273,   209,   209,   210,   210,   205,   201,   202,
      88,   448,    38,    38,   205,   204,    95,   172,   292,   386,
     209,   484,   210,    38,    72,    73,   300,   275,   597,   391,
     206,    46,    46,    71,    71,    47,    47,   206,   -29,    68,
      68,   426,   427,   203,   525,    38,   186,   284,     9,   198,
     543,   463,   258,   258,   401,   344,   345,   205,   487,   351,
     207,   188,   284,   495,   522,   559,   287,   493,   408,   532,
     204,   533,   187,    85,   208,   209,    87,   210,   284,   189,
     206,   494,    38,   526,   602,   500,   501,   191,   503,   504,
     505,   506,   507,   508,   509,   510,   511,   512,   513,   514,
     515,   516,   517,   518,   519,   382,   521,    90,   190,    94,
     475,   358,   205,   421,   422,   423,   424,   425,   408,    75,
      76,   284,   193,   339,   476,   426,   427,    77,    78,   194,
     529,   564,   371,   387,   196,   206,   -30,   370,   405,    71,
     544,    47,   219,   381,   220,    68,   -31,   371,  -217,   495,
     201,   202,    83,    84,   495,   221,   392,   560,    89,    78,
      93,    78,   222,   371,   224,   475,   268,   269,   228,   587,
     276,   277,   201,   202,   229,   371,   284,   203,   230,   476,
     231,   292,   461,   232,   209,   203,   210,   233,   234,   301,
     183,   302,   303,   304,   305,   306,   307,   235,   236,   565,
     496,   474,   237,   464,   204,   238,   590,   485,   239,   240,
     495,   241,   204,   423,   424,   425,   242,   491,   243,   244,
     245,   459,   267,   426,   427,   370,   246,    71,   247,    47,
     185,   599,   178,    68,   248,   249,   184,   584,   250,   251,
     586,   252,   253,   -32,   -33,   270,   205,   486,   278,   -34,
     308,   283,   322,   288,   205,   293,   209,   297,   210,   299,
     324,   421,   422,   423,   424,   425,   325,   329,   330,   206,
     332,   540,   335,   426,   427,   349,   353,   206,   354,   355,
     360,   365,   384,   366,   530,   550,   496,   309,   373,   555,
     385,   496,   383,   382,   389,   395,   382,   396,   430,   493,
     397,   442,   431,   447,   434,   435,   436,   207,   437,   438,
     284,   457,   310,   494,   382,   439,   440,   406,   407,   408,
     443,   208,   411,   412,   311,   209,   446,   210,   460,   450,
      19,   381,   453,   451,   381,   452,   454,   473,   462,   455,
     471,   312,   502,   466,   313,   314,   315,   496,   472,   316,
     317,   520,   381,   488,   489,   490,   527,   523,   470,   524,
     318,   531,   548,   382,     4,   528,   558,   563,   569,   571,
     573,   577,   578,   581,   583,   589,   594,   600,   601,   382,
     606,   174,   467,     7,    45,   535,   483,    53,   605,   342,
     370,     5,    71,     6,    47,    59,   180,    62,    68,     7,
       8,   381,   591,     9,   271,     0,     0,     0,    10,     0,
       0,     0,     0,     0,    11,   340,    12,   381,     0,    18,
      13,    14,     0,    15,     0,     0,    16,     0,    17,     0,
       0,    21,    22,    23,     0,    18,     0,     0,     0,    25,
       0,    19,   257,     0,     0,    20,     0,    21,    22,    23,
      24,     0,     0,     0,     0,    25,     0,     0,     0,     4,
     419,   420,   421,   422,   423,   424,   425,    28,     0,     0,
       0,    30,     0,     0,   426,   427,     0,     0,    26,     0,
       0,     0,    27,    28,    29,     0,     5,    30,     6,    31,
       0,     0,     0,    32,     7,     0,     0,     0,     9,    33,
      34,     0,    35,    10,     0,     0,     0,    36,    37,     0,
       0,    12,     0,     0,     0,    13,    14,     0,    15,     0,
       0,    16,     0,    17,     0,     0,     0,     0,     0,     0,
      18,    38,     0,     0,     0,     0,    19,   266,     0,     0,
      20,     0,    21,    22,    23,     0,     0,     0,     0,     0,
      25,     0,     0,     0,     4,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    26,     0,     0,     0,    27,    28,    29,
       0,     5,    30,     6,    31,     0,     0,     0,    32,     7,
       0,     0,     0,     9,    33,    34,     0,    35,    10,     0,
       0,     0,    36,    37,     0,     0,    12,     0,     0,     0,
      13,    14,     0,    15,     0,     0,    16,     0,    17,     0,
       0,     0,     0,     0,     0,    18,    38,     0,     0,     0,
       0,    19,   368,     0,     0,    20,     0,    21,    22,    23,
       0,     0,     0,     0,     0,    25,     0,     0,     0,     4,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    26,     0,
       0,     0,    27,    28,    29,     0,     5,    30,     6,    31,
       0,     0,     0,    32,     7,     0,     0,     0,     9,    33,
      34,     0,    35,    10,     0,     0,     0,    36,    37,     0,
       0,    12,     0,     0,     0,    13,    14,     0,    15,     0,
       0,    16,     0,    17,     0,     0,     0,     0,     0,     0,
      18,    38,     0,     0,     0,     0,    19,   468,     0,     0,
      20,     0,    21,    22,    23,     0,     0,     0,     0,     0,
      25,     0,     0,   301,     4,   302,   303,   304,   305,   306,
     307,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    26,     0,     0,     0,    27,    28,    29,
       0,     5,    30,     6,    31,     0,     0,     0,    32,     7,
       0,     0,     0,     9,    33,    34,     0,    35,     0,     0,
       0,     0,    36,    37,     0,     0,    12,     0,     0,     0,
      13,    14,     0,    15,   308,     0,    16,   301,    17,   302,
     303,   304,   305,   306,   307,    18,    38,     0,     0,     0,
       0,     0,   566,     0,     0,    20,     0,    21,    22,    23,
       0,     0,     0,     0,     0,    25,     0,     0,     0,     0,
       0,   309,     0,     0,     0,     0,   301,     0,   302,   303,
     304,   305,   306,   307,     0,     0,     4,     0,    26,     0,
       0,     0,    27,    28,    29,     0,   310,    30,   308,    31,
       0,     0,     0,    32,     0,     0,     0,     0,   311,     0,
      34,     0,    35,     5,     0,     6,     0,    36,    37,     0,
       0,     7,     0,     0,     0,   312,     0,     0,   313,   314,
     315,     0,     0,   316,   317,   309,     0,   308,    12,     0,
       0,     0,    13,    14,   318,     0,     0,     0,    16,     0,
      17,     0,     0,     0,     0,     0,     0,    18,     0,     0,
     310,     0,     0,     0,     0,     0,     0,     0,     0,    21,
      22,    23,   311,     0,   309,     0,     0,    25,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   312,
       0,     0,   313,   314,   315,     0,     0,   316,   317,   310,
       0,     0,     0,     0,    27,    28,    29,     0,   318,    30,
       0,   311,     0,     4,     0,    32,     0,     0,     0,     0,
       0,   256,    34,     0,    35,     0,     0,     0,   312,    36,
      37,   313,   314,   375,     0,     0,   316,   317,     0,     0,
       5,     0,     6,     0,     0,     0,     0,   318,     7,     8,
       0,     0,     9,     0,     0,     0,     0,    10,     0,     0,
       0,     0,     0,    11,     0,    12,     0,     0,     0,    13,
      14,     0,    15,     0,     0,    16,     0,    17,     0,     0,
       0,     0,     0,     0,    18,     0,     0,     0,     0,     0,
      19,   257,     0,     0,    20,     0,    21,    22,    23,    24,
       0,     0,     0,     0,    25,     0,     0,     0,     4,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    26,     0,     0,
       0,    27,    28,    29,     0,     5,    30,     6,    31,     0,
       0,     0,    32,     7,     8,     0,     0,     9,    33,    34,
       0,    35,    10,     0,     0,     0,    36,    37,    11,     0,
      12,     0,     0,     0,    13,    14,     0,    15,     0,     0,
      16,     0,    17,     0,     0,     0,     0,     0,     0,    18,
      38,     0,     0,     0,     0,    19,     0,     0,     0,    20,
       0,    21,    22,    23,    24,     0,     0,     0,     0,    25,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       4,     0,    26,     0,     0,     0,    27,    28,    29,     0,
       0,    30,     0,    31,     0,     0,     0,    32,     0,     0,
       0,     0,     0,    33,    34,     0,    35,     5,     0,     6,
       0,    36,    37,     0,   203,     7,     0,     0,     0,     9,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    12,     0,     0,    38,    13,    14,     0,    15,
       0,   204,    16,     0,    17,     0,     0,     0,     0,     0,
       0,    18,     0,     0,     0,     0,     0,     0,     4,     0,
       0,    20,     0,    21,    22,    23,     0,     0,     0,     0,
       0,    25,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   205,     0,     5,     0,     6,     0,     0,
       0,     0,   203,     7,    26,     0,     0,     9,    27,    28,
      29,     0,     0,    30,     0,    31,   206,     0,     0,    32,
      12,     0,     0,     0,    13,    14,    34,    15,    35,   204,
      16,     0,    17,    36,    37,     0,     0,     0,     0,    18,
       0,     0,     0,     0,     0,     0,   538,     0,     0,    20,
       0,    21,    22,    23,     0,     0,     0,    38,     0,    25,
       0,     0,     0,     4,     0,     0,     0,     0,     0,     0,
       0,   205,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   534,    26,     0,     0,     0,    27,    28,    29,     0,
       5,    30,     6,    31,   206,     0,     0,    32,     7,     0,
       0,     0,     9,     0,    34,     0,    35,     0,     0,     0,
       0,    36,    37,     0,     0,    12,     0,     0,     0,    13,
      14,     0,    15,     0,   553,    16,     0,    17,     0,     0,
       0,     0,     0,     0,    18,    38,     0,     0,     0,     0,
       0,     4,     0,     0,    20,     0,    21,    22,    23,     0,
       0,     0,     0,     0,    25,   406,   407,   408,   409,   410,
     411,   412,   413,   414,     0,     0,     0,     0,     5,     0,
       6,     0,     0,     0,     0,     0,     7,    26,     0,     0,
       9,    27,    28,    29,     0,     0,    30,     0,    31,     0,
       0,     0,    32,    12,     0,     0,     0,    13,    14,    34,
      15,    35,     0,    16,     0,    17,    36,    37,     0,     0,
       0,     0,    18,   406,   407,   408,   409,   410,   411,   412,
     413,   414,    20,     0,    21,    22,    23,     0,     0,     0,
      38,     0,    25,     0,     0,     0,   406,   407,   408,   409,
     410,   411,   412,   413,   414,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    26,     0,     0,     0,    27,
      28,    29,     0,     0,    30,     0,    31,     0,     0,     0,
      32,     0,     0,     0,     0,     0,     0,    34,     0,    35,
       0,     0,     0,     0,    36,    37,   406,   407,   408,   409,
     410,   411,   412,   415,   414,   416,   417,   418,   419,   420,
     421,   422,   423,   424,   425,     0,     0,     0,    38,     0,
       0,     0,   426,   427,     0,     0,     0,   441,   406,   407,
     408,   409,   410,   411,   412,   413,   414,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     406,   407,   408,   409,   410,   411,   412,   413,   414,     0,
       0,   415,     0,   416,   417,   418,   419,   420,   421,   422,
     423,   424,   425,   406,   407,   408,   409,   410,   411,   412,
     426,   427,     0,     0,   415,   449,   416,   417,   418,   419,
     420,   421,   422,   423,   424,   425,     0,     0,     0,     0,
       0,     0,     0,   426,   427,     0,     0,     0,   492,   406,
     407,   408,   409,   410,   411,   412,   413,   414,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   406,   407,   408,
     409,   410,   411,   412,     0,     0,   416,   417,   418,   419,
     420,   421,   422,   423,   424,   425,   406,   407,   408,   409,
     410,   411,   412,   426,   427,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   415,     0,   416,   417,
     418,   419,   420,   421,   422,   423,   424,   425,   406,   407,
     408,   409,   410,   411,   412,   426,   427,   562,   415,   561,
     416,   417,   418,   419,   420,   421,   422,   423,   424,   425,
       0,     0,     0,     0,     0,     0,     0,   426,   427,     0,
       0,     0,     0,   416,   417,   418,   419,   420,   421,   422,
     423,   424,   425,     0,     0,     0,     0,     0,     0,     0,
     426,   427,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   415,     0,   416,
     417,   418,   419,   420,   421,   422,   423,   424,   425,     0,
       0,     0,     0,     0,     0,     0,   426,   427,   417,   418,
     419,   420,   421,   422,   423,   424,   425,     0,     0,     0,
       0,     4,     0,     0,   426,   427,     0,     0,   418,   419,
     420,   421,   422,   423,   424,   425,     0,     0,     0,     0,
       0,     0,     0,   426,   427,     0,     0,     0,     5,     0,
       6,     0,     0,     0,     0,     0,     7,     8,     0,     0,
       9,   419,   420,   421,   422,   423,   424,   425,     0,     0,
       0,    11,     0,    12,     0,   426,   427,    13,    14,     0,
      15,     0,     0,    16,     0,    17,     0,     0,     0,     0,
       0,     0,    18,     0,     0,     0,     0,     0,     0,     4,
       0,     0,    20,     0,    21,    22,    23,    24,     0,     0,
     175,     0,    25,     0,   176,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     5,     0,     6,     0,
       0,     0,     0,     0,     7,    26,     0,     0,     9,    27,
      28,    29,     0,     0,    30,     0,    31,     0,     0,     0,
      32,    12,     0,     0,     0,    13,    14,    34,    15,    35,
       0,    16,     0,    17,    36,    37,     0,     0,     0,     0,
      18,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      20,     0,    21,    22,    23,     0,     0,     0,     0,     0,
      25,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    26,     0,     0,     0,    27,    28,    29,
       0,     0,    30,     0,    31,     0,     0,    96,    32,    97,
      98,    99,     0,   100,   101,    34,   102,    35,     0,   103,
       0,   104,    36,    37,     0,     0,     0,     0,   105,   106,
     107,   108,     0,   109,   110,   111,   112,   113,     0,   114,
       0,   115,   116,   117,     0,     0,   118,     0,     0,     0,
       0,   119,     0,   120,   121,   122,   123,   124,   125,     0,
     126,   127,   128,   129,   130,     0,     0,   131,     0,     0,
     132,     0,     0,     0,     0,   133,   134,     0,   135,     0,
       0,     0,   136,   137,   138,     0,   139,   140,   141,   142,
     143,     0,   144,     0,   145,   146,   147,   148,   149,   150,
     151,   152,     0,   153,   154,   155,     0,     0,     0,   156,
       0,     0,   157,     0,     0,   158,   159,     0,     0,   160,
     161,   162,     0,     0,     0,   163,     0,   164,   165,   166,
     167,     0,     0,   168
};

static const yytype_int16 yycheck[] =
{
       2,    66,     2,    66,     2,     2,    46,   234,     2,   266,
     212,   235,    33,    67,   238,   280,   240,   208,   209,   243,
     278,     2,   246,    46,   284,   254,    15,   485,    37,    43,
       2,   134,    86,   134,   225,   474,    63,    64,    65,   478,
      49,   113,    69,     3,     4,   117,    46,   476,    46,    63,
     231,    91,    46,   234,    64,    65,   283,   150,     0,    69,
     241,    54,   475,   244,    96,   494,   247,   248,   274,   141,
     150,    85,   174,   176,    46,   176,   150,   491,    38,   172,
     493,   150,   496,    43,   171,    38,    87,   174,   150,   280,
      43,   297,   298,     3,     4,   175,     8,    90,   537,   173,
     357,   540,   283,   284,   118,    65,   175,   174,    13,    14,
      15,   368,    65,   175,   543,   129,   555,   575,   183,     8,
     183,   171,    11,    90,   353,   538,   150,   177,    38,   568,
     559,   312,   313,   314,   315,   316,   317,   318,   552,   171,
     553,   580,   109,   208,   209,   208,   209,   107,     3,     4,
      29,   175,   171,   171,   107,    65,    35,   173,   221,   177,
     225,   385,   225,   171,     3,     4,   229,   190,   177,   177,
     130,   173,   174,   173,   174,   173,   174,   130,   173,   173,
     174,   170,   171,    38,   444,   171,   173,   171,    43,   216,
     174,   177,   173,   174,   375,   249,   250,   107,   389,   253,
     160,   176,   171,   405,   428,   174,   216,   160,    15,   467,
      65,   468,   173,    25,   174,   280,    28,   280,   171,   173,
     130,   174,   171,   447,   173,   406,   407,   173,   409,   410,
     411,   412,   413,   414,   415,   416,   417,   418,   419,   420,
     421,   422,   423,   424,   425,   285,   427,    32,   176,    34,
     160,   272,   107,   158,   159,   160,   161,   162,    15,     3,
       4,   171,   173,   444,   174,   170,   171,     3,     4,   152,
     461,   528,   537,   294,   176,   130,   173,   279,   318,   279,
     482,   279,   173,   285,     8,   279,   173,   552,   176,   491,
       3,     4,     3,     4,   496,   176,   298,   499,     3,     4,
       3,     4,   173,   568,   176,   160,     3,     4,   174,   566,
       3,     4,     3,     4,   174,   580,   171,    38,   174,   174,
     174,   384,   362,   174,   389,    38,   389,   174,   174,     3,
     370,     5,     6,     7,     8,     9,    10,   174,   174,   530,
     405,   381,   174,   364,    65,   174,   573,   387,   174,   174,
     552,   174,    65,   160,   161,   162,   174,   397,   174,   174,
     174,   358,     3,   170,   171,   367,   174,   367,   174,   367,
     370,   595,   370,   367,   174,   174,   370,   558,   174,   174,
     561,   174,   174,   173,   173,   176,   107,   387,   176,   173,
      64,   178,   573,   175,   107,   175,   461,   174,   461,     8,
       8,   158,   159,   160,   161,   162,     8,     8,     8,   130,
       8,   476,    68,   170,   171,     5,   171,   130,   174,   173,
     152,   173,   150,   176,   464,   488,   491,   101,   175,   494,
     178,   496,   177,   473,   150,   175,   476,   175,   175,   160,
     174,     3,   175,   150,   175,   175,   175,   160,   175,   175,
     171,     8,   126,   174,   494,   175,   175,    13,    14,    15,
     175,   174,    18,    19,   138,   530,   175,   530,   152,   175,
      81,   473,   170,   175,   476,   175,   175,   150,   173,   175,
     172,   155,     3,   177,   158,   159,   160,   552,   175,   163,
     164,     3,   494,   175,   173,   173,     5,     8,   172,   175,
     174,   173,   173,   543,     4,   175,   175,   175,   175,   175,
     152,   176,   175,   175,   175,   175,   175,   152,   152,   559,
     173,    44,   366,    39,     2,   473,   384,     2,   601,   247,
     532,    31,   532,    33,   532,     2,    46,     2,   532,    39,
      40,   543,   575,    43,   183,    -1,    -1,    -1,    48,    -1,
      -1,    -1,    -1,    -1,    54,   245,    56,   559,    -1,    75,
      60,    61,    -1,    63,    -1,    -1,    66,    -1,    68,    -1,
      -1,    87,    88,    89,    -1,    75,    -1,    -1,    -1,    95,
      -1,    81,    82,    -1,    -1,    85,    -1,    87,    88,    89,
      90,    -1,    -1,    -1,    -1,    95,    -1,    -1,    -1,     4,
     156,   157,   158,   159,   160,   161,   162,   123,    -1,    -1,
      -1,   127,    -1,    -1,   170,   171,    -1,    -1,   118,    -1,
      -1,    -1,   122,   123,   124,    -1,    31,   127,    33,   129,
      -1,    -1,    -1,   133,    39,    -1,    -1,    -1,    43,   139,
     140,    -1,   142,    48,    -1,    -1,    -1,   147,   148,    -1,
      -1,    56,    -1,    -1,    -1,    60,    61,    -1,    63,    -1,
      -1,    66,    -1,    68,    -1,    -1,    -1,    -1,    -1,    -1,
      75,   171,    -1,    -1,    -1,    -1,    81,   177,    -1,    -1,
      85,    -1,    87,    88,    89,    -1,    -1,    -1,    -1,    -1,
      95,    -1,    -1,    -1,     4,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   118,    -1,    -1,    -1,   122,   123,   124,
      -1,    31,   127,    33,   129,    -1,    -1,    -1,   133,    39,
      -1,    -1,    -1,    43,   139,   140,    -1,   142,    48,    -1,
      -1,    -1,   147,   148,    -1,    -1,    56,    -1,    -1,    -1,
      60,    61,    -1,    63,    -1,    -1,    66,    -1,    68,    -1,
      -1,    -1,    -1,    -1,    -1,    75,   171,    -1,    -1,    -1,
      -1,    81,   177,    -1,    -1,    85,    -1,    87,    88,    89,
      -1,    -1,    -1,    -1,    -1,    95,    -1,    -1,    -1,     4,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,    -1,
      -1,    -1,   122,   123,   124,    -1,    31,   127,    33,   129,
      -1,    -1,    -1,   133,    39,    -1,    -1,    -1,    43,   139,
     140,    -1,   142,    48,    -1,    -1,    -1,   147,   148,    -1,
      -1,    56,    -1,    -1,    -1,    60,    61,    -1,    63,    -1,
      -1,    66,    -1,    68,    -1,    -1,    -1,    -1,    -1,    -1,
      75,   171,    -1,    -1,    -1,    -1,    81,   177,    -1,    -1,
      85,    -1,    87,    88,    89,    -1,    -1,    -1,    -1,    -1,
      95,    -1,    -1,     3,     4,     5,     6,     7,     8,     9,
      10,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   118,    -1,    -1,    -1,   122,   123,   124,
      -1,    31,   127,    33,   129,    -1,    -1,    -1,   133,    39,
      -1,    -1,    -1,    43,   139,   140,    -1,   142,    -1,    -1,
      -1,    -1,   147,   148,    -1,    -1,    56,    -1,    -1,    -1,
      60,    61,    -1,    63,    64,    -1,    66,     3,    68,     5,
       6,     7,     8,     9,    10,    75,   171,    -1,    -1,    -1,
      -1,    -1,   177,    -1,    -1,    85,    -1,    87,    88,    89,
      -1,    -1,    -1,    -1,    -1,    95,    -1,    -1,    -1,    -1,
      -1,   101,    -1,    -1,    -1,    -1,     3,    -1,     5,     6,
       7,     8,     9,    10,    -1,    -1,     4,    -1,   118,    -1,
      -1,    -1,   122,   123,   124,    -1,   126,   127,    64,   129,
      -1,    -1,    -1,   133,    -1,    -1,    -1,    -1,   138,    -1,
     140,    -1,   142,    31,    -1,    33,    -1,   147,   148,    -1,
      -1,    39,    -1,    -1,    -1,   155,    -1,    -1,   158,   159,
     160,    -1,    -1,   163,   164,   101,    -1,    64,    56,    -1,
      -1,    -1,    60,    61,   174,    -1,    -1,    -1,    66,    -1,
      68,    -1,    -1,    -1,    -1,    -1,    -1,    75,    -1,    -1,
     126,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    87,
      88,    89,   138,    -1,   101,    -1,    -1,    95,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   155,
      -1,    -1,   158,   159,   160,    -1,    -1,   163,   164,   126,
      -1,    -1,    -1,    -1,   122,   123,   124,    -1,   174,   127,
      -1,   138,    -1,     4,    -1,   133,    -1,    -1,    -1,    -1,
      -1,    12,   140,    -1,   142,    -1,    -1,    -1,   155,   147,
     148,   158,   159,   160,    -1,    -1,   163,   164,    -1,    -1,
      31,    -1,    33,    -1,    -1,    -1,    -1,   174,    39,    40,
      -1,    -1,    43,    -1,    -1,    -1,    -1,    48,    -1,    -1,
      -1,    -1,    -1,    54,    -1,    56,    -1,    -1,    -1,    60,
      61,    -1,    63,    -1,    -1,    66,    -1,    68,    -1,    -1,
      -1,    -1,    -1,    -1,    75,    -1,    -1,    -1,    -1,    -1,
      81,    82,    -1,    -1,    85,    -1,    87,    88,    89,    90,
      -1,    -1,    -1,    -1,    95,    -1,    -1,    -1,     4,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,    -1,    -1,
      -1,   122,   123,   124,    -1,    31,   127,    33,   129,    -1,
      -1,    -1,   133,    39,    40,    -1,    -1,    43,   139,   140,
      -1,   142,    48,    -1,    -1,    -1,   147,   148,    54,    -1,
      56,    -1,    -1,    -1,    60,    61,    -1,    63,    -1,    -1,
      66,    -1,    68,    -1,    -1,    -1,    -1,    -1,    -1,    75,
     171,    -1,    -1,    -1,    -1,    81,    -1,    -1,    -1,    85,
      -1,    87,    88,    89,    90,    -1,    -1,    -1,    -1,    95,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       4,    -1,   118,    -1,    -1,    -1,   122,   123,   124,    -1,
      -1,   127,    -1,   129,    -1,    -1,    -1,   133,    -1,    -1,
      -1,    -1,    -1,   139,   140,    -1,   142,    31,    -1,    33,
      -1,   147,   148,    -1,    38,    39,    -1,    -1,    -1,    43,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    56,    -1,    -1,   171,    60,    61,    -1,    63,
      -1,    65,    66,    -1,    68,    -1,    -1,    -1,    -1,    -1,
      -1,    75,    -1,    -1,    -1,    -1,    -1,    -1,     4,    -1,
      -1,    85,    -1,    87,    88,    89,    -1,    -1,    -1,    -1,
      -1,    95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   107,    -1,    31,    -1,    33,    -1,    -1,
      -1,    -1,    38,    39,   118,    -1,    -1,    43,   122,   123,
     124,    -1,    -1,   127,    -1,   129,   130,    -1,    -1,   133,
      56,    -1,    -1,    -1,    60,    61,   140,    63,   142,    65,
      66,    -1,    68,   147,   148,    -1,    -1,    -1,    -1,    75,
      -1,    -1,    -1,    -1,    -1,    -1,   160,    -1,    -1,    85,
      -1,    87,    88,    89,    -1,    -1,    -1,   171,    -1,    95,
      -1,    -1,    -1,     4,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   107,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    22,   118,    -1,    -1,    -1,   122,   123,   124,    -1,
      31,   127,    33,   129,   130,    -1,    -1,   133,    39,    -1,
      -1,    -1,    43,    -1,   140,    -1,   142,    -1,    -1,    -1,
      -1,   147,   148,    -1,    -1,    56,    -1,    -1,    -1,    60,
      61,    -1,    63,    -1,   160,    66,    -1,    68,    -1,    -1,
      -1,    -1,    -1,    -1,    75,   171,    -1,    -1,    -1,    -1,
      -1,     4,    -1,    -1,    85,    -1,    87,    88,    89,    -1,
      -1,    -1,    -1,    -1,    95,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    -1,    -1,    -1,    -1,    31,    -1,
      33,    -1,    -1,    -1,    -1,    -1,    39,   118,    -1,    -1,
      43,   122,   123,   124,    -1,    -1,   127,    -1,   129,    -1,
      -1,    -1,   133,    56,    -1,    -1,    -1,    60,    61,   140,
      63,   142,    -1,    66,    -1,    68,   147,   148,    -1,    -1,
      -1,    -1,    75,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    85,    -1,    87,    88,    89,    -1,    -1,    -1,
     171,    -1,    95,    -1,    -1,    -1,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   118,    -1,    -1,    -1,   122,
     123,   124,    -1,    -1,   127,    -1,   129,    -1,    -1,    -1,
     133,    -1,    -1,    -1,    -1,    -1,    -1,   140,    -1,   142,
      -1,    -1,    -1,    -1,   147,   148,    13,    14,    15,    16,
      17,    18,    19,   151,    21,   153,   154,   155,   156,   157,
     158,   159,   160,   161,   162,    -1,    -1,    -1,   171,    -1,
      -1,    -1,   170,   171,    -1,    -1,    -1,   175,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    -1,
      -1,   151,    -1,   153,   154,   155,   156,   157,   158,   159,
     160,   161,   162,    13,    14,    15,    16,    17,    18,    19,
     170,   171,    -1,    -1,   151,   175,   153,   154,   155,   156,
     157,   158,   159,   160,   161,   162,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   170,   171,    -1,    -1,    -1,   175,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    13,    14,    15,
      16,    17,    18,    19,    -1,    -1,   153,   154,   155,   156,
     157,   158,   159,   160,   161,   162,    13,    14,    15,    16,
      17,    18,    19,   170,   171,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   151,    -1,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,    13,    14,
      15,    16,    17,    18,    19,   170,   171,   172,   151,   152,
     153,   154,   155,   156,   157,   158,   159,   160,   161,   162,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   170,   171,    -1,
      -1,    -1,    -1,   153,   154,   155,   156,   157,   158,   159,
     160,   161,   162,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     170,   171,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,    -1,   153,
     154,   155,   156,   157,   158,   159,   160,   161,   162,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   170,   171,   154,   155,
     156,   157,   158,   159,   160,   161,   162,    -1,    -1,    -1,
      -1,     4,    -1,    -1,   170,   171,    -1,    -1,   155,   156,
     157,   158,   159,   160,   161,   162,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   170,   171,    -1,    -1,    -1,    31,    -1,
      33,    -1,    -1,    -1,    -1,    -1,    39,    40,    -1,    -1,
      43,   156,   157,   158,   159,   160,   161,   162,    -1,    -1,
      -1,    54,    -1,    56,    -1,   170,   171,    60,    61,    -1,
      63,    -1,    -1,    66,    -1,    68,    -1,    -1,    -1,    -1,
      -1,    -1,    75,    -1,    -1,    -1,    -1,    -1,    -1,     4,
      -1,    -1,    85,    -1,    87,    88,    89,    90,    -1,    -1,
      93,    -1,    95,    -1,    97,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    31,    -1,    33,    -1,
      -1,    -1,    -1,    -1,    39,   118,    -1,    -1,    43,   122,
     123,   124,    -1,    -1,   127,    -1,   129,    -1,    -1,    -1,
     133,    56,    -1,    -1,    -1,    60,    61,   140,    63,   142,
      -1,    66,    -1,    68,   147,   148,    -1,    -1,    -1,    -1,
      75,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      85,    -1,    87,    88,    89,    -1,    -1,    -1,    -1,    -1,
      95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   118,    -1,    -1,    -1,   122,   123,   124,
      -1,    -1,   127,    -1,   129,    -1,    -1,    23,   133,    25,
      26,    27,    -1,    29,    30,   140,    32,   142,    -1,    35,
      -1,    37,   147,   148,    -1,    -1,    -1,    -1,    44,    45,
      46,    47,    -1,    49,    50,    51,    52,    53,    -1,    55,
      -1,    57,    58,    59,    -1,    -1,    62,    -1,    -1,    -1,
      -1,    67,    -1,    69,    70,    71,    72,    73,    74,    -1,
      76,    77,    78,    79,    80,    -1,    -1,    83,    -1,    -1,
      86,    -1,    -1,    -1,    -1,    91,    92,    -1,    94,    -1,
      -1,    -1,    98,    99,   100,    -1,   102,   103,   104,   105,
     106,    -1,   108,    -1,   110,   111,   112,   113,   114,   115,
     116,   117,    -1,   119,   120,   121,    -1,    -1,    -1,   125,
      -1,    -1,   128,    -1,    -1,   131,   132,    -1,    -1,   135,
     136,   137,    -1,    -1,    -1,   141,    -1,   143,   144,   145,
     146,    -1,    -1,   149
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   180,   181,     0,     4,    31,    33,    39,    40,    43,
      48,    54,    56,    60,    61,    63,    66,    68,    75,    81,
      85,    87,    88,    89,    90,    95,   118,   122,   123,   124,
     127,   129,   133,   139,   140,   142,   147,   148,   171,   185,
     186,   187,   188,   189,   192,   193,   200,   211,   225,   229,
     231,   232,   233,   234,   237,   238,   241,   243,   244,   245,
     246,   248,   249,   250,   251,   252,   254,   256,   273,   274,
     275,   276,     3,     4,   174,     3,     4,     3,     4,   227,
      87,   230,     8,     3,     4,   230,   174,   230,   231,     3,
     227,   199,   200,     3,   227,   231,    23,    25,    26,    27,
      29,    30,    32,    35,    37,    44,    45,    46,    47,    49,
      50,    51,    52,    53,    55,    57,    58,    59,    62,    67,
      69,    70,    71,    72,    73,    74,    76,    77,    78,    79,
      80,    83,    86,    91,    92,    94,    98,    99,   100,   102,
     103,   104,   105,   106,   108,   110,   111,   112,   113,   114,
     115,   116,   117,   119,   120,   121,   125,   128,   131,   132,
     135,   136,   137,   141,   143,   144,   145,   146,   149,   201,
     203,   272,   173,   182,   182,    93,    97,   191,   211,   232,
     237,   243,   247,   254,   273,   276,   173,   173,   176,   173,
     176,   173,   184,   173,   152,   242,   176,   255,   256,   255,
     255,     3,     4,    38,    65,   107,   130,   160,   174,   205,
     228,   257,   258,   271,   211,   273,   274,   276,   255,   173,
       8,   176,   173,   274,   176,   254,   134,   176,   174,   174,
     174,   174,   174,   174,   174,   174,   174,   174,   174,   174,
     174,   174,   174,   174,   174,   174,   174,   174,   174,   174,
     174,   174,   174,   174,   150,   172,    12,    82,   185,   190,
     193,   232,   234,   245,   246,   249,   177,     3,     3,     4,
     176,   271,   235,   109,   239,   243,     3,     4,   176,   183,
     253,   257,   257,   178,   171,   174,   198,   255,   175,   208,
     209,   210,   228,   175,   218,   257,   267,   174,   221,     8,
     228,     3,     5,     6,     7,     8,     9,    10,    64,   101,
     126,   138,   155,   158,   159,   160,   163,   164,   174,   214,
     215,   216,   214,   217,     8,     8,   202,   217,   216,     8,
       8,   216,     8,   216,   214,    68,   216,   212,   213,   214,
     272,   216,   212,   214,   274,   274,     8,    11,   204,     5,
     277,   274,   203,   171,   174,   173,   184,   177,   199,   236,
     152,    96,   199,   223,   240,   173,   176,   183,   177,   185,
     200,   252,   257,   175,   217,   160,   213,   194,   195,   196,
     197,   200,   254,   177,   150,   178,   177,   199,   219,   150,
     223,   177,   200,   220,   223,   175,   175,   174,   214,   214,
     214,   214,   214,   214,   214,   254,    13,    14,    15,    16,
      17,    18,    19,    20,    21,   151,   153,   154,   155,   156,
     157,   158,   159,   160,   161,   162,   170,   171,   150,   175,
     175,   175,   150,   175,   175,   175,   175,   175,   175,   175,
     175,   175,     3,   175,   150,   175,   175,   150,   175,   175,
     175,   175,   175,   170,   175,   175,   203,     8,   184,   246,
     152,   254,   173,   177,   199,   224,   177,   189,   177,   184,
     172,   172,   175,   150,   254,   160,   174,   198,   205,   228,
     263,   265,   266,   210,   216,   254,   276,   257,   175,   173,
     173,   254,   175,   160,   174,   198,   205,   259,   261,   262,
     214,   214,     3,   214,   214,   214,   214,   214,   214,   214,
     214,   214,   214,   214,   214,   214,   214,   214,   214,   214,
       3,   214,   216,     8,   175,   213,   216,     5,   175,   257,
     254,   173,   183,   184,    22,   197,   265,   253,   160,   194,
     205,   264,   265,   174,   198,   263,   269,   270,   173,   226,
     228,   261,   253,   160,   194,   205,   260,   261,   175,   174,
     198,   152,   172,   175,   184,   257,   177,   265,   253,   175,
     265,   175,   194,   152,   268,   150,   173,   176,   175,   261,
     253,   175,   265,   175,   214,   194,   214,   184,   265,   175,
     217,   269,   206,   265,   175,    37,    49,   177,   207,   216,
     152,   152,   173,   222,   223,   222,   173
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
#line 330 "parser.y"
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
#line 343 "parser.y"
    { (yyval.stmt_list) = NULL; ;}
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 344 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_reference((yyvsp[(2) - (2)].type))); ;}
    break;

  case 5:

/* Line 1455 of yacc.c  */
#line 345 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_type_decl((yyvsp[(2) - (2)].type))); ;}
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 346 "parser.y"
    { (yyval.stmt_list) = (yyvsp[(1) - (3)].stmt_list);
						  reg_type((yyvsp[(2) - (3)].type), (yyvsp[(2) - (3)].type)->name, 0);
						;}
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 349 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_type_decl((yyvsp[(2) - (2)].type)));
						  reg_type((yyvsp[(2) - (2)].type), (yyvsp[(2) - (2)].type)->name, 0);
						;}
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 352 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_module((yyvsp[(2) - (2)].type))); ;}
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 353 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_library((yyvsp[(2) - (2)].typelib))); ;}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 354 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), (yyvsp[(2) - (2)].statement)); ;}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 357 "parser.y"
    { (yyval.stmt_list) = NULL; ;}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 358 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_reference((yyvsp[(2) - (2)].type))); ;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 359 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_type_decl((yyvsp[(2) - (2)].type))); ;}
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 360 "parser.y"
    { (yyval.stmt_list) = (yyvsp[(1) - (3)].stmt_list); reg_type((yyvsp[(2) - (3)].type), (yyvsp[(2) - (3)].type)->name, 0); ;}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 361 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_type_decl((yyvsp[(2) - (2)].type)));
						  reg_type((yyvsp[(2) - (2)].type), (yyvsp[(2) - (2)].type)->name, 0);
						;}
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 364 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_module((yyvsp[(2) - (2)].type))); ;}
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 365 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), (yyvsp[(2) - (2)].statement)); ;}
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 366 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_importlib((yyvsp[(2) - (2)].str))); ;}
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 367 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_library((yyvsp[(2) - (2)].typelib))); ;}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 370 "parser.y"
    { (yyval.stmt_list) = NULL; ;}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 371 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), (yyvsp[(2) - (2)].statement)); ;}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 379 "parser.y"
    { (yyval.statement) = make_statement_cppquote((yyvsp[(1) - (1)].str)); ;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 380 "parser.y"
    { (yyval.statement) = make_statement_type_decl((yyvsp[(1) - (2)].type)); ;}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 381 "parser.y"
    { (yyval.statement) = make_statement_declaration((yyvsp[(1) - (2)].var)); ;}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 382 "parser.y"
    { (yyval.statement) = make_statement_import((yyvsp[(1) - (1)].str)); ;}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 383 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (2)].statement); ;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 390 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type); (yyval.type)->attrs = check_enum_attrs((yyvsp[(1) - (2)].attr_list)); ;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 391 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type); (yyval.type)->attrs = check_struct_attrs((yyvsp[(1) - (2)].attr_list)); ;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 392 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type); (yyval.type)->attrs = check_union_attrs((yyvsp[(1) - (2)].attr_list)); ;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 395 "parser.y"
    { (yyval.str) = (yyvsp[(3) - (4)].str); ;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 397 "parser.y"
    { assert(yychar == YYEMPTY);
						  (yyval.import) = xmalloc(sizeof(struct _import_t));
						  (yyval.import)->name = (yyvsp[(2) - (3)].str);
						  (yyval.import)->import_performed = do_import((yyvsp[(2) - (3)].str));
						  if (!(yyval.import)->import_performed) yychar = aEOF;
						;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 405 "parser.y"
    { (yyval.str) = (yyvsp[(1) - (3)].import)->name;
						  if ((yyvsp[(1) - (3)].import)->import_performed) pop_import();
						  free((yyvsp[(1) - (3)].import));
						;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 412 "parser.y"
    { (yyval.str) = (yyvsp[(3) - (5)].str); if(!parse_only) add_importlib((yyvsp[(3) - (5)].str)); ;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 415 "parser.y"
    { (yyval.str) = (yyvsp[(2) - (2)].str); ;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 417 "parser.y"
    { (yyval.typelib) = make_library((yyvsp[(2) - (3)].str), check_library_attrs((yyvsp[(2) - (3)].str), (yyvsp[(1) - (3)].attr_list)));
						  if (!parse_only) start_typelib((yyval.typelib));
						;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 422 "parser.y"
    { (yyval.typelib) = (yyvsp[(1) - (4)].typelib);
						  (yyval.typelib)->stmts = (yyvsp[(2) - (4)].stmt_list);
						  if (!parse_only) end_typelib();
						;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 428 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 432 "parser.y"
    { check_arg_attrs((yyvsp[(1) - (1)].var)); (yyval.var_list) = append_var( NULL, (yyvsp[(1) - (1)].var) ); ;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 433 "parser.y"
    { check_arg_attrs((yyvsp[(3) - (3)].var)); (yyval.var_list) = append_var( (yyvsp[(1) - (3)].var_list), (yyvsp[(3) - (3)].var) ); ;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 437 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[(1) - (3)].var_list), make_var(strdup("...")) ); ;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 441 "parser.y"
    { if ((yyvsp[(2) - (3)].declspec)->stgclass != STG_NONE && (yyvsp[(2) - (3)].declspec)->stgclass != STG_REGISTER)
						    error_loc("invalid storage class for function parameter\n");
						  (yyval.var) = declare_var((yyvsp[(1) - (3)].attr_list), (yyvsp[(2) - (3)].declspec), (yyvsp[(3) - (3)].declarator), TRUE);
						  free((yyvsp[(2) - (3)].declspec)); free((yyvsp[(3) - (3)].declarator));
						;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 446 "parser.y"
    { if ((yyvsp[(1) - (2)].declspec)->stgclass != STG_NONE && (yyvsp[(1) - (2)].declspec)->stgclass != STG_REGISTER)
						    error_loc("invalid storage class for function parameter\n");
						  (yyval.var) = declare_var(NULL, (yyvsp[(1) - (2)].declspec), (yyvsp[(2) - (2)].declarator), TRUE);
						  free((yyvsp[(1) - (2)].declspec)); free((yyvsp[(2) - (2)].declarator));
						;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 453 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (3)].expr); ;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 454 "parser.y"
    { (yyval.expr) = make_expr(EXPR_VOID); ;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 457 "parser.y"
    { (yyval.attr_list) = NULL; ;}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 462 "parser.y"
    { (yyval.attr_list) = (yyvsp[(2) - (3)].attr_list); ;}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 465 "parser.y"
    { (yyval.attr_list) = append_attr( NULL, (yyvsp[(1) - (1)].attr) ); ;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 466 "parser.y"
    { (yyval.attr_list) = append_attr( (yyvsp[(1) - (3)].attr_list), (yyvsp[(3) - (3)].attr) ); ;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 467 "parser.y"
    { (yyval.attr_list) = append_attr( (yyvsp[(1) - (4)].attr_list), (yyvsp[(4) - (4)].attr) ); ;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 470 "parser.y"
    { (yyval.str_list) = append_str( NULL, (yyvsp[(1) - (1)].str) ); ;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 471 "parser.y"
    { (yyval.str_list) = append_str( (yyvsp[(1) - (3)].str_list), (yyvsp[(3) - (3)].str) ); ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 474 "parser.y"
    { (yyval.attr) = NULL; ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 475 "parser.y"
    { (yyval.attr) = make_attr(ATTR_AGGREGATABLE); ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 476 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ANNOTATION, (yyvsp[(3) - (4)].str)); ;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 477 "parser.y"
    { (yyval.attr) = make_attr(ATTR_APPOBJECT); ;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 478 "parser.y"
    { (yyval.attr) = make_attr(ATTR_ASYNC); ;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 479 "parser.y"
    { (yyval.attr) = make_attr(ATTR_AUTO_HANDLE); ;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 480 "parser.y"
    { (yyval.attr) = make_attr(ATTR_BINDABLE); ;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 481 "parser.y"
    { (yyval.attr) = make_attr(ATTR_BROADCAST); ;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 482 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_CALLAS, (yyvsp[(3) - (4)].var)); ;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 483 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_CASE, (yyvsp[(3) - (4)].expr_list)); ;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 484 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); ;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 485 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_DONT_SERIALIZE */ ;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 486 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_SERIALIZE */ ;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 487 "parser.y"
    { (yyval.attr) = make_attr(ATTR_CONTROL); ;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 488 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULT); ;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 489 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULTCOLLELEM); ;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 490 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_DEFAULTVALUE, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 491 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULTVTABLE); ;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 492 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DISPLAYBIND); ;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 493 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_DLLNAME, (yyvsp[(3) - (4)].str)); ;}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 494 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DUAL); ;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 495 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ENDPOINT, (yyvsp[(3) - (4)].str_list)); ;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 496 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ENTRY, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 497 "parser.y"
    { (yyval.attr) = make_attr(ATTR_EXPLICIT_HANDLE); ;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 498 "parser.y"
    { (yyval.attr) = make_attr(ATTR_HANDLE); ;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 499 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPCONTEXT, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 500 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPFILE, (yyvsp[(3) - (4)].str)); ;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 501 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRING, (yyvsp[(3) - (4)].str)); ;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 502 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRINGCONTEXT, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 503 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRINGDLL, (yyvsp[(3) - (4)].str)); ;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 504 "parser.y"
    { (yyval.attr) = make_attr(ATTR_HIDDEN); ;}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 505 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ID, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 506 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IDEMPOTENT); ;}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 507 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_IIDIS, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 508 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IMMEDIATEBIND); ;}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 509 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_IMPLICIT_HANDLE, (yyvsp[(4) - (5)].str)); ;}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 510 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IN); ;}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 511 "parser.y"
    { (yyval.attr) = make_attr(ATTR_INPUTSYNC); ;}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 512 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_LENGTHIS, (yyvsp[(3) - (4)].expr_list)); ;}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 513 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_LIBLCID, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 514 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PARAMLCID); ;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 515 "parser.y"
    { (yyval.attr) = make_attr(ATTR_LOCAL); ;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 516 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONBROWSABLE); ;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 517 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONCREATABLE); ;}
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 518 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONEXTENSIBLE); ;}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 519 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OBJECT); ;}
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 520 "parser.y"
    { (yyval.attr) = make_attr(ATTR_ODL); ;}
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 521 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OLEAUTOMATION); ;}
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 522 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OPTIONAL); ;}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 523 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OUT); ;}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 524 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_POINTERDEFAULT, (yyvsp[(3) - (4)].num)); ;}
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 525 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPGET); ;}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 526 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPPUT); ;}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 527 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPPUTREF); ;}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 528 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PUBLIC); ;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 530 "parser.y"
    { expr_list_t *list = append_expr( NULL, (yyvsp[(3) - (6)].expr) );
						  list = append_expr( list, (yyvsp[(5) - (6)].expr) );
						  (yyval.attr) = make_attrp(ATTR_RANGE, list); ;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 533 "parser.y"
    { (yyval.attr) = make_attr(ATTR_READONLY); ;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 534 "parser.y"
    { (yyval.attr) = make_attr(ATTR_REQUESTEDIT); ;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 535 "parser.y"
    { (yyval.attr) = make_attr(ATTR_RESTRICTED); ;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 536 "parser.y"
    { (yyval.attr) = make_attr(ATTR_RETVAL); ;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 537 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SIZEIS, (yyvsp[(3) - (4)].expr_list)); ;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 538 "parser.y"
    { (yyval.attr) = make_attr(ATTR_SOURCE); ;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 539 "parser.y"
    { (yyval.attr) = make_attr(ATTR_STRICTCONTEXTHANDLE); ;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 540 "parser.y"
    { (yyval.attr) = make_attr(ATTR_STRING); ;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 541 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SWITCHIS, (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 542 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SWITCHTYPE, (yyvsp[(3) - (4)].type)); ;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 543 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_TRANSMITAS, (yyvsp[(3) - (4)].type)); ;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 544 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_UUID, (yyvsp[(3) - (4)].uuid)); ;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 545 "parser.y"
    { (yyval.attr) = make_attr(ATTR_V1ENUM); ;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 546 "parser.y"
    { (yyval.attr) = make_attr(ATTR_VARARG); ;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 547 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_VERSION, (yyvsp[(3) - (4)].num)); ;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 548 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_WIREMARSHAL, (yyvsp[(3) - (4)].type)); ;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 549 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_POINTERTYPE, (yyvsp[(1) - (1)].num)); ;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 554 "parser.y"
    { if (!is_valid_uuid((yyvsp[(1) - (1)].str)))
						    error_loc("invalid UUID: %s\n", (yyvsp[(1) - (1)].str));
						  (yyval.uuid) = parse_uuid((yyvsp[(1) - (1)].str)); ;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 559 "parser.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); ;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 560 "parser.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); ;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 561 "parser.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); ;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 562 "parser.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); ;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 565 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 566 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[(1) - (2)].var_list), (yyvsp[(2) - (2)].var) ); ;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 569 "parser.y"
    { attr_t *a = make_attrp(ATTR_CASE, append_expr( NULL, (yyvsp[(2) - (4)].expr) ));
						  (yyval.var) = (yyvsp[(4) - (4)].var); if (!(yyval.var)) (yyval.var) = make_var(NULL);
						  (yyval.var)->attrs = append_attr( (yyval.var)->attrs, a );
						;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 573 "parser.y"
    { attr_t *a = make_attr(ATTR_DEFAULT);
						  (yyval.var) = (yyvsp[(3) - (3)].var); if (!(yyval.var)) (yyval.var) = make_var(NULL);
						  (yyval.var)->attrs = append_attr( (yyval.var)->attrs, a );
						;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 579 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 580 "parser.y"
    { (yyval.var_list) = (yyvsp[(1) - (2)].var_list); ;}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 584 "parser.y"
    { if (!(yyvsp[(1) - (1)].var)->eval)
						    (yyvsp[(1) - (1)].var)->eval = make_exprl(EXPR_NUM, 0 /* default for first enum entry */);
                                                  (yyval.var_list) = append_var( NULL, (yyvsp[(1) - (1)].var) );
						;}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 588 "parser.y"
    { if (!(yyvsp[(3) - (3)].var)->eval)
                                                  {
                                                    var_t *last = LIST_ENTRY( list_tail((yyval.var_list)), var_t, entry );
                                                    (yyvsp[(3) - (3)].var)->eval = make_exprl(EXPR_NUM, last->eval->cval + 1);
                                                  }
                                                  (yyval.var_list) = append_var( (yyvsp[(1) - (3)].var_list), (yyvsp[(3) - (3)].var) );
						;}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 597 "parser.y"
    { (yyval.var) = reg_const((yyvsp[(1) - (3)].var));
						  (yyval.var)->eval = (yyvsp[(3) - (3)].expr);
                                                  (yyval.var)->type = type_new_int(TYPE_BASIC_INT, 0);
						;}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 601 "parser.y"
    { (yyval.var) = reg_const((yyvsp[(1) - (1)].var));
                                                  (yyval.var)->type = type_new_int(TYPE_BASIC_INT, 0);
						;}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 606 "parser.y"
    { (yyval.type) = type_new_enum((yyvsp[(2) - (5)].str), TRUE, (yyvsp[(4) - (5)].var_list)); ;}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 609 "parser.y"
    { (yyval.expr_list) = append_expr( NULL, (yyvsp[(1) - (1)].expr) ); ;}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 610 "parser.y"
    { (yyval.expr_list) = append_expr( (yyvsp[(1) - (3)].expr_list), (yyvsp[(3) - (3)].expr) ); ;}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 623 "parser.y"
    { (yyval.expr) = make_expr(EXPR_VOID); ;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 627 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_NUM, (yyvsp[(1) - (1)].num)); ;}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 628 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_HEXNUM, (yyvsp[(1) - (1)].num)); ;}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 629 "parser.y"
    { (yyval.expr) = make_exprd(EXPR_DOUBLE, (yyvsp[(1) - (1)].dbl)); ;}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 630 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_TRUEFALSE, 0); ;}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 631 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_NUM, 0); ;}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 632 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_TRUEFALSE, 1); ;}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 633 "parser.y"
    { (yyval.expr) = make_exprs(EXPR_STRLIT, (yyvsp[(1) - (1)].str)); ;}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 634 "parser.y"
    { (yyval.expr) = make_exprs(EXPR_WSTRLIT, (yyvsp[(1) - (1)].str)); ;}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 635 "parser.y"
    { (yyval.expr) = make_exprs(EXPR_CHARCONST, (yyvsp[(1) - (1)].str)); ;}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 636 "parser.y"
    { (yyval.expr) = make_exprs(EXPR_IDENTIFIER, (yyvsp[(1) - (1)].str)); ;}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 637 "parser.y"
    { (yyval.expr) = make_expr3(EXPR_COND, (yyvsp[(1) - (5)].expr), (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].expr)); ;}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 638 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_LOGOR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 639 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_LOGAND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 640 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_OR , (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 641 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_XOR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 642 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_AND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 643 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_EQUALITY, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 644 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_INEQUALITY, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 645 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_GTR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 646 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_LESS, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 647 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_GTREQL, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 648 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_LESSEQL, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 649 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SHL, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 650 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SHR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 651 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_ADD, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 652 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SUB, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 653 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MOD, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 654 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MUL, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 655 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_DIV, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 656 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_LOGNOT, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 657 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_NOT, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 658 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_POS, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 659 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_NEG, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 660 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_ADDRESSOF, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 661 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_PPTR, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 662 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MEMBER, make_expr1(EXPR_PPTR, (yyvsp[(1) - (3)].expr)), make_exprs(EXPR_IDENTIFIER, (yyvsp[(3) - (3)].str))); ;}
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 663 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MEMBER, (yyvsp[(1) - (3)].expr), make_exprs(EXPR_IDENTIFIER, (yyvsp[(3) - (3)].str))); ;}
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 665 "parser.y"
    { (yyval.expr) = make_exprt(EXPR_CAST, declare_var(NULL, (yyvsp[(2) - (5)].declspec), (yyvsp[(3) - (5)].declarator), 0), (yyvsp[(5) - (5)].expr)); free((yyvsp[(2) - (5)].declspec)); free((yyvsp[(3) - (5)].declarator)); ;}
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 667 "parser.y"
    { (yyval.expr) = make_exprt(EXPR_SIZEOF, declare_var(NULL, (yyvsp[(3) - (5)].declspec), (yyvsp[(4) - (5)].declarator), 0), NULL); free((yyvsp[(3) - (5)].declspec)); free((yyvsp[(4) - (5)].declarator)); ;}
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 668 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_ARRAY, (yyvsp[(1) - (4)].expr), (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 669 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (3)].expr); ;}
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 672 "parser.y"
    { (yyval.expr_list) = append_expr( NULL, (yyvsp[(1) - (1)].expr) ); ;}
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 673 "parser.y"
    { (yyval.expr_list) = append_expr( (yyvsp[(1) - (3)].expr_list), (yyvsp[(3) - (3)].expr) ); ;}
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 676 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr);
						  if (!(yyval.expr)->is_const)
						      error_loc("expression is not an integer constant\n");
						;}
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 682 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr);
						  if (!(yyval.expr)->is_const && (yyval.expr)->type != EXPR_STRLIT && (yyval.expr)->type != EXPR_WSTRLIT)
						      error_loc("expression is not constant\n");
						;}
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 688 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 689 "parser.y"
    { (yyval.var_list) = append_var_list((yyvsp[(1) - (2)].var_list), (yyvsp[(2) - (2)].var_list)); ;}
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 693 "parser.y"
    { const char *first = LIST_ENTRY(list_head((yyvsp[(3) - (4)].declarator_list)), declarator_t, entry)->var->name;
						  check_field_attrs(first, (yyvsp[(1) - (4)].attr_list));
						  (yyval.var_list) = set_var_types((yyvsp[(1) - (4)].attr_list), (yyvsp[(2) - (4)].declspec), (yyvsp[(3) - (4)].declarator_list));
						;}
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 697 "parser.y"
    { var_t *v = make_var(NULL);
						  v->type = (yyvsp[(2) - (3)].type); v->attrs = (yyvsp[(1) - (3)].attr_list);
						  (yyval.var_list) = append_var(NULL, v);
						;}
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 704 "parser.y"
    { (yyval.var) = (yyvsp[(1) - (2)].var); ;}
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 705 "parser.y"
    { (yyval.var) = make_var(NULL); (yyval.var)->attrs = (yyvsp[(1) - (2)].attr_list); ;}
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 708 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 709 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[(1) - (2)].var_list), (yyvsp[(2) - (2)].var) ); ;}
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 713 "parser.y"
    { (yyval.var) = (yyvsp[(1) - (2)].var); ;}
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 714 "parser.y"
    { (yyval.var) = NULL; ;}
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 717 "parser.y"
    { (yyval.var) = declare_var(check_field_attrs((yyvsp[(3) - (3)].declarator)->var->name, (yyvsp[(1) - (3)].attr_list)),
						                (yyvsp[(2) - (3)].declspec), (yyvsp[(3) - (3)].declarator), FALSE);
						  free((yyvsp[(3) - (3)].declarator));
						;}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 724 "parser.y"
    { var_t *v;
						  v = declare_var(check_function_attrs((yyvsp[(3) - (3)].declarator)->var->name, (yyvsp[(1) - (3)].attr_list)),
						               (yyvsp[(2) - (3)].declspec), (yyvsp[(3) - (3)].declarator), FALSE);
						  free((yyvsp[(3) - (3)].declarator));
						  (yyval.func) = make_func(v);
						;}
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 734 "parser.y"
    { (yyval.var) = declare_var((yyvsp[(1) - (3)].attr_list), (yyvsp[(2) - (3)].declspec), (yyvsp[(3) - (3)].declarator), FALSE);
						  free((yyvsp[(3) - (3)].declarator));
						;}
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 737 "parser.y"
    { (yyval.var) = declare_var(NULL, (yyvsp[(1) - (2)].declspec), (yyvsp[(2) - (2)].declarator), FALSE);
						  free((yyvsp[(2) - (2)].declarator));
						;}
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 742 "parser.y"
    { (yyval.var) = NULL; ;}
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 746 "parser.y"
    { (yyval.str) = NULL; ;}
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 747 "parser.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); ;}
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 748 "parser.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); ;}
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 751 "parser.y"
    { (yyval.var) = make_var((yyvsp[(1) - (1)].str)); ;}
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 753 "parser.y"
    { (yyval.var) = make_var((yyvsp[(1) - (1)].str)); ;}
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 756 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 757 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 759 "parser.y"
    { (yyval.type) = type_new_int(type_basic_get_type((yyvsp[(2) - (2)].type)), -1); ;}
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 760 "parser.y"
    { (yyval.type) = type_new_int(type_basic_get_type((yyvsp[(2) - (2)].type)), 1); ;}
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 761 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT, 1); ;}
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 762 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 763 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 764 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 765 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 766 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 773 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT, 0); ;}
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 774 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT16, 0); ;}
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 775 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT8, 0); ;}
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 776 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT32, 0); ;}
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 777 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_HYPER, 0); ;}
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 778 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT64, 0); ;}
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 779 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_CHAR, 0); ;}
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 780 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT3264, 0); ;}
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 783 "parser.y"
    { (yyval.type) = type_new_coclass((yyvsp[(2) - (2)].str)); ;}
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 784 "parser.y"
    { (yyval.type) = find_type((yyvsp[(2) - (2)].str), 0);
						  if (type_get_type_detect_alias((yyval.type)) != TYPE_COCLASS)
						    error_loc("%s was not declared a coclass at %s:%d\n",
							      (yyvsp[(2) - (2)].str), (yyval.type)->loc_info.input_name,
							      (yyval.type)->loc_info.line_number);
						;}
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 792 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type);
						  check_def((yyval.type));
						  (yyval.type)->attrs = check_coclass_attrs((yyvsp[(2) - (2)].type)->name, (yyvsp[(1) - (2)].attr_list));
						;}
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 799 "parser.y"
    { (yyval.type) = type_coclass_define((yyvsp[(1) - (5)].type), (yyvsp[(3) - (5)].ifref_list)); ;}
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 802 "parser.y"
    { (yyval.ifref_list) = NULL; ;}
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 803 "parser.y"
    { (yyval.ifref_list) = append_ifref( (yyvsp[(1) - (2)].ifref_list), (yyvsp[(2) - (2)].ifref) ); ;}
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 807 "parser.y"
    { (yyval.ifref) = make_ifref((yyvsp[(2) - (2)].type)); (yyval.ifref)->attrs = (yyvsp[(1) - (2)].attr_list); ;}
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 810 "parser.y"
    { (yyval.type) = get_type(TYPE_INTERFACE, (yyvsp[(2) - (2)].str), 0); ;}
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 811 "parser.y"
    { (yyval.type) = get_type(TYPE_INTERFACE, (yyvsp[(2) - (2)].str), 0); ;}
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 814 "parser.y"
    { attr_t *attrs;
						  is_object_interface = TRUE;
						  (yyval.type) = (yyvsp[(2) - (2)].type);
						  check_def((yyval.type));
						  attrs = make_attr(ATTR_DISPINTERFACE);
						  (yyval.type)->attrs = append_attr( check_dispiface_attrs((yyvsp[(2) - (2)].type)->name, (yyvsp[(1) - (2)].attr_list)), attrs );
						  (yyval.type)->defined = TRUE;
						;}
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 824 "parser.y"
    { (yyval.var_list) = NULL; ;}
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 825 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[(1) - (3)].var_list), (yyvsp[(2) - (3)].var) ); ;}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 828 "parser.y"
    { (yyval.stmt_list) = NULL; ;}
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 829 "parser.y"
    { (yyval.stmt_list) = append_func( (yyvsp[(1) - (3)].stmt_list), (yyvsp[(2) - (3)].func) ); ;}
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 835 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (5)].type);
						  type_dispinterface_define((yyval.type), (yyvsp[(3) - (5)].var_list), (yyvsp[(4) - (5)].stmt_list));
						;}
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 839 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (5)].type);
						  type_dispinterface_define_from_iface((yyval.type), (yyvsp[(3) - (5)].type));
						;}
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 844 "parser.y"
    { (yyval.type) = NULL; ;}
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 845 "parser.y"
    { (yyval.type) = find_type_or_error2((yyvsp[(2) - (2)].str), 0); is_object_interface = 1; ;}
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 848 "parser.y"
    { (yyval.type) = get_type(TYPE_INTERFACE, (yyvsp[(2) - (2)].str), 0); ;}
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 849 "parser.y"
    { (yyval.type) = get_type(TYPE_INTERFACE, (yyvsp[(2) - (2)].str), 0); ;}
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 852 "parser.y"
    { (yyval.ifinfo).interface = (yyvsp[(2) - (2)].type);
						  (yyval.ifinfo).old_pointer_default = pointer_default;
						  if (is_attr((yyvsp[(1) - (2)].attr_list), ATTR_POINTERDEFAULT))
						    pointer_default = get_attrv((yyvsp[(1) - (2)].attr_list), ATTR_POINTERDEFAULT);
						  check_def((yyvsp[(2) - (2)].type));
						  (yyvsp[(2) - (2)].type)->attrs = check_iface_attrs((yyvsp[(2) - (2)].type)->name, (yyvsp[(1) - (2)].attr_list));
						  is_object_interface = is_object((yyvsp[(2) - (2)].type));
						  (yyvsp[(2) - (2)].type)->defined = TRUE;
						;}
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 864 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (6)].ifinfo).interface;
						  type_interface_define((yyval.type), (yyvsp[(2) - (6)].type), (yyvsp[(4) - (6)].stmt_list));
						  pointer_default = (yyvsp[(1) - (6)].ifinfo).old_pointer_default;
						;}
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 872 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (8)].ifinfo).interface;
						  type_interface_define((yyval.type), find_type_or_error2((yyvsp[(3) - (8)].str), 0), (yyvsp[(6) - (8)].stmt_list));
						  pointer_default = (yyvsp[(1) - (8)].ifinfo).old_pointer_default;
						;}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 876 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (2)].type); ;}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 880 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (2)].type); ;}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 881 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (2)].type); ;}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 884 "parser.y"
    { (yyval.type) = type_new_module((yyvsp[(2) - (2)].str)); ;}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 885 "parser.y"
    { (yyval.type) = type_new_module((yyvsp[(2) - (2)].str)); ;}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 888 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type);
						  (yyval.type)->attrs = check_module_attrs((yyvsp[(2) - (2)].type)->name, (yyvsp[(1) - (2)].attr_list));
						;}
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 894 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (5)].type);
                                                  type_module_define((yyval.type), (yyvsp[(3) - (5)].stmt_list));
						;}
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 900 "parser.y"
    { (yyval.stgclass) = STG_EXTERN; ;}
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 901 "parser.y"
    { (yyval.stgclass) = STG_STATIC; ;}
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 902 "parser.y"
    { (yyval.stgclass) = STG_REGISTER; ;}
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 906 "parser.y"
    { (yyval.attr) = make_attr(ATTR_INLINE); ;}
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 910 "parser.y"
    { (yyval.attr) = make_attr(ATTR_CONST); ;}
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 913 "parser.y"
    { (yyval.attr_list) = NULL; ;}
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 914 "parser.y"
    { (yyval.attr_list) = append_attr((yyvsp[(1) - (2)].attr_list), (yyvsp[(2) - (2)].attr)); ;}
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 917 "parser.y"
    { (yyval.declspec) = make_decl_spec((yyvsp[(1) - (2)].type), (yyvsp[(2) - (2)].declspec), NULL, NULL, STG_NONE); ;}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 919 "parser.y"
    { (yyval.declspec) = make_decl_spec((yyvsp[(2) - (3)].type), (yyvsp[(1) - (3)].declspec), (yyvsp[(3) - (3)].declspec), NULL, STG_NONE); ;}
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 922 "parser.y"
    { (yyval.declspec) = NULL; ;}
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 927 "parser.y"
    { (yyval.declspec) = make_decl_spec(NULL, (yyvsp[(2) - (2)].declspec), NULL, (yyvsp[(1) - (2)].attr), STG_NONE); ;}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 928 "parser.y"
    { (yyval.declspec) = make_decl_spec(NULL, (yyvsp[(2) - (2)].declspec), NULL, (yyvsp[(1) - (2)].attr), STG_NONE); ;}
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 929 "parser.y"
    { (yyval.declspec) = make_decl_spec(NULL, (yyvsp[(2) - (2)].declspec), NULL, NULL, (yyvsp[(1) - (2)].stgclass)); ;}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 934 "parser.y"
    { (yyval.declarator) = (yyvsp[(3) - (3)].declarator); (yyval.declarator)->type = append_ptrchain_type((yyval.declarator)->type, type_new_pointer(pointer_default, NULL, (yyvsp[(2) - (3)].attr_list))); ;}
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 935 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (2)].declarator); (yyval.declarator)->type->attrs = append_attr((yyval.declarator)->type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[(1) - (2)].str))); ;}
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 940 "parser.y"
    { (yyval.declarator) = make_declarator((yyvsp[(1) - (1)].var)); ;}
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 941 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (3)].declarator); ;}
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 942 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (2)].declarator); (yyval.declarator)->array = append_array((yyval.declarator)->array, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 943 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (4)].declarator);
						  (yyval.declarator)->func_type = append_ptrchain_type((yyval.declarator)->type, type_new_function((yyvsp[(3) - (4)].var_list)));
						  (yyval.declarator)->type = NULL;
						;}
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 952 "parser.y"
    { (yyval.declarator) = (yyvsp[(3) - (3)].declarator); (yyval.declarator)->type = append_ptrchain_type((yyval.declarator)->type, type_new_pointer(pointer_default, NULL, (yyvsp[(2) - (3)].attr_list))); ;}
    break;

  case 294:

/* Line 1455 of yacc.c  */
#line 953 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (2)].declarator); (yyval.declarator)->type->attrs = append_attr((yyval.declarator)->type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[(1) - (2)].str))); ;}
    break;

  case 296:

/* Line 1455 of yacc.c  */
#line 960 "parser.y"
    { (yyval.declarator) = (yyvsp[(3) - (3)].declarator); (yyval.declarator)->type = append_ptrchain_type((yyval.declarator)->type, type_new_pointer(pointer_default, NULL, (yyvsp[(2) - (3)].attr_list))); ;}
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 961 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (2)].declarator); (yyval.declarator)->type->attrs = append_attr((yyval.declarator)->type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[(1) - (2)].str))); ;}
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 965 "parser.y"
    { (yyval.declarator) = make_declarator(NULL); ;}
    break;

  case 300:

/* Line 1455 of yacc.c  */
#line 971 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (3)].declarator); ;}
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 972 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (2)].declarator); (yyval.declarator)->array = append_array((yyval.declarator)->array, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 302:

/* Line 1455 of yacc.c  */
#line 973 "parser.y"
    { (yyval.declarator) = make_declarator(NULL); (yyval.declarator)->array = append_array((yyval.declarator)->array, (yyvsp[(1) - (1)].expr)); ;}
    break;

  case 303:

/* Line 1455 of yacc.c  */
#line 975 "parser.y"
    { (yyval.declarator) = make_declarator(NULL);
						  (yyval.declarator)->func_type = append_ptrchain_type((yyval.declarator)->type, type_new_function((yyvsp[(2) - (3)].var_list)));
						  (yyval.declarator)->type = NULL;
						;}
    break;

  case 304:

/* Line 1455 of yacc.c  */
#line 980 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (4)].declarator);
						  (yyval.declarator)->func_type = append_ptrchain_type((yyval.declarator)->type, type_new_function((yyvsp[(3) - (4)].var_list)));
						  (yyval.declarator)->type = NULL;
						;}
    break;

  case 305:

/* Line 1455 of yacc.c  */
#line 989 "parser.y"
    { (yyval.declarator) = (yyvsp[(3) - (3)].declarator); (yyval.declarator)->type = append_ptrchain_type((yyval.declarator)->type, type_new_pointer(pointer_default, NULL, (yyvsp[(2) - (3)].attr_list))); ;}
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 990 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (2)].declarator); (yyval.declarator)->type->attrs = append_attr((yyval.declarator)->type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[(1) - (2)].str))); ;}
    break;

  case 308:

/* Line 1455 of yacc.c  */
#line 997 "parser.y"
    { (yyval.declarator) = (yyvsp[(3) - (3)].declarator); (yyval.declarator)->type = append_ptrchain_type((yyval.declarator)->type, type_new_pointer(pointer_default, NULL, (yyvsp[(2) - (3)].attr_list))); ;}
    break;

  case 309:

/* Line 1455 of yacc.c  */
#line 998 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (2)].declarator); (yyval.declarator)->type->attrs = append_attr((yyval.declarator)->type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[(1) - (2)].str))); ;}
    break;

  case 310:

/* Line 1455 of yacc.c  */
#line 1002 "parser.y"
    { (yyval.declarator) = make_declarator(NULL); ;}
    break;

  case 312:

/* Line 1455 of yacc.c  */
#line 1010 "parser.y"
    { (yyval.declarator) = make_declarator((yyvsp[(1) - (1)].var)); ;}
    break;

  case 313:

/* Line 1455 of yacc.c  */
#line 1011 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (3)].declarator); ;}
    break;

  case 314:

/* Line 1455 of yacc.c  */
#line 1012 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (2)].declarator); (yyval.declarator)->array = append_array((yyval.declarator)->array, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 315:

/* Line 1455 of yacc.c  */
#line 1013 "parser.y"
    { (yyval.declarator) = make_declarator(NULL); (yyval.declarator)->array = append_array((yyval.declarator)->array, (yyvsp[(1) - (1)].expr)); ;}
    break;

  case 316:

/* Line 1455 of yacc.c  */
#line 1015 "parser.y"
    { (yyval.declarator) = make_declarator(NULL);
						  (yyval.declarator)->func_type = append_ptrchain_type((yyval.declarator)->type, type_new_function((yyvsp[(2) - (3)].var_list)));
						  (yyval.declarator)->type = NULL;
						;}
    break;

  case 317:

/* Line 1455 of yacc.c  */
#line 1020 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (4)].declarator);
						  (yyval.declarator)->func_type = append_ptrchain_type((yyval.declarator)->type, type_new_function((yyvsp[(3) - (4)].var_list)));
						  (yyval.declarator)->type = NULL;
						;}
    break;

  case 318:

/* Line 1455 of yacc.c  */
#line 1027 "parser.y"
    { (yyval.declarator_list) = append_declarator( NULL, (yyvsp[(1) - (1)].declarator) ); ;}
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 1028 "parser.y"
    { (yyval.declarator_list) = append_declarator( (yyvsp[(1) - (3)].declarator_list), (yyvsp[(3) - (3)].declarator) ); ;}
    break;

  case 320:

/* Line 1455 of yacc.c  */
#line 1031 "parser.y"
    { (yyval.expr) = NULL; ;}
    break;

  case 321:

/* Line 1455 of yacc.c  */
#line 1032 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (2)].expr); ;}
    break;

  case 322:

/* Line 1455 of yacc.c  */
#line 1035 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (2)].declarator); (yyval.declarator)->bits = (yyvsp[(2) - (2)].expr);
						  if (!(yyval.declarator)->bits && !(yyval.declarator)->var->name)
						    error_loc("unnamed fields are not allowed\n");
						;}
    break;

  case 323:

/* Line 1455 of yacc.c  */
#line 1042 "parser.y"
    { (yyval.declarator_list) = append_declarator( NULL, (yyvsp[(1) - (1)].declarator) ); ;}
    break;

  case 324:

/* Line 1455 of yacc.c  */
#line 1044 "parser.y"
    { (yyval.declarator_list) = append_declarator( (yyvsp[(1) - (3)].declarator_list), (yyvsp[(3) - (3)].declarator) ); ;}
    break;

  case 325:

/* Line 1455 of yacc.c  */
#line 1048 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (1)].declarator); ;}
    break;

  case 326:

/* Line 1455 of yacc.c  */
#line 1049 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (3)].declarator); (yyvsp[(1) - (3)].declarator)->var->eval = (yyvsp[(3) - (3)].expr); ;}
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 1053 "parser.y"
    { (yyval.num) = RPC_FC_RP; ;}
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 1054 "parser.y"
    { (yyval.num) = RPC_FC_UP; ;}
    break;

  case 329:

/* Line 1455 of yacc.c  */
#line 1055 "parser.y"
    { (yyval.num) = RPC_FC_FP; ;}
    break;

  case 330:

/* Line 1455 of yacc.c  */
#line 1058 "parser.y"
    { (yyval.type) = type_new_struct((yyvsp[(2) - (5)].str), TRUE, (yyvsp[(4) - (5)].var_list)); ;}
    break;

  case 331:

/* Line 1455 of yacc.c  */
#line 1061 "parser.y"
    { (yyval.type) = type_new_void(); ;}
    break;

  case 332:

/* Line 1455 of yacc.c  */
#line 1062 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); ;}
    break;

  case 333:

/* Line 1455 of yacc.c  */
#line 1063 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (1)].type); ;}
    break;

  case 334:

/* Line 1455 of yacc.c  */
#line 1064 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (1)].type); ;}
    break;

  case 335:

/* Line 1455 of yacc.c  */
#line 1065 "parser.y"
    { (yyval.type) = type_new_enum((yyvsp[(2) - (2)].str), FALSE, NULL); ;}
    break;

  case 336:

/* Line 1455 of yacc.c  */
#line 1066 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (1)].type); ;}
    break;

  case 337:

/* Line 1455 of yacc.c  */
#line 1067 "parser.y"
    { (yyval.type) = type_new_struct((yyvsp[(2) - (2)].str), FALSE, NULL); ;}
    break;

  case 338:

/* Line 1455 of yacc.c  */
#line 1068 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (1)].type); ;}
    break;

  case 339:

/* Line 1455 of yacc.c  */
#line 1069 "parser.y"
    { (yyval.type) = type_new_nonencapsulated_union((yyvsp[(2) - (2)].str), FALSE, NULL); ;}
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 1070 "parser.y"
    { (yyval.type) = make_safearray((yyvsp[(3) - (4)].type)); ;}
    break;

  case 341:

/* Line 1455 of yacc.c  */
#line 1074 "parser.y"
    { reg_typedefs((yyvsp[(3) - (4)].declspec), (yyvsp[(4) - (4)].declarator_list), check_typedef_attrs((yyvsp[(2) - (4)].attr_list)));
						  (yyval.statement) = make_statement_typedef((yyvsp[(4) - (4)].declarator_list));
						;}
    break;

  case 342:

/* Line 1455 of yacc.c  */
#line 1080 "parser.y"
    { (yyval.type) = type_new_nonencapsulated_union((yyvsp[(2) - (5)].str), TRUE, (yyvsp[(4) - (5)].var_list)); ;}
    break;

  case 343:

/* Line 1455 of yacc.c  */
#line 1083 "parser.y"
    { (yyval.type) = type_new_encapsulated_union((yyvsp[(2) - (10)].str), (yyvsp[(5) - (10)].var), (yyvsp[(7) - (10)].var), (yyvsp[(9) - (10)].var_list)); ;}
    break;

  case 344:

/* Line 1455 of yacc.c  */
#line 1087 "parser.y"
    { (yyval.num) = MAKEVERSION((yyvsp[(1) - (1)].num), 0); ;}
    break;

  case 345:

/* Line 1455 of yacc.c  */
#line 1088 "parser.y"
    { (yyval.num) = MAKEVERSION((yyvsp[(1) - (3)].num), (yyvsp[(3) - (3)].num)); ;}
    break;



/* Line 1455 of yacc.c  */
#line 5041 "parser.tab.c"
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
#line 1091 "parser.y"


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
    /* ATTR_PARAMLCID */           { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "lcid" },
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
            /* FIXME */
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

