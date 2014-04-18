/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison implementation for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
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
#define YYBISON_VERSION "2.5"

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

/* Line 268 of yacc.c  */
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
static statement_t *make_statement_pragma(const char *str);
static statement_t *make_statement_cppquote(const char *str);
static statement_t *make_statement_importlib(const char *str);
static statement_t *make_statement_module(type_t *type);
static statement_t *make_statement_typedef(var_list_t *names);
static statement_t *make_statement_import(const char *str);
static statement_t *make_statement_typedef(var_list_t *names);
static statement_list_t *append_statement(statement_list_t *list, statement_t *stmt);
static statement_list_t *append_statements(statement_list_t *, statement_list_t *);
static attr_list_t *append_attribs(attr_list_t *, attr_list_t *);



/* Line 268 of yacc.c  */
#line 209 "parser.tab.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
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
     SHL = 269,
     SHR = 270,
     MEMBERPTR = 271,
     EQUALITY = 272,
     INEQUALITY = 273,
     GREATEREQUAL = 274,
     LESSEQUAL = 275,
     LOGICALOR = 276,
     LOGICALAND = 277,
     ELLIPSIS = 278,
     tAGGREGATABLE = 279,
     tALLOCATE = 280,
     tANNOTATION = 281,
     tAPPOBJECT = 282,
     tASYNC = 283,
     tASYNCUUID = 284,
     tAUTOHANDLE = 285,
     tBINDABLE = 286,
     tBOOLEAN = 287,
     tBROADCAST = 288,
     tBYTE = 289,
     tBYTECOUNT = 290,
     tCALLAS = 291,
     tCALLBACK = 292,
     tCASE = 293,
     tCDECL = 294,
     tCHAR = 295,
     tCOCLASS = 296,
     tCODE = 297,
     tCOMMSTATUS = 298,
     tCONST = 299,
     tCONTEXTHANDLE = 300,
     tCONTEXTHANDLENOSERIALIZE = 301,
     tCONTEXTHANDLESERIALIZE = 302,
     tCONTROL = 303,
     tCPPQUOTE = 304,
     tDECODE = 305,
     tDEFAULT = 306,
     tDEFAULTBIND = 307,
     tDEFAULTCOLLELEM = 308,
     tDEFAULTVALUE = 309,
     tDEFAULTVTABLE = 310,
     tDISABLECONSISTENCYCHECK = 311,
     tDISPLAYBIND = 312,
     tDISPINTERFACE = 313,
     tDLLNAME = 314,
     tDOUBLE = 315,
     tDUAL = 316,
     tENABLEALLOCATE = 317,
     tENCODE = 318,
     tENDPOINT = 319,
     tENTRY = 320,
     tENUM = 321,
     tERRORSTATUST = 322,
     tEXPLICITHANDLE = 323,
     tEXTERN = 324,
     tFALSE = 325,
     tFASTCALL = 326,
     tFAULTSTATUS = 327,
     tFLOAT = 328,
     tFORCEALLOCATE = 329,
     tHANDLE = 330,
     tHANDLET = 331,
     tHELPCONTEXT = 332,
     tHELPFILE = 333,
     tHELPSTRING = 334,
     tHELPSTRINGCONTEXT = 335,
     tHELPSTRINGDLL = 336,
     tHIDDEN = 337,
     tHYPER = 338,
     tID = 339,
     tIDEMPOTENT = 340,
     tIGNORE = 341,
     tIIDIS = 342,
     tIMMEDIATEBIND = 343,
     tIMPLICITHANDLE = 344,
     tIMPORT = 345,
     tIMPORTLIB = 346,
     tIN = 347,
     tIN_LINE = 348,
     tINLINE = 349,
     tINPUTSYNC = 350,
     tINT = 351,
     tINT3264 = 352,
     tINT64 = 353,
     tINTERFACE = 354,
     tLCID = 355,
     tLENGTHIS = 356,
     tLIBRARY = 357,
     tLICENSED = 358,
     tLOCAL = 359,
     tLONG = 360,
     tMAYBE = 361,
     tMESSAGE = 362,
     tMETHODS = 363,
     tMODULE = 364,
     tNAMESPACE = 365,
     tNOCODE = 366,
     tNONBROWSABLE = 367,
     tNONCREATABLE = 368,
     tNONEXTENSIBLE = 369,
     tNOTIFY = 370,
     tNOTIFYFLAG = 371,
     tNULL = 372,
     tOBJECT = 373,
     tODL = 374,
     tOLEAUTOMATION = 375,
     tOPTIMIZE = 376,
     tOPTIONAL = 377,
     tOUT = 378,
     tPARTIALIGNORE = 379,
     tPASCAL = 380,
     tPOINTERDEFAULT = 381,
     tPROGID = 382,
     tPROPERTIES = 383,
     tPROPGET = 384,
     tPROPPUT = 385,
     tPROPPUTREF = 386,
     tPROXY = 387,
     tPTR = 388,
     tPUBLIC = 389,
     tRANGE = 390,
     tREADONLY = 391,
     tREF = 392,
     tREGISTER = 393,
     tREPRESENTAS = 394,
     tREQUESTEDIT = 395,
     tRESTRICTED = 396,
     tRETVAL = 397,
     tSAFEARRAY = 398,
     tSHORT = 399,
     tSIGNED = 400,
     tSIZEIS = 401,
     tSIZEOF = 402,
     tSMALL = 403,
     tSOURCE = 404,
     tSTATIC = 405,
     tSTDCALL = 406,
     tSTRICTCONTEXTHANDLE = 407,
     tSTRING = 408,
     tSTRUCT = 409,
     tSWITCH = 410,
     tSWITCHIS = 411,
     tSWITCHTYPE = 412,
     tTHREADING = 413,
     tTRANSMITAS = 414,
     tTRUE = 415,
     tTYPEDEF = 416,
     tUIDEFAULT = 417,
     tUNION = 418,
     tUNIQUE = 419,
     tUNSIGNED = 420,
     tUSESGETLASTERROR = 421,
     tUSERMARSHAL = 422,
     tUUID = 423,
     tV1ENUM = 424,
     tVARARG = 425,
     tVERSION = 426,
     tVIPROGID = 427,
     tVOID = 428,
     tWCHAR = 429,
     tWIREMARSHAL = 430,
     tAPARTMENT = 431,
     tNEUTRAL = 432,
     tSINGLE = 433,
     tFREE = 434,
     tBOTH = 435,
     ADDRESSOF = 436,
     NEG = 437,
     POS = 438,
     PPTR = 439,
     CAST = 440
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 293 of yacc.c  */
#line 129 "parser.y"

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



/* Line 293 of yacc.c  */
#line 459 "parser.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 343 of yacc.c  */
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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
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
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
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

# define YYCOPY_NEEDED 1

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

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
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
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   3077

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  210
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  101
/* YYNRULES -- Number of rules.  */
#define YYNRULES  387
/* YYNRULES -- Number of states.  */
#define YYNSTATES  679

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   440

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   194,     2,     2,     2,   193,   186,     2,
     207,   208,   191,   190,   181,   189,   201,   192,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   183,   206,
     187,   209,   188,   182,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   202,     2,   203,   185,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   204,   184,   205,   195,     2,     2,     2,
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
     175,   176,   177,   178,   179,   180,   196,   197,   198,   199,
     200
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     6,    12,    15,    18,    22,    25,
      28,    31,    34,    35,    38,    44,    47,    51,    54,    57,
      60,    63,    66,    67,    70,    71,    73,    75,    78,    81,
      83,    86,    88,    90,    93,    95,    98,   100,   103,   106,
     109,   112,   117,   121,   125,   131,   134,   138,   143,   144,
     146,   148,   152,   154,   158,   162,   165,   169,   173,   176,
     177,   179,   183,   185,   189,   194,   196,   200,   201,   203,
     208,   210,   212,   214,   216,   218,   223,   228,   230,   232,
     234,   236,   238,   240,   242,   244,   246,   248,   253,   255,
     257,   259,   264,   266,   268,   270,   275,   280,   282,   284,
     286,   288,   293,   298,   303,   308,   313,   315,   320,   322,
     324,   329,   331,   336,   338,   340,   345,   350,   352,   354,
     356,   358,   360,   362,   364,   366,   368,   370,   372,   374,
     376,   378,   383,   385,   387,   389,   394,   399,   401,   403,
     405,   407,   409,   416,   418,   423,   425,   427,   429,   434,
     436,   438,   440,   445,   450,   455,   460,   462,   464,   469,
     474,   479,   481,   483,   488,   493,   498,   500,   502,   504,
     506,   508,   510,   512,   513,   516,   521,   525,   526,   529,
     531,   533,   537,   541,   543,   549,   551,   555,   556,   558,
     560,   562,   564,   566,   568,   570,   572,   574,   576,   578,
     584,   588,   592,   596,   600,   604,   608,   612,   616,   620,
     624,   628,   632,   636,   640,   644,   648,   652,   656,   659,
     662,   665,   668,   671,   674,   678,   682,   688,   694,   699,
     703,   705,   709,   711,   713,   714,   717,   722,   726,   729,
     732,   733,   736,   739,   741,   745,   748,   750,   754,   757,
     758,   760,   761,   763,   765,   767,   769,   771,   773,   775,
     778,   781,   783,   785,   787,   789,   791,   793,   794,   796,
     798,   801,   803,   806,   809,   811,   813,   815,   818,   821,
     824,   830,   833,   834,   837,   840,   843,   846,   849,   852,
     856,   859,   863,   869,   875,   876,   879,   882,   885,   888,
     895,   904,   907,   910,   913,   916,   919,   922,   928,   930,
     932,   934,   936,   938,   939,   942,   945,   949,   950,   952,
     955,   958,   961,   965,   968,   970,   972,   976,   979,   984,
     988,   991,   993,   997,  1000,  1001,  1003,  1007,  1010,  1012,
    1016,  1021,  1025,  1028,  1030,  1034,  1037,  1038,  1040,  1042,
    1046,  1049,  1051,  1055,  1060,  1062,  1066,  1067,  1070,  1073,
    1075,  1079,  1081,  1085,  1087,  1089,  1091,  1093,  1095,  1097,
    1099,  1101,  1107,  1109,  1111,  1113,  1115,  1118,  1120,  1123,
    1125,  1128,  1133,  1139,  1145,  1156,  1158,  1162
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     211,     0,    -1,   212,    -1,    -1,   212,   266,   204,   212,
     205,    -1,   212,   278,    -1,   212,   277,    -1,   212,   263,
     206,    -1,   212,   265,    -1,   212,   281,    -1,   212,   224,
      -1,   212,   216,    -1,    -1,   213,   278,    -1,   213,   266,
     204,   213,   205,    -1,   213,   277,    -1,   213,   263,   206,
      -1,   213,   265,    -1,   213,   281,    -1,   213,   216,    -1,
     213,   221,    -1,   213,   224,    -1,    -1,   214,   216,    -1,
      -1,   206,    -1,   218,    -1,   217,   206,    -1,   256,   206,
      -1,   220,    -1,   308,   206,    -1,     4,    -1,   242,    -1,
      66,     3,    -1,   306,    -1,   154,     3,    -1,   309,    -1,
     163,     3,    -1,   231,   242,    -1,   231,   306,    -1,   231,
     309,    -1,    49,   207,     9,   208,    -1,    90,     9,   206,
      -1,   219,   213,    13,    -1,    91,   207,     9,   208,   215,
      -1,   102,     3,    -1,   231,   222,   204,    -1,   223,   213,
     205,   215,    -1,    -1,   227,    -1,   228,    -1,   226,   181,
     228,    -1,   226,    -1,   226,   181,    23,    -1,   231,   286,
     297,    -1,   286,   297,    -1,   202,   245,   203,    -1,   202,
     191,   203,    -1,   202,   203,    -1,    -1,   231,    -1,   202,
     232,   203,    -1,   234,    -1,   232,   181,   234,    -1,   232,
     203,   202,   234,    -1,     9,    -1,   233,   181,     9,    -1,
      -1,    24,    -1,    26,   207,     9,   208,    -1,    27,    -1,
      28,    -1,    30,    -1,    31,    -1,    33,    -1,    36,   207,
     259,   208,    -1,    38,   207,   246,   208,    -1,    42,    -1,
      43,    -1,    45,    -1,    46,    -1,    47,    -1,    48,    -1,
      50,    -1,    51,    -1,    52,    -1,    53,    -1,    54,   207,
     248,   208,    -1,    55,    -1,    56,    -1,    57,    -1,    59,
     207,     9,   208,    -1,    61,    -1,    62,    -1,    63,    -1,
      64,   207,   233,   208,    -1,    65,   207,   248,   208,    -1,
      68,    -1,    72,    -1,    74,    -1,    75,    -1,    77,   207,
     247,   208,    -1,    78,   207,     9,   208,    -1,    79,   207,
       9,   208,    -1,    80,   207,   247,   208,    -1,    81,   207,
       9,   208,    -1,    82,    -1,    84,   207,   247,   208,    -1,
      85,    -1,    86,    -1,    87,   207,   245,   208,    -1,    88,
      -1,    89,   207,   228,   208,    -1,    92,    -1,    95,    -1,
     101,   207,   243,   208,    -1,   100,   207,   247,   208,    -1,
     100,    -1,   103,    -1,   104,    -1,   106,    -1,   107,    -1,
     111,    -1,   112,    -1,   113,    -1,   114,    -1,   115,    -1,
     116,    -1,   118,    -1,   119,    -1,   120,    -1,   121,   207,
       9,   208,    -1,   122,    -1,   123,    -1,   124,    -1,   126,
     207,   305,   208,    -1,   127,   207,     9,   208,    -1,   129,
      -1,   130,    -1,   131,    -1,   132,    -1,   134,    -1,   135,
     207,   247,   181,   247,   208,    -1,   136,    -1,   139,   207,
     307,   208,    -1,   140,    -1,   141,    -1,   142,    -1,   146,
     207,   243,   208,    -1,   149,    -1,   152,    -1,   153,    -1,
     156,   207,   245,   208,    -1,   157,   207,   307,   208,    -1,
     159,   207,   307,   208,    -1,   158,   207,   304,   208,    -1,
     162,    -1,   166,    -1,   167,   207,   307,   208,    -1,   168,
     207,   235,   208,    -1,    29,   207,   235,   208,    -1,   169,
      -1,   170,    -1,   171,   207,   310,   208,    -1,   172,   207,
       9,   208,    -1,   175,   207,   307,   208,    -1,   305,    -1,
      12,    -1,     9,    -1,    39,    -1,    71,    -1,   125,    -1,
     151,    -1,    -1,   237,   238,    -1,    38,   247,   183,   253,
      -1,    51,   183,   253,    -1,    -1,   240,   181,    -1,   240,
      -1,   241,    -1,   240,   181,   241,    -1,   259,   209,   247,
      -1,   259,    -1,    66,   258,   204,   239,   205,    -1,   244,
      -1,   243,   181,   244,    -1,    -1,   245,    -1,     6,    -1,
       7,    -1,     8,    -1,    70,    -1,   117,    -1,   160,    -1,
       9,    -1,    10,    -1,    11,    -1,     3,    -1,   245,   182,
     245,   183,   245,    -1,   245,    21,   245,    -1,   245,    22,
     245,    -1,   245,   184,   245,    -1,   245,   185,   245,    -1,
     245,   186,   245,    -1,   245,    17,   245,    -1,   245,    18,
     245,    -1,   245,   188,   245,    -1,   245,   187,   245,    -1,
     245,    19,   245,    -1,   245,    20,   245,    -1,   245,    14,
     245,    -1,   245,    15,   245,    -1,   245,   190,   245,    -1,
     245,   189,   245,    -1,   245,   193,   245,    -1,   245,   191,
     245,    -1,   245,   192,   245,    -1,   194,   245,    -1,   195,
     245,    -1,   190,   245,    -1,   189,   245,    -1,   186,   245,
      -1,   191,   245,    -1,   245,    16,     3,    -1,   245,   201,
       3,    -1,   207,   286,   293,   208,   245,    -1,   147,   207,
     286,   293,   208,    -1,   245,   202,   245,   203,    -1,   207,
     245,   208,    -1,   247,    -1,   246,   181,   247,    -1,   245,
      -1,   245,    -1,    -1,   249,   250,    -1,   230,   286,   302,
     206,    -1,   230,   309,   206,    -1,   254,   206,    -1,   231,
     206,    -1,    -1,   252,   251,    -1,   254,   206,    -1,   206,
      -1,   230,   286,   289,    -1,   230,   306,    -1,   256,    -1,
     231,   286,   303,    -1,   286,   303,    -1,    -1,   259,    -1,
      -1,     3,    -1,     5,    -1,     3,    -1,     5,    -1,    34,
      -1,   174,    -1,   262,    -1,   145,   262,    -1,   165,   262,
      -1,   165,    -1,    73,    -1,    60,    -1,    32,    -1,    67,
      -1,    76,    -1,    -1,    96,    -1,    96,    -1,   144,   261,
      -1,   148,    -1,   105,   261,    -1,    83,   261,    -1,    98,
      -1,    40,    -1,    97,    -1,    41,     3,    -1,    41,     5,
      -1,   231,   263,    -1,   264,   204,   267,   205,   215,    -1,
     110,     3,    -1,    -1,   267,   268,    -1,   230,   278,    -1,
      58,     3,    -1,    58,     5,    -1,   231,   269,    -1,   128,
     183,    -1,   271,   254,   206,    -1,   108,   183,    -1,   272,
     255,   206,    -1,   270,   204,   271,   272,   205,    -1,   270,
     204,   275,   206,   205,    -1,    -1,   183,     5,    -1,    99,
       3,    -1,    99,     5,    -1,   231,   275,    -1,   276,   274,
     204,   214,   205,   215,    -1,   276,   183,     3,   204,   220,
     214,   205,   215,    -1,   273,   215,    -1,   275,   206,    -1,
     269,   206,    -1,   109,     3,    -1,   109,     5,    -1,   231,
     279,    -1,   280,   204,   214,   205,   215,    -1,    69,    -1,
     150,    -1,   138,    -1,    94,    -1,    44,    -1,    -1,   285,
     284,    -1,   307,   287,    -1,   288,   307,   287,    -1,    -1,
     288,    -1,   284,   287,    -1,   283,   287,    -1,   282,   287,
      -1,   191,   285,   289,    -1,   236,   289,    -1,   290,    -1,
     259,    -1,   207,   289,   208,    -1,   290,   229,    -1,   290,
     207,   225,   208,    -1,   191,   285,   293,    -1,   236,   293,
      -1,   294,    -1,   191,   285,   297,    -1,   236,   297,    -1,
      -1,   291,    -1,   207,   292,   208,    -1,   294,   229,    -1,
     229,    -1,   207,   225,   208,    -1,   294,   207,   225,   208,
      -1,   191,   285,   297,    -1,   236,   297,    -1,   298,    -1,
     191,   285,   297,    -1,   236,   297,    -1,    -1,   295,    -1,
     259,    -1,   207,   296,   208,    -1,   298,   229,    -1,   229,
      -1,   207,   225,   208,    -1,   298,   207,   225,   208,    -1,
     289,    -1,   299,   181,   289,    -1,    -1,   183,   248,    -1,
     295,   300,    -1,   301,    -1,   302,   181,   301,    -1,   289,
      -1,   289,   209,   248,    -1,   176,    -1,   177,    -1,   178,
      -1,   179,    -1,   180,    -1,   137,    -1,   164,    -1,   133,
      -1,   154,   258,   204,   249,   205,    -1,   173,    -1,     5,
      -1,   260,    -1,   242,    -1,    66,     3,    -1,   306,    -1,
     154,     3,    -1,   309,    -1,   163,     3,    -1,   143,   207,
     307,   208,    -1,   230,   161,   230,   286,   299,    -1,   163,
     258,   204,   252,   205,    -1,   163,   258,   155,   207,   254,
     208,   257,   204,   237,   205,    -1,     6,    -1,     6,   201,
       6,    -1,     7,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   308,   308,   322,   323,   325,   326,   327,   330,   333,
     334,   335,   338,   339,   340,   342,   343,   344,   347,   348,
     349,   350,   353,   354,   357,   358,   362,   363,   364,   365,
     366,   367,   371,   372,   373,   374,   375,   376,   377,   378,
     379,   382,   384,   392,   398,   402,   404,   408,   415,   416,
     419,   420,   423,   424,   428,   433,   440,   444,   445,   448,
     449,   453,   456,   457,   458,   461,   462,   465,   466,   467,
     468,   469,   470,   471,   472,   473,   474,   475,   476,   477,
     478,   479,   480,   481,   482,   483,   484,   485,   486,   487,
     488,   489,   490,   491,   492,   493,   494,   495,   496,   497,
     498,   499,   500,   501,   502,   503,   504,   505,   506,   507,
     508,   509,   510,   511,   512,   513,   514,   515,   516,   517,
     518,   519,   520,   521,   522,   523,   524,   525,   526,   527,
     528,   529,   530,   531,   532,   533,   534,   535,   536,   537,
     538,   539,   540,   544,   545,   546,   547,   548,   549,   550,
     551,   552,   553,   554,   555,   556,   557,   558,   559,   560,
     561,   562,   563,   564,   565,   566,   567,   571,   572,   577,
     578,   579,   580,   583,   584,   587,   591,   597,   598,   599,
     602,   606,   618,   622,   627,   630,   631,   634,   635,   638,
     639,   640,   641,   642,   643,   644,   645,   646,   647,   648,
     649,   650,   651,   652,   653,   654,   655,   656,   657,   658,
     659,   660,   661,   662,   663,   664,   665,   666,   667,   668,
     669,   670,   671,   672,   673,   674,   675,   677,   679,   680,
     683,   684,   687,   693,   699,   700,   703,   708,   715,   716,
     719,   720,   724,   725,   728,   732,   738,   746,   750,   755,
     756,   759,   760,   761,   764,   766,   769,   770,   771,   772,
     773,   774,   775,   776,   777,   778,   779,   782,   783,   786,
     787,   788,   789,   790,   791,   792,   793,   796,   797,   805,
     811,   815,   818,   819,   823,   826,   827,   830,   839,   840,
     843,   844,   847,   853,   859,   860,   863,   864,   867,   877,
     886,   892,   896,   897,   900,   901,   904,   909,   916,   917,
     918,   922,   926,   929,   930,   933,   934,   938,   939,   943,
     944,   945,   949,   951,   953,   957,   958,   959,   960,   968,
     970,   972,   977,   979,   984,   985,   990,   991,   992,   993,
     998,  1007,  1009,  1010,  1015,  1017,  1021,  1022,  1029,  1030,
    1031,  1032,  1033,  1038,  1046,  1047,  1050,  1051,  1054,  1061,
    1062,  1067,  1068,  1072,  1073,  1074,  1075,  1076,  1080,  1081,
    1082,  1085,  1088,  1089,  1090,  1091,  1092,  1093,  1094,  1095,
    1096,  1097,  1100,  1107,  1109,  1115,  1116,  1117
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "aIDENTIFIER", "aPRAGMA", "aKNOWNTYPE",
  "aNUM", "aHEXNUM", "aDOUBLE", "aSTRING", "aWSTRING", "aSQSTRING",
  "aUUID", "aEOF", "SHL", "SHR", "MEMBERPTR", "EQUALITY", "INEQUALITY",
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
  "tLONG", "tMAYBE", "tMESSAGE", "tMETHODS", "tMODULE", "tNAMESPACE",
  "tNOCODE", "tNONBROWSABLE", "tNONCREATABLE", "tNONEXTENSIBLE", "tNOTIFY",
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
  "'{'", "'}'", "';'", "'('", "')'", "'='", "$accept", "input",
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
  "namespacedef", "coclass_ints", "coclass_int", "dispinterface",
  "dispinterfacehdr", "dispint_props", "dispint_meths", "dispinterfacedef",
  "inherit", "interface", "interfacehdr", "interfacedef", "interfacedec",
  "module", "modulehdr", "moduledef", "storage_cls_spec",
  "function_specifier", "type_qualifier", "m_type_qual_list", "decl_spec",
  "m_decl_spec_no_type", "decl_spec_no_type", "declarator",
  "direct_declarator", "abstract_declarator",
  "abstract_declarator_no_direct", "m_abstract_declarator",
  "abstract_direct_declarator", "any_declarator",
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
     425,   426,   427,   428,   429,   430,   431,   432,   433,   434,
     435,    44,    63,    58,   124,    94,    38,    60,    62,    45,
      43,    42,    47,    37,    33,   126,   436,   437,   438,   439,
     440,    46,    91,    93,   123,   125,    59,    40,    41,    61
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   210,   211,   212,   212,   212,   212,   212,   212,   212,
     212,   212,   213,   213,   213,   213,   213,   213,   213,   213,
     213,   213,   214,   214,   215,   215,   216,   216,   216,   216,
     216,   216,   217,   217,   217,   217,   217,   217,   217,   217,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   225,
     226,   226,   227,   227,   228,   228,   229,   229,   229,   230,
     230,   231,   232,   232,   232,   233,   233,   234,   234,   234,
     234,   234,   234,   234,   234,   234,   234,   234,   234,   234,
     234,   234,   234,   234,   234,   234,   234,   234,   234,   234,
     234,   234,   234,   234,   234,   234,   234,   234,   234,   234,
     234,   234,   234,   234,   234,   234,   234,   234,   234,   234,
     234,   234,   234,   234,   234,   234,   234,   234,   234,   234,
     234,   234,   234,   234,   234,   234,   234,   234,   234,   234,
     234,   234,   234,   234,   234,   234,   234,   234,   234,   234,
     234,   234,   234,   234,   234,   234,   234,   234,   234,   234,
     234,   234,   234,   234,   234,   234,   234,   234,   234,   234,
     234,   234,   234,   234,   234,   234,   234,   235,   235,   236,
     236,   236,   236,   237,   237,   238,   238,   239,   239,   239,
     240,   240,   241,   241,   242,   243,   243,   244,   244,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   245,
     246,   246,   247,   248,   249,   249,   250,   250,   251,   251,
     252,   252,   253,   253,   254,   254,   255,   256,   256,   257,
     257,   258,   258,   258,   259,   259,   260,   260,   260,   260,
     260,   260,   260,   260,   260,   260,   260,   261,   261,   262,
     262,   262,   262,   262,   262,   262,   262,   263,   263,   264,
     265,   266,   267,   267,   268,   269,   269,   270,   271,   271,
     272,   272,   273,   273,   274,   274,   275,   275,   276,   277,
     277,   277,   278,   278,   279,   279,   280,   281,   282,   282,
     282,   283,   284,   285,   285,   286,   286,   287,   287,   288,
     288,   288,   289,   289,   289,   290,   290,   290,   290,   291,
     291,   291,   292,   292,   293,   293,   294,   294,   294,   294,
     294,   295,   295,   295,   296,   296,   297,   297,   298,   298,
     298,   298,   298,   298,   299,   299,   300,   300,   301,   302,
     302,   303,   303,   304,   304,   304,   304,   304,   305,   305,
     305,   306,   307,   307,   307,   307,   307,   307,   307,   307,
     307,   307,   308,   309,   309,   310,   310,   310
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     0,     5,     2,     2,     3,     2,     2,
       2,     2,     0,     2,     5,     2,     3,     2,     2,     2,
       2,     2,     0,     2,     0,     1,     1,     2,     2,     1,
       2,     1,     1,     2,     1,     2,     1,     2,     2,     2,
       2,     4,     3,     3,     5,     2,     3,     4,     0,     1,
       1,     3,     1,     3,     3,     2,     3,     3,     2,     0,
       1,     3,     1,     3,     4,     1,     3,     0,     1,     4,
       1,     1,     1,     1,     1,     4,     4,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     4,     1,     1,
       1,     4,     1,     1,     1,     4,     4,     1,     1,     1,
       1,     4,     4,     4,     4,     4,     1,     4,     1,     1,
       4,     1,     4,     1,     1,     4,     4,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     4,     1,     1,     1,     4,     4,     1,     1,     1,
       1,     1,     6,     1,     4,     1,     1,     1,     4,     1,
       1,     1,     4,     4,     4,     4,     1,     1,     4,     4,
       4,     1,     1,     4,     4,     4,     1,     1,     1,     1,
       1,     1,     1,     0,     2,     4,     3,     0,     2,     1,
       1,     3,     3,     1,     5,     1,     3,     0,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     5,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     2,     2,
       2,     2,     2,     2,     3,     3,     5,     5,     4,     3,
       1,     3,     1,     1,     0,     2,     4,     3,     2,     2,
       0,     2,     2,     1,     3,     2,     1,     3,     2,     0,
       1,     0,     1,     1,     1,     1,     1,     1,     1,     2,
       2,     1,     1,     1,     1,     1,     1,     0,     1,     1,
       2,     1,     2,     2,     1,     1,     1,     2,     2,     2,
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
       2,     4,     5,     5,    10,     1,     3,     1
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       3,     0,     2,     1,    31,   373,   264,   256,   275,     0,
     312,     0,     0,   263,   251,   265,   308,   262,   266,   267,
       0,   311,   269,   276,   274,     0,   267,     0,   310,     0,
     267,     0,   271,   309,   251,   251,   261,   372,   257,    67,
      11,     0,    26,    12,    29,    12,    10,     0,    60,   375,
       0,   374,   258,     0,     0,     8,     0,     0,     0,    24,
       0,   294,     6,     5,     0,     9,   317,   317,   317,     0,
       0,   377,   317,     0,   379,   277,   278,     0,   285,   286,
     376,   253,     0,   268,   273,     0,   296,   297,   272,   281,
       0,   270,   259,   378,     0,   380,     0,   260,    68,     0,
      70,    71,     0,    72,    73,    74,     0,     0,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,     0,    88,
      89,    90,     0,    92,    93,    94,     0,     0,    97,    98,
      99,   100,     0,     0,     0,     0,     0,   106,     0,   108,
     109,     0,   111,     0,   113,   114,   117,     0,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,   128,   129,
     130,     0,   132,   133,   134,     0,     0,   137,   138,   139,
     140,   370,   141,     0,   143,   368,     0,   145,   146,   147,
       0,   149,   150,   151,     0,     0,     0,     0,   156,   369,
     157,     0,     0,   161,   162,     0,     0,     0,     0,    62,
     166,    27,    59,    59,    59,   251,     0,     0,   251,   251,
       0,   375,   279,   287,   298,   306,     0,   377,   379,    28,
       7,   282,     3,   303,     0,    25,   301,   302,     0,     0,
      22,   321,   318,   320,   319,   254,   255,   169,   170,   171,
     172,   313,     0,     0,   325,   361,   324,   248,   375,   377,
     317,   379,   315,    30,     0,   177,    42,     0,   234,     0,
     240,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   187,     0,
       0,     0,     0,     0,   187,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    67,    61,    43,     0,    19,    20,
      21,     0,    17,     0,    15,    13,    18,    24,     0,    60,
     376,    45,   304,   305,   378,   380,    46,   247,    59,    59,
       0,    59,     0,     0,   295,    22,    59,     0,     0,   323,
       0,     0,    48,   327,   316,    41,     0,   179,   180,   183,
     381,    59,    59,    59,     0,   168,   167,     0,     0,   198,
     189,   190,   191,   195,   196,   197,   192,   193,     0,   194,
       0,     0,     0,     0,     0,     0,     0,   232,     0,   230,
     233,     0,     0,    65,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   346,     0,     0,   185,   188,
       0,     0,     0,     0,     0,     0,     0,     0,   363,   364,
     365,   366,   367,     0,     0,     0,     0,   385,   387,     0,
       0,     0,    63,    67,     0,    16,    12,    47,     0,    24,
       0,   283,     4,   288,     0,     0,     0,     0,     0,     0,
      59,    24,    23,    60,   314,   322,   326,   362,     0,    58,
       0,     0,    52,    49,    50,   184,   178,     0,   371,     0,
     235,     0,   383,    60,   241,     0,    69,   160,    75,     0,
     222,   221,   220,   223,   218,   219,     0,   334,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    76,    87,    91,     0,    95,    96,   101,   102,   103,
     104,   105,   107,   110,   112,   346,   313,    48,   351,   346,
     348,   347,    55,   343,   116,   187,   115,   131,   135,   136,
       0,   144,   148,   152,   153,   155,   154,   158,   159,     0,
     163,   164,   165,    64,     0,    59,   354,   382,   280,   284,
     290,     0,   377,   289,   292,     0,     0,   246,   293,    22,
      24,   307,    57,    56,   328,     0,   181,   182,     0,   379,
     249,   239,   238,   334,   229,   313,    48,   338,   334,   335,
       0,   331,   211,   212,   224,   205,   206,   209,   210,   200,
     201,     0,   202,   203,   204,   208,   207,   214,   213,   216,
     217,   215,   225,     0,   231,    66,    54,   346,   313,     0,
     346,     0,   342,    48,   350,   186,     0,   386,    24,    14,
       0,   244,   291,    59,   299,    53,    51,   356,   359,     0,
     237,     0,   250,     0,   334,   313,     0,   346,     0,   330,
       0,    48,   337,     0,   228,   341,   346,   352,   345,   349,
       0,   142,    44,   355,    24,     0,   358,     0,   236,   173,
     227,   329,   346,   339,   333,   336,   226,     0,   199,   344,
     353,   300,   357,   360,     0,   332,   340,     0,     0,   384,
     174,     0,    59,    59,   243,   176,     0,   175,   242
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     2,   202,   326,   226,   298,    41,    42,    43,
      44,   299,   210,    45,   300,   441,   442,   443,   444,   508,
      47,   309,   198,   374,   199,   347,   509,   664,   670,   336,
     337,   338,   248,   387,   388,   367,   368,   369,   371,   341,
     450,   454,   343,   675,   676,   546,    50,   621,    82,   510,
      51,    84,    52,   301,    54,   302,   303,   318,   421,    57,
      58,   321,   427,    59,   229,    60,    61,   304,   305,   215,
      64,   306,    66,    67,    68,   327,    69,   231,    70,   245,
     246,   569,   628,   570,   571,   511,   601,   512,   513,   537,
     646,   618,   619,   247,   403,   200,   249,    72,    73,   251,
     409
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -526
static const yytype_int16 yypact[] =
{
    -526,    59,  1737,  -526,  -526,  -526,  -526,  -526,  -526,   163,
    -526,  -138,   169,  -526,   194,  -526,  -526,  -526,  -526,    12,
      73,  -526,  -526,  -526,  -526,   219,    12,   120,  -526,   -64,
      12,   300,  -526,  -526,   266,   270,   300,  -526,  -526,  2902,
    -526,   -48,  -526,  -526,  -526,  -526,  -526,    46,  2579,   -44,
     -23,  -526,  -526,    39,    19,  -526,    48,    47,    54,    62,
      76,   -33,  -526,  -526,    82,  -526,    96,    96,    96,    57,
    2751,    86,    96,    87,    90,  -526,  -526,   223,  -526,  -526,
      72,  -526,    95,  -526,  -526,   103,  -526,  -526,  -526,  -526,
    2751,  -526,  -526,    99,   112,  -112,  -109,  -526,  -526,    21,
    -526,  -526,    56,  -526,  -526,  -526,    93,   156,  -526,  -526,
    -526,  -526,  -526,  -526,  -526,  -526,  -526,  -526,   157,  -526,
    -526,  -526,   162,  -526,  -526,  -526,   164,   168,  -526,  -526,
    -526,  -526,   170,   171,   173,   174,   175,  -526,   177,  -526,
    -526,   178,  -526,   179,  -526,  -526,   184,   186,  -526,  -526,
    -526,  -526,  -526,  -526,  -526,  -526,  -526,  -526,  -526,  -526,
    -526,   187,  -526,  -526,  -526,   193,   196,  -526,  -526,  -526,
    -526,  -526,  -526,   197,  -526,  -526,   200,  -526,  -526,  -526,
     201,  -526,  -526,  -526,   202,   203,   215,   216,  -526,  -526,
    -526,   220,   225,  -526,  -526,   228,   231,   234,   -56,  -526,
    -526,  -526,  1620,   877,   136,   301,   373,   329,   347,   356,
     226,   195,  -526,  -526,  -526,  -526,    57,   237,   239,  -526,
    -526,  -526,  -526,  -526,   -22,  -526,  -526,  -526,   357,   242,
    -526,  -526,  -526,  -526,  -526,  -526,  -526,  -526,  -526,  -526,
    -526,  -526,    57,    57,  -526,   238,  -141,  -526,  -526,  -526,
      96,  -526,  -526,  -526,   241,   365,  -526,   243,  -526,   247,
    -526,   453,   161,   365,   713,   713,   454,   455,   713,   713,
     456,   457,   713,   458,   713,   713,  2168,   713,   713,   459,
     -54,   461,   713,  2751,   713,   713,  2751,   135,  2751,  2751,
     161,   130,   462,  2751,  2902,   271,  -526,   265,  -526,  -526,
    -526,   273,  -526,   276,  -526,  -526,  -526,    62,  2665,  -526,
     277,  -526,  -526,  -526,   277,  -105,  -526,  -526,   -18,  1111,
     294,   -76,   280,   278,  -526,  -526,  1213,    42,   279,  -526,
     713,    94,  2168,  -526,  -526,  -526,   284,   309,  -526,   282,
    -526,   -13,   136,   -11,   288,  -526,  -526,   289,   291,  -526,
    -526,  -526,  -526,  -526,  -526,  -526,  -526,  -526,   285,  -526,
     713,   713,   713,   713,   713,   713,   703,  2390,   -91,  -526,
    2390,   292,   296,  -526,   -70,   298,   302,   303,   304,   305,
     306,   307,   398,   308,  2665,    88,   310,   -66,  -526,  2390,
     311,   313,   314,   321,   315,   -63,  2174,   316,  -526,  -526,
    -526,  -526,  -526,   317,   318,   319,   322,   328,  -526,   323,
     324,   326,  -526,  2902,   499,  -526,  -526,  -526,    57,    62,
     -21,  -526,  -526,  -526,   352,  2665,   331,  1503,   334,   419,
    1315,    62,  -526,  2665,  -526,  -526,  -526,  -526,   450,  -526,
    1425,   336,   366,  -526,  -526,  -526,   365,   713,  -526,  2665,
    -526,   338,  -526,   342,  -526,   343,  -526,  -526,  -526,  2665,
      18,    18,    18,    18,    18,    18,  2260,   248,   713,   713,
     547,   713,   713,   713,   713,   713,   713,   713,   713,   713,
     713,   713,   713,   713,   713,   713,   713,   713,   548,   713,
     713,  -526,  -526,  -526,   543,  -526,  -526,  -526,  -526,  -526,
    -526,  -526,  -526,  -526,  -526,    88,  -526,  1823,  -526,    88,
    -526,  -526,  -526,  -137,  -526,   713,  -526,  -526,  -526,  -526,
     713,  -526,  -526,  -526,  -526,  -526,  -526,  -526,  -526,   549,
    -526,  -526,  -526,  -526,   346,   994,  -526,   375,  -526,  -526,
    -526,    57,   166,  -526,  -526,  2665,   351,  -526,  -526,  -526,
      62,  -526,  -526,  -526,  -526,  2082,  -526,  -526,    88,   354,
     365,  -526,  -526,   248,  -526,  -526,  1950,  -526,   248,  -526,
     350,  -135,    25,    25,  -526,   603,   603,   165,   165,   817,
    2285,  2369,  2408,  2440,   740,   165,   165,    64,    64,    18,
      18,    18,  -526,  2329,  -526,  -526,  -526,    70,  -526,   353,
      88,   355,  -526,  2168,  -526,  -526,   358,  -526,    62,  -526,
      57,  -526,  -526,  1417,  -526,  -526,  -526,   379,  -526,   -94,
    -526,   364,  -526,   361,   100,  -526,   369,    88,   370,  -526,
     713,  2168,  -526,   713,  -526,  -526,    70,  -526,  -526,  -526,
     371,  -526,  -526,  -526,    62,   713,  -526,    88,  -526,  -526,
    -526,  -526,    70,  -526,  -526,  -526,    18,   384,  2390,  -526,
    -526,  -526,  -526,  -526,    -3,  -526,  -526,   713,   411,  -526,
    -526,   412,   -86,   -86,  -526,  -526,   390,  -526,  -526
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -526,  -526,   359,   -28,  -305,  -300,    -1,  -526,  -526,  -526,
     176,  -526,  -526,  -526,    23,  -468,  -526,  -526,  -257,  -232,
    -189,    -2,  -526,  -526,  -267,   312,   -65,  -526,  -526,  -526,
    -526,   152,     7,   320,    92,   210,  -526,  -261,  -256,  -526,
    -526,  -526,  -526,   -60,  -187,  -526,   181,  -526,    22,   -67,
    -526,   131,    40,    16,  -526,    24,    26,  -526,  -526,   555,
    -526,  -526,  -526,  -526,  -526,   -12,  -526,    28,     4,  -526,
    -526,    29,  -526,  -526,  -287,  -477,   -38,    17,   -14,  -220,
    -526,  -526,  -526,  -500,  -526,  -525,  -526,  -467,  -526,  -526,
    -526,   -32,  -526,   393,  -526,   341,     1,   -46,  -526,     3,
    -526
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -253
static const yytype_int16 yytable[] =
{
      48,    40,   244,    71,   243,    74,    63,   417,   376,    49,
     216,   379,   375,   381,   333,   308,   386,   203,    53,   383,
     430,   393,   328,   329,   250,    46,    55,   412,    56,   597,
      62,    65,   424,   617,   470,   667,   214,    12,   596,   599,
     434,   470,   602,  -252,   257,   235,   259,   236,   668,   217,
    -252,   218,   232,   232,   232,   211,    94,    96,   232,     3,
     235,   331,   236,   623,   212,   331,   332,   331,   629,    77,
     603,    92,   631,   235,   437,   236,    97,    25,    25,   171,
     470,   237,    85,   175,   233,   234,    10,   647,   624,   252,
     490,   235,  -252,   236,   -37,   260,   237,   349,   626,  -252,
     350,   351,   352,   353,   354,   355,   320,   435,    83,   237,
     189,   494,   648,   238,    10,   515,    39,   491,   515,   538,
     674,   636,   617,    89,   651,   294,    39,   237,   238,   420,
     635,   551,   425,   638,   426,   640,   407,   408,   495,   237,
      10,   238,   516,    90,    10,   522,   533,   295,   652,   244,
     228,   243,   449,   425,   425,   451,   455,    88,   201,   238,
     654,    91,   -32,   657,   356,    16,    75,   239,    76,   659,
     345,   238,    78,   346,    79,   244,   244,   243,   243,   468,
     469,   470,   239,   219,    39,   665,   557,   419,   339,    39,
      21,    39,   448,   240,   452,   239,   348,    80,   536,    81,
      48,    48,   669,    71,    71,    74,    74,   204,   240,    49,
      49,   357,   322,   239,   483,   484,   485,   486,   487,   488,
     489,   240,    86,   221,    87,   239,   488,   489,   261,   594,
      94,    96,   254,   241,    28,   567,   232,   394,   385,   240,
     397,   358,   404,   405,   613,   220,    33,   411,   241,   242,
     614,   240,   222,   223,   359,   485,   486,   487,   224,   606,
     244,   506,   243,   262,   242,   488,   489,   334,   225,    93,
     418,    81,   331,    95,   384,    81,  -252,   507,   -33,   506,
     360,   604,   227,   361,   362,   438,   230,   237,   364,   365,
     331,   565,   -34,   253,   385,   507,   -36,   439,   616,   255,
     263,   366,   331,  -252,   310,   -35,    81,   566,   642,   256,
     434,   398,   399,   400,   401,   402,   258,    48,    40,   238,
      71,   611,    74,    63,   433,   432,    49,    71,   467,    74,
     384,   567,   312,    49,   313,    53,   567,   434,    39,   632,
       8,   453,    46,    55,   661,    56,   505,    62,    65,   434,
     314,   244,    81,   243,   483,   484,   485,   486,   487,   315,
     323,    81,   324,   264,   265,   434,   488,   489,   235,   266,
     236,   267,  -245,   239,  -245,   268,   311,   269,   270,   339,
     271,   272,   273,    19,   274,   275,   276,   541,   535,   662,
     643,   277,   567,   278,   279,   216,    22,    23,    24,   240,
     280,   -38,   568,   281,   282,    26,   671,   283,   284,   285,
     286,   558,   468,   469,   470,   471,   472,   473,   474,   475,
     476,   563,   287,   288,   539,   545,   542,   289,   433,   432,
     316,    71,   290,    74,   217,   291,   218,    49,   292,   565,
     211,   293,   600,   -39,    30,   -40,   325,   330,    32,   335,
     331,   340,   559,   349,   342,   566,   350,   351,   352,   353,
     354,   355,   344,   372,   373,   377,   378,   380,   390,   385,
     392,   410,   414,   413,   244,   370,   243,   423,   370,   415,
     416,  -252,   429,   425,   425,   382,   428,   436,   389,   445,
     446,   447,   459,   622,   389,   396,   456,   457,   568,   458,
     492,   627,   520,   568,   493,   384,   496,   216,   534,    20,
     497,   498,   499,   500,   501,   502,   504,   385,   514,   517,
     356,   518,   519,   521,   524,   525,   526,   527,   385,   529,
     528,   530,   531,    48,   532,   540,    71,   543,    74,   548,
     370,   440,    49,   244,   554,   243,   560,   555,   561,   562,
     574,   592,   595,   384,   608,   607,   610,   612,   630,   568,
     620,   637,   645,   639,   384,   385,   641,   357,   649,   650,
     460,   461,   462,   463,   464,   465,   466,   653,   655,   660,
     477,   319,   478,   479,   480,   481,   482,   483,   484,   485,
     486,   487,   666,   385,   672,   673,   678,   358,   556,   488,
     489,   384,   406,   213,   395,   549,   503,   605,   547,   317,
     359,   433,   432,   677,    71,   663,    74,   468,   469,   470,
      49,   391,   473,   474,     0,     0,     0,     0,     0,   384,
       0,     0,     0,     0,     0,     0,   360,     0,     0,   361,
     362,   363,     0,     0,   364,   365,     0,     0,   463,     0,
       0,     0,     0,   552,     0,     0,     0,   366,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   572,   573,
       0,   575,   576,   577,   578,   579,   580,   581,   582,   583,
     584,   585,   586,   587,   588,   589,   590,   591,     0,   593,
       0,     0,     0,     0,     0,     0,   349,     0,     5,   350,
     351,   352,   353,   354,   355,     0,   349,     0,     0,   350,
     351,   352,   353,   354,   355,   389,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     6,     0,     7,     0,     0,
       0,     0,     0,     8,     0,     0,     0,    10,     0,     0,
       0,     0,     0,     0,   468,   469,   470,   471,   472,   473,
     474,     0,     0,    13,     0,     0,     0,     0,     0,   205,
      15,     0,    16,   356,     0,     0,    17,     0,     0,    18,
       0,     0,     0,   356,     0,     0,    19,     0,     0,     0,
     481,   482,   483,   484,   485,   486,   487,    21,     0,    22,
      23,    24,     0,     0,   488,   489,     0,     0,    26,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     357,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     357,   468,   469,   470,   471,   472,   473,   474,     0,   476,
     656,    28,     0,   658,     0,     0,    29,    30,    31,     0,
     358,    32,     0,    33,     0,   370,     0,   208,     0,     0,
     358,     0,     0,   359,     0,     0,   209,     0,    36,     0,
       0,     0,     0,   359,     0,     0,    37,    38,     0,     0,
       0,     4,     5,     0,     0,     0,     0,     0,     0,   360,
       0,     0,   361,   362,   363,     0,     0,   364,   365,   360,
       0,     0,   361,   362,   363,     0,     0,   364,   365,     6,
     366,     7,     0,     0,     0,     0,     0,     8,     9,     0,
     366,    10,     0,     0,     0,     0,    11,   481,   482,   483,
     484,   485,   486,   487,     0,    12,     0,    13,     0,     0,
       0,   488,   489,    14,    15,     0,    16,     0,     0,     0,
      17,     0,     0,    18,     0,     0,     0,     0,     0,     0,
      19,     0,     0,     0,     0,     0,     0,    20,   297,     0,
       0,    21,     0,    22,    23,    24,    25,     0,     0,     0,
       0,     0,    26,     0,     0,     0,     0,    27,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     4,     5,
       0,   478,   479,   480,   481,   482,   483,   484,   485,   486,
     487,     0,     0,     0,     0,    28,     0,     0,   488,   489,
      29,    30,    31,     0,     0,    32,     6,    33,     7,     0,
       0,    34,     0,     0,     8,     9,     0,     0,    10,     0,
      35,     0,    36,    11,     0,     0,     0,     0,     0,     0,
      37,    38,    12,     0,    13,     0,     0,     0,     0,     0,
      14,    15,     0,    16,     0,     0,     0,    17,     0,     0,
      18,     0,     0,     0,     0,     0,     0,    19,     0,    39,
       0,     0,   307,     0,    20,   297,     0,     0,    21,     0,
      22,    23,    24,    25,     0,     0,     0,     0,     0,    26,
       0,     0,     0,     0,    27,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     4,     5,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    28,     0,     0,     0,     0,    29,    30,    31,
       0,     0,    32,     6,    33,     7,     0,     0,    34,     0,
       0,     8,     9,     0,     0,    10,     0,    35,     0,    36,
      11,     0,     0,     0,     0,     0,     0,    37,    38,    12,
       0,    13,     0,     0,     0,     0,     0,    14,    15,     0,
      16,     0,     0,     0,    17,     0,     0,    18,     0,     0,
       0,     0,     0,     0,    19,     0,    39,     0,     0,   609,
       0,    20,     0,     0,     0,    21,     0,    22,    23,    24,
      25,     0,     0,     0,     0,     0,    26,     4,     5,     0,
       0,    27,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     6,     0,     7,     0,    28,
       0,     0,     0,     8,    29,    30,    31,    10,     0,    32,
       0,    33,    11,     0,     0,    34,     0,     0,     0,     0,
       0,     0,     0,    13,    35,     0,    36,     0,     0,    14,
      15,     0,    16,     0,    37,    38,    17,     0,     0,    18,
       0,     0,     0,     0,     0,     0,    19,     0,     0,     0,
       0,     0,     0,    20,     0,     0,     0,    21,     0,    22,
      23,    24,     0,    39,     0,     0,   422,     0,    26,     4,
       5,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     6,     0,     7,
       0,    28,     0,     0,     0,     8,    29,    30,    31,    10,
       0,    32,     0,    33,    11,     0,     0,    34,     0,     0,
       0,     0,     0,     0,     0,    13,    35,     0,    36,     0,
       0,    14,    15,     0,    16,     0,    37,    38,    17,     0,
       0,    18,     0,     0,     0,     0,     0,     0,    19,     0,
       0,     0,     0,     0,     0,    20,     0,     0,     0,    21,
       0,    22,    23,    24,     0,    39,     0,     0,   431,     0,
      26,     4,     5,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   468,
     469,   470,   471,   472,   473,   474,   475,   476,     0,     6,
       0,     7,     0,    28,     0,     0,     0,     8,    29,    30,
      31,    10,     0,    32,     0,    33,    11,     0,     0,    34,
       0,     0,     0,     0,     0,     0,     0,    13,    35,     0,
      36,     0,     0,    14,    15,     0,    16,     0,    37,    38,
      17,     0,     0,    18,     0,     0,     0,     0,     0,     0,
      19,     0,     0,     0,     0,     0,     0,    20,     5,     0,
       0,    21,     0,    22,    23,    24,     0,    39,     0,     0,
     550,     0,    26,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     6,     0,     7,     0,     0,
       0,     0,     0,     8,     0,     0,     0,    10,     0,     0,
       0,     0,     0,     0,     0,    28,     0,     0,     0,     0,
      29,    30,    31,    13,     0,    32,     0,    33,     0,   205,
      15,    34,    16,     0,     0,     0,    17,     0,     0,    18,
      35,     0,    36,     0,     0,     0,    19,     0,     0,     0,
      37,    38,     0,     0,     0,     0,     0,    21,     0,    22,
      23,    24,     0,     0,     0,     0,     0,   477,    26,   478,
     479,   480,   481,   482,   483,   484,   485,   486,   487,    39,
       0,     0,   644,     0,     4,     5,   488,   489,   553,     0,
       0,     0,     0,   296,     0,     0,     0,     0,     0,     0,
       0,    28,     0,     0,     0,     0,    29,    30,    31,     0,
       0,    32,     6,    33,     7,     0,     0,   208,     0,     0,
       8,     9,     0,     0,    10,     0,   209,     0,    36,    11,
       0,     0,     0,     0,     0,     0,    37,    38,    12,     0,
      13,     0,     0,     0,     0,     0,    14,    15,     0,    16,
       0,     0,     0,    17,     0,     0,    18,     0,     0,     0,
       0,     0,     0,    19,     0,    39,     0,     0,   544,     0,
      20,   297,     0,     0,    21,     0,    22,    23,    24,    25,
       0,     0,     0,     0,     0,    26,     0,     0,     0,     0,
      27,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     4,     5,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    28,     0,
       0,     0,     0,    29,    30,    31,     0,     0,    32,     6,
      33,     7,     0,     0,    34,     0,     0,     8,     9,     0,
       0,    10,     0,    35,     0,    36,    11,     0,     0,     0,
       0,     0,     0,    37,    38,    12,     0,    13,     0,     0,
       0,     0,     0,    14,    15,     0,    16,     0,     0,     0,
      17,     0,     0,    18,     0,     0,     0,     0,     0,     0,
      19,     0,    39,     0,     0,     0,     0,    20,     5,     0,
       0,    21,     0,    22,    23,    24,    25,     0,     0,     0,
       0,     0,    26,     0,     0,     0,     0,    27,     0,     0,
       0,     0,     0,     0,     0,     6,     0,     7,     0,     0,
       0,     0,   237,     8,     0,     0,     0,    10,     0,     0,
       0,     0,     0,     0,     0,    28,     0,     0,     0,     0,
      29,    30,    31,    13,     0,    32,     0,    33,     0,   205,
      15,    34,    16,     0,   238,     0,    17,     0,   -59,    18,
      35,     0,    36,     0,     0,     0,    19,     0,     0,     0,
      37,    38,     0,     0,     0,     0,     0,    21,     0,    22,
      23,    24,     0,     0,     0,     0,     0,     0,    26,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    39,
       0,     0,     0,     0,     0,     0,     0,     0,   239,     0,
       0,     0,     0,     0,     0,     5,     0,     0,     0,     0,
       0,    28,     0,     0,     0,     0,    29,    30,    31,     0,
       0,    32,     0,    33,   240,     0,     0,   208,     0,     0,
       0,     0,     6,     0,     7,     0,   209,     0,    36,   237,
       8,     0,     0,     0,    10,     0,    37,    38,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      13,     0,     0,     0,   598,     0,   205,    15,     0,    16,
       0,   238,     0,    17,     0,    39,    18,     0,     0,     0,
       0,     0,     0,    19,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    21,     0,    22,    23,    24,     0,
       0,     0,     0,     0,     0,    26,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   239,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     5,    28,     0,
       0,     0,     0,    29,    30,    31,     0,     0,    32,     0,
      33,   240,     0,     0,   208,   615,     0,     0,     0,     0,
       0,     0,     0,   209,     6,    36,     7,     0,     0,     0,
       0,     0,     8,    37,    38,     0,    10,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   625,    13,     0,     0,     0,     0,     0,   205,    15,
       0,    16,    39,     0,     0,    17,     0,     0,    18,     0,
       0,     0,     0,     0,     0,    19,     0,     0,     0,     0,
       0,     0,     0,     5,     0,     0,    21,     0,    22,    23,
      24,     0,     0,     0,     0,     0,     0,    26,   468,   469,
     470,   471,   472,   473,   474,   475,   476,     0,     0,     0,
       6,     0,     7,     0,     0,     0,     0,     0,     8,     0,
       0,     0,    10,     0,     0,     0,     0,     0,     0,     0,
      28,     0,     0,     0,     0,    29,    30,    31,    13,     0,
      32,     0,    33,     0,   205,    15,   208,    16,     0,     0,
       0,    17,     0,     0,    18,   209,     0,    36,     0,     0,
       0,    19,     0,     0,     0,    37,    38,     0,     0,     0,
       0,     0,    21,     0,    22,    23,    24,     0,     0,     0,
       0,     0,     0,    26,   468,   469,   470,   471,   472,   473,
     474,   475,   476,     0,    39,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   468,
     469,   470,   471,   472,   473,   474,    28,     0,     0,     0,
       0,    29,    30,    31,     0,     0,    32,     0,    33,     0,
       0,     0,   208,     0,     0,     0,     0,     0,     0,     0,
       0,   209,     0,    36,     0,     0,     0,     0,     0,     0,
       0,    37,    38,   468,   469,   470,   471,   472,   473,   474,
     475,   476,     0,     0,     0,     0,   477,     0,   478,   479,
     480,   481,   482,   483,   484,   485,   486,   487,     0,     0,
      39,     0,     0,     0,     0,   488,   489,     0,     0,     0,
       0,     0,   523,   468,   469,   470,   471,   472,   473,   474,
     475,   476,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   468,   469,   470,   471,   472,   473,
     474,   475,   476,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   468,   469,   470,   471,   472,   473,   474,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   477,     0,   478,   479,   480,   481,   482,   483,
     484,   485,   486,   487,   468,   469,   470,   471,   472,   473,
     474,   488,   489,     0,     0,     0,     0,     0,   564,   478,
     479,   480,   481,   482,   483,   484,   485,   486,   487,     0,
       0,     0,     0,     0,     0,     0,   488,   489,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   477,     0,   478,   479,   480,   481,   482,   483,   484,
     485,   486,   487,     0,     0,     0,     0,     0,     0,     0,
     488,   489,   634,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   477,   633,   478,   479,   480,   481,   482,   483,   484,
     485,   486,   487,     0,     0,     0,     0,     0,     0,     0,
     488,   489,   477,     0,   478,   479,   480,   481,   482,   483,
     484,   485,   486,   487,     5,     0,     0,     0,     0,     0,
       0,   488,   489,   479,   480,   481,   482,   483,   484,   485,
     486,   487,     0,     0,     0,     0,     0,     0,     0,   488,
     489,     6,     0,     7,     0,     0,     0,     0,     0,     8,
       9,     0,     0,    10,     0,     0,   480,   481,   482,   483,
     484,   485,   486,   487,     0,     0,     0,    12,     0,    13,
       0,   488,   489,     0,     0,   205,    15,     0,    16,     0,
       0,     0,    17,     0,     0,    18,     0,     0,     0,     0,
       0,     0,    19,     0,     0,     0,     0,     0,     0,     0,
       5,     0,     0,    21,     0,    22,    23,    24,    25,     0,
       0,   206,     0,     0,    26,     0,     0,     0,   207,     0,
       0,     0,     0,     0,     0,     0,     0,     6,     0,     7,
       0,     0,     0,     0,     0,     8,     0,     0,     0,    10,
       0,     0,     0,     0,     0,     0,     0,    28,     0,     0,
       0,     0,    29,    30,    31,    13,     0,    32,     0,    33,
       0,   205,    15,   208,    16,     0,     0,     0,    17,     0,
       0,    18,   209,     0,    36,     0,     0,     0,    19,     0,
       0,     0,    37,    38,     0,     0,     5,     0,     0,    21,
       0,    22,    23,    24,     0,     0,     0,     0,     0,     0,
      26,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     6,     0,     7,     0,     0,     0,     0,
       0,     8,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    28,     0,     0,     0,     0,    29,    30,
      31,    13,     0,    32,     0,    33,     0,   205,    15,   208,
       0,     0,     0,     0,    17,     0,     0,    18,   209,     0,
      36,     0,     0,     0,    19,     0,     0,     0,    37,    38,
       0,     0,     0,     0,     0,     0,     0,    22,    23,    24,
       0,     0,     0,     0,     0,     0,    26,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    29,    30,    31,     0,     0,    32,
       0,     0,     0,     0,     0,   208,     0,     0,     0,     0,
       0,     0,     0,     0,   209,     0,    36,     0,     0,     0,
       0,     0,     0,     0,    37,    38,    98,     0,    99,   100,
     101,   102,   103,   104,     0,   105,     0,     0,   106,     0,
     107,     0,     0,     0,   108,   109,     0,   110,   111,   112,
     113,     0,   114,   115,   116,   117,   118,   119,   120,   121,
       0,   122,     0,   123,   124,   125,   126,   127,     0,     0,
     128,     0,     0,     0,   129,     0,   130,   131,     0,   132,
     133,   134,   135,   136,   137,     0,   138,   139,   140,   141,
     142,   143,     0,     0,   144,     0,     0,   145,     0,     0,
       0,     0,   146,   147,     0,   148,   149,     0,   150,   151,
       0,     0,     0,   152,   153,   154,   155,   156,   157,     0,
     158,   159,   160,   161,   162,   163,   164,     0,   165,   166,
       0,   167,   168,   169,   170,   171,   172,   173,   174,   175,
       0,   176,   177,   178,   179,     0,     0,     0,   180,     0,
       0,   181,     0,     0,   182,   183,     0,     0,   184,   185,
     186,   187,     0,     0,   188,     0,   189,     0,   190,   191,
     192,   193,   194,   195,   196,     0,     0,   197
};

#define yypact_value_is_default(yystate) \
  ((yystate) == (-526))

#define yytable_value_is_error(yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
       2,     2,    69,     2,    69,     2,     2,   307,   269,     2,
      48,   272,   268,   274,   246,   204,   277,    45,     2,   276,
     325,   282,   242,   243,    70,     2,     2,   294,     2,   506,
       2,     2,   108,   558,    16,    38,    48,    58,   505,   507,
     327,    16,   509,   155,    90,     3,   155,     5,    51,    48,
     155,    48,    66,    67,    68,    48,    34,    35,    72,     0,
       3,   202,     5,   563,    48,   202,   207,   202,   568,   207,
     207,    31,   207,     3,   330,     5,    36,    99,    99,   133,
      16,    39,     9,   137,    67,    68,    44,   181,   565,    72,
     181,     3,   204,     5,   206,   204,    39,     3,   566,   204,
       6,     7,     8,     9,    10,    11,   128,   327,    96,    39,
     164,   181,   206,    71,    44,   181,   202,   208,   181,   419,
     206,   598,   647,     3,   624,   181,   202,    39,    71,   318,
     597,   431,   321,   600,   321,   603,     6,     7,   208,    39,
      44,    71,   208,   207,    44,   208,   413,   203,   625,   216,
     183,   216,   341,   342,   343,   342,   343,    26,   206,    71,
     627,    30,   206,   631,    70,    69,     3,   125,     5,   636,
       9,    71,     3,    12,     5,   242,   243,   242,   243,    14,
      15,    16,   125,   206,   202,   652,   447,   205,   255,   202,
      94,   202,   205,   151,   205,   125,   263,     3,   418,     5,
     202,   203,   205,   202,   203,   202,   203,   161,   151,   202,
     203,   117,   224,   125,   189,   190,   191,   192,   193,   201,
     202,   151,     3,   204,     5,   125,   201,   202,   207,   490,
     208,   209,     9,   191,   138,   467,   250,   283,   276,   151,
     286,   147,   288,   289,   549,   206,   150,   293,   191,   207,
     550,   151,   204,   206,   160,   191,   192,   193,   204,   520,
     327,   191,   327,   207,   207,   201,   202,   250,   206,     3,
     308,     5,   202,     3,   276,     5,   204,   207,   206,   191,
     186,   513,   206,   189,   190,   191,   204,    39,   194,   195,
     202,   191,   206,   206,   332,   207,   206,   203,   555,   204,
     207,   207,   202,   204,     3,   206,     5,   207,   608,   206,
     597,   176,   177,   178,   179,   180,   204,   319,   319,    71,
     319,   541,   319,   319,   326,   326,   319,   326,   366,   326,
     332,   563,     3,   326,     5,   319,   568,   624,   202,   571,
      40,   343,   319,   319,   644,   319,   384,   319,   319,   636,
       3,   418,     5,   418,   189,   190,   191,   192,   193,     3,
       3,     5,     5,   207,   207,   652,   201,   202,     3,   207,
       5,   207,   206,   125,   208,   207,     3,   207,   207,   446,
     207,   207,   207,    83,   207,   207,   207,   425,   416,   645,
     610,   207,   624,   207,   207,   433,    96,    97,    98,   151,
     207,   206,   467,   207,   207,   105,   667,   207,   207,   207,
     207,   449,    14,    15,    16,    17,    18,    19,    20,    21,
      22,   459,   207,   207,   420,   427,   425,   207,   430,   430,
     204,   430,   207,   430,   433,   207,   433,   430,   207,   191,
     433,   207,   507,   206,   144,   206,   204,   209,   148,   208,
     202,   208,   449,     3,   207,   207,     6,     7,     8,     9,
      10,    11,     9,     9,     9,     9,     9,     9,     9,   507,
       9,     9,   207,   202,   541,   265,   541,   183,   268,   206,
     204,   204,   204,   672,   673,   275,   206,   208,   278,   205,
     181,   209,   207,   560,   284,   285,   208,   208,   563,   208,
     208,   566,   181,   568,   208,   507,   208,   545,     9,    90,
     208,   208,   208,   208,   208,   208,   208,   555,   208,   208,
      70,   208,   208,   208,   208,   208,   208,   208,   566,   201,
     208,   208,   208,   535,   208,   183,   535,   206,   535,   205,
     330,   331,   535,   610,   208,   610,   208,   181,   206,   206,
       3,     3,     9,   555,   208,     6,   181,   206,   208,   624,
     206,   208,   183,   208,   566,   603,   208,   117,   204,   208,
     360,   361,   362,   363,   364,   365,   366,   208,   208,   208,
     182,   222,   184,   185,   186,   187,   188,   189,   190,   191,
     192,   193,   208,   631,   183,   183,   206,   147,   446,   201,
     202,   603,   290,    48,   284,   429,   208,   515,   427,   216,
     160,   613,   613,   673,   613,   647,   613,    14,    15,    16,
     613,   280,    19,    20,    -1,    -1,    -1,    -1,    -1,   631,
      -1,    -1,    -1,    -1,    -1,    -1,   186,    -1,    -1,   189,
     190,   191,    -1,    -1,   194,   195,    -1,    -1,   438,    -1,
      -1,    -1,    -1,   203,    -1,    -1,    -1,   207,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   468,   469,
      -1,   471,   472,   473,   474,   475,   476,   477,   478,   479,
     480,   481,   482,   483,   484,   485,   486,   487,    -1,   489,
      -1,    -1,    -1,    -1,    -1,    -1,     3,    -1,     5,     6,
       7,     8,     9,    10,    11,    -1,     3,    -1,    -1,     6,
       7,     8,     9,    10,    11,   515,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    32,    -1,    34,    -1,    -1,
      -1,    -1,    -1,    40,    -1,    -1,    -1,    44,    -1,    -1,
      -1,    -1,    -1,    -1,    14,    15,    16,    17,    18,    19,
      20,    -1,    -1,    60,    -1,    -1,    -1,    -1,    -1,    66,
      67,    -1,    69,    70,    -1,    -1,    73,    -1,    -1,    76,
      -1,    -1,    -1,    70,    -1,    -1,    83,    -1,    -1,    -1,
     187,   188,   189,   190,   191,   192,   193,    94,    -1,    96,
      97,    98,    -1,    -1,   201,   202,    -1,    -1,   105,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     117,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     117,    14,    15,    16,    17,    18,    19,    20,    -1,    22,
     630,   138,    -1,   633,    -1,    -1,   143,   144,   145,    -1,
     147,   148,    -1,   150,    -1,   645,    -1,   154,    -1,    -1,
     147,    -1,    -1,   160,    -1,    -1,   163,    -1,   165,    -1,
      -1,    -1,    -1,   160,    -1,    -1,   173,   174,    -1,    -1,
      -1,     4,     5,    -1,    -1,    -1,    -1,    -1,    -1,   186,
      -1,    -1,   189,   190,   191,    -1,    -1,   194,   195,   186,
      -1,    -1,   189,   190,   191,    -1,    -1,   194,   195,    32,
     207,    34,    -1,    -1,    -1,    -1,    -1,    40,    41,    -1,
     207,    44,    -1,    -1,    -1,    -1,    49,   187,   188,   189,
     190,   191,   192,   193,    -1,    58,    -1,    60,    -1,    -1,
      -1,   201,   202,    66,    67,    -1,    69,    -1,    -1,    -1,
      73,    -1,    -1,    76,    -1,    -1,    -1,    -1,    -1,    -1,
      83,    -1,    -1,    -1,    -1,    -1,    -1,    90,    91,    -1,
      -1,    94,    -1,    96,    97,    98,    99,    -1,    -1,    -1,
      -1,    -1,   105,    -1,    -1,    -1,    -1,   110,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     4,     5,
      -1,   184,   185,   186,   187,   188,   189,   190,   191,   192,
     193,    -1,    -1,    -1,    -1,   138,    -1,    -1,   201,   202,
     143,   144,   145,    -1,    -1,   148,    32,   150,    34,    -1,
      -1,   154,    -1,    -1,    40,    41,    -1,    -1,    44,    -1,
     163,    -1,   165,    49,    -1,    -1,    -1,    -1,    -1,    -1,
     173,   174,    58,    -1,    60,    -1,    -1,    -1,    -1,    -1,
      66,    67,    -1,    69,    -1,    -1,    -1,    73,    -1,    -1,
      76,    -1,    -1,    -1,    -1,    -1,    -1,    83,    -1,   202,
      -1,    -1,   205,    -1,    90,    91,    -1,    -1,    94,    -1,
      96,    97,    98,    99,    -1,    -1,    -1,    -1,    -1,   105,
      -1,    -1,    -1,    -1,   110,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     4,     5,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   138,    -1,    -1,    -1,    -1,   143,   144,   145,
      -1,    -1,   148,    32,   150,    34,    -1,    -1,   154,    -1,
      -1,    40,    41,    -1,    -1,    44,    -1,   163,    -1,   165,
      49,    -1,    -1,    -1,    -1,    -1,    -1,   173,   174,    58,
      -1,    60,    -1,    -1,    -1,    -1,    -1,    66,    67,    -1,
      69,    -1,    -1,    -1,    73,    -1,    -1,    76,    -1,    -1,
      -1,    -1,    -1,    -1,    83,    -1,   202,    -1,    -1,   205,
      -1,    90,    -1,    -1,    -1,    94,    -1,    96,    97,    98,
      99,    -1,    -1,    -1,    -1,    -1,   105,     4,     5,    -1,
      -1,   110,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    32,    -1,    34,    -1,   138,
      -1,    -1,    -1,    40,   143,   144,   145,    44,    -1,   148,
      -1,   150,    49,    -1,    -1,   154,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    60,   163,    -1,   165,    -1,    -1,    66,
      67,    -1,    69,    -1,   173,   174,    73,    -1,    -1,    76,
      -1,    -1,    -1,    -1,    -1,    -1,    83,    -1,    -1,    -1,
      -1,    -1,    -1,    90,    -1,    -1,    -1,    94,    -1,    96,
      97,    98,    -1,   202,    -1,    -1,   205,    -1,   105,     4,
       5,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    32,    -1,    34,
      -1,   138,    -1,    -1,    -1,    40,   143,   144,   145,    44,
      -1,   148,    -1,   150,    49,    -1,    -1,   154,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    60,   163,    -1,   165,    -1,
      -1,    66,    67,    -1,    69,    -1,   173,   174,    73,    -1,
      -1,    76,    -1,    -1,    -1,    -1,    -1,    -1,    83,    -1,
      -1,    -1,    -1,    -1,    -1,    90,    -1,    -1,    -1,    94,
      -1,    96,    97,    98,    -1,   202,    -1,    -1,   205,    -1,
     105,     4,     5,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    -1,    32,
      -1,    34,    -1,   138,    -1,    -1,    -1,    40,   143,   144,
     145,    44,    -1,   148,    -1,   150,    49,    -1,    -1,   154,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    60,   163,    -1,
     165,    -1,    -1,    66,    67,    -1,    69,    -1,   173,   174,
      73,    -1,    -1,    76,    -1,    -1,    -1,    -1,    -1,    -1,
      83,    -1,    -1,    -1,    -1,    -1,    -1,    90,     5,    -1,
      -1,    94,    -1,    96,    97,    98,    -1,   202,    -1,    -1,
     205,    -1,   105,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    32,    -1,    34,    -1,    -1,
      -1,    -1,    -1,    40,    -1,    -1,    -1,    44,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   138,    -1,    -1,    -1,    -1,
     143,   144,   145,    60,    -1,   148,    -1,   150,    -1,    66,
      67,   154,    69,    -1,    -1,    -1,    73,    -1,    -1,    76,
     163,    -1,   165,    -1,    -1,    -1,    83,    -1,    -1,    -1,
     173,   174,    -1,    -1,    -1,    -1,    -1,    94,    -1,    96,
      97,    98,    -1,    -1,    -1,    -1,    -1,   182,   105,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   193,   202,
      -1,    -1,   205,    -1,     4,     5,   201,   202,   203,    -1,
      -1,    -1,    -1,    13,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   138,    -1,    -1,    -1,    -1,   143,   144,   145,    -1,
      -1,   148,    32,   150,    34,    -1,    -1,   154,    -1,    -1,
      40,    41,    -1,    -1,    44,    -1,   163,    -1,   165,    49,
      -1,    -1,    -1,    -1,    -1,    -1,   173,   174,    58,    -1,
      60,    -1,    -1,    -1,    -1,    -1,    66,    67,    -1,    69,
      -1,    -1,    -1,    73,    -1,    -1,    76,    -1,    -1,    -1,
      -1,    -1,    -1,    83,    -1,   202,    -1,    -1,   205,    -1,
      90,    91,    -1,    -1,    94,    -1,    96,    97,    98,    99,
      -1,    -1,    -1,    -1,    -1,   105,    -1,    -1,    -1,    -1,
     110,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     4,     5,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   138,    -1,
      -1,    -1,    -1,   143,   144,   145,    -1,    -1,   148,    32,
     150,    34,    -1,    -1,   154,    -1,    -1,    40,    41,    -1,
      -1,    44,    -1,   163,    -1,   165,    49,    -1,    -1,    -1,
      -1,    -1,    -1,   173,   174,    58,    -1,    60,    -1,    -1,
      -1,    -1,    -1,    66,    67,    -1,    69,    -1,    -1,    -1,
      73,    -1,    -1,    76,    -1,    -1,    -1,    -1,    -1,    -1,
      83,    -1,   202,    -1,    -1,    -1,    -1,    90,     5,    -1,
      -1,    94,    -1,    96,    97,    98,    99,    -1,    -1,    -1,
      -1,    -1,   105,    -1,    -1,    -1,    -1,   110,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    32,    -1,    34,    -1,    -1,
      -1,    -1,    39,    40,    -1,    -1,    -1,    44,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   138,    -1,    -1,    -1,    -1,
     143,   144,   145,    60,    -1,   148,    -1,   150,    -1,    66,
      67,   154,    69,    -1,    71,    -1,    73,    -1,   161,    76,
     163,    -1,   165,    -1,    -1,    -1,    83,    -1,    -1,    -1,
     173,   174,    -1,    -1,    -1,    -1,    -1,    94,    -1,    96,
      97,    98,    -1,    -1,    -1,    -1,    -1,    -1,   105,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   125,    -1,
      -1,    -1,    -1,    -1,    -1,     5,    -1,    -1,    -1,    -1,
      -1,   138,    -1,    -1,    -1,    -1,   143,   144,   145,    -1,
      -1,   148,    -1,   150,   151,    -1,    -1,   154,    -1,    -1,
      -1,    -1,    32,    -1,    34,    -1,   163,    -1,   165,    39,
      40,    -1,    -1,    -1,    44,    -1,   173,   174,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      60,    -1,    -1,    -1,   191,    -1,    66,    67,    -1,    69,
      -1,    71,    -1,    73,    -1,   202,    76,    -1,    -1,    -1,
      -1,    -1,    -1,    83,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    94,    -1,    96,    97,    98,    -1,
      -1,    -1,    -1,    -1,    -1,   105,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   125,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     5,   138,    -1,
      -1,    -1,    -1,   143,   144,   145,    -1,    -1,   148,    -1,
     150,   151,    -1,    -1,   154,    23,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   163,    32,   165,    34,    -1,    -1,    -1,
      -1,    -1,    40,   173,   174,    -1,    44,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   191,    60,    -1,    -1,    -1,    -1,    -1,    66,    67,
      -1,    69,   202,    -1,    -1,    73,    -1,    -1,    76,    -1,
      -1,    -1,    -1,    -1,    -1,    83,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     5,    -1,    -1,    94,    -1,    96,    97,
      98,    -1,    -1,    -1,    -1,    -1,    -1,   105,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    -1,    -1,    -1,
      32,    -1,    34,    -1,    -1,    -1,    -1,    -1,    40,    -1,
      -1,    -1,    44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     138,    -1,    -1,    -1,    -1,   143,   144,   145,    60,    -1,
     148,    -1,   150,    -1,    66,    67,   154,    69,    -1,    -1,
      -1,    73,    -1,    -1,    76,   163,    -1,   165,    -1,    -1,
      -1,    83,    -1,    -1,    -1,   173,   174,    -1,    -1,    -1,
      -1,    -1,    94,    -1,    96,    97,    98,    -1,    -1,    -1,
      -1,    -1,    -1,   105,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    -1,   202,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    14,
      15,    16,    17,    18,    19,    20,   138,    -1,    -1,    -1,
      -1,   143,   144,   145,    -1,    -1,   148,    -1,   150,    -1,
      -1,    -1,   154,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   163,    -1,   165,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   173,   174,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    -1,    -1,    -1,    -1,   182,    -1,   184,   185,
     186,   187,   188,   189,   190,   191,   192,   193,    -1,    -1,
     202,    -1,    -1,    -1,    -1,   201,   202,    -1,    -1,    -1,
      -1,    -1,   208,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    14,    15,    16,    17,    18,    19,    20,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   182,    -1,   184,   185,   186,   187,   188,   189,
     190,   191,   192,   193,    14,    15,    16,    17,    18,    19,
      20,   201,   202,    -1,    -1,    -1,    -1,    -1,   208,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   193,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   201,   202,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   182,    -1,   184,   185,   186,   187,   188,   189,   190,
     191,   192,   193,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     201,   202,   203,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,   192,   193,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     201,   202,   182,    -1,   184,   185,   186,   187,   188,   189,
     190,   191,   192,   193,     5,    -1,    -1,    -1,    -1,    -1,
      -1,   201,   202,   185,   186,   187,   188,   189,   190,   191,
     192,   193,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   201,
     202,    32,    -1,    34,    -1,    -1,    -1,    -1,    -1,    40,
      41,    -1,    -1,    44,    -1,    -1,   186,   187,   188,   189,
     190,   191,   192,   193,    -1,    -1,    -1,    58,    -1,    60,
      -1,   201,   202,    -1,    -1,    66,    67,    -1,    69,    -1,
      -1,    -1,    73,    -1,    -1,    76,    -1,    -1,    -1,    -1,
      -1,    -1,    83,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       5,    -1,    -1,    94,    -1,    96,    97,    98,    99,    -1,
      -1,   102,    -1,    -1,   105,    -1,    -1,    -1,   109,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    32,    -1,    34,
      -1,    -1,    -1,    -1,    -1,    40,    -1,    -1,    -1,    44,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   138,    -1,    -1,
      -1,    -1,   143,   144,   145,    60,    -1,   148,    -1,   150,
      -1,    66,    67,   154,    69,    -1,    -1,    -1,    73,    -1,
      -1,    76,   163,    -1,   165,    -1,    -1,    -1,    83,    -1,
      -1,    -1,   173,   174,    -1,    -1,     5,    -1,    -1,    94,
      -1,    96,    97,    98,    -1,    -1,    -1,    -1,    -1,    -1,
     105,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    32,    -1,    34,    -1,    -1,    -1,    -1,
      -1,    40,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   138,    -1,    -1,    -1,    -1,   143,   144,
     145,    60,    -1,   148,    -1,   150,    -1,    66,    67,   154,
      -1,    -1,    -1,    -1,    73,    -1,    -1,    76,   163,    -1,
     165,    -1,    -1,    -1,    83,    -1,    -1,    -1,   173,   174,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    96,    97,    98,
      -1,    -1,    -1,    -1,    -1,    -1,   105,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   143,   144,   145,    -1,    -1,   148,
      -1,    -1,    -1,    -1,    -1,   154,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   163,    -1,   165,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   173,   174,    24,    -1,    26,    27,
      28,    29,    30,    31,    -1,    33,    -1,    -1,    36,    -1,
      38,    -1,    -1,    -1,    42,    43,    -1,    45,    46,    47,
      48,    -1,    50,    51,    52,    53,    54,    55,    56,    57,
      -1,    59,    -1,    61,    62,    63,    64,    65,    -1,    -1,
      68,    -1,    -1,    -1,    72,    -1,    74,    75,    -1,    77,
      78,    79,    80,    81,    82,    -1,    84,    85,    86,    87,
      88,    89,    -1,    -1,    92,    -1,    -1,    95,    -1,    -1,
      -1,    -1,   100,   101,    -1,   103,   104,    -1,   106,   107,
      -1,    -1,    -1,   111,   112,   113,   114,   115,   116,    -1,
     118,   119,   120,   121,   122,   123,   124,    -1,   126,   127,
      -1,   129,   130,   131,   132,   133,   134,   135,   136,   137,
      -1,   139,   140,   141,   142,    -1,    -1,    -1,   146,    -1,
      -1,   149,    -1,    -1,   152,   153,    -1,    -1,   156,   157,
     158,   159,    -1,    -1,   162,    -1,   164,    -1,   166,   167,
     168,   169,   170,   171,   172,    -1,    -1,   175
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   211,   212,     0,     4,     5,    32,    34,    40,    41,
      44,    49,    58,    60,    66,    67,    69,    73,    76,    83,
      90,    94,    96,    97,    98,    99,   105,   110,   138,   143,
     144,   145,   148,   150,   154,   163,   165,   173,   174,   202,
     216,   217,   218,   219,   220,   223,   224,   230,   231,   242,
     256,   260,   262,   263,   264,   265,   266,   269,   270,   273,
     275,   276,   277,   278,   280,   281,   282,   283,   284,   286,
     288,   306,   307,   308,   309,     3,     5,   207,     3,     5,
       3,     5,   258,    96,   261,     9,     3,     5,   261,     3,
     207,   261,   262,     3,   258,     3,   258,   262,    24,    26,
      27,    28,    29,    30,    31,    33,    36,    38,    42,    43,
      45,    46,    47,    48,    50,    51,    52,    53,    54,    55,
      56,    57,    59,    61,    62,    63,    64,    65,    68,    72,
      74,    75,    77,    78,    79,    80,    81,    82,    84,    85,
      86,    87,    88,    89,    92,    95,   100,   101,   103,   104,
     106,   107,   111,   112,   113,   114,   115,   116,   118,   119,
     120,   121,   122,   123,   124,   126,   127,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   139,   140,   141,   142,
     146,   149,   152,   153,   156,   157,   158,   159,   162,   164,
     166,   167,   168,   169,   170,   171,   172,   175,   232,   234,
     305,   206,   213,   213,   161,    66,   102,   109,   154,   163,
     222,   242,   263,   269,   275,   279,   286,   306,   309,   206,
     206,   204,   204,   206,   204,   206,   215,   206,   183,   274,
     204,   287,   288,   287,   287,     3,     5,    39,    71,   125,
     151,   191,   207,   236,   259,   289,   290,   303,   242,   306,
     307,   309,   287,   206,     9,   204,   206,   307,   204,   155,
     204,   207,   207,   207,   207,   207,   207,   207,   207,   207,
     207,   207,   207,   207,   207,   207,   207,   207,   207,   207,
     207,   207,   207,   207,   207,   207,   207,   207,   207,   207,
     207,   207,   207,   207,   181,   203,    13,    91,   216,   221,
     224,   263,   265,   266,   277,   278,   281,   205,   230,   231,
       3,     3,     3,     5,     3,     3,   204,   303,   267,   212,
     128,   271,   275,     3,     5,   204,   214,   285,   289,   289,
     209,   202,   207,   229,   287,   208,   239,   240,   241,   259,
     208,   249,   207,   252,     9,     9,    12,   235,   259,     3,
       6,     7,     8,     9,    10,    11,    70,   117,   147,   160,
     186,   189,   190,   191,   194,   195,   207,   245,   246,   247,
     245,   248,     9,     9,   233,   248,   247,     9,     9,   247,
       9,   247,   245,   228,   231,   286,   247,   243,   244,   245,
       9,   305,     9,   247,   307,   243,   245,   307,   176,   177,
     178,   179,   180,   304,   307,   307,   235,     6,     7,   310,
       9,   307,   234,   202,   207,   206,   204,   215,   286,   205,
     230,   268,   205,   183,   108,   230,   254,   272,   206,   204,
     214,   205,   216,   231,   284,   289,   208,   248,   191,   203,
     245,   225,   226,   227,   228,   205,   181,   209,   205,   230,
     250,   254,   205,   231,   251,   254,   208,   208,   208,   207,
     245,   245,   245,   245,   245,   245,   245,   286,    14,    15,
      16,    17,    18,    19,    20,    21,    22,   182,   184,   185,
     186,   187,   188,   189,   190,   191,   192,   193,   201,   202,
     181,   208,   208,   208,   181,   208,   208,   208,   208,   208,
     208,   208,   208,   208,   208,   286,   191,   207,   229,   236,
     259,   295,   297,   298,   208,   181,   208,   208,   208,   208,
     181,   208,   208,   208,   208,   208,   208,   208,   208,   201,
     208,   208,   208,   234,     9,   213,   289,   299,   215,   278,
     183,   286,   306,   206,   205,   231,   255,   256,   205,   220,
     205,   215,   203,   203,   208,   181,   241,   247,   286,   309,
     208,   206,   206,   286,   208,   191,   207,   229,   236,   291,
     293,   294,   245,   245,     3,   245,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   245,
     245,   245,     3,   245,   247,     9,   297,   285,   191,   225,
     236,   296,   297,   207,   229,   244,   247,     6,   208,   205,
     181,   289,   206,   214,   215,    23,   228,   295,   301,   302,
     206,   257,   259,   293,   285,   191,   225,   236,   292,   293,
     208,   207,   229,   183,   203,   297,   285,   208,   297,   208,
     225,   208,   215,   289,   205,   183,   300,   181,   206,   204,
     208,   293,   285,   208,   297,   208,   245,   225,   245,   297,
     208,   215,   248,   301,   237,   297,   208,    38,    51,   205,
     238,   247,   183,   183,   206,   253,   254,   253,   206
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


/* This macro is provided for backward compatibility. */

#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
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

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  YYSIZE_T yysize1;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = 0;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
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
      int yyn = yypact[*yyssp];
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
                yysize1 = yysize + yytnamerr (0, yytname[yyx]);
                if (! (yysize <= yysize1
                       && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                  return 2;
                yysize = yysize1;
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  yysize1 = yysize + yystrlen (yyformat);
  if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
    return 2;
  yysize = yysize1;

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
          yyp++;
          yyformat++;
        }
  }
  return 0;
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


/*----------.
| yyparse.  |
`----------*/

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
  if (yypact_value_is_default (yyn))
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

/* Line 1806 of yacc.c  */
#line 308 "parser.y"
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
						}
    break;

  case 3:

/* Line 1806 of yacc.c  */
#line 322 "parser.y"
    { (yyval.stmt_list) = NULL; }
    break;

  case 4:

/* Line 1806 of yacc.c  */
#line 324 "parser.y"
    { (yyval.stmt_list) = append_statements((yyvsp[(1) - (5)].stmt_list), (yyvsp[(4) - (5)].stmt_list)); }
    break;

  case 5:

/* Line 1806 of yacc.c  */
#line 325 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_reference((yyvsp[(2) - (2)].type))); }
    break;

  case 6:

/* Line 1806 of yacc.c  */
#line 326 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_type_decl((yyvsp[(2) - (2)].type))); }
    break;

  case 7:

/* Line 1806 of yacc.c  */
#line 327 "parser.y"
    { (yyval.stmt_list) = (yyvsp[(1) - (3)].stmt_list);
						  reg_type((yyvsp[(2) - (3)].type), (yyvsp[(2) - (3)].type)->name, 0);
						}
    break;

  case 8:

/* Line 1806 of yacc.c  */
#line 330 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_type_decl((yyvsp[(2) - (2)].type)));
						  reg_type((yyvsp[(2) - (2)].type), (yyvsp[(2) - (2)].type)->name, 0);
						}
    break;

  case 9:

/* Line 1806 of yacc.c  */
#line 333 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_module((yyvsp[(2) - (2)].type))); }
    break;

  case 10:

/* Line 1806 of yacc.c  */
#line 334 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_library((yyvsp[(2) - (2)].typelib))); }
    break;

  case 11:

/* Line 1806 of yacc.c  */
#line 335 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), (yyvsp[(2) - (2)].statement)); }
    break;

  case 12:

/* Line 1806 of yacc.c  */
#line 338 "parser.y"
    { (yyval.stmt_list) = NULL; }
    break;

  case 13:

/* Line 1806 of yacc.c  */
#line 339 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_reference((yyvsp[(2) - (2)].type))); }
    break;

  case 14:

/* Line 1806 of yacc.c  */
#line 341 "parser.y"
    { (yyval.stmt_list) = append_statements((yyvsp[(1) - (5)].stmt_list), (yyvsp[(4) - (5)].stmt_list)); }
    break;

  case 15:

/* Line 1806 of yacc.c  */
#line 342 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_type_decl((yyvsp[(2) - (2)].type))); }
    break;

  case 16:

/* Line 1806 of yacc.c  */
#line 343 "parser.y"
    { (yyval.stmt_list) = (yyvsp[(1) - (3)].stmt_list); reg_type((yyvsp[(2) - (3)].type), (yyvsp[(2) - (3)].type)->name, 0); }
    break;

  case 17:

/* Line 1806 of yacc.c  */
#line 344 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_type_decl((yyvsp[(2) - (2)].type)));
						  reg_type((yyvsp[(2) - (2)].type), (yyvsp[(2) - (2)].type)->name, 0);
						}
    break;

  case 18:

/* Line 1806 of yacc.c  */
#line 347 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_module((yyvsp[(2) - (2)].type))); }
    break;

  case 19:

/* Line 1806 of yacc.c  */
#line 348 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), (yyvsp[(2) - (2)].statement)); }
    break;

  case 20:

/* Line 1806 of yacc.c  */
#line 349 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_importlib((yyvsp[(2) - (2)].str))); }
    break;

  case 21:

/* Line 1806 of yacc.c  */
#line 350 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), make_statement_library((yyvsp[(2) - (2)].typelib))); }
    break;

  case 22:

/* Line 1806 of yacc.c  */
#line 353 "parser.y"
    { (yyval.stmt_list) = NULL; }
    break;

  case 23:

/* Line 1806 of yacc.c  */
#line 354 "parser.y"
    { (yyval.stmt_list) = append_statement((yyvsp[(1) - (2)].stmt_list), (yyvsp[(2) - (2)].statement)); }
    break;

  case 26:

/* Line 1806 of yacc.c  */
#line 362 "parser.y"
    { (yyval.statement) = make_statement_cppquote((yyvsp[(1) - (1)].str)); }
    break;

  case 27:

/* Line 1806 of yacc.c  */
#line 363 "parser.y"
    { (yyval.statement) = make_statement_type_decl((yyvsp[(1) - (2)].type)); }
    break;

  case 28:

/* Line 1806 of yacc.c  */
#line 364 "parser.y"
    { (yyval.statement) = make_statement_declaration((yyvsp[(1) - (2)].var)); }
    break;

  case 29:

/* Line 1806 of yacc.c  */
#line 365 "parser.y"
    { (yyval.statement) = make_statement_import((yyvsp[(1) - (1)].str)); }
    break;

  case 30:

/* Line 1806 of yacc.c  */
#line 366 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (2)].statement); }
    break;

  case 31:

/* Line 1806 of yacc.c  */
#line 367 "parser.y"
    { (yyval.statement) = make_statement_pragma((yyvsp[(1) - (1)].str)); }
    break;

  case 33:

/* Line 1806 of yacc.c  */
#line 372 "parser.y"
    { (yyval.type) = type_new_enum((yyvsp[(2) - (2)].str), FALSE, NULL); }
    break;

  case 35:

/* Line 1806 of yacc.c  */
#line 374 "parser.y"
    { (yyval.type) = type_new_struct((yyvsp[(2) - (2)].str), FALSE, NULL); }
    break;

  case 37:

/* Line 1806 of yacc.c  */
#line 376 "parser.y"
    { (yyval.type) = type_new_nonencapsulated_union((yyvsp[(2) - (2)].str), FALSE, NULL); }
    break;

  case 38:

/* Line 1806 of yacc.c  */
#line 377 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type); (yyval.type)->attrs = check_enum_attrs((yyvsp[(1) - (2)].attr_list)); }
    break;

  case 39:

/* Line 1806 of yacc.c  */
#line 378 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type); (yyval.type)->attrs = check_struct_attrs((yyvsp[(1) - (2)].attr_list)); }
    break;

  case 40:

/* Line 1806 of yacc.c  */
#line 379 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type); (yyval.type)->attrs = check_union_attrs((yyvsp[(1) - (2)].attr_list)); }
    break;

  case 41:

/* Line 1806 of yacc.c  */
#line 382 "parser.y"
    { (yyval.str) = (yyvsp[(3) - (4)].str); }
    break;

  case 42:

/* Line 1806 of yacc.c  */
#line 384 "parser.y"
    { assert(yychar == YYEMPTY);
						  (yyval.import) = xmalloc(sizeof(struct _import_t));
						  (yyval.import)->name = (yyvsp[(2) - (3)].str);
						  (yyval.import)->import_performed = do_import((yyvsp[(2) - (3)].str));
						  if (!(yyval.import)->import_performed) yychar = aEOF;
						}
    break;

  case 43:

/* Line 1806 of yacc.c  */
#line 392 "parser.y"
    { (yyval.str) = (yyvsp[(1) - (3)].import)->name;
						  if ((yyvsp[(1) - (3)].import)->import_performed) pop_import();
						  free((yyvsp[(1) - (3)].import));
						}
    break;

  case 44:

/* Line 1806 of yacc.c  */
#line 399 "parser.y"
    { (yyval.str) = (yyvsp[(3) - (5)].str); if(!parse_only) add_importlib((yyvsp[(3) - (5)].str)); }
    break;

  case 45:

/* Line 1806 of yacc.c  */
#line 402 "parser.y"
    { (yyval.str) = (yyvsp[(2) - (2)].str); }
    break;

  case 46:

/* Line 1806 of yacc.c  */
#line 404 "parser.y"
    { (yyval.typelib) = make_library((yyvsp[(2) - (3)].str), check_library_attrs((yyvsp[(2) - (3)].str), (yyvsp[(1) - (3)].attr_list)));
						  if (!parse_only) start_typelib((yyval.typelib));
						}
    break;

  case 47:

/* Line 1806 of yacc.c  */
#line 409 "parser.y"
    { (yyval.typelib) = (yyvsp[(1) - (4)].typelib);
						  (yyval.typelib)->stmts = (yyvsp[(2) - (4)].stmt_list);
						  if (!parse_only) end_typelib();
						}
    break;

  case 48:

/* Line 1806 of yacc.c  */
#line 415 "parser.y"
    { (yyval.var_list) = NULL; }
    break;

  case 50:

/* Line 1806 of yacc.c  */
#line 419 "parser.y"
    { check_arg_attrs((yyvsp[(1) - (1)].var)); (yyval.var_list) = append_var( NULL, (yyvsp[(1) - (1)].var) ); }
    break;

  case 51:

/* Line 1806 of yacc.c  */
#line 420 "parser.y"
    { check_arg_attrs((yyvsp[(3) - (3)].var)); (yyval.var_list) = append_var( (yyvsp[(1) - (3)].var_list), (yyvsp[(3) - (3)].var) ); }
    break;

  case 53:

/* Line 1806 of yacc.c  */
#line 424 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[(1) - (3)].var_list), make_var(strdup("...")) ); }
    break;

  case 54:

/* Line 1806 of yacc.c  */
#line 428 "parser.y"
    { if ((yyvsp[(2) - (3)].declspec)->stgclass != STG_NONE && (yyvsp[(2) - (3)].declspec)->stgclass != STG_REGISTER)
						    error_loc("invalid storage class for function parameter\n");
						  (yyval.var) = declare_var((yyvsp[(1) - (3)].attr_list), (yyvsp[(2) - (3)].declspec), (yyvsp[(3) - (3)].declarator), TRUE);
						  free((yyvsp[(2) - (3)].declspec)); free((yyvsp[(3) - (3)].declarator));
						}
    break;

  case 55:

/* Line 1806 of yacc.c  */
#line 433 "parser.y"
    { if ((yyvsp[(1) - (2)].declspec)->stgclass != STG_NONE && (yyvsp[(1) - (2)].declspec)->stgclass != STG_REGISTER)
						    error_loc("invalid storage class for function parameter\n");
						  (yyval.var) = declare_var(NULL, (yyvsp[(1) - (2)].declspec), (yyvsp[(2) - (2)].declarator), TRUE);
						  free((yyvsp[(1) - (2)].declspec)); free((yyvsp[(2) - (2)].declarator));
						}
    break;

  case 56:

/* Line 1806 of yacc.c  */
#line 440 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (3)].expr);
						  if (!(yyval.expr)->is_const)
						      error_loc("array dimension is not an integer constant\n");
						}
    break;

  case 57:

/* Line 1806 of yacc.c  */
#line 444 "parser.y"
    { (yyval.expr) = make_expr(EXPR_VOID); }
    break;

  case 58:

/* Line 1806 of yacc.c  */
#line 445 "parser.y"
    { (yyval.expr) = make_expr(EXPR_VOID); }
    break;

  case 59:

/* Line 1806 of yacc.c  */
#line 448 "parser.y"
    { (yyval.attr_list) = NULL; }
    break;

  case 61:

/* Line 1806 of yacc.c  */
#line 453 "parser.y"
    { (yyval.attr_list) = (yyvsp[(2) - (3)].attr_list); }
    break;

  case 62:

/* Line 1806 of yacc.c  */
#line 456 "parser.y"
    { (yyval.attr_list) = append_attr( NULL, (yyvsp[(1) - (1)].attr) ); }
    break;

  case 63:

/* Line 1806 of yacc.c  */
#line 457 "parser.y"
    { (yyval.attr_list) = append_attr( (yyvsp[(1) - (3)].attr_list), (yyvsp[(3) - (3)].attr) ); }
    break;

  case 64:

/* Line 1806 of yacc.c  */
#line 458 "parser.y"
    { (yyval.attr_list) = append_attr( (yyvsp[(1) - (4)].attr_list), (yyvsp[(4) - (4)].attr) ); }
    break;

  case 65:

/* Line 1806 of yacc.c  */
#line 461 "parser.y"
    { (yyval.str_list) = append_str( NULL, (yyvsp[(1) - (1)].str) ); }
    break;

  case 66:

/* Line 1806 of yacc.c  */
#line 462 "parser.y"
    { (yyval.str_list) = append_str( (yyvsp[(1) - (3)].str_list), (yyvsp[(3) - (3)].str) ); }
    break;

  case 67:

/* Line 1806 of yacc.c  */
#line 465 "parser.y"
    { (yyval.attr) = NULL; }
    break;

  case 68:

/* Line 1806 of yacc.c  */
#line 466 "parser.y"
    { (yyval.attr) = make_attr(ATTR_AGGREGATABLE); }
    break;

  case 69:

/* Line 1806 of yacc.c  */
#line 467 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ANNOTATION, (yyvsp[(3) - (4)].str)); }
    break;

  case 70:

/* Line 1806 of yacc.c  */
#line 468 "parser.y"
    { (yyval.attr) = make_attr(ATTR_APPOBJECT); }
    break;

  case 71:

/* Line 1806 of yacc.c  */
#line 469 "parser.y"
    { (yyval.attr) = make_attr(ATTR_ASYNC); }
    break;

  case 72:

/* Line 1806 of yacc.c  */
#line 470 "parser.y"
    { (yyval.attr) = make_attr(ATTR_AUTO_HANDLE); }
    break;

  case 73:

/* Line 1806 of yacc.c  */
#line 471 "parser.y"
    { (yyval.attr) = make_attr(ATTR_BINDABLE); }
    break;

  case 74:

/* Line 1806 of yacc.c  */
#line 472 "parser.y"
    { (yyval.attr) = make_attr(ATTR_BROADCAST); }
    break;

  case 75:

/* Line 1806 of yacc.c  */
#line 473 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_CALLAS, (yyvsp[(3) - (4)].var)); }
    break;

  case 76:

/* Line 1806 of yacc.c  */
#line 474 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_CASE, (yyvsp[(3) - (4)].expr_list)); }
    break;

  case 77:

/* Line 1806 of yacc.c  */
#line 475 "parser.y"
    { (yyval.attr) = make_attr(ATTR_CODE); }
    break;

  case 78:

/* Line 1806 of yacc.c  */
#line 476 "parser.y"
    { (yyval.attr) = make_attr(ATTR_COMMSTATUS); }
    break;

  case 79:

/* Line 1806 of yacc.c  */
#line 477 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); }
    break;

  case 80:

/* Line 1806 of yacc.c  */
#line 478 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_DONT_SERIALIZE */ }
    break;

  case 81:

/* Line 1806 of yacc.c  */
#line 479 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_SERIALIZE */ }
    break;

  case 82:

/* Line 1806 of yacc.c  */
#line 480 "parser.y"
    { (yyval.attr) = make_attr(ATTR_CONTROL); }
    break;

  case 83:

/* Line 1806 of yacc.c  */
#line 481 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DECODE); }
    break;

  case 84:

/* Line 1806 of yacc.c  */
#line 482 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULT); }
    break;

  case 85:

/* Line 1806 of yacc.c  */
#line 483 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULTBIND); }
    break;

  case 86:

/* Line 1806 of yacc.c  */
#line 484 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULTCOLLELEM); }
    break;

  case 87:

/* Line 1806 of yacc.c  */
#line 485 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_DEFAULTVALUE, (yyvsp[(3) - (4)].expr)); }
    break;

  case 88:

/* Line 1806 of yacc.c  */
#line 486 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DEFAULTVTABLE); }
    break;

  case 89:

/* Line 1806 of yacc.c  */
#line 487 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DISABLECONSISTENCYCHECK); }
    break;

  case 90:

/* Line 1806 of yacc.c  */
#line 488 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DISPLAYBIND); }
    break;

  case 91:

/* Line 1806 of yacc.c  */
#line 489 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_DLLNAME, (yyvsp[(3) - (4)].str)); }
    break;

  case 92:

/* Line 1806 of yacc.c  */
#line 490 "parser.y"
    { (yyval.attr) = make_attr(ATTR_DUAL); }
    break;

  case 93:

/* Line 1806 of yacc.c  */
#line 491 "parser.y"
    { (yyval.attr) = make_attr(ATTR_ENABLEALLOCATE); }
    break;

  case 94:

/* Line 1806 of yacc.c  */
#line 492 "parser.y"
    { (yyval.attr) = make_attr(ATTR_ENCODE); }
    break;

  case 95:

/* Line 1806 of yacc.c  */
#line 493 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ENDPOINT, (yyvsp[(3) - (4)].str_list)); }
    break;

  case 96:

/* Line 1806 of yacc.c  */
#line 494 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ENTRY, (yyvsp[(3) - (4)].expr)); }
    break;

  case 97:

/* Line 1806 of yacc.c  */
#line 495 "parser.y"
    { (yyval.attr) = make_attr(ATTR_EXPLICIT_HANDLE); }
    break;

  case 98:

/* Line 1806 of yacc.c  */
#line 496 "parser.y"
    { (yyval.attr) = make_attr(ATTR_FAULTSTATUS); }
    break;

  case 99:

/* Line 1806 of yacc.c  */
#line 497 "parser.y"
    { (yyval.attr) = make_attr(ATTR_FORCEALLOCATE); }
    break;

  case 100:

/* Line 1806 of yacc.c  */
#line 498 "parser.y"
    { (yyval.attr) = make_attr(ATTR_HANDLE); }
    break;

  case 101:

/* Line 1806 of yacc.c  */
#line 499 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPCONTEXT, (yyvsp[(3) - (4)].expr)); }
    break;

  case 102:

/* Line 1806 of yacc.c  */
#line 500 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPFILE, (yyvsp[(3) - (4)].str)); }
    break;

  case 103:

/* Line 1806 of yacc.c  */
#line 501 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRING, (yyvsp[(3) - (4)].str)); }
    break;

  case 104:

/* Line 1806 of yacc.c  */
#line 502 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRINGCONTEXT, (yyvsp[(3) - (4)].expr)); }
    break;

  case 105:

/* Line 1806 of yacc.c  */
#line 503 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_HELPSTRINGDLL, (yyvsp[(3) - (4)].str)); }
    break;

  case 106:

/* Line 1806 of yacc.c  */
#line 504 "parser.y"
    { (yyval.attr) = make_attr(ATTR_HIDDEN); }
    break;

  case 107:

/* Line 1806 of yacc.c  */
#line 505 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ID, (yyvsp[(3) - (4)].expr)); }
    break;

  case 108:

/* Line 1806 of yacc.c  */
#line 506 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IDEMPOTENT); }
    break;

  case 109:

/* Line 1806 of yacc.c  */
#line 507 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IGNORE); }
    break;

  case 110:

/* Line 1806 of yacc.c  */
#line 508 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_IIDIS, (yyvsp[(3) - (4)].expr)); }
    break;

  case 111:

/* Line 1806 of yacc.c  */
#line 509 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IMMEDIATEBIND); }
    break;

  case 112:

/* Line 1806 of yacc.c  */
#line 510 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_IMPLICIT_HANDLE, (yyvsp[(3) - (4)].var)); }
    break;

  case 113:

/* Line 1806 of yacc.c  */
#line 511 "parser.y"
    { (yyval.attr) = make_attr(ATTR_IN); }
    break;

  case 114:

/* Line 1806 of yacc.c  */
#line 512 "parser.y"
    { (yyval.attr) = make_attr(ATTR_INPUTSYNC); }
    break;

  case 115:

/* Line 1806 of yacc.c  */
#line 513 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_LENGTHIS, (yyvsp[(3) - (4)].expr_list)); }
    break;

  case 116:

/* Line 1806 of yacc.c  */
#line 514 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_LIBLCID, (yyvsp[(3) - (4)].expr)); }
    break;

  case 117:

/* Line 1806 of yacc.c  */
#line 515 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PARAMLCID); }
    break;

  case 118:

/* Line 1806 of yacc.c  */
#line 516 "parser.y"
    { (yyval.attr) = make_attr(ATTR_LICENSED); }
    break;

  case 119:

/* Line 1806 of yacc.c  */
#line 517 "parser.y"
    { (yyval.attr) = make_attr(ATTR_LOCAL); }
    break;

  case 120:

/* Line 1806 of yacc.c  */
#line 518 "parser.y"
    { (yyval.attr) = make_attr(ATTR_MAYBE); }
    break;

  case 121:

/* Line 1806 of yacc.c  */
#line 519 "parser.y"
    { (yyval.attr) = make_attr(ATTR_MESSAGE); }
    break;

  case 122:

/* Line 1806 of yacc.c  */
#line 520 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NOCODE); }
    break;

  case 123:

/* Line 1806 of yacc.c  */
#line 521 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONBROWSABLE); }
    break;

  case 124:

/* Line 1806 of yacc.c  */
#line 522 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONCREATABLE); }
    break;

  case 125:

/* Line 1806 of yacc.c  */
#line 523 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NONEXTENSIBLE); }
    break;

  case 126:

/* Line 1806 of yacc.c  */
#line 524 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NOTIFY); }
    break;

  case 127:

/* Line 1806 of yacc.c  */
#line 525 "parser.y"
    { (yyval.attr) = make_attr(ATTR_NOTIFYFLAG); }
    break;

  case 128:

/* Line 1806 of yacc.c  */
#line 526 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OBJECT); }
    break;

  case 129:

/* Line 1806 of yacc.c  */
#line 527 "parser.y"
    { (yyval.attr) = make_attr(ATTR_ODL); }
    break;

  case 130:

/* Line 1806 of yacc.c  */
#line 528 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OLEAUTOMATION); }
    break;

  case 131:

/* Line 1806 of yacc.c  */
#line 529 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_OPTIMIZE, (yyvsp[(3) - (4)].str)); }
    break;

  case 132:

/* Line 1806 of yacc.c  */
#line 530 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OPTIONAL); }
    break;

  case 133:

/* Line 1806 of yacc.c  */
#line 531 "parser.y"
    { (yyval.attr) = make_attr(ATTR_OUT); }
    break;

  case 134:

/* Line 1806 of yacc.c  */
#line 532 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PARTIALIGNORE); }
    break;

  case 135:

/* Line 1806 of yacc.c  */
#line 533 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_POINTERDEFAULT, (yyvsp[(3) - (4)].num)); }
    break;

  case 136:

/* Line 1806 of yacc.c  */
#line 534 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_PROGID, (yyvsp[(3) - (4)].str)); }
    break;

  case 137:

/* Line 1806 of yacc.c  */
#line 535 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPGET); }
    break;

  case 138:

/* Line 1806 of yacc.c  */
#line 536 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPPUT); }
    break;

  case 139:

/* Line 1806 of yacc.c  */
#line 537 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROPPUTREF); }
    break;

  case 140:

/* Line 1806 of yacc.c  */
#line 538 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PROXY); }
    break;

  case 141:

/* Line 1806 of yacc.c  */
#line 539 "parser.y"
    { (yyval.attr) = make_attr(ATTR_PUBLIC); }
    break;

  case 142:

/* Line 1806 of yacc.c  */
#line 541 "parser.y"
    { expr_list_t *list = append_expr( NULL, (yyvsp[(3) - (6)].expr) );
						  list = append_expr( list, (yyvsp[(5) - (6)].expr) );
						  (yyval.attr) = make_attrp(ATTR_RANGE, list); }
    break;

  case 143:

/* Line 1806 of yacc.c  */
#line 544 "parser.y"
    { (yyval.attr) = make_attr(ATTR_READONLY); }
    break;

  case 144:

/* Line 1806 of yacc.c  */
#line 545 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_REPRESENTAS, (yyvsp[(3) - (4)].type)); }
    break;

  case 145:

/* Line 1806 of yacc.c  */
#line 546 "parser.y"
    { (yyval.attr) = make_attr(ATTR_REQUESTEDIT); }
    break;

  case 146:

/* Line 1806 of yacc.c  */
#line 547 "parser.y"
    { (yyval.attr) = make_attr(ATTR_RESTRICTED); }
    break;

  case 147:

/* Line 1806 of yacc.c  */
#line 548 "parser.y"
    { (yyval.attr) = make_attr(ATTR_RETVAL); }
    break;

  case 148:

/* Line 1806 of yacc.c  */
#line 549 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SIZEIS, (yyvsp[(3) - (4)].expr_list)); }
    break;

  case 149:

/* Line 1806 of yacc.c  */
#line 550 "parser.y"
    { (yyval.attr) = make_attr(ATTR_SOURCE); }
    break;

  case 150:

/* Line 1806 of yacc.c  */
#line 551 "parser.y"
    { (yyval.attr) = make_attr(ATTR_STRICTCONTEXTHANDLE); }
    break;

  case 151:

/* Line 1806 of yacc.c  */
#line 552 "parser.y"
    { (yyval.attr) = make_attr(ATTR_STRING); }
    break;

  case 152:

/* Line 1806 of yacc.c  */
#line 553 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SWITCHIS, (yyvsp[(3) - (4)].expr)); }
    break;

  case 153:

/* Line 1806 of yacc.c  */
#line 554 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_SWITCHTYPE, (yyvsp[(3) - (4)].type)); }
    break;

  case 154:

/* Line 1806 of yacc.c  */
#line 555 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_TRANSMITAS, (yyvsp[(3) - (4)].type)); }
    break;

  case 155:

/* Line 1806 of yacc.c  */
#line 556 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_THREADING, (yyvsp[(3) - (4)].num)); }
    break;

  case 156:

/* Line 1806 of yacc.c  */
#line 557 "parser.y"
    { (yyval.attr) = make_attr(ATTR_UIDEFAULT); }
    break;

  case 157:

/* Line 1806 of yacc.c  */
#line 558 "parser.y"
    { (yyval.attr) = make_attr(ATTR_USESGETLASTERROR); }
    break;

  case 158:

/* Line 1806 of yacc.c  */
#line 559 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_USERMARSHAL, (yyvsp[(3) - (4)].type)); }
    break;

  case 159:

/* Line 1806 of yacc.c  */
#line 560 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_UUID, (yyvsp[(3) - (4)].uuid)); }
    break;

  case 160:

/* Line 1806 of yacc.c  */
#line 561 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_ASYNCUUID, (yyvsp[(3) - (4)].uuid)); }
    break;

  case 161:

/* Line 1806 of yacc.c  */
#line 562 "parser.y"
    { (yyval.attr) = make_attr(ATTR_V1ENUM); }
    break;

  case 162:

/* Line 1806 of yacc.c  */
#line 563 "parser.y"
    { (yyval.attr) = make_attr(ATTR_VARARG); }
    break;

  case 163:

/* Line 1806 of yacc.c  */
#line 564 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_VERSION, (yyvsp[(3) - (4)].num)); }
    break;

  case 164:

/* Line 1806 of yacc.c  */
#line 565 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_VIPROGID, (yyvsp[(3) - (4)].str)); }
    break;

  case 165:

/* Line 1806 of yacc.c  */
#line 566 "parser.y"
    { (yyval.attr) = make_attrp(ATTR_WIREMARSHAL, (yyvsp[(3) - (4)].type)); }
    break;

  case 166:

/* Line 1806 of yacc.c  */
#line 567 "parser.y"
    { (yyval.attr) = make_attrv(ATTR_POINTERTYPE, (yyvsp[(1) - (1)].num)); }
    break;

  case 168:

/* Line 1806 of yacc.c  */
#line 572 "parser.y"
    { if (!is_valid_uuid((yyvsp[(1) - (1)].str)))
						    error_loc("invalid UUID: %s\n", (yyvsp[(1) - (1)].str));
						  (yyval.uuid) = parse_uuid((yyvsp[(1) - (1)].str)); }
    break;

  case 169:

/* Line 1806 of yacc.c  */
#line 577 "parser.y"
    { (yyval.str) = xstrdup("__cdecl"); }
    break;

  case 170:

/* Line 1806 of yacc.c  */
#line 578 "parser.y"
    { (yyval.str) = xstrdup("__fastcall"); }
    break;

  case 171:

/* Line 1806 of yacc.c  */
#line 579 "parser.y"
    { (yyval.str) = xstrdup("__pascal"); }
    break;

  case 172:

/* Line 1806 of yacc.c  */
#line 580 "parser.y"
    { (yyval.str) = xstrdup("__stdcall"); }
    break;

  case 173:

/* Line 1806 of yacc.c  */
#line 583 "parser.y"
    { (yyval.var_list) = NULL; }
    break;

  case 174:

/* Line 1806 of yacc.c  */
#line 584 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[(1) - (2)].var_list), (yyvsp[(2) - (2)].var) ); }
    break;

  case 175:

/* Line 1806 of yacc.c  */
#line 587 "parser.y"
    { attr_t *a = make_attrp(ATTR_CASE, append_expr( NULL, (yyvsp[(2) - (4)].expr) ));
						  (yyval.var) = (yyvsp[(4) - (4)].var); if (!(yyval.var)) (yyval.var) = make_var(NULL);
						  (yyval.var)->attrs = append_attr( (yyval.var)->attrs, a );
						}
    break;

  case 176:

/* Line 1806 of yacc.c  */
#line 591 "parser.y"
    { attr_t *a = make_attr(ATTR_DEFAULT);
						  (yyval.var) = (yyvsp[(3) - (3)].var); if (!(yyval.var)) (yyval.var) = make_var(NULL);
						  (yyval.var)->attrs = append_attr( (yyval.var)->attrs, a );
						}
    break;

  case 177:

/* Line 1806 of yacc.c  */
#line 597 "parser.y"
    { (yyval.var_list) = NULL; }
    break;

  case 178:

/* Line 1806 of yacc.c  */
#line 598 "parser.y"
    { (yyval.var_list) = (yyvsp[(1) - (2)].var_list); }
    break;

  case 180:

/* Line 1806 of yacc.c  */
#line 602 "parser.y"
    { if (!(yyvsp[(1) - (1)].var)->eval)
						    (yyvsp[(1) - (1)].var)->eval = make_exprl(EXPR_NUM, 0 /* default for first enum entry */);
                                                  (yyval.var_list) = append_var( NULL, (yyvsp[(1) - (1)].var) );
						}
    break;

  case 181:

/* Line 1806 of yacc.c  */
#line 606 "parser.y"
    { if (!(yyvsp[(3) - (3)].var)->eval)
                                                  {
                                                    var_t *last = LIST_ENTRY( list_tail((yyval.var_list)), var_t, entry );
                                                    enum expr_type type = EXPR_NUM;
                                                    if (last->eval->type == EXPR_HEXNUM) type = EXPR_HEXNUM;
                                                    if (last->eval->cval + 1 < 0) type = EXPR_HEXNUM;
                                                    (yyvsp[(3) - (3)].var)->eval = make_exprl(type, last->eval->cval + 1);
                                                  }
                                                  (yyval.var_list) = append_var( (yyvsp[(1) - (3)].var_list), (yyvsp[(3) - (3)].var) );
						}
    break;

  case 182:

/* Line 1806 of yacc.c  */
#line 618 "parser.y"
    { (yyval.var) = reg_const((yyvsp[(1) - (3)].var));
						  (yyval.var)->eval = (yyvsp[(3) - (3)].expr);
                                                  (yyval.var)->type = type_new_int(TYPE_BASIC_INT, 0);
						}
    break;

  case 183:

/* Line 1806 of yacc.c  */
#line 622 "parser.y"
    { (yyval.var) = reg_const((yyvsp[(1) - (1)].var));
                                                  (yyval.var)->type = type_new_int(TYPE_BASIC_INT, 0);
						}
    break;

  case 184:

/* Line 1806 of yacc.c  */
#line 627 "parser.y"
    { (yyval.type) = type_new_enum((yyvsp[(2) - (5)].str), TRUE, (yyvsp[(4) - (5)].var_list)); }
    break;

  case 185:

/* Line 1806 of yacc.c  */
#line 630 "parser.y"
    { (yyval.expr_list) = append_expr( NULL, (yyvsp[(1) - (1)].expr) ); }
    break;

  case 186:

/* Line 1806 of yacc.c  */
#line 631 "parser.y"
    { (yyval.expr_list) = append_expr( (yyvsp[(1) - (3)].expr_list), (yyvsp[(3) - (3)].expr) ); }
    break;

  case 187:

/* Line 1806 of yacc.c  */
#line 634 "parser.y"
    { (yyval.expr) = make_expr(EXPR_VOID); }
    break;

  case 189:

/* Line 1806 of yacc.c  */
#line 638 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_NUM, (yyvsp[(1) - (1)].num)); }
    break;

  case 190:

/* Line 1806 of yacc.c  */
#line 639 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_HEXNUM, (yyvsp[(1) - (1)].num)); }
    break;

  case 191:

/* Line 1806 of yacc.c  */
#line 640 "parser.y"
    { (yyval.expr) = make_exprd(EXPR_DOUBLE, (yyvsp[(1) - (1)].dbl)); }
    break;

  case 192:

/* Line 1806 of yacc.c  */
#line 641 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_TRUEFALSE, 0); }
    break;

  case 193:

/* Line 1806 of yacc.c  */
#line 642 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_NUM, 0); }
    break;

  case 194:

/* Line 1806 of yacc.c  */
#line 643 "parser.y"
    { (yyval.expr) = make_exprl(EXPR_TRUEFALSE, 1); }
    break;

  case 195:

/* Line 1806 of yacc.c  */
#line 644 "parser.y"
    { (yyval.expr) = make_exprs(EXPR_STRLIT, (yyvsp[(1) - (1)].str)); }
    break;

  case 196:

/* Line 1806 of yacc.c  */
#line 645 "parser.y"
    { (yyval.expr) = make_exprs(EXPR_WSTRLIT, (yyvsp[(1) - (1)].str)); }
    break;

  case 197:

/* Line 1806 of yacc.c  */
#line 646 "parser.y"
    { (yyval.expr) = make_exprs(EXPR_CHARCONST, (yyvsp[(1) - (1)].str)); }
    break;

  case 198:

/* Line 1806 of yacc.c  */
#line 647 "parser.y"
    { (yyval.expr) = make_exprs(EXPR_IDENTIFIER, (yyvsp[(1) - (1)].str)); }
    break;

  case 199:

/* Line 1806 of yacc.c  */
#line 648 "parser.y"
    { (yyval.expr) = make_expr3(EXPR_COND, (yyvsp[(1) - (5)].expr), (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].expr)); }
    break;

  case 200:

/* Line 1806 of yacc.c  */
#line 649 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_LOGOR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 201:

/* Line 1806 of yacc.c  */
#line 650 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_LOGAND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 202:

/* Line 1806 of yacc.c  */
#line 651 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_OR , (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 203:

/* Line 1806 of yacc.c  */
#line 652 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_XOR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 204:

/* Line 1806 of yacc.c  */
#line 653 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_AND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 205:

/* Line 1806 of yacc.c  */
#line 654 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_EQUALITY, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 206:

/* Line 1806 of yacc.c  */
#line 655 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_INEQUALITY, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 207:

/* Line 1806 of yacc.c  */
#line 656 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_GTR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 208:

/* Line 1806 of yacc.c  */
#line 657 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_LESS, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 209:

/* Line 1806 of yacc.c  */
#line 658 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_GTREQL, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 210:

/* Line 1806 of yacc.c  */
#line 659 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_LESSEQL, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 211:

/* Line 1806 of yacc.c  */
#line 660 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SHL, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 212:

/* Line 1806 of yacc.c  */
#line 661 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SHR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 213:

/* Line 1806 of yacc.c  */
#line 662 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_ADD, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 214:

/* Line 1806 of yacc.c  */
#line 663 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_SUB, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 215:

/* Line 1806 of yacc.c  */
#line 664 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MOD, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 216:

/* Line 1806 of yacc.c  */
#line 665 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MUL, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 217:

/* Line 1806 of yacc.c  */
#line 666 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_DIV, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 218:

/* Line 1806 of yacc.c  */
#line 667 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_LOGNOT, (yyvsp[(2) - (2)].expr)); }
    break;

  case 219:

/* Line 1806 of yacc.c  */
#line 668 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_NOT, (yyvsp[(2) - (2)].expr)); }
    break;

  case 220:

/* Line 1806 of yacc.c  */
#line 669 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_POS, (yyvsp[(2) - (2)].expr)); }
    break;

  case 221:

/* Line 1806 of yacc.c  */
#line 670 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_NEG, (yyvsp[(2) - (2)].expr)); }
    break;

  case 222:

/* Line 1806 of yacc.c  */
#line 671 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_ADDRESSOF, (yyvsp[(2) - (2)].expr)); }
    break;

  case 223:

/* Line 1806 of yacc.c  */
#line 672 "parser.y"
    { (yyval.expr) = make_expr1(EXPR_PPTR, (yyvsp[(2) - (2)].expr)); }
    break;

  case 224:

/* Line 1806 of yacc.c  */
#line 673 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MEMBER, make_expr1(EXPR_PPTR, (yyvsp[(1) - (3)].expr)), make_exprs(EXPR_IDENTIFIER, (yyvsp[(3) - (3)].str))); }
    break;

  case 225:

/* Line 1806 of yacc.c  */
#line 674 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_MEMBER, (yyvsp[(1) - (3)].expr), make_exprs(EXPR_IDENTIFIER, (yyvsp[(3) - (3)].str))); }
    break;

  case 226:

/* Line 1806 of yacc.c  */
#line 676 "parser.y"
    { (yyval.expr) = make_exprt(EXPR_CAST, declare_var(NULL, (yyvsp[(2) - (5)].declspec), (yyvsp[(3) - (5)].declarator), 0), (yyvsp[(5) - (5)].expr)); free((yyvsp[(2) - (5)].declspec)); free((yyvsp[(3) - (5)].declarator)); }
    break;

  case 227:

/* Line 1806 of yacc.c  */
#line 678 "parser.y"
    { (yyval.expr) = make_exprt(EXPR_SIZEOF, declare_var(NULL, (yyvsp[(3) - (5)].declspec), (yyvsp[(4) - (5)].declarator), 0), NULL); free((yyvsp[(3) - (5)].declspec)); free((yyvsp[(4) - (5)].declarator)); }
    break;

  case 228:

/* Line 1806 of yacc.c  */
#line 679 "parser.y"
    { (yyval.expr) = make_expr2(EXPR_ARRAY, (yyvsp[(1) - (4)].expr), (yyvsp[(3) - (4)].expr)); }
    break;

  case 229:

/* Line 1806 of yacc.c  */
#line 680 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (3)].expr); }
    break;

  case 230:

/* Line 1806 of yacc.c  */
#line 683 "parser.y"
    { (yyval.expr_list) = append_expr( NULL, (yyvsp[(1) - (1)].expr) ); }
    break;

  case 231:

/* Line 1806 of yacc.c  */
#line 684 "parser.y"
    { (yyval.expr_list) = append_expr( (yyvsp[(1) - (3)].expr_list), (yyvsp[(3) - (3)].expr) ); }
    break;

  case 232:

/* Line 1806 of yacc.c  */
#line 687 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr);
						  if (!(yyval.expr)->is_const)
						      error_loc("expression is not an integer constant\n");
						}
    break;

  case 233:

/* Line 1806 of yacc.c  */
#line 693 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr);
						  if (!(yyval.expr)->is_const && (yyval.expr)->type != EXPR_STRLIT && (yyval.expr)->type != EXPR_WSTRLIT)
						      error_loc("expression is not constant\n");
						}
    break;

  case 234:

/* Line 1806 of yacc.c  */
#line 699 "parser.y"
    { (yyval.var_list) = NULL; }
    break;

  case 235:

/* Line 1806 of yacc.c  */
#line 700 "parser.y"
    { (yyval.var_list) = append_var_list((yyvsp[(1) - (2)].var_list), (yyvsp[(2) - (2)].var_list)); }
    break;

  case 236:

/* Line 1806 of yacc.c  */
#line 704 "parser.y"
    { const char *first = LIST_ENTRY(list_head((yyvsp[(3) - (4)].declarator_list)), declarator_t, entry)->var->name;
						  check_field_attrs(first, (yyvsp[(1) - (4)].attr_list));
						  (yyval.var_list) = set_var_types((yyvsp[(1) - (4)].attr_list), (yyvsp[(2) - (4)].declspec), (yyvsp[(3) - (4)].declarator_list));
						}
    break;

  case 237:

/* Line 1806 of yacc.c  */
#line 708 "parser.y"
    { var_t *v = make_var(NULL);
						  v->type = (yyvsp[(2) - (3)].type); v->attrs = (yyvsp[(1) - (3)].attr_list);
						  (yyval.var_list) = append_var(NULL, v);
						}
    break;

  case 238:

/* Line 1806 of yacc.c  */
#line 715 "parser.y"
    { (yyval.var) = (yyvsp[(1) - (2)].var); }
    break;

  case 239:

/* Line 1806 of yacc.c  */
#line 716 "parser.y"
    { (yyval.var) = make_var(NULL); (yyval.var)->attrs = (yyvsp[(1) - (2)].attr_list); }
    break;

  case 240:

/* Line 1806 of yacc.c  */
#line 719 "parser.y"
    { (yyval.var_list) = NULL; }
    break;

  case 241:

/* Line 1806 of yacc.c  */
#line 720 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[(1) - (2)].var_list), (yyvsp[(2) - (2)].var) ); }
    break;

  case 242:

/* Line 1806 of yacc.c  */
#line 724 "parser.y"
    { (yyval.var) = (yyvsp[(1) - (2)].var); }
    break;

  case 243:

/* Line 1806 of yacc.c  */
#line 725 "parser.y"
    { (yyval.var) = NULL; }
    break;

  case 244:

/* Line 1806 of yacc.c  */
#line 728 "parser.y"
    { (yyval.var) = declare_var(check_field_attrs((yyvsp[(3) - (3)].declarator)->var->name, (yyvsp[(1) - (3)].attr_list)),
						                (yyvsp[(2) - (3)].declspec), (yyvsp[(3) - (3)].declarator), FALSE);
						  free((yyvsp[(3) - (3)].declarator));
						}
    break;

  case 245:

/* Line 1806 of yacc.c  */
#line 732 "parser.y"
    { var_t *v = make_var(NULL);
						  v->type = (yyvsp[(2) - (2)].type); v->attrs = (yyvsp[(1) - (2)].attr_list);
						  (yyval.var) = v;
						}
    break;

  case 246:

/* Line 1806 of yacc.c  */
#line 738 "parser.y"
    { (yyval.var) = (yyvsp[(1) - (1)].var);
						  if (type_get_type((yyval.var)->type) != TYPE_FUNCTION)
						    error_loc("only methods may be declared inside the methods section of a dispinterface\n");
						  check_function_attrs((yyval.var)->name, (yyval.var)->attrs);
						}
    break;

  case 247:

/* Line 1806 of yacc.c  */
#line 747 "parser.y"
    { (yyval.var) = declare_var((yyvsp[(1) - (3)].attr_list), (yyvsp[(2) - (3)].declspec), (yyvsp[(3) - (3)].declarator), FALSE);
						  free((yyvsp[(3) - (3)].declarator));
						}
    break;

  case 248:

/* Line 1806 of yacc.c  */
#line 750 "parser.y"
    { (yyval.var) = declare_var(NULL, (yyvsp[(1) - (2)].declspec), (yyvsp[(2) - (2)].declarator), FALSE);
						  free((yyvsp[(2) - (2)].declarator));
						}
    break;

  case 249:

/* Line 1806 of yacc.c  */
#line 755 "parser.y"
    { (yyval.var) = NULL; }
    break;

  case 251:

/* Line 1806 of yacc.c  */
#line 759 "parser.y"
    { (yyval.str) = NULL; }
    break;

  case 252:

/* Line 1806 of yacc.c  */
#line 760 "parser.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 253:

/* Line 1806 of yacc.c  */
#line 761 "parser.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 254:

/* Line 1806 of yacc.c  */
#line 764 "parser.y"
    { (yyval.var) = make_var((yyvsp[(1) - (1)].str)); }
    break;

  case 255:

/* Line 1806 of yacc.c  */
#line 766 "parser.y"
    { (yyval.var) = make_var((yyvsp[(1) - (1)].str)); }
    break;

  case 256:

/* Line 1806 of yacc.c  */
#line 769 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); }
    break;

  case 257:

/* Line 1806 of yacc.c  */
#line 770 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); }
    break;

  case 259:

/* Line 1806 of yacc.c  */
#line 772 "parser.y"
    { (yyval.type) = type_new_int(type_basic_get_type((yyvsp[(2) - (2)].type)), -1); }
    break;

  case 260:

/* Line 1806 of yacc.c  */
#line 773 "parser.y"
    { (yyval.type) = type_new_int(type_basic_get_type((yyvsp[(2) - (2)].type)), 1); }
    break;

  case 261:

/* Line 1806 of yacc.c  */
#line 774 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT, 1); }
    break;

  case 262:

/* Line 1806 of yacc.c  */
#line 775 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); }
    break;

  case 263:

/* Line 1806 of yacc.c  */
#line 776 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); }
    break;

  case 264:

/* Line 1806 of yacc.c  */
#line 777 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); }
    break;

  case 265:

/* Line 1806 of yacc.c  */
#line 778 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); }
    break;

  case 266:

/* Line 1806 of yacc.c  */
#line 779 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); }
    break;

  case 269:

/* Line 1806 of yacc.c  */
#line 786 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT, 0); }
    break;

  case 270:

/* Line 1806 of yacc.c  */
#line 787 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT16, 0); }
    break;

  case 271:

/* Line 1806 of yacc.c  */
#line 788 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT8, 0); }
    break;

  case 272:

/* Line 1806 of yacc.c  */
#line 789 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT32, 0); }
    break;

  case 273:

/* Line 1806 of yacc.c  */
#line 790 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_HYPER, 0); }
    break;

  case 274:

/* Line 1806 of yacc.c  */
#line 791 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT64, 0); }
    break;

  case 275:

/* Line 1806 of yacc.c  */
#line 792 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_CHAR, 0); }
    break;

  case 276:

/* Line 1806 of yacc.c  */
#line 793 "parser.y"
    { (yyval.type) = type_new_int(TYPE_BASIC_INT3264, 0); }
    break;

  case 277:

/* Line 1806 of yacc.c  */
#line 796 "parser.y"
    { (yyval.type) = type_new_coclass((yyvsp[(2) - (2)].str)); }
    break;

  case 278:

/* Line 1806 of yacc.c  */
#line 797 "parser.y"
    { (yyval.type) = find_type((yyvsp[(2) - (2)].str), 0);
						  if (type_get_type_detect_alias((yyval.type)) != TYPE_COCLASS)
						    error_loc("%s was not declared a coclass at %s:%d\n",
							      (yyvsp[(2) - (2)].str), (yyval.type)->loc_info.input_name,
							      (yyval.type)->loc_info.line_number);
						}
    break;

  case 279:

/* Line 1806 of yacc.c  */
#line 805 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type);
						  check_def((yyval.type));
						  (yyval.type)->attrs = check_coclass_attrs((yyvsp[(2) - (2)].type)->name, (yyvsp[(1) - (2)].attr_list));
						}
    break;

  case 280:

/* Line 1806 of yacc.c  */
#line 812 "parser.y"
    { (yyval.type) = type_coclass_define((yyvsp[(1) - (5)].type), (yyvsp[(3) - (5)].ifref_list)); }
    break;

  case 281:

/* Line 1806 of yacc.c  */
#line 815 "parser.y"
    { (yyval.type) = NULL; }
    break;

  case 282:

/* Line 1806 of yacc.c  */
#line 818 "parser.y"
    { (yyval.ifref_list) = NULL; }
    break;

  case 283:

/* Line 1806 of yacc.c  */
#line 819 "parser.y"
    { (yyval.ifref_list) = append_ifref( (yyvsp[(1) - (2)].ifref_list), (yyvsp[(2) - (2)].ifref) ); }
    break;

  case 284:

/* Line 1806 of yacc.c  */
#line 823 "parser.y"
    { (yyval.ifref) = make_ifref((yyvsp[(2) - (2)].type)); (yyval.ifref)->attrs = (yyvsp[(1) - (2)].attr_list); }
    break;

  case 285:

/* Line 1806 of yacc.c  */
#line 826 "parser.y"
    { (yyval.type) = get_type(TYPE_INTERFACE, (yyvsp[(2) - (2)].str), 0); }
    break;

  case 286:

/* Line 1806 of yacc.c  */
#line 827 "parser.y"
    { (yyval.type) = get_type(TYPE_INTERFACE, (yyvsp[(2) - (2)].str), 0); }
    break;

  case 287:

/* Line 1806 of yacc.c  */
#line 830 "parser.y"
    { attr_t *attrs;
						  (yyval.type) = (yyvsp[(2) - (2)].type);
						  check_def((yyval.type));
						  attrs = make_attr(ATTR_DISPINTERFACE);
						  (yyval.type)->attrs = append_attr( check_dispiface_attrs((yyvsp[(2) - (2)].type)->name, (yyvsp[(1) - (2)].attr_list)), attrs );
						  (yyval.type)->defined = TRUE;
						}
    break;

  case 288:

/* Line 1806 of yacc.c  */
#line 839 "parser.y"
    { (yyval.var_list) = NULL; }
    break;

  case 289:

/* Line 1806 of yacc.c  */
#line 840 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[(1) - (3)].var_list), (yyvsp[(2) - (3)].var) ); }
    break;

  case 290:

/* Line 1806 of yacc.c  */
#line 843 "parser.y"
    { (yyval.var_list) = NULL; }
    break;

  case 291:

/* Line 1806 of yacc.c  */
#line 844 "parser.y"
    { (yyval.var_list) = append_var( (yyvsp[(1) - (3)].var_list), (yyvsp[(2) - (3)].var) ); }
    break;

  case 292:

/* Line 1806 of yacc.c  */
#line 850 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (5)].type);
						  type_dispinterface_define((yyval.type), (yyvsp[(3) - (5)].var_list), (yyvsp[(4) - (5)].var_list));
						}
    break;

  case 293:

/* Line 1806 of yacc.c  */
#line 854 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (5)].type);
						  type_dispinterface_define_from_iface((yyval.type), (yyvsp[(3) - (5)].type));
						}
    break;

  case 294:

/* Line 1806 of yacc.c  */
#line 859 "parser.y"
    { (yyval.type) = NULL; }
    break;

  case 295:

/* Line 1806 of yacc.c  */
#line 860 "parser.y"
    { (yyval.type) = find_type_or_error2((yyvsp[(2) - (2)].str), 0); }
    break;

  case 296:

/* Line 1806 of yacc.c  */
#line 863 "parser.y"
    { (yyval.type) = get_type(TYPE_INTERFACE, (yyvsp[(2) - (2)].str), 0); }
    break;

  case 297:

/* Line 1806 of yacc.c  */
#line 864 "parser.y"
    { (yyval.type) = get_type(TYPE_INTERFACE, (yyvsp[(2) - (2)].str), 0); }
    break;

  case 298:

/* Line 1806 of yacc.c  */
#line 867 "parser.y"
    { (yyval.ifinfo).interface = (yyvsp[(2) - (2)].type);
						  (yyval.ifinfo).old_pointer_default = pointer_default;
						  if (is_attr((yyvsp[(1) - (2)].attr_list), ATTR_POINTERDEFAULT))
						    pointer_default = get_attrv((yyvsp[(1) - (2)].attr_list), ATTR_POINTERDEFAULT);
						  check_def((yyvsp[(2) - (2)].type));
						  (yyvsp[(2) - (2)].type)->attrs = check_iface_attrs((yyvsp[(2) - (2)].type)->name, (yyvsp[(1) - (2)].attr_list));
						  (yyvsp[(2) - (2)].type)->defined = TRUE;
						}
    break;

  case 299:

/* Line 1806 of yacc.c  */
#line 878 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (6)].ifinfo).interface;
						  if((yyval.type) == (yyvsp[(2) - (6)].type))
						    error_loc("Interface can't inherit from itself\n");
						  type_interface_define((yyval.type), (yyvsp[(2) - (6)].type), (yyvsp[(4) - (6)].stmt_list));
						  pointer_default = (yyvsp[(1) - (6)].ifinfo).old_pointer_default;
						}
    break;

  case 300:

/* Line 1806 of yacc.c  */
#line 888 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (8)].ifinfo).interface;
						  type_interface_define((yyval.type), find_type_or_error2((yyvsp[(3) - (8)].str), 0), (yyvsp[(6) - (8)].stmt_list));
						  pointer_default = (yyvsp[(1) - (8)].ifinfo).old_pointer_default;
						}
    break;

  case 301:

/* Line 1806 of yacc.c  */
#line 892 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (2)].type); }
    break;

  case 302:

/* Line 1806 of yacc.c  */
#line 896 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (2)].type); }
    break;

  case 303:

/* Line 1806 of yacc.c  */
#line 897 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (2)].type); }
    break;

  case 304:

/* Line 1806 of yacc.c  */
#line 900 "parser.y"
    { (yyval.type) = type_new_module((yyvsp[(2) - (2)].str)); }
    break;

  case 305:

/* Line 1806 of yacc.c  */
#line 901 "parser.y"
    { (yyval.type) = type_new_module((yyvsp[(2) - (2)].str)); }
    break;

  case 306:

/* Line 1806 of yacc.c  */
#line 904 "parser.y"
    { (yyval.type) = (yyvsp[(2) - (2)].type);
						  (yyval.type)->attrs = check_module_attrs((yyvsp[(2) - (2)].type)->name, (yyvsp[(1) - (2)].attr_list));
						}
    break;

  case 307:

/* Line 1806 of yacc.c  */
#line 910 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (5)].type);
                                                  type_module_define((yyval.type), (yyvsp[(3) - (5)].stmt_list));
						}
    break;

  case 308:

/* Line 1806 of yacc.c  */
#line 916 "parser.y"
    { (yyval.stgclass) = STG_EXTERN; }
    break;

  case 309:

/* Line 1806 of yacc.c  */
#line 917 "parser.y"
    { (yyval.stgclass) = STG_STATIC; }
    break;

  case 310:

/* Line 1806 of yacc.c  */
#line 918 "parser.y"
    { (yyval.stgclass) = STG_REGISTER; }
    break;

  case 311:

/* Line 1806 of yacc.c  */
#line 922 "parser.y"
    { (yyval.attr) = make_attr(ATTR_INLINE); }
    break;

  case 312:

/* Line 1806 of yacc.c  */
#line 926 "parser.y"
    { (yyval.attr) = make_attr(ATTR_CONST); }
    break;

  case 313:

/* Line 1806 of yacc.c  */
#line 929 "parser.y"
    { (yyval.attr_list) = NULL; }
    break;

  case 314:

/* Line 1806 of yacc.c  */
#line 930 "parser.y"
    { (yyval.attr_list) = append_attr((yyvsp[(1) - (2)].attr_list), (yyvsp[(2) - (2)].attr)); }
    break;

  case 315:

/* Line 1806 of yacc.c  */
#line 933 "parser.y"
    { (yyval.declspec) = make_decl_spec((yyvsp[(1) - (2)].type), (yyvsp[(2) - (2)].declspec), NULL, NULL, STG_NONE); }
    break;

  case 316:

/* Line 1806 of yacc.c  */
#line 935 "parser.y"
    { (yyval.declspec) = make_decl_spec((yyvsp[(2) - (3)].type), (yyvsp[(1) - (3)].declspec), (yyvsp[(3) - (3)].declspec), NULL, STG_NONE); }
    break;

  case 317:

/* Line 1806 of yacc.c  */
#line 938 "parser.y"
    { (yyval.declspec) = NULL; }
    break;

  case 319:

/* Line 1806 of yacc.c  */
#line 943 "parser.y"
    { (yyval.declspec) = make_decl_spec(NULL, (yyvsp[(2) - (2)].declspec), NULL, (yyvsp[(1) - (2)].attr), STG_NONE); }
    break;

  case 320:

/* Line 1806 of yacc.c  */
#line 944 "parser.y"
    { (yyval.declspec) = make_decl_spec(NULL, (yyvsp[(2) - (2)].declspec), NULL, (yyvsp[(1) - (2)].attr), STG_NONE); }
    break;

  case 321:

/* Line 1806 of yacc.c  */
#line 945 "parser.y"
    { (yyval.declspec) = make_decl_spec(NULL, (yyvsp[(2) - (2)].declspec), NULL, NULL, (yyvsp[(1) - (2)].stgclass)); }
    break;

  case 322:

/* Line 1806 of yacc.c  */
#line 950 "parser.y"
    { (yyval.declarator) = (yyvsp[(3) - (3)].declarator); (yyval.declarator)->type = append_ptrchain_type((yyval.declarator)->type, type_new_pointer(pointer_default, NULL, (yyvsp[(2) - (3)].attr_list))); }
    break;

  case 323:

/* Line 1806 of yacc.c  */
#line 951 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (2)].declarator); if ((yyval.declarator)->func_type) (yyval.declarator)->func_type->attrs = append_attr((yyval.declarator)->func_type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[(1) - (2)].str)));
						           else if ((yyval.declarator)->type) (yyval.declarator)->type->attrs = append_attr((yyval.declarator)->type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[(1) - (2)].str))); }
    break;

  case 325:

/* Line 1806 of yacc.c  */
#line 957 "parser.y"
    { (yyval.declarator) = make_declarator((yyvsp[(1) - (1)].var)); }
    break;

  case 326:

/* Line 1806 of yacc.c  */
#line 958 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (3)].declarator); }
    break;

  case 327:

/* Line 1806 of yacc.c  */
#line 959 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (2)].declarator); (yyval.declarator)->array = append_array((yyval.declarator)->array, (yyvsp[(2) - (2)].expr)); }
    break;

  case 328:

/* Line 1806 of yacc.c  */
#line 960 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (4)].declarator);
						  (yyval.declarator)->func_type = append_ptrchain_type((yyval.declarator)->type, type_new_function((yyvsp[(3) - (4)].var_list)));
						  (yyval.declarator)->type = NULL;
						}
    break;

  case 329:

/* Line 1806 of yacc.c  */
#line 969 "parser.y"
    { (yyval.declarator) = (yyvsp[(3) - (3)].declarator); (yyval.declarator)->type = append_ptrchain_type((yyval.declarator)->type, type_new_pointer(pointer_default, NULL, (yyvsp[(2) - (3)].attr_list))); }
    break;

  case 330:

/* Line 1806 of yacc.c  */
#line 970 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (2)].declarator); if ((yyval.declarator)->func_type) (yyval.declarator)->func_type->attrs = append_attr((yyval.declarator)->func_type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[(1) - (2)].str)));
						           else if ((yyval.declarator)->type) (yyval.declarator)->type->attrs = append_attr((yyval.declarator)->type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[(1) - (2)].str))); }
    break;

  case 332:

/* Line 1806 of yacc.c  */
#line 978 "parser.y"
    { (yyval.declarator) = (yyvsp[(3) - (3)].declarator); (yyval.declarator)->type = append_ptrchain_type((yyval.declarator)->type, type_new_pointer(pointer_default, NULL, (yyvsp[(2) - (3)].attr_list))); }
    break;

  case 333:

/* Line 1806 of yacc.c  */
#line 979 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (2)].declarator); if ((yyval.declarator)->func_type) (yyval.declarator)->func_type->attrs = append_attr((yyval.declarator)->func_type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[(1) - (2)].str)));
						           else if ((yyval.declarator)->type) (yyval.declarator)->type->attrs = append_attr((yyval.declarator)->type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[(1) - (2)].str))); }
    break;

  case 334:

/* Line 1806 of yacc.c  */
#line 984 "parser.y"
    { (yyval.declarator) = make_declarator(NULL); }
    break;

  case 336:

/* Line 1806 of yacc.c  */
#line 990 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (3)].declarator); }
    break;

  case 337:

/* Line 1806 of yacc.c  */
#line 991 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (2)].declarator); (yyval.declarator)->array = append_array((yyval.declarator)->array, (yyvsp[(2) - (2)].expr)); }
    break;

  case 338:

/* Line 1806 of yacc.c  */
#line 992 "parser.y"
    { (yyval.declarator) = make_declarator(NULL); (yyval.declarator)->array = append_array((yyval.declarator)->array, (yyvsp[(1) - (1)].expr)); }
    break;

  case 339:

/* Line 1806 of yacc.c  */
#line 994 "parser.y"
    { (yyval.declarator) = make_declarator(NULL);
						  (yyval.declarator)->func_type = append_ptrchain_type((yyval.declarator)->type, type_new_function((yyvsp[(2) - (3)].var_list)));
						  (yyval.declarator)->type = NULL;
						}
    break;

  case 340:

/* Line 1806 of yacc.c  */
#line 999 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (4)].declarator);
						  (yyval.declarator)->func_type = append_ptrchain_type((yyval.declarator)->type, type_new_function((yyvsp[(3) - (4)].var_list)));
						  (yyval.declarator)->type = NULL;
						}
    break;

  case 341:

/* Line 1806 of yacc.c  */
#line 1008 "parser.y"
    { (yyval.declarator) = (yyvsp[(3) - (3)].declarator); (yyval.declarator)->type = append_ptrchain_type((yyval.declarator)->type, type_new_pointer(pointer_default, NULL, (yyvsp[(2) - (3)].attr_list))); }
    break;

  case 342:

/* Line 1806 of yacc.c  */
#line 1009 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (2)].declarator); (yyval.declarator)->type->attrs = append_attr((yyval.declarator)->type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[(1) - (2)].str))); }
    break;

  case 344:

/* Line 1806 of yacc.c  */
#line 1016 "parser.y"
    { (yyval.declarator) = (yyvsp[(3) - (3)].declarator); (yyval.declarator)->type = append_ptrchain_type((yyval.declarator)->type, type_new_pointer(pointer_default, NULL, (yyvsp[(2) - (3)].attr_list))); }
    break;

  case 345:

/* Line 1806 of yacc.c  */
#line 1017 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (2)].declarator); (yyval.declarator)->type->attrs = append_attr((yyval.declarator)->type->attrs, make_attrp(ATTR_CALLCONV, (yyvsp[(1) - (2)].str))); }
    break;

  case 346:

/* Line 1806 of yacc.c  */
#line 1021 "parser.y"
    { (yyval.declarator) = make_declarator(NULL); }
    break;

  case 348:

/* Line 1806 of yacc.c  */
#line 1029 "parser.y"
    { (yyval.declarator) = make_declarator((yyvsp[(1) - (1)].var)); }
    break;

  case 349:

/* Line 1806 of yacc.c  */
#line 1030 "parser.y"
    { (yyval.declarator) = (yyvsp[(2) - (3)].declarator); }
    break;

  case 350:

/* Line 1806 of yacc.c  */
#line 1031 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (2)].declarator); (yyval.declarator)->array = append_array((yyval.declarator)->array, (yyvsp[(2) - (2)].expr)); }
    break;

  case 351:

/* Line 1806 of yacc.c  */
#line 1032 "parser.y"
    { (yyval.declarator) = make_declarator(NULL); (yyval.declarator)->array = append_array((yyval.declarator)->array, (yyvsp[(1) - (1)].expr)); }
    break;

  case 352:

/* Line 1806 of yacc.c  */
#line 1034 "parser.y"
    { (yyval.declarator) = make_declarator(NULL);
						  (yyval.declarator)->func_type = append_ptrchain_type((yyval.declarator)->type, type_new_function((yyvsp[(2) - (3)].var_list)));
						  (yyval.declarator)->type = NULL;
						}
    break;

  case 353:

/* Line 1806 of yacc.c  */
#line 1039 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (4)].declarator);
						  (yyval.declarator)->func_type = append_ptrchain_type((yyval.declarator)->type, type_new_function((yyvsp[(3) - (4)].var_list)));
						  (yyval.declarator)->type = NULL;
						}
    break;

  case 354:

/* Line 1806 of yacc.c  */
#line 1046 "parser.y"
    { (yyval.declarator_list) = append_declarator( NULL, (yyvsp[(1) - (1)].declarator) ); }
    break;

  case 355:

/* Line 1806 of yacc.c  */
#line 1047 "parser.y"
    { (yyval.declarator_list) = append_declarator( (yyvsp[(1) - (3)].declarator_list), (yyvsp[(3) - (3)].declarator) ); }
    break;

  case 356:

/* Line 1806 of yacc.c  */
#line 1050 "parser.y"
    { (yyval.expr) = NULL; }
    break;

  case 357:

/* Line 1806 of yacc.c  */
#line 1051 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (2)].expr); }
    break;

  case 358:

/* Line 1806 of yacc.c  */
#line 1054 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (2)].declarator); (yyval.declarator)->bits = (yyvsp[(2) - (2)].expr);
						  if (!(yyval.declarator)->bits && !(yyval.declarator)->var->name)
						    error_loc("unnamed fields are not allowed\n");
						}
    break;

  case 359:

/* Line 1806 of yacc.c  */
#line 1061 "parser.y"
    { (yyval.declarator_list) = append_declarator( NULL, (yyvsp[(1) - (1)].declarator) ); }
    break;

  case 360:

/* Line 1806 of yacc.c  */
#line 1063 "parser.y"
    { (yyval.declarator_list) = append_declarator( (yyvsp[(1) - (3)].declarator_list), (yyvsp[(3) - (3)].declarator) ); }
    break;

  case 361:

/* Line 1806 of yacc.c  */
#line 1067 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (1)].declarator); }
    break;

  case 362:

/* Line 1806 of yacc.c  */
#line 1068 "parser.y"
    { (yyval.declarator) = (yyvsp[(1) - (3)].declarator); (yyvsp[(1) - (3)].declarator)->var->eval = (yyvsp[(3) - (3)].expr); }
    break;

  case 363:

/* Line 1806 of yacc.c  */
#line 1072 "parser.y"
    { (yyval.num) = THREADING_APARTMENT; }
    break;

  case 364:

/* Line 1806 of yacc.c  */
#line 1073 "parser.y"
    { (yyval.num) = THREADING_NEUTRAL; }
    break;

  case 365:

/* Line 1806 of yacc.c  */
#line 1074 "parser.y"
    { (yyval.num) = THREADING_SINGLE; }
    break;

  case 366:

/* Line 1806 of yacc.c  */
#line 1075 "parser.y"
    { (yyval.num) = THREADING_FREE; }
    break;

  case 367:

/* Line 1806 of yacc.c  */
#line 1076 "parser.y"
    { (yyval.num) = THREADING_BOTH; }
    break;

  case 368:

/* Line 1806 of yacc.c  */
#line 1080 "parser.y"
    { (yyval.num) = RPC_FC_RP; }
    break;

  case 369:

/* Line 1806 of yacc.c  */
#line 1081 "parser.y"
    { (yyval.num) = RPC_FC_UP; }
    break;

  case 370:

/* Line 1806 of yacc.c  */
#line 1082 "parser.y"
    { (yyval.num) = RPC_FC_FP; }
    break;

  case 371:

/* Line 1806 of yacc.c  */
#line 1085 "parser.y"
    { (yyval.type) = type_new_struct((yyvsp[(2) - (5)].str), TRUE, (yyvsp[(4) - (5)].var_list)); }
    break;

  case 372:

/* Line 1806 of yacc.c  */
#line 1088 "parser.y"
    { (yyval.type) = type_new_void(); }
    break;

  case 373:

/* Line 1806 of yacc.c  */
#line 1089 "parser.y"
    { (yyval.type) = find_type_or_error((yyvsp[(1) - (1)].str), 0); }
    break;

  case 374:

/* Line 1806 of yacc.c  */
#line 1090 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (1)].type); }
    break;

  case 375:

/* Line 1806 of yacc.c  */
#line 1091 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (1)].type); }
    break;

  case 376:

/* Line 1806 of yacc.c  */
#line 1092 "parser.y"
    { (yyval.type) = type_new_enum((yyvsp[(2) - (2)].str), FALSE, NULL); }
    break;

  case 377:

/* Line 1806 of yacc.c  */
#line 1093 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (1)].type); }
    break;

  case 378:

/* Line 1806 of yacc.c  */
#line 1094 "parser.y"
    { (yyval.type) = type_new_struct((yyvsp[(2) - (2)].str), FALSE, NULL); }
    break;

  case 379:

/* Line 1806 of yacc.c  */
#line 1095 "parser.y"
    { (yyval.type) = (yyvsp[(1) - (1)].type); }
    break;

  case 380:

/* Line 1806 of yacc.c  */
#line 1096 "parser.y"
    { (yyval.type) = type_new_nonencapsulated_union((yyvsp[(2) - (2)].str), FALSE, NULL); }
    break;

  case 381:

/* Line 1806 of yacc.c  */
#line 1097 "parser.y"
    { (yyval.type) = make_safearray((yyvsp[(3) - (4)].type)); }
    break;

  case 382:

/* Line 1806 of yacc.c  */
#line 1101 "parser.y"
    { (yyvsp[(1) - (5)].attr_list) = append_attribs((yyvsp[(1) - (5)].attr_list), (yyvsp[(3) - (5)].attr_list));
						  reg_typedefs((yyvsp[(4) - (5)].declspec), (yyvsp[(5) - (5)].declarator_list), check_typedef_attrs((yyvsp[(1) - (5)].attr_list)));
						  (yyval.statement) = make_statement_typedef((yyvsp[(5) - (5)].declarator_list));
						}
    break;

  case 383:

/* Line 1806 of yacc.c  */
#line 1108 "parser.y"
    { (yyval.type) = type_new_nonencapsulated_union((yyvsp[(2) - (5)].str), TRUE, (yyvsp[(4) - (5)].var_list)); }
    break;

  case 384:

/* Line 1806 of yacc.c  */
#line 1111 "parser.y"
    { (yyval.type) = type_new_encapsulated_union((yyvsp[(2) - (10)].str), (yyvsp[(5) - (10)].var), (yyvsp[(7) - (10)].var), (yyvsp[(9) - (10)].var_list)); }
    break;

  case 385:

/* Line 1806 of yacc.c  */
#line 1115 "parser.y"
    { (yyval.num) = MAKEVERSION((yyvsp[(1) - (1)].num), 0); }
    break;

  case 386:

/* Line 1806 of yacc.c  */
#line 1116 "parser.y"
    { (yyval.num) = MAKEVERSION((yyvsp[(1) - (3)].num), (yyvsp[(3) - (3)].num)); }
    break;

  case 387:

/* Line 1806 of yacc.c  */
#line 1117 "parser.y"
    { (yyval.num) = (yyvsp[(1) - (1)].num); }
    break;



/* Line 1806 of yacc.c  */
#line 5590 "parser.tab.c"
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
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
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
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
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



/* Line 2067 of yacc.c  */
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
      !type->name)
  {
    if (! is_attr(attrs, ATTR_PUBLIC) && ! is_attr (attrs, ATTR_HIDDEN))
      attrs = append_attr( attrs, make_attr(ATTR_PUBLIC) );
    type->name = gen_name();
  }
  else if (is_attr(attrs, ATTR_UUID) && !is_attr(attrs, ATTR_PUBLIC)
	   && !is_attr(attrs, ATTR_HIDDEN))
    attrs = append_attr( attrs, make_attr(ATTR_PUBLIC) );

  /* Append the SWITCHTYPE attribute to a union if it does not already have one.  */
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
    /* ATTR_VERSION */             { 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, "version" },
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

