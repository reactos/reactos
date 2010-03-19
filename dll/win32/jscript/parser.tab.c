
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
#line 210 "parser.tab.c"

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
     kRETURN = 273,
     kSWITCH = 274,
     kTHIS = 275,
     kTHROW = 276,
     kTRUE = 277,
     kFALSE = 278,
     kTRY = 279,
     kTYPEOF = 280,
     kVAR = 281,
     kVOID = 282,
     kWHILE = 283,
     kWITH = 284,
     tANDAND = 285,
     tOROR = 286,
     tINC = 287,
     tDEC = 288,
     tHTMLCOMMENT = 289,
     kDIVEQ = 290,
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
#line 150 "parser.y"

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
#line 313 "parser.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 325 "parser.tab.c"

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
#define YYLAST   1030

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  67
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  95
/* YYNRULES -- Number of rules.  */
#define YYNRULES  214
/* YYNRULES -- Number of states.  */
#define YYNSTATES  373

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
      67,    70,    71,    73,    77,    80,    84,    86,    90,    92,
      96,    99,   102,   103,   105,   108,   109,   111,   114,   116,
     119,   127,   133,   141,   147,   148,   149,   161,   162,   163,
     176,   184,   193,   197,   201,   205,   211,   215,   221,   225,
     231,   232,   234,   236,   239,   244,   248,   252,   256,   260,
     265,   271,   274,   275,   277,   279,   281,   283,   287,   288,
     290,   292,   296,   298,   300,   302,   306,   310,   312,   316,
     320,   322,   328,   330,   336,   338,   342,   344,   348,   350,
     354,   356,   360,   362,   366,   368,   372,   374,   378,   380,
     384,   386,   390,   392,   396,   398,   402,   404,   408,   410,
     414,   418,   422,   424,   428,   432,   434,   438,   440,   444,
     448,   450,   454,   458,   462,   464,   467,   470,   473,   476,
     479,   482,   485,   488,   491,   493,   496,   499,   501,   503,
     505,   508,   510,   512,   517,   521,   525,   528,   531,   536,
     540,   543,   547,   549,   553,   555,   557,   559,   561,   563,
     567,   570,   574,   578,   584,   587,   592,   594,   597,   598,
     600,   603,   607,   611,   617,   619,   621,   623,   624,   626,
     628,   630,   632,   634,   636,   638,   640,   642,   644,   646,
     648,   650,   652,   654,   656
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      68,     0,    -1,    70,    69,    -1,    34,    -1,    -1,    -1,
      70,    76,    -1,    72,   155,   159,    75,   160,    46,    73,
      37,    -1,    36,    -1,    70,    -1,    38,    -1,    74,    47,
      38,    -1,    -1,    74,    -1,    79,    -1,    80,    -1,    89,
      -1,    71,    -1,    90,    -1,    91,    -1,    92,    -1,    97,
      -1,    98,    -1,    99,    -1,   100,    -1,   101,    -1,   102,
      -1,   108,    -1,   109,    -1,    76,    -1,    77,    76,    -1,
      -1,    77,    -1,    46,    77,    37,    -1,    46,    37,    -1,
      26,    81,   158,    -1,    83,    -1,    81,    47,    83,    -1,
      84,    -1,    82,    47,    84,    -1,    38,    85,    -1,    38,
      87,    -1,    -1,    86,    -1,    48,   118,    -1,    -1,    88,
      -1,    48,   119,    -1,    49,    -1,   114,   158,    -1,    11,
     159,   113,   160,    76,    10,    76,    -1,    11,   159,   113,
     160,    76,    -1,     9,    76,    28,   159,   113,   160,   158,
      -1,    28,   159,   113,   160,    76,    -1,    -1,    -1,    13,
     159,   115,    93,   161,   112,    94,   161,   112,   160,    76,
      -1,    -1,    -1,    13,   159,    26,    82,    95,   161,   112,
      96,   161,   112,   160,    76,    -1,    13,   159,   141,    14,
     113,   160,    76,    -1,    13,   159,    26,    84,    14,   113,
     160,    76,    -1,     6,   155,   158,    -1,     3,   155,   158,
      -1,    18,   112,   158,    -1,    29,   159,   114,   160,    76,
      -1,    38,    50,    76,    -1,    19,   159,   114,   160,   103,
      -1,    46,   104,    37,    -1,    46,   104,   107,   104,    37,
      -1,    -1,   105,    -1,   106,    -1,   105,   106,    -1,     4,
     114,    50,    78,    -1,     7,    50,    78,    -1,    21,   114,
     158,    -1,    24,    79,   110,    -1,    24,    79,   111,    -1,
      24,    79,   110,   111,    -1,     5,   159,    38,   160,    79,
      -1,    12,    79,    -1,    -1,   114,    -1,   114,    -1,     1,
      -1,   118,    -1,   114,    47,   118,    -1,    -1,   116,    -1,
     119,    -1,   116,    47,   119,    -1,    39,    -1,    35,    -1,
     120,    -1,   141,    48,   118,    -1,   141,   117,   118,    -1,
     121,    -1,   141,    48,   119,    -1,   141,   117,   119,    -1,
     122,    -1,   122,    51,   118,    50,   118,    -1,   123,    -1,
     123,    51,   119,    50,   119,    -1,   124,    -1,   122,    31,
     124,    -1,   125,    -1,   123,    31,   125,    -1,   126,    -1,
     124,    30,   126,    -1,   127,    -1,   125,    30,   127,    -1,
     128,    -1,   126,    52,   128,    -1,   129,    -1,   127,    52,
     129,    -1,   130,    -1,   128,    53,   130,    -1,   131,    -1,
     129,    53,   131,    -1,   132,    -1,   130,    54,   132,    -1,
     133,    -1,   131,    54,   133,    -1,   134,    -1,   132,    40,
     134,    -1,   135,    -1,   133,    40,   135,    -1,   136,    -1,
     134,    42,   136,    -1,   134,    15,   136,    -1,   134,    14,
     136,    -1,   136,    -1,   135,    42,   136,    -1,   135,    15,
     136,    -1,   137,    -1,   136,    41,   137,    -1,   138,    -1,
     137,    55,   138,    -1,   137,    56,   138,    -1,   139,    -1,
     138,    57,   139,    -1,   138,    58,   139,    -1,   138,    59,
     139,    -1,   140,    -1,     8,   139,    -1,    27,   139,    -1,
      25,   139,    -1,    32,   139,    -1,    33,   139,    -1,    55,
     139,    -1,    56,   139,    -1,    60,   139,    -1,    61,   139,
      -1,   141,    -1,   141,    32,    -1,   141,    33,    -1,   142,
      -1,   144,    -1,   143,    -1,    16,   142,    -1,   147,    -1,
      71,    -1,   143,    62,   114,    63,    -1,   143,    64,    38,
      -1,    16,   143,   145,    -1,   143,   145,    -1,   144,   145,
      -1,   144,    62,   114,    63,    -1,   144,    64,    38,    -1,
      65,    66,    -1,    65,   146,    66,    -1,   118,    -1,   146,
      47,   118,    -1,    20,    -1,    38,    -1,   156,    -1,   148,
      -1,   152,    -1,    65,   114,    66,    -1,    62,    63,    -1,
      62,   150,    63,    -1,    62,   149,    63,    -1,    62,   149,
      47,   151,    63,    -1,   151,   118,    -1,   149,    47,   151,
     118,    -1,    47,    -1,   150,    47,    -1,    -1,   150,    -1,
      46,    37,    -1,    46,   153,    37,    -1,   154,    50,   118,
      -1,   153,    47,   154,    50,   118,    -1,    38,    -1,    44,
      -1,    43,    -1,    -1,    38,    -1,    17,    -1,   157,    -1,
      43,    -1,    44,    -1,    58,    -1,    35,    -1,    22,    -1,
      23,    -1,    49,    -1,     1,    -1,    65,    -1,     1,    -1,
      66,    -1,     1,    -1,    49,    -1,     1,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   256,   256,   260,   261,   265,   266,   271,   275,   279,
     283,   284,   289,   290,   294,   295,   296,   297,   298,   299,
     300,   301,   302,   303,   304,   305,   306,   307,   308,   312,
     313,   318,   319,   323,   324,   328,   333,   334,   339,   341,
     346,   351,   356,   357,   361,   366,   367,   371,   376,   380,
     385,   387,   392,   394,   397,   399,   396,   403,   405,   402,
     408,   410,   415,   420,   425,   430,   435,   440,   445,   447,
     452,   453,   457,   458,   463,   468,   473,   478,   479,   480,
     485,   490,   494,   495,   498,   499,   503,   504,   509,   510,
     514,   516,   520,   521,   525,   526,   528,   533,   535,   537,
     542,   543,   548,   550,   555,   556,   561,   563,   568,   569,
     574,   576,   581,   582,   587,   589,   594,   595,   600,   602,
     607,   608,   613,   615,   620,   621,   626,   627,   632,   633,
     635,   637,   642,   643,   645,   650,   651,   656,   658,   660,
     665,   666,   668,   670,   675,   676,   678,   679,   681,   682,
     683,   684,   685,   686,   690,   692,   694,   700,   701,   705,
     706,   710,   711,   712,   714,   716,   721,   723,   725,   727,
     732,   733,   737,   738,   743,   744,   745,   746,   747,   748,
     752,   753,   754,   755,   760,   762,   767,   768,   772,   773,
     777,   778,   783,   785,   790,   791,   792,   796,   797,   801,
     802,   803,   804,   805,   807,   812,   813,   816,   817,   820,
     821,   824,   825,   828,   829
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "kBREAK", "kCASE", "kCATCH", "kCONTINUE",
  "kDEFAULT", "kDELETE", "kDO", "kELSE", "kIF", "kFINALLY", "kFOR", "kIN",
  "kINSTANCEOF", "kNEW", "kNULL", "kRETURN", "kSWITCH", "kTHIS", "kTHROW",
  "kTRUE", "kFALSE", "kTRY", "kTYPEOF", "kVAR", "kVOID", "kWHILE", "kWITH",
  "tANDAND", "tOROR", "tINC", "tDEC", "tHTMLCOMMENT", "kDIVEQ",
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
  "Expression", "ExpressionNoIn_opt", "ExpressionNoIn", "AssignOper",
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
      76,    76,    76,    76,    76,    76,    76,    76,    76,    77,
      77,    78,    78,    79,    79,    80,    81,    81,    82,    82,
      83,    84,    85,    85,    86,    87,    87,    88,    89,    90,
      91,    91,    92,    92,    93,    94,    92,    95,    96,    92,
      92,    92,    97,    98,    99,   100,   101,   102,   103,   103,
     104,   104,   105,   105,   106,   107,   108,   109,   109,   109,
     110,   111,   112,   112,   113,   113,   114,   114,   115,   115,
     116,   116,   117,   117,   118,   118,   118,   119,   119,   119,
     120,   120,   121,   121,   122,   122,   123,   123,   124,   124,
     125,   125,   126,   126,   127,   127,   128,   128,   129,   129,
     130,   130,   131,   131,   132,   132,   133,   133,   134,   134,
     134,   134,   135,   135,   135,   136,   136,   137,   137,   137,
     138,   138,   138,   138,   139,   139,   139,   139,   139,   139,
     139,   139,   139,   139,   140,   140,   140,   141,   141,   142,
     142,   143,   143,   143,   143,   143,   144,   144,   144,   144,
     145,   145,   146,   146,   147,   147,   147,   147,   147,   147,
     148,   148,   148,   148,   149,   149,   150,   150,   151,   151,
     152,   152,   153,   153,   154,   154,   154,   155,   155,   156,
     156,   156,   156,   156,   156,   157,   157,   158,   158,   159,
     159,   160,   160,   161,   161
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     1,     0,     0,     2,     8,     1,     1,
       1,     3,     0,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       2,     0,     1,     3,     2,     3,     1,     3,     1,     3,
       2,     2,     0,     1,     2,     0,     1,     2,     1,     2,
       7,     5,     7,     5,     0,     0,    11,     0,     0,    12,
       7,     8,     3,     3,     3,     5,     3,     5,     3,     5,
       0,     1,     1,     2,     4,     3,     3,     3,     3,     4,
       5,     2,     0,     1,     1,     1,     1,     3,     0,     1,
       1,     3,     1,     1,     1,     3,     3,     1,     3,     3,
       1,     5,     1,     5,     1,     3,     1,     3,     1,     3,
       1,     3,     1,     3,     1,     3,     1,     3,     1,     3,
       1,     3,     1,     3,     1,     3,     1,     3,     1,     3,
       3,     3,     1,     3,     3,     1,     3,     1,     3,     3,
       1,     3,     3,     3,     1,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     1,     2,     2,     1,     1,     1,
       2,     1,     1,     4,     3,     3,     2,     2,     4,     3,
       2,     3,     1,     3,     1,     1,     1,     1,     1,     3,
       2,     3,     3,     5,     2,     4,     1,     2,     0,     1,
       2,     3,     3,     5,     1,     1,     1,     0,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       5,     0,     4,     1,   197,   197,     0,     0,     0,     0,
       0,   199,    82,     0,   174,     0,   205,   206,     0,     0,
       0,     0,     0,     0,     0,     0,     3,   204,     8,   175,
     201,   202,     0,    48,     0,     0,   203,     0,     0,   188,
       0,     2,    17,   197,     6,    14,    15,    16,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,     0,
      86,    94,   100,   104,   108,   112,   116,   120,   124,   128,
     135,   137,   140,   144,   154,   157,   159,   158,   161,   177,
     178,   176,   200,   198,     0,     0,   175,     0,   162,   145,
     154,     0,   210,   209,     0,    88,   160,   159,     0,    83,
       0,     0,     0,     0,   147,    42,     0,    36,   146,     0,
       0,   148,   149,     0,    34,   175,   201,   202,    29,     0,
       0,     0,   150,   151,   152,   153,   186,   180,     0,   189,
       0,     0,     0,   208,     0,   207,    49,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   155,   156,    93,    92,     0,     0,     0,
       0,     0,   166,     0,     0,   167,    63,    62,   190,   194,
     196,   195,     0,    85,     0,    84,     0,    54,    89,    90,
      97,   102,   106,   110,   114,   118,   122,   126,   132,   154,
     165,    64,     0,    76,    34,     0,     0,    77,    78,     0,
      40,    43,     0,    35,     0,     0,    66,    33,    30,   191,
       0,     0,   188,   182,   187,   181,   184,   179,    12,    87,
     105,     0,   109,   113,   117,   121,   125,   131,   130,   129,
     136,   138,   139,   141,   142,   143,    95,    96,     0,   164,
     170,   172,     0,     0,   169,     0,   212,   211,     0,    45,
      57,    38,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    81,    79,
      44,    37,     0,     0,     0,   192,   189,     0,    10,    13,
       0,     0,   163,     0,   171,   168,     0,    51,     0,    41,
      46,     0,     0,     0,   214,   213,    82,    91,   154,   107,
       0,   111,   115,   119,   123,   127,   134,   133,     0,    98,
      99,    70,    67,     0,    53,    65,     0,   183,   185,     0,
       0,   101,   173,     0,     0,    47,    39,    82,     0,    55,
       0,     0,     0,     0,    71,    72,     0,   193,    11,     5,
      52,    50,    58,     0,     0,   103,    60,     0,     0,    68,
      70,    73,    80,     9,     0,     0,    61,    82,    31,    31,
       0,     7,    82,     0,    32,    74,    75,    69,     0,     0,
       0,    56,    59
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    41,     2,    88,    43,   354,   279,   280,   118,
     119,   365,    45,    46,   106,   250,   107,   251,   200,   201,
     289,   290,    47,    48,    49,    50,   252,   344,   292,   355,
      51,    52,    53,    54,    55,    56,   312,   333,   334,   335,
     350,    57,    58,   197,   198,    98,   174,    59,   177,   178,
     265,    60,   179,    61,   180,    62,   181,    63,   182,    64,
     183,    65,   184,    66,   185,    67,   186,    68,   187,    69,
      70,    71,    72,    73,    74,    75,    76,    77,   162,   242,
      78,    79,   128,   129,   130,    80,   120,   121,    84,    81,
      82,   136,    94,   248,   296
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -287
static const yytype_int16 yypact[] =
{
    -287,    29,   490,  -287,   -18,   -18,   934,   730,    21,    21,
     965,  -287,   934,    21,  -287,   934,  -287,  -287,    36,   934,
      89,   934,    21,    21,   934,   934,  -287,  -287,  -287,   105,
    -287,  -287,   550,  -287,   934,   934,  -287,   934,   934,    61,
     934,  -287,   403,   -18,  -287,  -287,  -287,  -287,  -287,  -287,
    -287,  -287,  -287,  -287,  -287,  -287,  -287,  -287,  -287,    15,
    -287,  -287,    41,   106,   117,   129,   142,   158,    76,   162,
     131,    47,  -287,  -287,   141,  -287,   113,   148,  -287,  -287,
    -287,  -287,  -287,  -287,     5,     5,  -287,   147,  -287,  -287,
     169,   177,  -287,  -287,   370,   832,  -287,   113,     5,   164,
     934,    15,   610,   123,  -287,   159,    16,  -287,  -287,   370,
     934,  -287,  -287,   730,   438,   105,   175,   176,  -287,   670,
      86,   178,  -287,  -287,  -287,  -287,  -287,  -287,    62,    63,
     934,    28,    21,  -287,   934,  -287,  -287,   934,   934,   934,
     934,   934,   934,   934,   934,   934,   934,   934,   934,   934,
     934,   934,   934,  -287,  -287,  -287,  -287,   934,   934,   934,
     181,   781,  -287,   934,   189,  -287,  -287,  -287,  -287,  -287,
    -287,  -287,    21,  -287,     8,   164,   191,  -287,   183,  -287,
    -287,    56,   201,   180,   182,   179,   194,    53,   162,    10,
    -287,  -287,    12,  -287,  -287,    21,    36,   225,  -287,   934,
    -287,  -287,    89,  -287,     8,    12,  -287,  -287,  -287,  -287,
      94,   934,   193,  -287,  -287,  -287,  -287,  -287,   200,  -287,
     106,   192,   117,   129,   142,   158,    76,   162,   162,   162,
     131,    47,    47,  -287,  -287,  -287,  -287,  -287,    68,  -287,
    -287,  -287,    46,   109,  -287,   370,  -287,  -287,   730,   195,
     197,   227,    18,   934,   934,   934,   934,   934,   934,   934,
     934,   934,   934,   370,   934,   934,   199,   209,  -287,  -287,
    -287,  -287,   730,   730,   202,  -287,   204,   883,  -287,   206,
       8,   934,  -287,   934,  -287,  -287,     8,   239,   934,  -287,
    -287,   191,    18,   370,  -287,  -287,   934,  -287,   160,   201,
     218,   180,   182,   179,   194,    53,   162,   162,     8,  -287,
    -287,   250,  -287,     8,  -287,  -287,   934,  -287,  -287,   231,
     228,  -287,  -287,     5,   730,  -287,  -287,   934,     8,  -287,
     934,   730,   934,    66,   250,  -287,    36,  -287,  -287,  -287,
    -287,  -287,  -287,   730,    18,  -287,  -287,   -32,   226,  -287,
     250,  -287,  -287,   730,   238,    18,  -287,   934,   730,   730,
     240,  -287,   934,     8,   730,  -287,  -287,  -287,     8,   730,
     730,  -287,  -287
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -287,  -287,  -287,   -61,    -2,  -287,  -287,  -287,  -287,     0,
    -141,   -80,   -15,  -287,  -287,  -287,    78,   -10,  -287,  -287,
    -287,  -287,  -287,  -287,  -287,  -287,  -287,  -287,  -287,  -287,
    -287,  -287,  -287,  -287,  -287,  -287,  -287,   -68,  -287,   -50,
    -287,  -287,  -287,  -287,    88,  -286,  -105,    20,  -287,  -287,
     212,   -77,  -209,  -287,  -287,  -287,  -287,   150,    34,   152,
      33,   153,    35,   154,    38,   156,    40,   151,    42,   -94,
     157,    75,    64,  -287,     2,   290,   291,  -287,    19,  -287,
    -287,  -287,  -287,    93,    95,  -287,  -287,    96,    26,  -287,
    -287,   -73,    25,    17,  -278
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -197
static const yytype_int16 yytable[] =
{
      42,   188,    44,   103,   204,    42,   133,    91,    90,   246,
     329,   166,   167,   246,   327,   134,   133,   133,   358,   294,
      83,    90,    92,    90,   263,   191,    90,    90,   193,     3,
      42,    85,    99,   203,    95,   101,    90,    90,   100,    90,
      90,   342,   153,   154,   297,   155,   300,   109,   110,   156,
     227,   228,   229,   216,   135,   309,   310,   219,   264,   134,
     131,   221,   134,   202,   135,   135,   357,   295,   261,   132,
      89,   363,   137,   348,   247,   134,   368,   362,   247,   325,
     236,   237,   102,   104,   241,   108,    93,   254,   111,   112,
     144,   145,   138,   283,   217,   262,   165,   189,   122,   123,
      42,   124,   125,   349,   150,   151,   152,   255,   126,   212,
     214,    42,   284,   206,   175,   134,   190,    42,   146,   208,
     192,   345,   270,   209,   127,   213,   215,   105,   195,   175,
     205,   282,   169,   210,   275,   196,   139,   170,   171,    90,
     286,    90,    90,    90,    90,    90,    90,    90,    90,    90,
      90,    90,    90,    90,    90,   113,   134,   218,   308,   188,
     188,   188,   188,   188,   188,   188,   188,   306,   307,   140,
     188,   188,   285,   153,   154,   159,   155,   160,   161,   238,
     156,   268,   141,   243,   168,   169,   148,   149,   328,   157,
     170,   171,   153,   154,   188,   155,   142,   245,   143,   156,
     318,   153,   154,   147,   321,   172,   322,   199,   264,   266,
     163,   134,   164,   161,   233,   234,   235,   364,   364,   239,
     267,   272,   273,   231,   232,  -196,  -195,   244,   211,   249,
     253,   256,   257,   259,   260,   258,   188,   196,   278,   337,
     126,   293,   281,   288,   291,   311,    42,   313,   287,   324,
     340,   214,   316,   319,   332,   298,    90,   298,    90,    90,
      90,    90,    90,    90,    90,   175,   298,   298,   330,   338,
      42,    42,   314,   315,   339,   361,   359,   367,   353,   366,
     271,   326,   360,   175,   351,   269,   158,   220,   299,   301,
     298,   222,   302,   223,   226,   224,   303,   320,   225,   304,
      96,    97,   305,   323,   230,   276,   274,   277,     0,     0,
       0,     0,     0,   175,     0,     0,    99,     0,     0,     0,
       0,   352,    42,     0,   341,   331,     0,     0,     0,    42,
     336,   346,   298,     0,     0,     0,     0,     0,     0,     0,
       0,    42,     0,   356,     0,   343,     0,    99,     0,     0,
       0,    42,   347,    44,     0,     0,    42,    42,     0,     0,
       0,     0,    42,     0,   208,     0,     0,    42,    42,   371,
     372,   173,     0,     0,     0,     0,     0,    99,     6,     0,
     369,     0,    99,     0,     0,   370,    10,    11,     0,     0,
      14,     0,    16,    17,     0,    19,     0,    21,     0,     0,
       0,     0,    24,    25,  -162,    27,    28,     0,    86,     0,
       0,     0,     0,    30,    31,     0,    87,  -162,  -162,     0,
       0,     0,     0,     0,     0,    34,    35,     0,    36,     0,
      37,    38,    39,  -162,  -162,    40,     0,     0,     0,  -190,
       0,     0,  -162,  -162,  -162,  -162,     0,     0,     0,     0,
    -162,  -162,  -190,  -190,  -162,  -162,  -162,  -162,     0,     0,
    -162,     0,  -162,     0,     0,     0,     0,  -162,  -190,  -190,
       0,     0,     0,     0,     0,     0,     0,  -190,  -190,  -190,
    -190,     0,     0,     0,     0,  -190,  -190,     0,     0,  -190,
    -190,  -190,  -190,     4,     0,  -190,     5,  -190,     6,     7,
       0,     8,  -190,     9,     0,     0,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
       0,     0,    24,    25,    26,    27,    28,     0,    29,     0,
       0,     0,     0,    30,    31,     0,    32,     0,     0,    33,
       0,     0,     0,     0,     0,    34,    35,     0,    36,     0,
      37,    38,    39,     4,     0,    40,     5,     0,     6,     7,
       0,     8,     0,     9,     0,     0,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
       0,     0,    24,    25,     0,    27,    28,   114,   115,     0,
       0,     0,     0,   116,   117,     0,    32,     0,     0,    33,
       0,     0,     0,     0,     0,    34,    35,     0,    36,     0,
      37,    38,    39,     4,     0,    40,     5,     0,     6,     7,
       0,     8,     0,     9,     0,     0,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
       0,     0,    24,    25,     0,    27,    28,   194,    29,     0,
       0,     0,     0,    30,    31,     0,    32,     0,     0,    33,
       0,     0,     0,     0,     0,    34,    35,     0,    36,     0,
      37,    38,    39,     4,     0,    40,     5,     0,     6,     7,
       0,     8,     0,     9,     0,     0,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
       0,     0,    24,    25,     0,    27,    28,   207,    29,     0,
       0,     0,     0,    30,    31,     0,    32,     0,     0,    33,
       0,     0,     0,     0,     0,    34,    35,     0,    36,     0,
      37,    38,    39,     4,     0,    40,     5,     0,     6,     7,
       0,     8,     0,     9,     0,     0,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
       0,     0,    24,    25,     0,    27,    28,     0,    29,     0,
       0,     0,     0,    30,    31,     0,    32,     0,     0,    33,
       0,     0,     0,     0,     0,    34,    35,     0,    36,     6,
      37,    38,    39,     0,     0,    40,     0,    10,    11,     0,
       0,    14,     0,    16,    17,     0,    19,     0,    21,     0,
       0,     0,     0,    24,    25,     0,    27,    28,     0,    86,
       0,     0,     0,     0,    30,    31,     0,    87,     0,     0,
       0,     0,     0,     0,     0,     0,    34,    35,     0,    36,
       6,    37,    38,    39,     0,     0,    40,   240,    10,    11,
       0,     0,    14,     0,    16,    17,     0,    19,   176,    21,
       0,     0,     0,     0,    24,    25,     0,    27,    28,     0,
      86,     0,     0,     0,     0,    30,    31,     0,    87,     0,
       0,     0,     0,     0,     0,     0,     0,    34,    35,     0,
      36,     6,    37,    38,    39,     0,     0,    40,     0,    10,
      11,     0,     0,    14,     0,    16,    17,     0,    19,     0,
      21,     0,     0,     0,     0,    24,    25,     0,    27,    28,
       0,    86,     0,     0,     0,     0,    30,    31,     0,    87,
       0,     0,     0,     0,     0,     0,     0,     0,    34,    35,
       0,    36,     6,    37,    38,    39,   317,     0,    40,     0,
      10,    11,     0,     0,    14,     0,    16,    17,     0,    19,
       0,    21,     0,     0,     0,     0,    24,    25,     0,    27,
      28,     0,    86,     0,     0,     0,     0,    30,    31,     0,
      87,    10,    11,     0,     0,    14,     0,    16,    17,    34,
      35,     0,    36,     0,    37,    38,    39,     0,     0,    40,
      27,    28,     0,    86,     0,     0,     0,     0,    30,    31,
       0,    87,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    36,     0,     0,     0,    39,     0,     0,
      40
};

static const yytype_int16 yycheck[] =
{
       2,    95,     2,    18,   109,     7,     1,     7,     6,     1,
     296,    84,    85,     1,   292,    47,     1,     1,    50,     1,
      38,    19,     1,    21,    14,    98,    24,    25,   101,     0,
      32,     5,    12,   106,     9,    15,    34,    35,    13,    37,
      38,   327,    32,    33,   253,    35,   255,    22,    23,    39,
     144,   145,   146,   130,    49,   264,   265,   134,    48,    47,
      40,   138,    47,    47,    49,    49,   344,    49,    15,    43,
       6,   357,    31,     7,    66,    47,   362,   355,    66,   288,
     157,   158,    46,    19,   161,    21,    65,    31,    24,    25,
      14,    15,    51,    47,    66,    42,    77,    95,    34,    35,
     102,    37,    38,    37,    57,    58,    59,    51,    47,    47,
      47,   113,    66,   113,    94,    47,    97,   119,    42,   119,
     100,   330,   199,    37,    63,    63,    63,    38,     5,   109,
     110,    63,    38,    47,   211,    12,    30,    43,    44,   137,
     245,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   152,    50,    47,   132,   263,   253,
     254,   255,   256,   257,   258,   259,   260,   261,   262,    52,
     264,   265,    63,    32,    33,    62,    35,    64,    65,   159,
      39,   196,    53,   163,    37,    38,    55,    56,   293,    48,
      43,    44,    32,    33,   288,    35,    54,   172,    40,    39,
     277,    32,    33,    41,   281,    28,   283,    48,    48,   192,
      62,    47,    64,    65,   150,   151,   152,   358,   359,    38,
     195,   204,   205,   148,   149,    50,    50,    38,    50,    38,
      47,    30,    52,    54,    40,    53,   330,    12,    38,   316,
      47,    14,    50,    48,    47,    46,   248,    38,   248,    10,
     323,    47,    50,    47,     4,   253,   254,   255,   256,   257,
     258,   259,   260,   261,   262,   245,   264,   265,    50,    38,
     272,   273,   272,   273,    46,    37,    50,    37,   339,   359,
     202,   291,   350,   263,   334,   197,    74,   137,   254,   256,
     288,   139,   257,   140,   143,   141,   258,   280,   142,   259,
      10,    10,   260,   286,   147,   212,   210,   212,    -1,    -1,
      -1,    -1,    -1,   293,    -1,    -1,   296,    -1,    -1,    -1,
      -1,   336,   324,    -1,   324,   308,    -1,    -1,    -1,   331,
     313,   331,   330,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   343,    -1,   343,    -1,   328,    -1,   327,    -1,    -1,
      -1,   353,   332,   353,    -1,    -1,   358,   359,    -1,    -1,
      -1,    -1,   364,    -1,   364,    -1,    -1,   369,   370,   369,
     370,     1,    -1,    -1,    -1,    -1,    -1,   357,     8,    -1,
     363,    -1,   362,    -1,    -1,   368,    16,    17,    -1,    -1,
      20,    -1,    22,    23,    -1,    25,    -1,    27,    -1,    -1,
      -1,    -1,    32,    33,     1,    35,    36,    -1,    38,    -1,
      -1,    -1,    -1,    43,    44,    -1,    46,    14,    15,    -1,
      -1,    -1,    -1,    -1,    -1,    55,    56,    -1,    58,    -1,
      60,    61,    62,    30,    31,    65,    -1,    -1,    -1,     1,
      -1,    -1,    39,    40,    41,    42,    -1,    -1,    -1,    -1,
      47,    48,    14,    15,    51,    52,    53,    54,    -1,    -1,
      57,    -1,    59,    -1,    -1,    -1,    -1,    64,    30,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    40,    41,
      42,    -1,    -1,    -1,    -1,    47,    48,    -1,    -1,    51,
      52,    53,    54,     3,    -1,    57,     6,    59,     8,     9,
      -1,    11,    64,    13,    -1,    -1,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      -1,    -1,    32,    33,    34,    35,    36,    -1,    38,    -1,
      -1,    -1,    -1,    43,    44,    -1,    46,    -1,    -1,    49,
      -1,    -1,    -1,    -1,    -1,    55,    56,    -1,    58,    -1,
      60,    61,    62,     3,    -1,    65,     6,    -1,     8,     9,
      -1,    11,    -1,    13,    -1,    -1,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      -1,    -1,    32,    33,    -1,    35,    36,    37,    38,    -1,
      -1,    -1,    -1,    43,    44,    -1,    46,    -1,    -1,    49,
      -1,    -1,    -1,    -1,    -1,    55,    56,    -1,    58,    -1,
      60,    61,    62,     3,    -1,    65,     6,    -1,     8,     9,
      -1,    11,    -1,    13,    -1,    -1,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      -1,    -1,    32,    33,    -1,    35,    36,    37,    38,    -1,
      -1,    -1,    -1,    43,    44,    -1,    46,    -1,    -1,    49,
      -1,    -1,    -1,    -1,    -1,    55,    56,    -1,    58,    -1,
      60,    61,    62,     3,    -1,    65,     6,    -1,     8,     9,
      -1,    11,    -1,    13,    -1,    -1,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      -1,    -1,    32,    33,    -1,    35,    36,    37,    38,    -1,
      -1,    -1,    -1,    43,    44,    -1,    46,    -1,    -1,    49,
      -1,    -1,    -1,    -1,    -1,    55,    56,    -1,    58,    -1,
      60,    61,    62,     3,    -1,    65,     6,    -1,     8,     9,
      -1,    11,    -1,    13,    -1,    -1,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      -1,    -1,    32,    33,    -1,    35,    36,    -1,    38,    -1,
      -1,    -1,    -1,    43,    44,    -1,    46,    -1,    -1,    49,
      -1,    -1,    -1,    -1,    -1,    55,    56,    -1,    58,     8,
      60,    61,    62,    -1,    -1,    65,    -1,    16,    17,    -1,
      -1,    20,    -1,    22,    23,    -1,    25,    -1,    27,    -1,
      -1,    -1,    -1,    32,    33,    -1,    35,    36,    -1,    38,
      -1,    -1,    -1,    -1,    43,    44,    -1,    46,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    55,    56,    -1,    58,
       8,    60,    61,    62,    -1,    -1,    65,    66,    16,    17,
      -1,    -1,    20,    -1,    22,    23,    -1,    25,    26,    27,
      -1,    -1,    -1,    -1,    32,    33,    -1,    35,    36,    -1,
      38,    -1,    -1,    -1,    -1,    43,    44,    -1,    46,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    55,    56,    -1,
      58,     8,    60,    61,    62,    -1,    -1,    65,    -1,    16,
      17,    -1,    -1,    20,    -1,    22,    23,    -1,    25,    -1,
      27,    -1,    -1,    -1,    -1,    32,    33,    -1,    35,    36,
      -1,    38,    -1,    -1,    -1,    -1,    43,    44,    -1,    46,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    55,    56,
      -1,    58,     8,    60,    61,    62,    63,    -1,    65,    -1,
      16,    17,    -1,    -1,    20,    -1,    22,    23,    -1,    25,
      -1,    27,    -1,    -1,    -1,    -1,    32,    33,    -1,    35,
      36,    -1,    38,    -1,    -1,    -1,    -1,    43,    44,    -1,
      46,    16,    17,    -1,    -1,    20,    -1,    22,    23,    55,
      56,    -1,    58,    -1,    60,    61,    62,    -1,    -1,    65,
      35,    36,    -1,    38,    -1,    -1,    -1,    -1,    43,    44,
      -1,    46,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    58,    -1,    -1,    -1,    62,    -1,    -1,
      65
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    68,    70,     0,     3,     6,     8,     9,    11,    13,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    32,    33,    34,    35,    36,    38,
      43,    44,    46,    49,    55,    56,    58,    60,    61,    62,
      65,    69,    71,    72,    76,    79,    80,    89,    90,    91,
      92,    97,    98,    99,   100,   101,   102,   108,   109,   114,
     118,   120,   122,   124,   126,   128,   130,   132,   134,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   147,   148,
     152,   156,   157,    38,   155,   155,    38,    46,    71,   139,
     141,    76,     1,    65,   159,   159,   142,   143,   112,   114,
     159,   114,    46,    79,   139,    38,    81,    83,   139,   159,
     159,   139,   139,    50,    37,    38,    43,    44,    76,    77,
     153,   154,   139,   139,   139,   139,    47,    63,   149,   150,
     151,   114,   155,     1,    47,    49,   158,    31,    51,    30,
      52,    53,    54,    40,    14,    15,    42,    41,    55,    56,
      57,    58,    59,    32,    33,    35,    39,    48,   117,    62,
      64,    65,   145,    62,    64,   145,   158,   158,    37,    38,
      43,    44,    28,     1,   113,   114,    26,   115,   116,   119,
     121,   123,   125,   127,   129,   131,   133,   135,   136,   141,
     145,   158,   114,   158,    37,     5,    12,   110,   111,    48,
      85,    86,    47,   158,   113,   114,    76,    37,    76,    37,
      47,    50,    47,    63,    47,    63,   118,    66,   159,   118,
     124,   118,   126,   128,   130,   132,   134,   136,   136,   136,
     137,   138,   138,   139,   139,   139,   118,   118,   114,    38,
      66,   118,   146,   114,    38,   159,     1,    66,   160,    38,
      82,    84,    93,    47,    31,    51,    30,    52,    53,    54,
      40,    15,    42,    14,    48,   117,   160,   159,    79,   111,
     118,    83,   160,   160,   154,   118,   150,   151,    38,    74,
      75,    50,    63,    47,    66,    63,   113,    76,    48,    87,
      88,    47,    95,    14,     1,    49,   161,   119,   141,   125,
     119,   127,   129,   131,   133,   135,   136,   136,   113,   119,
     119,    46,   103,    38,    76,    76,    50,    63,   118,    47,
     160,   118,   118,   160,    10,   119,    84,   161,   113,   112,
      50,   160,     4,   104,   105,   106,   160,   118,    38,    46,
     158,    76,   112,   160,    94,   119,    76,   114,     7,    37,
     107,   106,    79,    70,    73,    96,    76,   161,    50,    50,
     104,    37,   161,   112,    77,    78,    78,    37,   112,   160,
     160,    76,    76
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
    { (yyval.statement) = new_empty_statement(ctx); ;}
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
#line 308 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 312 "parser.y"
    { (yyval.statement_list) = new_statement_list(ctx, (yyvsp[(1) - (1)].statement)); ;}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 314 "parser.y"
    { (yyval.statement_list) = statement_list_add((yyvsp[(1) - (2)].statement_list), (yyvsp[(2) - (2)].statement)); ;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 318 "parser.y"
    { (yyval.statement_list) = NULL; ;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 319 "parser.y"
    { (yyval.statement_list) = (yyvsp[(1) - (1)].statement_list); ;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 323 "parser.y"
    { (yyval.statement) = new_block_statement(ctx, (yyvsp[(2) - (3)].statement_list)); ;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 324 "parser.y"
    { (yyval.statement) = new_block_statement(ctx, NULL); ;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 329 "parser.y"
    { (yyval.statement) = new_var_statement(ctx, (yyvsp[(2) - (3)].variable_list)); ;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 333 "parser.y"
    { (yyval.variable_list) = new_variable_list(ctx, (yyvsp[(1) - (1)].variable_declaration)); ;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 335 "parser.y"
    { (yyval.variable_list) = variable_list_add(ctx, (yyvsp[(1) - (3)].variable_list), (yyvsp[(3) - (3)].variable_declaration)); ;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 340 "parser.y"
    { (yyval.variable_list) = new_variable_list(ctx, (yyvsp[(1) - (1)].variable_declaration)); ;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 342 "parser.y"
    { (yyval.variable_list) = variable_list_add(ctx, (yyvsp[(1) - (3)].variable_list), (yyvsp[(3) - (3)].variable_declaration)); ;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 347 "parser.y"
    { (yyval.variable_declaration) = new_variable_declaration(ctx, (yyvsp[(1) - (2)].identifier), (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 352 "parser.y"
    { (yyval.variable_declaration) = new_variable_declaration(ctx, (yyvsp[(1) - (2)].identifier), (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 356 "parser.y"
    { (yyval.expr) = NULL; ;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 357 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 362 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (2)].expr); ;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 366 "parser.y"
    { (yyval.expr) = NULL; ;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 367 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 372 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (2)].expr); ;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 376 "parser.y"
    { (yyval.statement) = new_empty_statement(ctx); ;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 381 "parser.y"
    { (yyval.statement) = new_expression_statement(ctx, (yyvsp[(1) - (2)].expr)); ;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 386 "parser.y"
    { (yyval.statement) = new_if_statement(ctx, (yyvsp[(3) - (7)].expr), (yyvsp[(5) - (7)].statement), (yyvsp[(7) - (7)].statement)); ;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 388 "parser.y"
    { (yyval.statement) = new_if_statement(ctx, (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].statement), NULL); ;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 393 "parser.y"
    { (yyval.statement) = new_while_statement(ctx, TRUE, (yyvsp[(5) - (7)].expr), (yyvsp[(2) - (7)].statement)); ;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 395 "parser.y"
    { (yyval.statement) = new_while_statement(ctx, FALSE, (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].statement)); ;}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 397 "parser.y"
    { if(!explicit_error(ctx, (yyvsp[(3) - (3)].expr), ';')) YYABORT; ;}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 399 "parser.y"
    { if(!explicit_error(ctx, (yyvsp[(6) - (6)].expr), ';')) YYABORT; ;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 401 "parser.y"
    { (yyval.statement) = new_for_statement(ctx, NULL, (yyvsp[(3) - (11)].expr), (yyvsp[(6) - (11)].expr), (yyvsp[(9) - (11)].expr), (yyvsp[(11) - (11)].statement)); ;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 403 "parser.y"
    { if(!explicit_error(ctx, (yyvsp[(4) - (4)].variable_list), ';')) YYABORT; ;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 405 "parser.y"
    { if(!explicit_error(ctx, (yyvsp[(7) - (7)].expr), ';')) YYABORT; ;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 407 "parser.y"
    { (yyval.statement) = new_for_statement(ctx, (yyvsp[(4) - (12)].variable_list), NULL, (yyvsp[(7) - (12)].expr), (yyvsp[(10) - (12)].expr), (yyvsp[(12) - (12)].statement)); ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 409 "parser.y"
    { (yyval.statement) = new_forin_statement(ctx, NULL, (yyvsp[(3) - (7)].expr), (yyvsp[(5) - (7)].expr), (yyvsp[(7) - (7)].statement)); ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 411 "parser.y"
    { (yyval.statement) = new_forin_statement(ctx, (yyvsp[(4) - (8)].variable_declaration), NULL, (yyvsp[(6) - (8)].expr), (yyvsp[(8) - (8)].statement)); ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 416 "parser.y"
    { (yyval.statement) = new_continue_statement(ctx, (yyvsp[(2) - (3)].identifier)); ;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 421 "parser.y"
    { (yyval.statement) = new_break_statement(ctx, (yyvsp[(2) - (3)].identifier)); ;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 426 "parser.y"
    { (yyval.statement) = new_return_statement(ctx, (yyvsp[(2) - (3)].expr)); ;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 431 "parser.y"
    { (yyval.statement) = new_with_statement(ctx, (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].statement)); ;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 436 "parser.y"
    { (yyval.statement) = new_labelled_statement(ctx, (yyvsp[(1) - (3)].identifier), (yyvsp[(3) - (3)].statement)); ;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 441 "parser.y"
    { (yyval.statement) = new_switch_statement(ctx, (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].case_clausule)); ;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 446 "parser.y"
    { (yyval.case_clausule) = new_case_block(ctx, (yyvsp[(2) - (3)].case_list), NULL, NULL); ;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 448 "parser.y"
    { (yyval.case_clausule) = new_case_block(ctx, (yyvsp[(2) - (5)].case_list), (yyvsp[(3) - (5)].case_clausule), (yyvsp[(4) - (5)].case_list)); ;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 452 "parser.y"
    { (yyval.case_list) = NULL; ;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 453 "parser.y"
    { (yyval.case_list) = (yyvsp[(1) - (1)].case_list); ;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 457 "parser.y"
    { (yyval.case_list) = new_case_list(ctx, (yyvsp[(1) - (1)].case_clausule)); ;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 459 "parser.y"
    { (yyval.case_list) = case_list_add(ctx, (yyvsp[(1) - (2)].case_list), (yyvsp[(2) - (2)].case_clausule)); ;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 464 "parser.y"
    { (yyval.case_clausule) = new_case_clausule(ctx, (yyvsp[(2) - (4)].expr), (yyvsp[(4) - (4)].statement_list)); ;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 469 "parser.y"
    { (yyval.case_clausule) = new_case_clausule(ctx, NULL, (yyvsp[(3) - (3)].statement_list)); ;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 474 "parser.y"
    { (yyval.statement) = new_throw_statement(ctx, (yyvsp[(2) - (3)].expr)); ;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 478 "parser.y"
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[(2) - (3)].statement), (yyvsp[(3) - (3)].catch_block), NULL); ;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 479 "parser.y"
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[(2) - (3)].statement), NULL, (yyvsp[(3) - (3)].statement)); ;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 481 "parser.y"
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[(2) - (4)].statement), (yyvsp[(3) - (4)].catch_block), (yyvsp[(4) - (4)].statement)); ;}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 486 "parser.y"
    { (yyval.catch_block) = new_catch_block(ctx, (yyvsp[(3) - (5)].identifier), (yyvsp[(5) - (5)].statement)); ;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 490 "parser.y"
    { (yyval.statement) = (yyvsp[(2) - (2)].statement); ;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 494 "parser.y"
    { (yyval.expr) = NULL; ;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 495 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 498 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 499 "parser.y"
    { set_error(ctx, IDS_SYNTAX_ERROR); YYABORT; ;}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 503 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 505 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_COMMA, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 509 "parser.y"
    { (yyval.expr) = NULL; ;}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 510 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 515 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 517 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_COMMA, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 520 "parser.y"
    { (yyval.ival) = (yyvsp[(1) - (1)].ival); ;}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 521 "parser.y"
    { (yyval.ival) = EXPR_ASSIGNDIV; ;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 525 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 527 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ASSIGN, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 529 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 534 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 536 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ASSIGN, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 538 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 542 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 544 "parser.y"
    { (yyval.expr) = new_conditional_expression(ctx, (yyvsp[(1) - (5)].expr), (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].expr)); ;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 549 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 551 "parser.y"
    { (yyval.expr) = new_conditional_expression(ctx, (yyvsp[(1) - (5)].expr), (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].expr)); ;}
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 555 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 557 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_OR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 562 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 564 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_OR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 568 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 570 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_AND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 575 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 577 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_AND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 581 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 583 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BOR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 588 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 590 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BOR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 594 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 596 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BXOR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 601 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 603 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BXOR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 607 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 609 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BAND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 614 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 616 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BAND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 620 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 622 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 626 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 628 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 632 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 634 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 636 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_INSTANCEOF, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 638 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_IN, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 642 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 644 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 646 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_INSTANCEOF, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 650 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 652 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 657 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 659 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ADD, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 661 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_SUB, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 665 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 667 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_MUL, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 669 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_DIV, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 671 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_MOD, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 675 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 677 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_DELETE, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 678 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_VOID, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 680 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_TYPEOF, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 681 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PREINC, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 682 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PREDEC, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 683 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PLUS, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 684 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_MINUS, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 685 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_BITNEG, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 686 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_LOGNEG, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 691 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 693 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_POSTINC, (yyvsp[(1) - (2)].expr)); ;}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 695 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_POSTDEC, (yyvsp[(1) - (2)].expr)); ;}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 700 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 701 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 705 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 706 "parser.y"
    { (yyval.expr) = new_new_expression(ctx, (yyvsp[(2) - (2)].expr), NULL); ;}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 710 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 711 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 713 "parser.y"
    { (yyval.expr) = new_array_expression(ctx, (yyvsp[(1) - (4)].expr), (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 715 "parser.y"
    { (yyval.expr) = new_member_expression(ctx, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].identifier)); ;}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 717 "parser.y"
    { (yyval.expr) = new_new_expression(ctx, (yyvsp[(2) - (3)].expr), (yyvsp[(3) - (3)].argument_list)); ;}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 722 "parser.y"
    { (yyval.expr) = new_call_expression(ctx, (yyvsp[(1) - (2)].expr), (yyvsp[(2) - (2)].argument_list)); ;}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 724 "parser.y"
    { (yyval.expr) = new_call_expression(ctx, (yyvsp[(1) - (2)].expr), (yyvsp[(2) - (2)].argument_list)); ;}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 726 "parser.y"
    { (yyval.expr) = new_array_expression(ctx, (yyvsp[(1) - (4)].expr), (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 728 "parser.y"
    { (yyval.expr) = new_member_expression(ctx, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].identifier)); ;}
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 732 "parser.y"
    { (yyval.argument_list) = NULL; ;}
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 733 "parser.y"
    { (yyval.argument_list) = (yyvsp[(2) - (3)].argument_list); ;}
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 737 "parser.y"
    { (yyval.argument_list) = new_argument_list(ctx, (yyvsp[(1) - (1)].expr)); ;}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 739 "parser.y"
    { (yyval.argument_list) = argument_list_add(ctx, (yyvsp[(1) - (3)].argument_list), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 743 "parser.y"
    { (yyval.expr) = new_this_expression(ctx); ;}
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 744 "parser.y"
    { (yyval.expr) = new_identifier_expression(ctx, (yyvsp[(1) - (1)].identifier)); ;}
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 745 "parser.y"
    { (yyval.expr) = new_literal_expression(ctx, (yyvsp[(1) - (1)].literal)); ;}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 746 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 747 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 748 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (3)].expr); ;}
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 752 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, NULL, 0); ;}
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 753 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, NULL, (yyvsp[(2) - (3)].ival)+1); ;}
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 754 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, (yyvsp[(2) - (3)].element_list), 0); ;}
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 756 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, (yyvsp[(2) - (5)].element_list), (yyvsp[(4) - (5)].ival)+1); ;}
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 761 "parser.y"
    { (yyval.element_list) = new_element_list(ctx, (yyvsp[(1) - (2)].ival), (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 763 "parser.y"
    { (yyval.element_list) = element_list_add(ctx, (yyvsp[(1) - (4)].element_list), (yyvsp[(3) - (4)].ival), (yyvsp[(4) - (4)].expr)); ;}
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 767 "parser.y"
    { (yyval.ival) = 1; ;}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 768 "parser.y"
    { (yyval.ival) = (yyvsp[(1) - (2)].ival) + 1; ;}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 772 "parser.y"
    { (yyval.ival) = 0; ;}
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 773 "parser.y"
    { (yyval.ival) = (yyvsp[(1) - (1)].ival); ;}
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 777 "parser.y"
    { (yyval.expr) = new_prop_and_value_expression(ctx, NULL); ;}
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 779 "parser.y"
    { (yyval.expr) = new_prop_and_value_expression(ctx, (yyvsp[(2) - (3)].property_list)); ;}
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 784 "parser.y"
    { (yyval.property_list) = new_property_list(ctx, (yyvsp[(1) - (3)].literal), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 786 "parser.y"
    { (yyval.property_list) = property_list_add(ctx, (yyvsp[(1) - (5)].property_list), (yyvsp[(3) - (5)].literal), (yyvsp[(5) - (5)].expr)); ;}
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 790 "parser.y"
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[(1) - (1)].identifier)); ;}
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 791 "parser.y"
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[(1) - (1)].wstr)); ;}
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 792 "parser.y"
    { (yyval.literal) = (yyvsp[(1) - (1)].literal); ;}
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 796 "parser.y"
    { (yyval.identifier) = NULL; ;}
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 797 "parser.y"
    { (yyval.identifier) = (yyvsp[(1) - (1)].identifier); ;}
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 801 "parser.y"
    { (yyval.literal) = new_null_literal(ctx); ;}
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 802 "parser.y"
    { (yyval.literal) = (yyvsp[(1) - (1)].literal); ;}
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 803 "parser.y"
    { (yyval.literal) = (yyvsp[(1) - (1)].literal); ;}
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 804 "parser.y"
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[(1) - (1)].wstr)); ;}
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 805 "parser.y"
    { (yyval.literal) = parse_regexp(ctx);
                                  if(!(yyval.literal)) YYABORT; ;}
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 807 "parser.y"
    { (yyval.literal) = parse_regexp(ctx);
                                  if(!(yyval.literal)) YYABORT; ;}
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 812 "parser.y"
    { (yyval.literal) = new_boolean_literal(ctx, VARIANT_TRUE); ;}
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 813 "parser.y"
    { (yyval.literal) = new_boolean_literal(ctx, VARIANT_FALSE); ;}
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 817 "parser.y"
    { if(!allow_auto_semicolon(ctx)) {YYABORT;} ;}
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 821 "parser.y"
    { set_error(ctx, IDS_LBRACKET); YYABORT; ;}
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 825 "parser.y"
    { set_error(ctx, IDS_RBRACKET); YYABORT; ;}
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 829 "parser.y"
    { set_error(ctx, IDS_SEMICOLON); YYABORT; ;}
    break;



/* Line 1455 of yacc.c  */
#line 3497 "parser.tab.c"
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
#line 831 "parser.y"


static BOOL allow_auto_semicolon(parser_ctx_t *ctx)
{
    return ctx->nl || ctx->ptr == ctx->end || *(ctx->ptr-1) == '}';
}

static literal_t *new_string_literal(parser_ctx_t *ctx, const WCHAR *str)
{
    literal_t *ret = parser_alloc(ctx, sizeof(literal_t));

    ret->type = LT_STRING;
    ret->u.wstr = str;

    return ret;
}

static literal_t *new_null_literal(parser_ctx_t *ctx)
{
    literal_t *ret = parser_alloc(ctx, sizeof(literal_t));

    ret->type = LT_NULL;

    return ret;
}

static literal_t *new_boolean_literal(parser_ctx_t *ctx, VARIANT_BOOL bval)
{
    literal_t *ret = parser_alloc(ctx, sizeof(literal_t));

    ret->type = LT_BOOL;
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
    if(--ctx->ref)
        return;

    script_release(ctx->script);
    heap_free(ctx->begin);
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

    parser_ctx->begin = heap_strdupW(code);
    if(!parser_ctx->begin) {
        heap_free(parser_ctx);
        return E_OUTOFMEMORY;
    }

    parser_ctx->ptr = parser_ctx->begin;
    parser_ctx->end = parser_ctx->begin + strlenW(parser_ctx->begin);

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

