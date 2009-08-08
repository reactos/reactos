
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
#define YYPURE 1

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
#line 19 "parser.y"


#include "jscript.h"
#include "engine.h"

#define YYLEX_PARAM ctx
#define YYPARSE_PARAM ctx

static int parser_error(const char*);
static void set_error(parser_ctx_t*,UINT);
static BOOL explicit_error(parser_ctx_t*,void*,WCHAR);
static BOOL allow_auto_semicolon(parser_ctx_t*);
static void program_parsed(parser_ctx_t*,source_elements_t*);
static source_elements_t *function_body_parsed(parser_ctx_t*,source_elements_t*);

typedef struct _statement_list_t {
    statement_t *head;
    statement_t *tail;
} statement_list_t;

static literal_t *new_string_literal(parser_ctx_t*,const WCHAR*);
static literal_t *new_null_literal(parser_ctx_t*);
static literal_t *new_undefined_literal(parser_ctx_t*);
static literal_t *new_boolean_literal(parser_ctx_t*,VARIANT_BOOL);

typedef struct _property_list_t {
    prop_val_t *head;
    prop_val_t *tail;
} property_list_t;

static property_list_t *new_property_list(parser_ctx_t*,literal_t*,expression_t*);
static property_list_t *property_list_add(parser_ctx_t*,property_list_t*,literal_t*,expression_t*);

typedef struct _element_list_t {
    array_element_t *head;
    array_element_t *tail;
} element_list_t;

static element_list_t *new_element_list(parser_ctx_t*,int,expression_t*);
static element_list_t *element_list_add(parser_ctx_t*,element_list_t*,int,expression_t*);

typedef struct _argument_list_t {
    argument_t *head;
    argument_t *tail;
} argument_list_t;

static argument_list_t *new_argument_list(parser_ctx_t*,expression_t*);
static argument_list_t *argument_list_add(parser_ctx_t*,argument_list_t*,expression_t*);

typedef struct _case_list_t {
    case_clausule_t *head;
    case_clausule_t *tail;
} case_list_t;

static catch_block_t *new_catch_block(parser_ctx_t*,const WCHAR*,statement_t*);
static case_clausule_t *new_case_clausule(parser_ctx_t*,expression_t*,statement_list_t*);
static case_list_t *new_case_list(parser_ctx_t*,case_clausule_t*);
static case_list_t *case_list_add(parser_ctx_t*,case_list_t*,case_clausule_t*);
static case_clausule_t *new_case_block(parser_ctx_t*,case_list_t*,case_clausule_t*,case_list_t*);

typedef struct _variable_list_t {
    variable_declaration_t *head;
    variable_declaration_t *tail;
} variable_list_t;

static variable_declaration_t *new_variable_declaration(parser_ctx_t*,const WCHAR*,expression_t*);
static variable_list_t *new_variable_list(parser_ctx_t*,variable_declaration_t*);
static variable_list_t *variable_list_add(parser_ctx_t*,variable_list_t*,variable_declaration_t*);

static statement_t *new_block_statement(parser_ctx_t*,statement_list_t*);
static statement_t *new_var_statement(parser_ctx_t*,variable_list_t*);
static statement_t *new_empty_statement(parser_ctx_t*);
static statement_t *new_expression_statement(parser_ctx_t*,expression_t*);
static statement_t *new_if_statement(parser_ctx_t*,expression_t*,statement_t*,statement_t*);
static statement_t *new_while_statement(parser_ctx_t*,BOOL,expression_t*,statement_t*);
static statement_t *new_for_statement(parser_ctx_t*,variable_list_t*,expression_t*,expression_t*,
        expression_t*,statement_t*);
static statement_t *new_forin_statement(parser_ctx_t*,variable_declaration_t*,expression_t*,expression_t*,statement_t*);
static statement_t *new_continue_statement(parser_ctx_t*,const WCHAR*);
static statement_t *new_break_statement(parser_ctx_t*,const WCHAR*);
static statement_t *new_return_statement(parser_ctx_t*,expression_t*);
static statement_t *new_with_statement(parser_ctx_t*,expression_t*,statement_t*);
static statement_t *new_labelled_statement(parser_ctx_t*,const WCHAR*,statement_t*);
static statement_t *new_switch_statement(parser_ctx_t*,expression_t*,case_clausule_t*);
static statement_t *new_throw_statement(parser_ctx_t*,expression_t*);
static statement_t *new_try_statement(parser_ctx_t*,statement_t*,catch_block_t*,statement_t*);

struct statement_list_t {
   statement_t *head;
   statement_t *tail;
};

static statement_list_t *new_statement_list(parser_ctx_t*,statement_t*);
static statement_list_t *statement_list_add(statement_list_t*,statement_t*);

typedef struct _parameter_list_t {
    parameter_t *head;
    parameter_t *tail;
} parameter_list_t;

static parameter_list_t *new_parameter_list(parser_ctx_t*,const WCHAR*);
static parameter_list_t *parameter_list_add(parser_ctx_t*,parameter_list_t*,const WCHAR*);

static void push_func(parser_ctx_t*);
static inline void pop_func(parser_ctx_t *ctx)
{
    ctx->func_stack = ctx->func_stack->next;
}

static expression_t *new_function_expression(parser_ctx_t*,const WCHAR*,parameter_list_t*,
        source_elements_t*,const WCHAR*,DWORD);
static expression_t *new_binary_expression(parser_ctx_t*,expression_type_t,expression_t*,expression_t*);
static expression_t *new_unary_expression(parser_ctx_t*,expression_type_t,expression_t*);
static expression_t *new_conditional_expression(parser_ctx_t*,expression_t*,expression_t*,expression_t*);
static expression_t *new_array_expression(parser_ctx_t*,expression_t*,expression_t*);
static expression_t *new_member_expression(parser_ctx_t*,expression_t*,const WCHAR*);
static expression_t *new_new_expression(parser_ctx_t*,expression_t*,argument_list_t*);
static expression_t *new_call_expression(parser_ctx_t*,expression_t*,argument_list_t*);
static expression_t *new_this_expression(parser_ctx_t*);
static expression_t *new_identifier_expression(parser_ctx_t*,const WCHAR*);
static expression_t *new_literal_expression(parser_ctx_t*,literal_t*);
static expression_t *new_array_literal_expression(parser_ctx_t*,element_list_t*,int);
static expression_t *new_prop_and_value_expression(parser_ctx_t*,property_list_t*);

static source_elements_t *new_source_elements(parser_ctx_t*);
static source_elements_t *source_elements_add_statement(source_elements_t*,statement_t*);



/* Line 189 of yacc.c  */
#line 211 "parser.tab.c"

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
     kBREAK = 258,
     kCASE = 259,
     kCATCH = 260,
     kCONTINUE = 261,
     kDEFAULT = 262,
     kDELETE = 263,
     kDO = 264,
     kELSE = 265,
     kIF = 266,
     kFINALLY = 267,
     kFOR = 268,
     kIN = 269,
     kINSTANCEOF = 270,
     kNEW = 271,
     kNULL = 272,
     kUNDEFINED = 273,
     kRETURN = 274,
     kSWITCH = 275,
     kTHIS = 276,
     kTHROW = 277,
     kTRUE = 278,
     kFALSE = 279,
     kTRY = 280,
     kTYPEOF = 281,
     kVAR = 282,
     kVOID = 283,
     kWHILE = 284,
     kWITH = 285,
     tANDAND = 286,
     tOROR = 287,
     tINC = 288,
     tDEC = 289,
     tHTMLCOMMENT = 290,
     kFUNCTION = 291,
     tIdentifier = 292,
     tAssignOper = 293,
     tEqOper = 294,
     tShiftOper = 295,
     tRelOper = 296,
     tNumericLiteral = 297,
     tStringLiteral = 298,
     LOWER_THAN_ELSE = 299
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 151 "parser.y"

    int                     ival;
    const WCHAR             *srcptr;
    LPCWSTR                 wstr;
    literal_t               *literal;
    struct _argument_list_t *argument_list;
    case_clausule_t         *case_clausule;
    struct _case_list_t     *case_list;
    catch_block_t           *catch_block;
    struct _element_list_t  *element_list;
    expression_t            *expr;
    const WCHAR            *identifier;
    struct _parameter_list_t *parameter_list;
    struct _property_list_t *property_list;
    source_elements_t       *source_elements;
    statement_t             *statement;
    struct _statement_list_t *statement_list;
    struct _variable_list_t *variable_list;
    variable_declaration_t  *variable_declaration;



/* Line 214 of yacc.c  */
#line 314 "parser.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 326 "parser.tab.c"

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
#define YYLAST   942

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  67
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  94
/* YYNRULES -- Number of rules.  */
#define YYNRULES  211
/* YYNRULES -- Number of states.  */
#define YYNSTATES  370

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   299

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    61,     2,     2,     2,    59,    54,     2,
      65,    66,    57,    55,    47,    56,    64,    58,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    50,    49,
       2,    48,     2,    51,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    62,     2,    63,    53,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    46,    52,    37,    60,     2,     2,     2,
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
      35,    36,    38,    39,    40,    41,    42,    43,    44,    45
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,     8,     9,    10,    13,    22,    24,
      26,    28,    32,    33,    35,    37,    39,    41,    43,    45,
      47,    49,    51,    53,    55,    57,    59,    61,    63,    65,
      68,    69,    71,    75,    78,    82,    84,    88,    90,    94,
      97,   100,   101,   103,   106,   107,   109,   112,   114,   117,
     125,   131,   139,   145,   146,   147,   159,   160,   161,   174,
     182,   191,   195,   199,   203,   209,   213,   219,   223,   229,
     230,   232,   234,   237,   242,   246,   250,   254,   258,   263,
     269,   272,   273,   275,   277,   279,   281,   285,   286,   288,
     290,   294,   296,   300,   304,   306,   310,   314,   316,   322,
     324,   330,   332,   336,   338,   342,   344,   348,   350,   354,
     356,   360,   362,   366,   368,   372,   374,   378,   380,   384,
     386,   390,   392,   396,   398,   402,   404,   408,   412,   416,
     418,   422,   426,   428,   432,   434,   438,   442,   444,   448,
     452,   456,   458,   461,   464,   467,   470,   473,   476,   479,
     482,   485,   487,   490,   493,   495,   497,   499,   502,   504,
     506,   511,   515,   519,   522,   525,   530,   534,   537,   541,
     543,   547,   549,   551,   553,   555,   557,   561,   564,   568,
     572,   578,   581,   586,   588,   591,   592,   594,   597,   601,
     605,   611,   613,   615,   617,   618,   620,   622,   624,   626,
     628,   630,   632,   634,   636,   638,   640,   642,   644,   646,
     648,   650
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      68,     0,    -1,    70,    69,    -1,    35,    -1,    -1,    -1,
      70,    76,    -1,    72,   154,   158,    75,   159,    46,    73,
      37,    -1,    36,    -1,    70,    -1,    38,    -1,    74,    47,
      38,    -1,    -1,    74,    -1,    79,    -1,    80,    -1,    89,
      -1,    90,    -1,    91,    -1,    92,    -1,    97,    -1,    98,
      -1,    99,    -1,   100,    -1,   101,    -1,   102,    -1,   108,
      -1,   109,    -1,    76,    -1,    77,    76,    -1,    -1,    77,
      -1,    46,    77,    37,    -1,    46,    37,    -1,    27,    81,
     157,    -1,    83,    -1,    81,    47,    83,    -1,    84,    -1,
      82,    47,    84,    -1,    38,    85,    -1,    38,    87,    -1,
      -1,    86,    -1,    48,   117,    -1,    -1,    88,    -1,    48,
     118,    -1,    49,    -1,   114,   157,    -1,    11,   158,   113,
     159,    76,    10,    76,    -1,    11,   158,   113,   159,    76,
      -1,     9,    76,    29,   158,   113,   159,   157,    -1,    29,
     158,   113,   159,    76,    -1,    -1,    -1,    13,   158,   115,
      93,   160,   112,    94,   160,   112,   159,    76,    -1,    -1,
      -1,    13,   158,    27,    82,    95,   160,   112,    96,   160,
     112,   159,    76,    -1,    13,   158,   140,    14,   113,   159,
      76,    -1,    13,   158,    27,    84,    14,   113,   159,    76,
      -1,     6,   154,   157,    -1,     3,   154,   157,    -1,    19,
     112,   157,    -1,    30,   158,   114,   159,    76,    -1,    38,
      50,    76,    -1,    20,   158,   114,   159,   103,    -1,    46,
     104,    37,    -1,    46,   104,   107,   104,    37,    -1,    -1,
     105,    -1,   106,    -1,   105,   106,    -1,     4,   114,    50,
      78,    -1,     7,    50,    78,    -1,    22,   114,   157,    -1,
      25,    79,   110,    -1,    25,    79,   111,    -1,    25,    79,
     110,   111,    -1,     5,   158,    38,   159,    79,    -1,    12,
      79,    -1,    -1,   114,    -1,   114,    -1,     1,    -1,   117,
      -1,   114,    47,   117,    -1,    -1,   116,    -1,   118,    -1,
     116,    47,   118,    -1,   119,    -1,   140,    48,   117,    -1,
     140,    39,   117,    -1,   120,    -1,   140,    48,   118,    -1,
     140,    39,   118,    -1,   121,    -1,   121,    51,   117,    50,
     117,    -1,   122,    -1,   122,    51,   118,    50,   118,    -1,
     123,    -1,   121,    32,   123,    -1,   124,    -1,   122,    32,
     124,    -1,   125,    -1,   123,    31,   125,    -1,   126,    -1,
     124,    31,   126,    -1,   127,    -1,   125,    52,   127,    -1,
     128,    -1,   126,    52,   128,    -1,   129,    -1,   127,    53,
     129,    -1,   130,    -1,   128,    53,   130,    -1,   131,    -1,
     129,    54,   131,    -1,   132,    -1,   130,    54,   132,    -1,
     133,    -1,   131,    40,   133,    -1,   134,    -1,   132,    40,
     134,    -1,   135,    -1,   133,    42,   135,    -1,   133,    15,
     135,    -1,   133,    14,   135,    -1,   135,    -1,   134,    42,
     135,    -1,   134,    15,   135,    -1,   136,    -1,   135,    41,
     136,    -1,   137,    -1,   136,    55,   137,    -1,   136,    56,
     137,    -1,   138,    -1,   137,    57,   138,    -1,   137,    58,
     138,    -1,   137,    59,   138,    -1,   139,    -1,     8,   138,
      -1,    28,   138,    -1,    26,   138,    -1,    33,   138,    -1,
      34,   138,    -1,    55,   138,    -1,    56,   138,    -1,    60,
     138,    -1,    61,   138,    -1,   140,    -1,   140,    33,    -1,
     140,    34,    -1,   141,    -1,   143,    -1,   142,    -1,    16,
     141,    -1,   146,    -1,    71,    -1,   142,    62,   114,    63,
      -1,   142,    64,    38,    -1,    16,   142,   144,    -1,   142,
     144,    -1,   143,   144,    -1,   143,    62,   114,    63,    -1,
     143,    64,    38,    -1,    65,    66,    -1,    65,   145,    66,
      -1,   117,    -1,   145,    47,   117,    -1,    21,    -1,    38,
      -1,   155,    -1,   147,    -1,   151,    -1,    65,   114,    66,
      -1,    62,    63,    -1,    62,   149,    63,    -1,    62,   148,
      63,    -1,    62,   148,    47,   150,    63,    -1,   150,   117,
      -1,   148,    47,   150,   117,    -1,    47,    -1,   149,    47,
      -1,    -1,   149,    -1,    46,    37,    -1,    46,   152,    37,
      -1,   153,    50,   117,    -1,   152,    47,   153,    50,   117,
      -1,    38,    -1,    44,    -1,    43,    -1,    -1,    38,    -1,
      17,    -1,    18,    -1,   156,    -1,    43,    -1,    44,    -1,
      58,    -1,    23,    -1,    24,    -1,    49,    -1,     1,    -1,
      65,    -1,     1,    -1,    66,    -1,     1,    -1,    49,    -1,
       1,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   256,   256,   260,   261,   265,   266,   271,   275,   279,
     283,   284,   289,   290,   294,   295,   296,   297,   298,   299,
     300,   301,   302,   303,   304,   305,   306,   307,   311,   312,
     317,   318,   322,   323,   327,   332,   333,   338,   340,   345,
     350,   355,   356,   360,   365,   366,   370,   375,   379,   384,
     386,   391,   393,   396,   398,   395,   402,   404,   401,   407,
     409,   414,   419,   424,   429,   434,   439,   444,   446,   451,
     452,   456,   457,   462,   467,   472,   477,   478,   479,   484,
     489,   493,   494,   497,   498,   502,   503,   508,   509,   513,
     515,   520,   521,   523,   528,   530,   532,   537,   538,   543,
     545,   550,   551,   556,   558,   563,   564,   569,   571,   576,
     577,   582,   584,   589,   590,   595,   597,   602,   603,   608,
     610,   615,   616,   621,   622,   627,   628,   630,   632,   637,
     638,   640,   645,   646,   651,   653,   655,   660,   661,   663,
     665,   670,   671,   673,   674,   676,   677,   678,   679,   680,
     681,   685,   687,   689,   695,   696,   700,   701,   705,   706,
     707,   709,   711,   716,   718,   720,   722,   727,   728,   732,
     733,   738,   739,   740,   741,   742,   743,   747,   748,   749,
     750,   755,   757,   762,   763,   767,   768,   772,   773,   778,
     780,   785,   786,   787,   791,   792,   796,   797,   798,   799,
     800,   801,   806,   807,   810,   811,   814,   815,   818,   819,
     822,   823
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "kBREAK", "kCASE", "kCATCH", "kCONTINUE",
  "kDEFAULT", "kDELETE", "kDO", "kELSE", "kIF", "kFINALLY", "kFOR", "kIN",
  "kINSTANCEOF", "kNEW", "kNULL", "kUNDEFINED", "kRETURN", "kSWITCH",
  "kTHIS", "kTHROW", "kTRUE", "kFALSE", "kTRY", "kTYPEOF", "kVAR", "kVOID",
  "kWHILE", "kWITH", "tANDAND", "tOROR", "tINC", "tDEC", "tHTMLCOMMENT",
  "kFUNCTION", "'}'", "tIdentifier", "tAssignOper", "tEqOper",
  "tShiftOper", "tRelOper", "tNumericLiteral", "tStringLiteral",
  "LOWER_THAN_ELSE", "'{'", "','", "'='", "';'", "':'", "'?'", "'|'",
  "'^'", "'&'", "'+'", "'-'", "'*'", "'/'", "'%'", "'~'", "'!'", "'['",
  "']'", "'.'", "'('", "')'", "$accept", "Program", "HtmlComment",
  "SourceElements", "FunctionExpression", "KFunction", "FunctionBody",
  "FormalParameterList", "FormalParameterList_opt", "Statement",
  "StatementList", "StatementList_opt", "Block", "VariableStatement",
  "VariableDeclarationList", "VariableDeclarationListNoIn",
  "VariableDeclaration", "VariableDeclarationNoIn", "Initialiser_opt",
  "Initialiser", "InitialiserNoIn_opt", "InitialiserNoIn",
  "EmptyStatement", "ExpressionStatement", "IfStatement",
  "IterationStatement", "$@1", "$@2", "$@3", "$@4", "ContinueStatement",
  "BreakStatement", "ReturnStatement", "WithStatement",
  "LabelledStatement", "SwitchStatement", "CaseBlock", "CaseClausules_opt",
  "CaseClausules", "CaseClausule", "DefaultClausule", "ThrowStatement",
  "TryStatement", "Catch", "Finally", "Expression_opt", "Expression_err",
  "Expression", "ExpressionNoIn_opt", "ExpressionNoIn",
  "AssignmentExpression", "AssignmentExpressionNoIn",
  "ConditionalExpression", "ConditionalExpressionNoIn",
  "LogicalORExpression", "LogicalORExpressionNoIn", "LogicalANDExpression",
  "LogicalANDExpressionNoIn", "BitwiseORExpression",
  "BitwiseORExpressionNoIn", "BitwiseXORExpression",
  "BitwiseXORExpressionNoIn", "BitwiseANDExpression",
  "BitwiseANDExpressionNoIn", "EqualityExpression",
  "EqualityExpressionNoIn", "RelationalExpression",
  "RelationalExpressionNoIn", "ShiftExpression", "AdditiveExpression",
  "MultiplicativeExpression", "UnaryExpression", "PostfixExpression",
  "LeftHandSideExpression", "NewExpression", "MemberExpression",
  "CallExpression", "Arguments", "ArgumentList", "PrimaryExpression",
  "ArrayLiteral", "ElementList", "Elision", "Elision_opt", "ObjectLiteral",
  "PropertyNameAndValueList", "PropertyName", "Identifier_opt", "Literal",
  "BooleanLiteral", "semicolon_opt", "left_bracket", "right_bracket",
  "semicolon", 0
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
     285,   286,   287,   288,   289,   290,   291,   125,   292,   293,
     294,   295,   296,   297,   298,   299,   123,    44,    61,    59,
      58,    63,   124,    94,    38,    43,    45,    42,    47,    37,
     126,    33,    91,    93,    46,    40,    41
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    67,    68,    69,    69,    70,    70,    71,    72,    73,
      74,    74,    75,    75,    76,    76,    76,    76,    76,    76,
      76,    76,    76,    76,    76,    76,    76,    76,    77,    77,
      78,    78,    79,    79,    80,    81,    81,    82,    82,    83,
      84,    85,    85,    86,    87,    87,    88,    89,    90,    91,
      91,    92,    92,    93,    94,    92,    95,    96,    92,    92,
      92,    97,    98,    99,   100,   101,   102,   103,   103,   104,
     104,   105,   105,   106,   107,   108,   109,   109,   109,   110,
     111,   112,   112,   113,   113,   114,   114,   115,   115,   116,
     116,   117,   117,   117,   118,   118,   118,   119,   119,   120,
     120,   121,   121,   122,   122,   123,   123,   124,   124,   125,
     125,   126,   126,   127,   127,   128,   128,   129,   129,   130,
     130,   131,   131,   132,   132,   133,   133,   133,   133,   134,
     134,   134,   135,   135,   136,   136,   136,   137,   137,   137,
     137,   138,   138,   138,   138,   138,   138,   138,   138,   138,
     138,   139,   139,   139,   140,   140,   141,   141,   142,   142,
     142,   142,   142,   143,   143,   143,   143,   144,   144,   145,
     145,   146,   146,   146,   146,   146,   146,   147,   147,   147,
     147,   148,   148,   149,   149,   150,   150,   151,   151,   152,
     152,   153,   153,   153,   154,   154,   155,   155,   155,   155,
     155,   155,   156,   156,   157,   157,   158,   158,   159,   159,
     160,   160
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     1,     0,     0,     2,     8,     1,     1,
       1,     3,     0,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       0,     1,     3,     2,     3,     1,     3,     1,     3,     2,
       2,     0,     1,     2,     0,     1,     2,     1,     2,     7,
       5,     7,     5,     0,     0,    11,     0,     0,    12,     7,
       8,     3,     3,     3,     5,     3,     5,     3,     5,     0,
       1,     1,     2,     4,     3,     3,     3,     3,     4,     5,
       2,     0,     1,     1,     1,     1,     3,     0,     1,     1,
       3,     1,     3,     3,     1,     3,     3,     1,     5,     1,
       5,     1,     3,     1,     3,     1,     3,     1,     3,     1,
       3,     1,     3,     1,     3,     1,     3,     1,     3,     1,
       3,     1,     3,     1,     3,     1,     3,     3,     3,     1,
       3,     3,     1,     3,     1,     3,     3,     1,     3,     3,
       3,     1,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     1,     2,     2,     1,     1,     1,     2,     1,     1,
       4,     3,     3,     2,     2,     4,     3,     2,     3,     1,
       3,     1,     1,     1,     1,     1,     3,     2,     3,     3,
       5,     2,     4,     1,     2,     0,     1,     2,     3,     3,
       5,     1,     1,     1,     0,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       5,     0,     4,     1,   194,   194,     0,     0,     0,     0,
       0,   196,   197,    81,     0,   171,     0,   202,   203,     0,
       0,     0,     0,     0,     0,     0,     0,     3,     8,   172,
     199,   200,     0,    47,     0,     0,   201,     0,     0,   185,
       0,     2,   159,   194,     6,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,     0,
      85,    91,    97,   101,   105,   109,   113,   117,   121,   125,
     132,   134,   137,   141,   151,   154,   156,   155,   158,   174,
     175,   173,   198,   195,     0,     0,   172,     0,   142,   151,
       0,   207,   206,     0,    87,   157,   156,     0,    82,     0,
       0,     0,     0,   144,    41,     0,    35,   143,     0,     0,
     145,   146,     0,    33,   172,   199,   200,    28,     0,     0,
       0,   147,   148,   149,   150,   183,   177,     0,   186,     0,
       0,     0,   205,     0,   204,    48,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   152,   153,     0,     0,     0,     0,     0,   163,
       0,     0,   164,    62,    61,   187,   191,   193,   192,     0,
      84,     0,    83,     0,    53,    88,    89,    94,    99,   103,
     107,   111,   115,   119,   123,   129,   151,   162,    63,     0,
      75,    33,     0,     0,    76,    77,     0,    39,    42,     0,
      34,     0,     0,    65,    32,    29,   188,     0,     0,   185,
     179,   184,   178,   181,   176,    12,    86,   102,     0,   106,
     110,   114,   118,   122,   128,   127,   126,   133,   135,   136,
     138,   139,   140,    93,    92,     0,   161,   167,   169,     0,
       0,   166,     0,   209,   208,     0,    44,    56,    37,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    80,    78,    43,    36,     0,
       0,     0,   189,   186,     0,    10,    13,     0,     0,   160,
       0,   168,   165,     0,    50,     0,    40,    45,     0,     0,
       0,   211,   210,    81,    90,   151,   104,     0,   108,   112,
     116,   120,   124,   131,   130,     0,    96,    95,    69,    66,
       0,    52,    64,     0,   180,   182,     0,     0,    98,   170,
       0,     0,    46,    38,    81,     0,    54,     0,     0,     0,
       0,    70,    71,     0,   190,    11,     5,    51,    49,    57,
       0,     0,   100,    59,     0,     0,    67,    69,    72,    79,
       9,     0,     0,    60,    81,    30,    30,     0,     7,    81,
       0,    31,    73,    74,    68,     0,     0,     0,    55,    58
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    41,     2,    42,    43,   351,   276,   277,   117,
     118,   362,    45,    46,   105,   247,   106,   248,   197,   198,
     286,   287,    47,    48,    49,    50,   249,   341,   289,   352,
      51,    52,    53,    54,    55,    56,   309,   330,   331,   332,
     347,    57,    58,   194,   195,    97,   171,    59,   174,   175,
      60,   176,    61,   177,    62,   178,    63,   179,    64,   180,
      65,   181,    66,   182,    67,   183,    68,   184,    69,    70,
      71,    72,    73,    74,    75,    76,    77,   159,   239,    78,
      79,   127,   128,   129,    80,   119,   120,    84,    81,    82,
     135,    93,   245,   293
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -278
static const yytype_int16 yypact[] =
{
    -278,    20,   433,  -278,    -3,    -3,   877,   673,    12,    12,
     196,  -278,  -278,   877,    12,  -278,   877,  -278,  -278,    15,
     877,    28,   877,    12,    12,   877,   877,  -278,  -278,     1,
    -278,  -278,   493,  -278,   877,   877,  -278,   877,   877,    59,
     877,  -278,  -278,    -3,  -278,  -278,  -278,  -278,  -278,  -278,
    -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,    16,
    -278,  -278,   -13,    83,    72,    32,    85,   102,    88,   114,
     153,   104,  -278,  -278,    98,  -278,   -40,   -28,  -278,  -278,
    -278,  -278,  -278,  -278,     8,     8,  -278,   113,  -278,   182,
     135,  -278,  -278,   345,   775,  -278,   -40,     8,   119,   877,
      16,   553,   131,  -278,   120,    51,  -278,  -278,   345,   877,
    -278,  -278,   673,   381,     1,   136,   149,  -278,   613,    62,
     156,  -278,  -278,  -278,  -278,  -278,  -278,    60,    70,   877,
     -26,    12,  -278,   877,  -278,  -278,   877,   877,   877,   877,
     877,   877,   877,   877,   877,   877,   877,   877,   877,   877,
     877,   877,  -278,  -278,   877,   877,   877,   173,   724,  -278,
     877,   180,  -278,  -278,  -278,  -278,  -278,  -278,  -278,    12,
    -278,     6,   119,   185,  -278,   186,  -278,  -278,    -6,   210,
     192,   193,   191,   207,    14,   114,    87,  -278,  -278,     7,
    -278,  -278,    12,    15,   237,  -278,   877,  -278,  -278,    28,
    -278,     6,     7,  -278,  -278,  -278,  -278,   116,   877,   203,
    -278,  -278,  -278,  -278,  -278,   213,  -278,    83,   202,    72,
      32,    85,   102,    88,   114,   114,   114,   153,   104,   104,
    -278,  -278,  -278,  -278,  -278,    71,  -278,  -278,  -278,   -17,
      82,  -278,   345,  -278,  -278,   673,   205,   208,   242,    13,
     877,   877,   877,   877,   877,   877,   877,   877,   877,   877,
     345,   877,   877,   211,   221,  -278,  -278,  -278,  -278,   673,
     673,   212,  -278,   216,   826,  -278,   217,     6,   877,  -278,
     877,  -278,  -278,     6,   250,   877,  -278,  -278,   185,    13,
     345,  -278,  -278,   877,  -278,   188,   210,   215,   192,   193,
     191,   207,    14,   114,   114,     6,  -278,  -278,   265,  -278,
       6,  -278,  -278,   877,  -278,  -278,   233,   226,  -278,  -278,
       8,   673,  -278,  -278,   877,     6,  -278,   877,   673,   877,
       9,   265,  -278,    15,  -278,  -278,  -278,  -278,  -278,  -278,
     673,    13,  -278,  -278,    91,   223,  -278,   265,  -278,  -278,
     673,   238,    13,  -278,   877,   673,   673,   239,  -278,   877,
       6,   673,  -278,  -278,  -278,     6,   673,   673,  -278,  -278
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -278,  -278,  -278,   -62,  -278,  -278,  -278,  -278,  -278,    -2,
    -130,   -79,    -8,  -278,  -278,  -278,    80,    10,  -278,  -278,
    -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,  -278,
    -278,  -278,  -278,  -278,  -278,  -278,  -278,   -67,  -278,   -49,
    -278,  -278,  -278,  -278,    99,  -246,  -102,   -12,  -278,  -278,
     -43,  -157,  -278,  -278,  -278,  -278,   160,    46,   161,    47,
     162,    48,   163,    49,   164,    50,   165,    52,   -61,   167,
      81,    54,  -278,    33,   298,   300,  -278,   -27,  -278,  -278,
    -278,  -278,   105,   106,  -278,  -278,   109,     5,  -278,  -278,
     -82,    18,  -158,  -277
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -194
static const yytype_int16 yytable[] =
{
      44,    98,   163,   164,   100,    90,   201,   243,   243,   132,
      85,   102,   324,    91,   291,   188,   345,   132,   190,   136,
       3,   133,   156,   200,   157,   158,   251,    94,   130,   258,
     280,   263,    99,   185,   160,    83,   161,   158,   137,    89,
     214,   108,   109,   269,   270,   252,   346,   326,   131,   281,
     162,   112,   132,    89,   133,    89,   259,   134,    89,    89,
      88,   101,   292,   133,   354,   134,   104,    89,    89,   187,
      89,    89,   244,   244,   103,   359,   107,    92,   339,   110,
     111,   172,   224,   225,   226,   140,   213,   189,   121,   122,
     216,   123,   124,   294,   218,   297,   172,   202,   199,   206,
     134,   260,   143,   144,   306,   307,   125,   209,   360,   207,
     203,   233,   234,   365,   138,   238,   205,   211,   133,   317,
     152,   153,   126,   210,   139,   320,   261,   186,   322,   133,
     145,   152,   153,   212,   279,   262,   192,   154,   133,   141,
     283,   355,   142,   193,   235,   282,   155,   328,   240,   215,
     165,   166,   333,   267,   166,   146,   167,   168,   305,   167,
     168,   149,   150,   151,   169,   272,   133,   340,   196,    89,
     342,    89,    89,    89,    89,    89,    89,    89,    89,    89,
      89,    89,    89,    89,    89,   265,  -193,   242,   325,   185,
     185,   185,   185,   185,   185,   185,   185,   303,   304,  -192,
     185,   185,   366,   230,   231,   232,   208,   367,   147,   148,
     264,   236,    10,    11,    12,   152,   153,    15,   241,    17,
      18,   152,   153,   246,   185,   361,   361,   261,   228,   229,
     172,   315,    28,   250,    86,   318,   262,   319,   337,    30,
      31,   253,    87,   284,   254,   256,   255,   257,   172,   193,
     125,   275,   278,   285,    36,   288,   290,   308,    39,   310,
     321,    40,   313,   211,   316,   327,   185,   311,   312,   329,
     334,   335,   336,   356,   350,   358,   364,   363,   172,   268,
     357,    98,   348,   295,    89,   295,    89,    89,    89,    89,
      89,    89,    89,   266,   295,   295,   217,   296,   323,   219,
     298,   220,   299,   221,   300,   222,   301,   223,    95,   302,
      96,     0,    98,   227,   273,   274,   271,   344,   295,   338,
       0,     0,     0,     0,     0,   349,   343,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   353,     0,
       0,     0,    98,     0,     0,     0,   170,    98,    44,     0,
       0,     0,     0,     6,     0,     0,     0,     0,     0,   205,
     295,    10,    11,    12,   368,   369,    15,     0,    17,    18,
       0,    20,     0,    22,     0,     0,     0,     0,    25,    26,
       0,    28,  -187,    86,     0,     0,     0,     0,    30,    31,
       0,    87,     0,     0,     0,  -187,  -187,     0,     0,     0,
      34,    35,     0,    36,     0,    37,    38,    39,     0,     0,
      40,     0,  -187,  -187,     0,     0,     0,     0,     0,     0,
    -187,  -187,  -187,  -187,     0,     0,     0,     0,  -187,  -187,
       0,     0,  -187,  -187,  -187,  -187,     4,     0,  -187,     5,
    -187,     6,     7,     0,     8,  -187,     9,     0,     0,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,     0,     0,    25,    26,    27,    28,
       0,    29,     0,     0,     0,     0,    30,    31,     0,    32,
       0,     0,    33,     0,     0,     0,     0,     0,    34,    35,
       0,    36,     0,    37,    38,    39,     4,     0,    40,     5,
       0,     6,     7,     0,     8,     0,     9,     0,     0,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,     0,     0,    25,    26,     0,    28,
     113,   114,     0,     0,     0,     0,   115,   116,     0,    32,
       0,     0,    33,     0,     0,     0,     0,     0,    34,    35,
       0,    36,     0,    37,    38,    39,     4,     0,    40,     5,
       0,     6,     7,     0,     8,     0,     9,     0,     0,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,     0,     0,    25,    26,     0,    28,
     191,    29,     0,     0,     0,     0,    30,    31,     0,    32,
       0,     0,    33,     0,     0,     0,     0,     0,    34,    35,
       0,    36,     0,    37,    38,    39,     4,     0,    40,     5,
       0,     6,     7,     0,     8,     0,     9,     0,     0,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,     0,     0,    25,    26,     0,    28,
     204,    29,     0,     0,     0,     0,    30,    31,     0,    32,
       0,     0,    33,     0,     0,     0,     0,     0,    34,    35,
       0,    36,     0,    37,    38,    39,     4,     0,    40,     5,
       0,     6,     7,     0,     8,     0,     9,     0,     0,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,     0,     0,    25,    26,     0,    28,
       0,    29,     0,     0,     0,     0,    30,    31,     0,    32,
       0,     0,    33,     0,     0,     0,     0,     0,    34,    35,
       0,    36,     6,    37,    38,    39,     0,     0,    40,     0,
      10,    11,    12,     0,     0,    15,     0,    17,    18,     0,
      20,     0,    22,     0,     0,     0,     0,    25,    26,     0,
      28,     0,    86,     0,     0,     0,     0,    30,    31,     0,
      87,     0,     0,     0,     0,     0,     0,     0,     0,    34,
      35,     0,    36,     6,    37,    38,    39,     0,     0,    40,
     237,    10,    11,    12,     0,     0,    15,     0,    17,    18,
       0,    20,   173,    22,     0,     0,     0,     0,    25,    26,
       0,    28,     0,    86,     0,     0,     0,     0,    30,    31,
       0,    87,     0,     0,     0,     0,     0,     0,     0,     0,
      34,    35,     0,    36,     6,    37,    38,    39,     0,     0,
      40,     0,    10,    11,    12,     0,     0,    15,     0,    17,
      18,     0,    20,     0,    22,     0,     0,     0,     0,    25,
      26,     0,    28,     0,    86,     0,     0,     0,     0,    30,
      31,     0,    87,     0,     0,     0,     0,     0,     0,     0,
       0,    34,    35,     0,    36,     6,    37,    38,    39,   314,
       0,    40,     0,    10,    11,    12,     0,     0,    15,     0,
      17,    18,     0,    20,     0,    22,     0,     0,     0,     0,
      25,    26,     0,    28,     0,    86,     0,     0,     0,     0,
      30,    31,     0,    87,     0,     0,     0,     0,     0,     0,
       0,     0,    34,    35,     0,    36,     0,    37,    38,    39,
       0,     0,    40
};

static const yytype_int16 yycheck[] =
{
       2,    13,    84,    85,    16,     7,   108,     1,     1,     1,
       5,    19,   289,     1,     1,    97,     7,     1,   100,    32,
       0,    47,    62,   105,    64,    65,    32,     9,    40,    15,
      47,   189,    14,    94,    62,    38,    64,    65,    51,     6,
      66,    23,    24,   201,   202,    51,    37,   293,    43,    66,
      77,    50,     1,    20,    47,    22,    42,    49,    25,    26,
       6,    46,    49,    47,   341,    49,    38,    34,    35,    96,
      37,    38,    66,    66,    20,   352,    22,    65,   324,    25,
      26,    93,   143,   144,   145,    53,   129,    99,    34,    35,
     133,    37,    38,   250,   137,   252,   108,   109,    47,    37,
      49,    14,    14,    15,   261,   262,    47,    47,   354,    47,
     112,   154,   155,   359,    31,   158,   118,    47,    47,   277,
      33,    34,    63,    63,    52,   283,    39,    94,   285,    47,
      42,    33,    34,    63,    63,    48,     5,    39,    47,    54,
     242,    50,    40,    12,   156,    63,    48,   305,   160,   131,
      37,    38,   310,   196,    38,    41,    43,    44,   260,    43,
      44,    57,    58,    59,    29,   208,    47,   325,    48,   136,
     327,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     147,   148,   149,   150,   151,   193,    50,   169,   290,   250,
     251,   252,   253,   254,   255,   256,   257,   258,   259,    50,
     261,   262,   360,   149,   150,   151,    50,   365,    55,    56,
     192,    38,    16,    17,    18,    33,    34,    21,    38,    23,
      24,    33,    34,    38,   285,   355,   356,    39,   147,   148,
     242,   274,    36,    47,    38,   278,    48,   280,   320,    43,
      44,    31,    46,   245,    52,    54,    53,    40,   260,    12,
      47,    38,    50,    48,    58,    47,    14,    46,    62,    38,
      10,    65,    50,    47,    47,    50,   327,   269,   270,     4,
     313,    38,    46,    50,   336,    37,    37,   356,   290,   199,
     347,   293,   331,   250,   251,   252,   253,   254,   255,   256,
     257,   258,   259,   194,   261,   262,   136,   251,   288,   138,
     253,   139,   254,   140,   255,   141,   256,   142,    10,   257,
      10,    -1,   324,   146,   209,   209,   207,   329,   285,   321,
      -1,    -1,    -1,    -1,    -1,   333,   328,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   340,    -1,
      -1,    -1,   354,    -1,    -1,    -1,     1,   359,   350,    -1,
      -1,    -1,    -1,     8,    -1,    -1,    -1,    -1,    -1,   361,
     327,    16,    17,    18,   366,   367,    21,    -1,    23,    24,
      -1,    26,    -1,    28,    -1,    -1,    -1,    -1,    33,    34,
      -1,    36,     1,    38,    -1,    -1,    -1,    -1,    43,    44,
      -1,    46,    -1,    -1,    -1,    14,    15,    -1,    -1,    -1,
      55,    56,    -1,    58,    -1,    60,    61,    62,    -1,    -1,
      65,    -1,    31,    32,    -1,    -1,    -1,    -1,    -1,    -1,
      39,    40,    41,    42,    -1,    -1,    -1,    -1,    47,    48,
      -1,    -1,    51,    52,    53,    54,     3,    -1,    57,     6,
      59,     8,     9,    -1,    11,    64,    13,    -1,    -1,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    -1,    -1,    33,    34,    35,    36,
      -1,    38,    -1,    -1,    -1,    -1,    43,    44,    -1,    46,
      -1,    -1,    49,    -1,    -1,    -1,    -1,    -1,    55,    56,
      -1,    58,    -1,    60,    61,    62,     3,    -1,    65,     6,
      -1,     8,     9,    -1,    11,    -1,    13,    -1,    -1,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    -1,    -1,    33,    34,    -1,    36,
      37,    38,    -1,    -1,    -1,    -1,    43,    44,    -1,    46,
      -1,    -1,    49,    -1,    -1,    -1,    -1,    -1,    55,    56,
      -1,    58,    -1,    60,    61,    62,     3,    -1,    65,     6,
      -1,     8,     9,    -1,    11,    -1,    13,    -1,    -1,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    -1,    -1,    33,    34,    -1,    36,
      37,    38,    -1,    -1,    -1,    -1,    43,    44,    -1,    46,
      -1,    -1,    49,    -1,    -1,    -1,    -1,    -1,    55,    56,
      -1,    58,    -1,    60,    61,    62,     3,    -1,    65,     6,
      -1,     8,     9,    -1,    11,    -1,    13,    -1,    -1,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    -1,    -1,    33,    34,    -1,    36,
      37,    38,    -1,    -1,    -1,    -1,    43,    44,    -1,    46,
      -1,    -1,    49,    -1,    -1,    -1,    -1,    -1,    55,    56,
      -1,    58,    -1,    60,    61,    62,     3,    -1,    65,     6,
      -1,     8,     9,    -1,    11,    -1,    13,    -1,    -1,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    -1,    -1,    33,    34,    -1,    36,
      -1,    38,    -1,    -1,    -1,    -1,    43,    44,    -1,    46,
      -1,    -1,    49,    -1,    -1,    -1,    -1,    -1,    55,    56,
      -1,    58,     8,    60,    61,    62,    -1,    -1,    65,    -1,
      16,    17,    18,    -1,    -1,    21,    -1,    23,    24,    -1,
      26,    -1,    28,    -1,    -1,    -1,    -1,    33,    34,    -1,
      36,    -1,    38,    -1,    -1,    -1,    -1,    43,    44,    -1,
      46,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    55,
      56,    -1,    58,     8,    60,    61,    62,    -1,    -1,    65,
      66,    16,    17,    18,    -1,    -1,    21,    -1,    23,    24,
      -1,    26,    27,    28,    -1,    -1,    -1,    -1,    33,    34,
      -1,    36,    -1,    38,    -1,    -1,    -1,    -1,    43,    44,
      -1,    46,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      55,    56,    -1,    58,     8,    60,    61,    62,    -1,    -1,
      65,    -1,    16,    17,    18,    -1,    -1,    21,    -1,    23,
      24,    -1,    26,    -1,    28,    -1,    -1,    -1,    -1,    33,
      34,    -1,    36,    -1,    38,    -1,    -1,    -1,    -1,    43,
      44,    -1,    46,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    55,    56,    -1,    58,     8,    60,    61,    62,    63,
      -1,    65,    -1,    16,    17,    18,    -1,    -1,    21,    -1,
      23,    24,    -1,    26,    -1,    28,    -1,    -1,    -1,    -1,
      33,    34,    -1,    36,    -1,    38,    -1,    -1,    -1,    -1,
      43,    44,    -1,    46,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    55,    56,    -1,    58,    -1,    60,    61,    62,
      -1,    -1,    65
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    68,    70,     0,     3,     6,     8,     9,    11,    13,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    33,    34,    35,    36,    38,
      43,    44,    46,    49,    55,    56,    58,    60,    61,    62,
      65,    69,    71,    72,    76,    79,    80,    89,    90,    91,
      92,    97,    98,    99,   100,   101,   102,   108,   109,   114,
     117,   119,   121,   123,   125,   127,   129,   131,   133,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   146,   147,
     151,   155,   156,    38,   154,   154,    38,    46,   138,   140,
      76,     1,    65,   158,   158,   141,   142,   112,   114,   158,
     114,    46,    79,   138,    38,    81,    83,   138,   158,   158,
     138,   138,    50,    37,    38,    43,    44,    76,    77,   152,
     153,   138,   138,   138,   138,    47,    63,   148,   149,   150,
     114,   154,     1,    47,    49,   157,    32,    51,    31,    52,
      53,    54,    40,    14,    15,    42,    41,    55,    56,    57,
      58,    59,    33,    34,    39,    48,    62,    64,    65,   144,
      62,    64,   144,   157,   157,    37,    38,    43,    44,    29,
       1,   113,   114,    27,   115,   116,   118,   120,   122,   124,
     126,   128,   130,   132,   134,   135,   140,   144,   157,   114,
     157,    37,     5,    12,   110,   111,    48,    85,    86,    47,
     157,   113,   114,    76,    37,    76,    37,    47,    50,    47,
      63,    47,    63,   117,    66,   158,   117,   123,   117,   125,
     127,   129,   131,   133,   135,   135,   135,   136,   137,   137,
     138,   138,   138,   117,   117,   114,    38,    66,   117,   145,
     114,    38,   158,     1,    66,   159,    38,    82,    84,    93,
      47,    32,    51,    31,    52,    53,    54,    40,    15,    42,
      14,    39,    48,   159,   158,    79,   111,   117,    83,   159,
     159,   153,   117,   149,   150,    38,    74,    75,    50,    63,
      47,    66,    63,   113,    76,    48,    87,    88,    47,    95,
      14,     1,    49,   160,   118,   140,   124,   118,   126,   128,
     130,   132,   134,   135,   135,   113,   118,   118,    46,   103,
      38,    76,    76,    50,    63,   117,    47,   159,   117,   117,
     159,    10,   118,    84,   160,   113,   112,    50,   159,     4,
     104,   105,   106,   159,   117,    38,    46,   157,    76,   112,
     159,    94,   118,    76,   114,     7,    37,   107,   106,    79,
      70,    73,    96,    76,   160,    50,    50,   104,    37,   160,
     112,    77,    78,    78,    37,   112,   159,   159,    76,    76
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
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval)
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
/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

    /* Number of syntax errors so far.  */
    int yynerrs;

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
#line 257 "parser.y"
    { program_parsed(ctx, (yyvsp[(1) - (2)].source_elements)); ;}
    break;

  case 3:

/* Line 1455 of yacc.c  */
#line 260 "parser.y"
    {;}
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 261 "parser.y"
    {;}
    break;

  case 5:

/* Line 1455 of yacc.c  */
#line 265 "parser.y"
    { (yyval.source_elements) = new_source_elements(ctx); ;}
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 267 "parser.y"
    { (yyval.source_elements) = source_elements_add_statement((yyvsp[(1) - (2)].source_elements), (yyvsp[(2) - (2)].statement)); ;}
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 272 "parser.y"
    { (yyval.expr) = new_function_expression(ctx, (yyvsp[(2) - (8)].identifier), (yyvsp[(4) - (8)].parameter_list), (yyvsp[(7) - (8)].source_elements), (yyvsp[(1) - (8)].srcptr), (yyvsp[(8) - (8)].srcptr)-(yyvsp[(1) - (8)].srcptr)+1); ;}
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 275 "parser.y"
    { push_func(ctx); (yyval.srcptr) = (yyvsp[(1) - (1)].srcptr); ;}
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 279 "parser.y"
    { (yyval.source_elements) = function_body_parsed(ctx, (yyvsp[(1) - (1)].source_elements)); ;}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 283 "parser.y"
    { (yyval.parameter_list) = new_parameter_list(ctx, (yyvsp[(1) - (1)].identifier)); ;}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 285 "parser.y"
    { (yyval.parameter_list) = parameter_list_add(ctx, (yyvsp[(1) - (3)].parameter_list), (yyvsp[(3) - (3)].identifier)); ;}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 289 "parser.y"
    { (yyval.parameter_list) = NULL; ;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 290 "parser.y"
    { (yyval.parameter_list) = (yyvsp[(1) - (1)].parameter_list); ;}
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 294 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 295 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 296 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 297 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 298 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 299 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 300 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 301 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 302 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 303 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 304 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 305 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 306 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 307 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 311 "parser.y"
    { (yyval.statement_list) = new_statement_list(ctx, (yyvsp[(1) - (1)].statement)); ;}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 313 "parser.y"
    { (yyval.statement_list) = statement_list_add((yyvsp[(1) - (2)].statement_list), (yyvsp[(2) - (2)].statement)); ;}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 317 "parser.y"
    { (yyval.statement_list) = NULL; ;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 318 "parser.y"
    { (yyval.statement_list) = (yyvsp[(1) - (1)].statement_list); ;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 322 "parser.y"
    { (yyval.statement) = new_block_statement(ctx, (yyvsp[(2) - (3)].statement_list)); ;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 323 "parser.y"
    { (yyval.statement) = new_block_statement(ctx, NULL); ;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 328 "parser.y"
    { (yyval.statement) = new_var_statement(ctx, (yyvsp[(2) - (3)].variable_list)); ;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 332 "parser.y"
    { (yyval.variable_list) = new_variable_list(ctx, (yyvsp[(1) - (1)].variable_declaration)); ;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 334 "parser.y"
    { (yyval.variable_list) = variable_list_add(ctx, (yyvsp[(1) - (3)].variable_list), (yyvsp[(3) - (3)].variable_declaration)); ;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 339 "parser.y"
    { (yyval.variable_list) = new_variable_list(ctx, (yyvsp[(1) - (1)].variable_declaration)); ;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 341 "parser.y"
    { (yyval.variable_list) = variable_list_add(ctx, (yyvsp[(1) - (3)].variable_list), (yyvsp[(3) - (3)].variable_declaration)); ;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 346 "parser.y"
    { (yyval.variable_declaration) = new_variable_declaration(ctx, (yyvsp[(1) - (2)].identifier), (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 351 "parser.y"
    { (yyval.variable_declaration) = new_variable_declaration(ctx, (yyvsp[(1) - (2)].identifier), (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 355 "parser.y"
    { (yyval.expr) = NULL; ;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 356 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 361 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (2)].expr); ;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 365 "parser.y"
    { (yyval.expr) = NULL; ;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 366 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 371 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (2)].expr); ;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 375 "parser.y"
    { (yyval.statement) = new_empty_statement(ctx); ;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 380 "parser.y"
    { (yyval.statement) = new_expression_statement(ctx, (yyvsp[(1) - (2)].expr)); ;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 385 "parser.y"
    { (yyval.statement) = new_if_statement(ctx, (yyvsp[(3) - (7)].expr), (yyvsp[(5) - (7)].statement), (yyvsp[(7) - (7)].statement)); ;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 387 "parser.y"
    { (yyval.statement) = new_if_statement(ctx, (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].statement), NULL); ;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 392 "parser.y"
    { (yyval.statement) = new_while_statement(ctx, TRUE, (yyvsp[(5) - (7)].expr), (yyvsp[(2) - (7)].statement)); ;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 394 "parser.y"
    { (yyval.statement) = new_while_statement(ctx, FALSE, (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].statement)); ;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 396 "parser.y"
    { if(!explicit_error(ctx, (yyvsp[(3) - (3)].expr), ';')) YYABORT; ;}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 398 "parser.y"
    { if(!explicit_error(ctx, (yyvsp[(6) - (6)].expr), ';')) YYABORT; ;}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 400 "parser.y"
    { (yyval.statement) = new_for_statement(ctx, NULL, (yyvsp[(3) - (11)].expr), (yyvsp[(6) - (11)].expr), (yyvsp[(9) - (11)].expr), (yyvsp[(11) - (11)].statement)); ;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 402 "parser.y"
    { if(!explicit_error(ctx, (yyvsp[(4) - (4)].variable_list), ';')) YYABORT; ;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 404 "parser.y"
    { if(!explicit_error(ctx, (yyvsp[(7) - (7)].expr), ';')) YYABORT; ;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 406 "parser.y"
    { (yyval.statement) = new_for_statement(ctx, (yyvsp[(4) - (12)].variable_list), NULL, (yyvsp[(7) - (12)].expr), (yyvsp[(10) - (12)].expr), (yyvsp[(12) - (12)].statement)); ;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 408 "parser.y"
    { (yyval.statement) = new_forin_statement(ctx, NULL, (yyvsp[(3) - (7)].expr), (yyvsp[(5) - (7)].expr), (yyvsp[(7) - (7)].statement)); ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 410 "parser.y"
    { (yyval.statement) = new_forin_statement(ctx, (yyvsp[(4) - (8)].variable_declaration), NULL, (yyvsp[(6) - (8)].expr), (yyvsp[(8) - (8)].statement)); ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 415 "parser.y"
    { (yyval.statement) = new_continue_statement(ctx, (yyvsp[(2) - (3)].identifier)); ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 420 "parser.y"
    { (yyval.statement) = new_break_statement(ctx, (yyvsp[(2) - (3)].identifier)); ;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 425 "parser.y"
    { (yyval.statement) = new_return_statement(ctx, (yyvsp[(2) - (3)].expr)); ;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 430 "parser.y"
    { (yyval.statement) = new_with_statement(ctx, (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].statement)); ;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 435 "parser.y"
    { (yyval.statement) = new_labelled_statement(ctx, (yyvsp[(1) - (3)].identifier), (yyvsp[(3) - (3)].statement)); ;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 440 "parser.y"
    { (yyval.statement) = new_switch_statement(ctx, (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].case_clausule)); ;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 445 "parser.y"
    { (yyval.case_clausule) = new_case_block(ctx, (yyvsp[(2) - (3)].case_list), NULL, NULL); ;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 447 "parser.y"
    { (yyval.case_clausule) = new_case_block(ctx, (yyvsp[(2) - (5)].case_list), (yyvsp[(3) - (5)].case_clausule), (yyvsp[(4) - (5)].case_list)); ;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 451 "parser.y"
    { (yyval.case_list) = NULL; ;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 452 "parser.y"
    { (yyval.case_list) = (yyvsp[(1) - (1)].case_list); ;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 456 "parser.y"
    { (yyval.case_list) = new_case_list(ctx, (yyvsp[(1) - (1)].case_clausule)); ;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 458 "parser.y"
    { (yyval.case_list) = case_list_add(ctx, (yyvsp[(1) - (2)].case_list), (yyvsp[(2) - (2)].case_clausule)); ;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 463 "parser.y"
    { (yyval.case_clausule) = new_case_clausule(ctx, (yyvsp[(2) - (4)].expr), (yyvsp[(4) - (4)].statement_list)); ;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 468 "parser.y"
    { (yyval.case_clausule) = new_case_clausule(ctx, NULL, (yyvsp[(3) - (3)].statement_list)); ;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 473 "parser.y"
    { (yyval.statement) = new_throw_statement(ctx, (yyvsp[(2) - (3)].expr)); ;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 477 "parser.y"
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[(2) - (3)].statement), (yyvsp[(3) - (3)].catch_block), NULL); ;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 478 "parser.y"
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[(2) - (3)].statement), NULL, (yyvsp[(3) - (3)].statement)); ;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 480 "parser.y"
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[(2) - (4)].statement), (yyvsp[(3) - (4)].catch_block), (yyvsp[(4) - (4)].statement)); ;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 485 "parser.y"
    { (yyval.catch_block) = new_catch_block(ctx, (yyvsp[(3) - (5)].identifier), (yyvsp[(5) - (5)].statement)); ;}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 489 "parser.y"
    { (yyval.statement) = (yyvsp[(2) - (2)].statement); ;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 493 "parser.y"
    { (yyval.expr) = NULL; ;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 494 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 497 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 498 "parser.y"
    { set_error(ctx, IDS_SYNTAX_ERROR); YYABORT; ;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 502 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 504 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_COMMA, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 508 "parser.y"
    { (yyval.expr) = NULL; ;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 509 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 514 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 516 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_COMMA, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 520 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 522 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ASSIGN, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 524 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 529 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 531 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ASSIGN, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 533 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 537 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 539 "parser.y"
    { (yyval.expr) = new_conditional_expression(ctx, (yyvsp[(1) - (5)].expr), (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].expr)); ;}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 544 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 546 "parser.y"
    { (yyval.expr) = new_conditional_expression(ctx, (yyvsp[(1) - (5)].expr), (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].expr)); ;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 550 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 552 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_OR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 557 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 559 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_OR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 563 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 565 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_AND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 570 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 572 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_AND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 576 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 578 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BOR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 583 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 585 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BOR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 589 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 591 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BXOR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 596 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 598 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BXOR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 602 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 604 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BAND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 609 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 611 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BAND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 615 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 617 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 621 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 623 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 627 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 629 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 631 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_INSTANCEOF, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 633 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_IN, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 637 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 639 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 641 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_INSTANCEOF, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 645 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 647 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 652 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 654 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ADD, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 656 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_SUB, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 660 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 662 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_MUL, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 664 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_DIV, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 666 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_MOD, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 670 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 672 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_DELETE, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 673 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_VOID, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 675 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_TYPEOF, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 676 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PREINC, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 677 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PREDEC, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 678 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PLUS, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 679 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_MINUS, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 680 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_BITNEG, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 681 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_LOGNEG, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 686 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 688 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_POSTINC, (yyvsp[(1) - (2)].expr)); ;}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 690 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_POSTDEC, (yyvsp[(1) - (2)].expr)); ;}
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 695 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 696 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 700 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 701 "parser.y"
    { (yyval.expr) = new_new_expression(ctx, (yyvsp[(2) - (2)].expr), NULL); ;}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 705 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 706 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 708 "parser.y"
    { (yyval.expr) = new_array_expression(ctx, (yyvsp[(1) - (4)].expr), (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 710 "parser.y"
    { (yyval.expr) = new_member_expression(ctx, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].identifier)); ;}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 712 "parser.y"
    { (yyval.expr) = new_new_expression(ctx, (yyvsp[(2) - (3)].expr), (yyvsp[(3) - (3)].argument_list)); ;}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 717 "parser.y"
    { (yyval.expr) = new_call_expression(ctx, (yyvsp[(1) - (2)].expr), (yyvsp[(2) - (2)].argument_list)); ;}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 719 "parser.y"
    { (yyval.expr) = new_call_expression(ctx, (yyvsp[(1) - (2)].expr), (yyvsp[(2) - (2)].argument_list)); ;}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 721 "parser.y"
    { (yyval.expr) = new_array_expression(ctx, (yyvsp[(1) - (4)].expr), (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 723 "parser.y"
    { (yyval.expr) = new_member_expression(ctx, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].identifier)); ;}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 727 "parser.y"
    { (yyval.argument_list) = NULL; ;}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 728 "parser.y"
    { (yyval.argument_list) = (yyvsp[(2) - (3)].argument_list); ;}
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 732 "parser.y"
    { (yyval.argument_list) = new_argument_list(ctx, (yyvsp[(1) - (1)].expr)); ;}
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 734 "parser.y"
    { (yyval.argument_list) = argument_list_add(ctx, (yyvsp[(1) - (3)].argument_list), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 738 "parser.y"
    { (yyval.expr) = new_this_expression(ctx); ;}
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 739 "parser.y"
    { (yyval.expr) = new_identifier_expression(ctx, (yyvsp[(1) - (1)].identifier)); ;}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 740 "parser.y"
    { (yyval.expr) = new_literal_expression(ctx, (yyvsp[(1) - (1)].literal)); ;}
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 741 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 742 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 743 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (3)].expr); ;}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 747 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, NULL, 0); ;}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 748 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, NULL, (yyvsp[(2) - (3)].ival)+1); ;}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 749 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, (yyvsp[(2) - (3)].element_list), 0); ;}
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 751 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, (yyvsp[(2) - (5)].element_list), (yyvsp[(4) - (5)].ival)+1); ;}
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 756 "parser.y"
    { (yyval.element_list) = new_element_list(ctx, (yyvsp[(1) - (2)].ival), (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 758 "parser.y"
    { (yyval.element_list) = element_list_add(ctx, (yyvsp[(1) - (4)].element_list), (yyvsp[(3) - (4)].ival), (yyvsp[(4) - (4)].expr)); ;}
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 762 "parser.y"
    { (yyval.ival) = 1; ;}
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 763 "parser.y"
    { (yyval.ival) = (yyvsp[(1) - (2)].ival) + 1; ;}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 767 "parser.y"
    { (yyval.ival) = 0; ;}
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 768 "parser.y"
    { (yyval.ival) = (yyvsp[(1) - (1)].ival); ;}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 772 "parser.y"
    { (yyval.expr) = new_prop_and_value_expression(ctx, NULL); ;}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 774 "parser.y"
    { (yyval.expr) = new_prop_and_value_expression(ctx, (yyvsp[(2) - (3)].property_list)); ;}
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 779 "parser.y"
    { (yyval.property_list) = new_property_list(ctx, (yyvsp[(1) - (3)].literal), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 781 "parser.y"
    { (yyval.property_list) = property_list_add(ctx, (yyvsp[(1) - (5)].property_list), (yyvsp[(3) - (5)].literal), (yyvsp[(5) - (5)].expr)); ;}
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 785 "parser.y"
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[(1) - (1)].identifier)); ;}
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 786 "parser.y"
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[(1) - (1)].wstr)); ;}
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 787 "parser.y"
    { (yyval.literal) = (yyvsp[(1) - (1)].literal); ;}
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 791 "parser.y"
    { (yyval.identifier) = NULL; ;}
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 792 "parser.y"
    { (yyval.identifier) = (yyvsp[(1) - (1)].identifier); ;}
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 796 "parser.y"
    { (yyval.literal) = new_null_literal(ctx); ;}
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 797 "parser.y"
    { (yyval.literal) = new_undefined_literal(ctx); ;}
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 798 "parser.y"
    { (yyval.literal) = (yyvsp[(1) - (1)].literal); ;}
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 799 "parser.y"
    { (yyval.literal) = (yyvsp[(1) - (1)].literal); ;}
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 800 "parser.y"
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[(1) - (1)].wstr)); ;}
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 801 "parser.y"
    { (yyval.literal) = parse_regexp(ctx);
                                  if(!(yyval.literal)) YYABORT; ;}
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 806 "parser.y"
    { (yyval.literal) = new_boolean_literal(ctx, TRUE); ;}
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 807 "parser.y"
    { (yyval.literal) = new_boolean_literal(ctx, FALSE); ;}
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 811 "parser.y"
    { if(!allow_auto_semicolon(ctx)) {YYABORT;} ;}
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 815 "parser.y"
    { set_error(ctx, IDS_LBRACKET); YYABORT; ;}
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 819 "parser.y"
    { set_error(ctx, IDS_RBRACKET); YYABORT; ;}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 823 "parser.y"
    { set_error(ctx, IDS_SEMICOLON); YYABORT; ;}
    break;



/* Line 1455 of yacc.c  */
#line 3455 "parser.tab.c"
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
#line 825 "parser.y"


static BOOL allow_auto_semicolon(parser_ctx_t *ctx)
{
    return ctx->nl || ctx->ptr == ctx->end || *(ctx->ptr-1) == '}';
}

static literal_t *new_string_literal(parser_ctx_t *ctx, const WCHAR *str)
{
    literal_t *ret = parser_alloc(ctx, sizeof(literal_t));

    ret->vt = VT_BSTR;
    ret->u.wstr = str;

    return ret;
}

static literal_t *new_null_literal(parser_ctx_t *ctx)
{
    literal_t *ret = parser_alloc(ctx, sizeof(literal_t));

    ret->vt = VT_NULL;

    return ret;
}

static literal_t *new_undefined_literal(parser_ctx_t *ctx)
{
    literal_t *ret = parser_alloc(ctx, sizeof(literal_t));

    ret->vt = VT_EMPTY;

    return ret;
}

static literal_t *new_boolean_literal(parser_ctx_t *ctx, VARIANT_BOOL bval)
{
    literal_t *ret = parser_alloc(ctx, sizeof(literal_t));

    ret->vt = VT_BOOL;
    ret->u.bval = bval;

    return ret;
}

static prop_val_t *new_prop_val(parser_ctx_t *ctx, literal_t *name, expression_t *value)
{
    prop_val_t *ret = parser_alloc(ctx, sizeof(prop_val_t));

    ret->name = name;
    ret->value = value;
    ret->next = NULL;

    return ret;
}

static property_list_t *new_property_list(parser_ctx_t *ctx, literal_t *name, expression_t *value)
{
    property_list_t *ret = parser_alloc_tmp(ctx, sizeof(property_list_t));

    ret->head = ret->tail = new_prop_val(ctx, name, value);

    return ret;
}

static property_list_t *property_list_add(parser_ctx_t *ctx, property_list_t *list, literal_t *name, expression_t *value)
{
    list->tail = list->tail->next = new_prop_val(ctx, name, value);

    return list;
}

static array_element_t *new_array_element(parser_ctx_t *ctx, int elision, expression_t *expr)
{
    array_element_t *ret = parser_alloc(ctx, sizeof(array_element_t));

    ret->elision = elision;
    ret->expr = expr;
    ret->next = NULL;

    return ret;
}

static element_list_t *new_element_list(parser_ctx_t *ctx, int elision, expression_t *expr)
{
    element_list_t *ret = parser_alloc_tmp(ctx, sizeof(element_list_t));

    ret->head = ret->tail = new_array_element(ctx, elision, expr);

    return ret;
}

static element_list_t *element_list_add(parser_ctx_t *ctx, element_list_t *list, int elision, expression_t *expr)
{
    list->tail = list->tail->next = new_array_element(ctx, elision, expr);

    return list;
}

static argument_t *new_argument(parser_ctx_t *ctx, expression_t *expr)
{
    argument_t *ret = parser_alloc(ctx, sizeof(argument_t));

    ret->expr = expr;
    ret->next = NULL;

    return ret;
}

static argument_list_t *new_argument_list(parser_ctx_t *ctx, expression_t *expr)
{
    argument_list_t *ret = parser_alloc_tmp(ctx, sizeof(argument_list_t));

    ret->head = ret->tail = new_argument(ctx, expr);

    return ret;
}

static argument_list_t *argument_list_add(parser_ctx_t *ctx, argument_list_t *list, expression_t *expr)
{
    list->tail = list->tail->next = new_argument(ctx, expr);

    return list;
}

static catch_block_t *new_catch_block(parser_ctx_t *ctx, const WCHAR *identifier, statement_t *statement)
{
    catch_block_t *ret = parser_alloc(ctx, sizeof(catch_block_t));

    ret->identifier = identifier;
    ret->statement = statement;

    return ret;
}

static case_clausule_t *new_case_clausule(parser_ctx_t *ctx, expression_t *expr, statement_list_t *stat_list)
{
    case_clausule_t *ret = parser_alloc(ctx, sizeof(case_clausule_t));

    ret->expr = expr;
    ret->stat = stat_list ? stat_list->head : NULL;
    ret->next = NULL;

    return ret;
}

static case_list_t *new_case_list(parser_ctx_t *ctx, case_clausule_t *case_clausule)
{
    case_list_t *ret = parser_alloc_tmp(ctx, sizeof(case_list_t));

    ret->head = ret->tail = case_clausule;

    return ret;
}

static case_list_t *case_list_add(parser_ctx_t *ctx, case_list_t *list, case_clausule_t *case_clausule)
{
    list->tail = list->tail->next = case_clausule;

    return list;
}

static case_clausule_t *new_case_block(parser_ctx_t *ctx, case_list_t *case_list1,
        case_clausule_t *default_clausule, case_list_t *case_list2)
{
    case_clausule_t *ret = NULL, *iter = NULL, *iter2;
    statement_t *stat = NULL;

    if(case_list1) {
        ret = case_list1->head;
        iter = case_list1->tail;
    }

    if(default_clausule) {
        if(ret)
            iter = iter->next = default_clausule;
        else
            ret = iter = default_clausule;
    }

    if(case_list2) {
        if(ret)
            iter->next = case_list2->head;
        else
            ret = case_list2->head;
    }

    if(!ret)
        return NULL;

    for(iter = ret; iter; iter = iter->next) {
        for(iter2 = iter; iter2 && !iter2->stat; iter2 = iter2->next);
        if(!iter2)
            break;

        while(iter != iter2) {
            iter->stat = iter2->stat;
            iter = iter->next;
        }

        if(stat) {
            while(stat->next)
                stat = stat->next;
            stat->next = iter->stat;
        }else {
            stat = iter->stat;
        }
    }

    return ret;
}

static statement_t *new_block_statement(parser_ctx_t *ctx, statement_list_t *list)
{
    block_statement_t *ret = parser_alloc(ctx, sizeof(block_statement_t));

    ret->stat.eval = block_statement_eval;
    ret->stat.next = NULL;
    ret->stat_list = list ? list->head : NULL;

    return &ret->stat;
}

static variable_declaration_t *new_variable_declaration(parser_ctx_t *ctx, const WCHAR *identifier, expression_t *expr)
{
    variable_declaration_t *ret = parser_alloc(ctx, sizeof(variable_declaration_t));
    var_list_t *var_list = parser_alloc(ctx, sizeof(var_list_t));

    ret->identifier = identifier;
    ret->expr = expr;
    ret->next = NULL;

    var_list->identifier = identifier;
    var_list->next = NULL;

    if(ctx->func_stack->var_tail)
        ctx->func_stack->var_tail = ctx->func_stack->var_tail->next = var_list;
    else
        ctx->func_stack->var_head = ctx->func_stack->var_tail = var_list;

    return ret;
}

static variable_list_t *new_variable_list(parser_ctx_t *ctx, variable_declaration_t *decl)
{
    variable_list_t *ret = parser_alloc_tmp(ctx, sizeof(variable_list_t));

    ret->head = ret->tail = decl;

    return ret;
}

static variable_list_t *variable_list_add(parser_ctx_t *ctx, variable_list_t *list, variable_declaration_t *decl)
{
    list->tail = list->tail->next = decl;

    return list;
}

static statement_t *new_var_statement(parser_ctx_t *ctx, variable_list_t *variable_list)
{
    var_statement_t *ret = parser_alloc(ctx, sizeof(var_statement_t));

    ret->stat.eval = var_statement_eval;
    ret->stat.next = NULL;
    ret->variable_list = variable_list->head;

    return &ret->stat;
}

static statement_t *new_empty_statement(parser_ctx_t *ctx)
{
    statement_t *ret = parser_alloc(ctx, sizeof(statement_t));

    ret->eval = empty_statement_eval;
    ret->next = NULL;

    return ret;
}

static statement_t *new_expression_statement(parser_ctx_t *ctx, expression_t *expr)
{
    expression_statement_t *ret = parser_alloc(ctx, sizeof(expression_statement_t));

    ret->stat.eval = expression_statement_eval;
    ret->stat.next = NULL;
    ret->expr = expr;

    return &ret->stat;
}

static statement_t *new_if_statement(parser_ctx_t *ctx, expression_t *expr, statement_t *if_stat, statement_t *else_stat)
{
    if_statement_t *ret = parser_alloc(ctx, sizeof(if_statement_t));

    ret->stat.eval = if_statement_eval;
    ret->stat.next = NULL;
    ret->expr = expr;
    ret->if_stat = if_stat;
    ret->else_stat = else_stat;

    return &ret->stat;
}

static statement_t *new_while_statement(parser_ctx_t *ctx, BOOL dowhile, expression_t *expr, statement_t *stat)
{
    while_statement_t *ret = parser_alloc(ctx, sizeof(while_statement_t));

    ret->stat.eval = while_statement_eval;
    ret->stat.next = NULL;
    ret->do_while = dowhile;
    ret->expr = expr;
    ret->statement = stat;

    return &ret->stat;
}

static statement_t *new_for_statement(parser_ctx_t *ctx, variable_list_t *variable_list, expression_t *begin_expr,
        expression_t *expr, expression_t *end_expr, statement_t *statement)
{
    for_statement_t *ret = parser_alloc(ctx, sizeof(for_statement_t));

    ret->stat.eval = for_statement_eval;
    ret->stat.next = NULL;
    ret->variable_list = variable_list ? variable_list->head : NULL;
    ret->begin_expr = begin_expr;
    ret->expr = expr;
    ret->end_expr = end_expr;
    ret->statement = statement;

    return &ret->stat;
}

static statement_t *new_forin_statement(parser_ctx_t *ctx, variable_declaration_t *variable, expression_t *expr,
        expression_t *in_expr, statement_t *statement)
{
    forin_statement_t *ret = parser_alloc(ctx, sizeof(forin_statement_t));

    ret->stat.eval = forin_statement_eval;
    ret->stat.next = NULL;
    ret->variable = variable;
    ret->expr = expr;
    ret->in_expr = in_expr;
    ret->statement = statement;

    return &ret->stat;
}

static statement_t *new_continue_statement(parser_ctx_t *ctx, const WCHAR *identifier)
{
    branch_statement_t *ret = parser_alloc(ctx, sizeof(branch_statement_t));

    ret->stat.eval = continue_statement_eval;
    ret->stat.next = NULL;
    ret->identifier = identifier;

    return &ret->stat;
}

static statement_t *new_break_statement(parser_ctx_t *ctx, const WCHAR *identifier)
{
    branch_statement_t *ret = parser_alloc(ctx, sizeof(branch_statement_t));

    ret->stat.eval = break_statement_eval;
    ret->stat.next = NULL;
    ret->identifier = identifier;

    return &ret->stat;
}

static statement_t *new_return_statement(parser_ctx_t *ctx, expression_t *expr)
{
    expression_statement_t *ret = parser_alloc(ctx, sizeof(expression_statement_t));

    ret->stat.eval = return_statement_eval;
    ret->stat.next = NULL;
    ret->expr = expr;

    return &ret->stat;
}

static statement_t *new_with_statement(parser_ctx_t *ctx, expression_t *expr, statement_t *statement)
{
    with_statement_t *ret = parser_alloc(ctx, sizeof(with_statement_t));

    ret->stat.eval = with_statement_eval;
    ret->stat.next = NULL;
    ret->expr = expr;
    ret->statement = statement;

    return &ret->stat;
}

static statement_t *new_labelled_statement(parser_ctx_t *ctx, const WCHAR *identifier, statement_t *statement)
{
    labelled_statement_t *ret = parser_alloc(ctx, sizeof(labelled_statement_t));

    ret->stat.eval = labelled_statement_eval;
    ret->stat.next = NULL;
    ret->identifier = identifier;
    ret->statement = statement;

    return &ret->stat;
}

static statement_t *new_switch_statement(parser_ctx_t *ctx, expression_t *expr, case_clausule_t *case_list)
{
    switch_statement_t *ret = parser_alloc(ctx, sizeof(switch_statement_t));

    ret->stat.eval = switch_statement_eval;
    ret->stat.next = NULL;
    ret->expr = expr;
    ret->case_list = case_list;

    return &ret->stat;
}

static statement_t *new_throw_statement(parser_ctx_t *ctx, expression_t *expr)
{
    expression_statement_t *ret = parser_alloc(ctx, sizeof(expression_statement_t));

    ret->stat.eval = throw_statement_eval;
    ret->stat.next = NULL;
    ret->expr = expr;

    return &ret->stat;
}

static statement_t *new_try_statement(parser_ctx_t *ctx, statement_t *try_statement,
       catch_block_t *catch_block, statement_t *finally_statement)
{
    try_statement_t *ret = parser_alloc(ctx, sizeof(try_statement_t));

    ret->stat.eval = try_statement_eval;
    ret->stat.next = NULL;
    ret->try_statement = try_statement;
    ret->catch_block = catch_block;
    ret->finally_statement = finally_statement;

    return &ret->stat;
}

static parameter_t *new_parameter(parser_ctx_t *ctx, const WCHAR *identifier)
{
    parameter_t *ret = parser_alloc(ctx, sizeof(parameter_t));

    ret->identifier = identifier;
    ret->next = NULL;

    return ret;
}

static parameter_list_t *new_parameter_list(parser_ctx_t *ctx, const WCHAR *identifier)
{
    parameter_list_t *ret = parser_alloc_tmp(ctx, sizeof(parameter_list_t));

    ret->head = ret->tail = new_parameter(ctx, identifier);

    return ret;
}

static parameter_list_t *parameter_list_add(parser_ctx_t *ctx, parameter_list_t *list, const WCHAR *identifier)
{
    list->tail = list->tail->next = new_parameter(ctx, identifier);

    return list;
}

static expression_t *new_function_expression(parser_ctx_t *ctx, const WCHAR *identifier,
       parameter_list_t *parameter_list, source_elements_t *source_elements, const WCHAR *src_str, DWORD src_len)
{
    function_expression_t *ret = parser_alloc(ctx, sizeof(function_expression_t));

    ret->expr.eval = function_expression_eval;
    ret->identifier = identifier;
    ret->parameter_list = parameter_list ? parameter_list->head : NULL;
    ret->source_elements = source_elements;
    ret->src_str = src_str;
    ret->src_len = src_len;

    if(ret->identifier) {
        function_declaration_t *decl = parser_alloc(ctx, sizeof(function_declaration_t));

        decl->expr = ret;
        decl->next = NULL;

        if(ctx->func_stack->func_tail)
            ctx->func_stack->func_tail = ctx->func_stack->func_tail->next = decl;
        else
            ctx->func_stack->func_head = ctx->func_stack->func_tail = decl;
    }

    return &ret->expr;
}

static const expression_eval_t expression_eval_table[] = {
   comma_expression_eval,
   logical_or_expression_eval,
   logical_and_expression_eval,
   binary_or_expression_eval,
   binary_xor_expression_eval,
   binary_and_expression_eval,
   instanceof_expression_eval,
   in_expression_eval,
   add_expression_eval,
   sub_expression_eval,
   mul_expression_eval,
   div_expression_eval,
   mod_expression_eval,
   delete_expression_eval,
   void_expression_eval,
   typeof_expression_eval,
   minus_expression_eval,
   plus_expression_eval,
   post_increment_expression_eval,
   post_decrement_expression_eval,
   pre_increment_expression_eval,
   pre_decrement_expression_eval,
   equal_expression_eval,
   equal2_expression_eval,
   not_equal_expression_eval,
   not_equal2_expression_eval,
   less_expression_eval,
   lesseq_expression_eval,
   greater_expression_eval,
   greatereq_expression_eval,
   binary_negation_expression_eval,
   logical_negation_expression_eval,
   left_shift_expression_eval,
   right_shift_expression_eval,
   right2_shift_expression_eval,
   assign_expression_eval,
   assign_lshift_expression_eval,
   assign_rshift_expression_eval,
   assign_rrshift_expression_eval,
   assign_add_expression_eval,
   assign_sub_expression_eval,
   assign_mul_expression_eval,
   assign_div_expression_eval,
   assign_mod_expression_eval,
   assign_and_expression_eval,
   assign_or_expression_eval,
   assign_xor_expression_eval,
};

static expression_t *new_binary_expression(parser_ctx_t *ctx, expression_type_t type,
       expression_t *expression1, expression_t *expression2)
{
    binary_expression_t *ret = parser_alloc(ctx, sizeof(binary_expression_t));

    ret->expr.eval = expression_eval_table[type];
    ret->expression1 = expression1;
    ret->expression2 = expression2;

    return &ret->expr;
}

static expression_t *new_unary_expression(parser_ctx_t *ctx, expression_type_t type, expression_t *expression)
{
    unary_expression_t *ret = parser_alloc(ctx, sizeof(unary_expression_t));

    ret->expr.eval = expression_eval_table[type];
    ret->expression = expression;

    return &ret->expr;
}

static expression_t *new_conditional_expression(parser_ctx_t *ctx, expression_t *expression,
       expression_t *true_expression, expression_t *false_expression)
{
    conditional_expression_t *ret = parser_alloc(ctx, sizeof(conditional_expression_t));

    ret->expr.eval = conditional_expression_eval;
    ret->expression = expression;
    ret->true_expression = true_expression;
    ret->false_expression = false_expression;

    return &ret->expr;
}

static expression_t *new_array_expression(parser_ctx_t *ctx, expression_t *member_expr, expression_t *expression)
{
    array_expression_t *ret = parser_alloc(ctx, sizeof(array_expression_t));

    ret->expr.eval = array_expression_eval;
    ret->member_expr = member_expr;
    ret->expression = expression;

    return &ret->expr;
}

static expression_t *new_member_expression(parser_ctx_t *ctx, expression_t *expression, const WCHAR *identifier)
{
    member_expression_t *ret = parser_alloc(ctx, sizeof(member_expression_t));

    ret->expr.eval = member_expression_eval;
    ret->expression = expression;
    ret->identifier = identifier;

    return &ret->expr;
}

static expression_t *new_new_expression(parser_ctx_t *ctx, expression_t *expression, argument_list_t *argument_list)
{
    call_expression_t *ret = parser_alloc(ctx, sizeof(call_expression_t));

    ret->expr.eval = new_expression_eval;
    ret->expression = expression;
    ret->argument_list = argument_list ? argument_list->head : NULL;

    return &ret->expr;
}

static expression_t *new_call_expression(parser_ctx_t *ctx, expression_t *expression, argument_list_t *argument_list)
{
    call_expression_t *ret = parser_alloc(ctx, sizeof(call_expression_t));

    ret->expr.eval = call_expression_eval;
    ret->expression = expression;
    ret->argument_list = argument_list ? argument_list->head : NULL;

    return &ret->expr;
}

static expression_t *new_this_expression(parser_ctx_t *ctx)
{
    expression_t *ret = parser_alloc(ctx, sizeof(expression_t));

    ret->eval = this_expression_eval;

    return ret;
}

static int parser_error(const char *str)
{
    return 0;
}

static void set_error(parser_ctx_t *ctx, UINT error)
{
    ctx->hres = JSCRIPT_ERROR|error;
}

static BOOL explicit_error(parser_ctx_t *ctx, void *obj, WCHAR next)
{
    if(obj || *(ctx->ptr-1)==next) return TRUE;

    set_error(ctx, IDS_SYNTAX_ERROR);
    return FALSE;
}


static expression_t *new_identifier_expression(parser_ctx_t *ctx, const WCHAR *identifier)
{
    identifier_expression_t *ret = parser_alloc(ctx, sizeof(identifier_expression_t));

    ret->expr.eval = identifier_expression_eval;
    ret->identifier = identifier;

    return &ret->expr;
}

static expression_t *new_array_literal_expression(parser_ctx_t *ctx, element_list_t *element_list, int length)
{
    array_literal_expression_t *ret = parser_alloc(ctx, sizeof(array_literal_expression_t));

    ret->expr.eval = array_literal_expression_eval;
    ret->element_list = element_list ? element_list->head : NULL;
    ret->length = length;

    return &ret->expr;
}

static expression_t *new_prop_and_value_expression(parser_ctx_t *ctx, property_list_t *property_list)
{
    property_value_expression_t *ret = parser_alloc(ctx, sizeof(property_value_expression_t));

    ret->expr.eval = property_value_expression_eval;
    ret->property_list = property_list ? property_list->head : NULL;

    return &ret->expr;
}

static expression_t *new_literal_expression(parser_ctx_t *ctx, literal_t *literal)
{
    literal_expression_t *ret = parser_alloc(ctx, sizeof(literal_expression_t));

    ret->expr.eval = literal_expression_eval;
    ret->literal = literal;

    return &ret->expr;
}

static source_elements_t *new_source_elements(parser_ctx_t *ctx)
{
    source_elements_t *ret = parser_alloc(ctx, sizeof(source_elements_t));

    memset(ret, 0, sizeof(*ret));

    return ret;
}

static source_elements_t *source_elements_add_statement(source_elements_t *source_elements, statement_t *statement)
{
    if(source_elements->statement_tail)
        source_elements->statement_tail = source_elements->statement_tail->next = statement;
    else
        source_elements->statement = source_elements->statement_tail = statement;

    return source_elements;
}

static statement_list_t *new_statement_list(parser_ctx_t *ctx, statement_t *statement)
{
    statement_list_t *ret =  parser_alloc_tmp(ctx, sizeof(statement_list_t));

    ret->head = ret->tail = statement;

    return ret;
}

static statement_list_t *statement_list_add(statement_list_t *list, statement_t *statement)
{
    list->tail = list->tail->next = statement;

    return list;
}

static void push_func(parser_ctx_t *ctx)
{
    func_stack_t *new_func = parser_alloc_tmp(ctx, sizeof(func_stack_t));

    new_func->func_head = new_func->func_tail = NULL;
    new_func->var_head = new_func->var_tail = NULL;

    new_func->next = ctx->func_stack;
    ctx->func_stack = new_func;
}

static source_elements_t *function_body_parsed(parser_ctx_t *ctx, source_elements_t *source)
{
    source->functions = ctx->func_stack->func_head;
    source->variables = ctx->func_stack->var_head;
    pop_func(ctx);

    return source;
}

static void program_parsed(parser_ctx_t *ctx, source_elements_t *source)
{
    source->functions = ctx->func_stack->func_head;
    source->variables = ctx->func_stack->var_head;
    pop_func(ctx);

    ctx->source = source;
    if(!ctx->lexer_error)
        ctx->hres = S_OK;
}

void parser_release(parser_ctx_t *ctx)
{
    obj_literal_t *iter;

    if(--ctx->ref)
        return;

    for(iter = ctx->obj_literals; iter; iter = iter->next)
        jsdisp_release(iter->obj);

    jsheap_free(&ctx->heap);
    heap_free(ctx);
}

HRESULT script_parse(script_ctx_t *ctx, const WCHAR *code, const WCHAR *delimiter,
        parser_ctx_t **ret)
{
    parser_ctx_t *parser_ctx;
    jsheap_t *mark;
    HRESULT hres;

    const WCHAR html_tagW[] = {'<','/','s','c','r','i','p','t','>',0};

    parser_ctx = heap_alloc_zero(sizeof(parser_ctx_t));
    if(!parser_ctx)
        return E_OUTOFMEMORY;

    parser_ctx->ref = 1;
    parser_ctx->hres = JSCRIPT_ERROR|IDS_SYNTAX_ERROR;
    parser_ctx->is_html = delimiter && !strcmpiW(delimiter, html_tagW);

    parser_ctx->begin = parser_ctx->ptr = code;
    parser_ctx->end = code + strlenW(code);

    script_addref(ctx);
    parser_ctx->script = ctx;

    mark = jsheap_mark(&ctx->tmp_heap);
    jsheap_init(&parser_ctx->heap);

    push_func(parser_ctx);

    parser_parse(parser_ctx);
    jsheap_clear(mark);
    if(FAILED(parser_ctx->hres)) {
        hres = parser_ctx->hres;
        parser_release(parser_ctx);
        return hres;
    }

    *ret = parser_ctx;
    return S_OK;
}

