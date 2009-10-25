
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
     kDIVEQ = 291,
     kFUNCTION = 292,
     tIdentifier = 293,
     tAssignOper = 294,
     tEqOper = 295,
     tShiftOper = 296,
     tRelOper = 297,
     tNumericLiteral = 298,
     tStringLiteral = 299,
     LOWER_THAN_ELSE = 300
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
#line 315 "parser.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 327 "parser.tab.c"

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
#define YYLAST   1042

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  68
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  95
/* YYNRULES -- Number of rules.  */
#define YYNRULES  215
/* YYNRULES -- Number of states.  */
#define YYNSTATES  374

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   300

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    62,     2,     2,     2,    60,    55,     2,
      66,    67,    58,    56,    48,    57,    65,    59,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    51,    50,
       2,    49,     2,    52,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    63,     2,    64,    54,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    47,    53,    38,    61,     2,     2,     2,
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
      35,    36,    37,    39,    40,    41,    42,    43,    44,    45,
      46
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
     648,   650,   652,   654,   656,   658
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      69,     0,    -1,    71,    70,    -1,    35,    -1,    -1,    -1,
      71,    77,    -1,    73,   156,   160,    76,   161,    47,    74,
      38,    -1,    37,    -1,    71,    -1,    39,    -1,    75,    48,
      39,    -1,    -1,    75,    -1,    80,    -1,    81,    -1,    90,
      -1,    72,    -1,    91,    -1,    92,    -1,    93,    -1,    98,
      -1,    99,    -1,   100,    -1,   101,    -1,   102,    -1,   103,
      -1,   109,    -1,   110,    -1,    77,    -1,    78,    77,    -1,
      -1,    78,    -1,    47,    78,    38,    -1,    47,    38,    -1,
      27,    82,   159,    -1,    84,    -1,    82,    48,    84,    -1,
      85,    -1,    83,    48,    85,    -1,    39,    86,    -1,    39,
      88,    -1,    -1,    87,    -1,    49,   119,    -1,    -1,    89,
      -1,    49,   120,    -1,    50,    -1,   115,   159,    -1,    11,
     160,   114,   161,    77,    10,    77,    -1,    11,   160,   114,
     161,    77,    -1,     9,    77,    29,   160,   114,   161,   159,
      -1,    29,   160,   114,   161,    77,    -1,    -1,    -1,    13,
     160,   116,    94,   162,   113,    95,   162,   113,   161,    77,
      -1,    -1,    -1,    13,   160,    27,    83,    96,   162,   113,
      97,   162,   113,   161,    77,    -1,    13,   160,   142,    14,
     114,   161,    77,    -1,    13,   160,    27,    85,    14,   114,
     161,    77,    -1,     6,   156,   159,    -1,     3,   156,   159,
      -1,    19,   113,   159,    -1,    30,   160,   115,   161,    77,
      -1,    39,    51,    77,    -1,    20,   160,   115,   161,   104,
      -1,    47,   105,    38,    -1,    47,   105,   108,   105,    38,
      -1,    -1,   106,    -1,   107,    -1,   106,   107,    -1,     4,
     115,    51,    79,    -1,     7,    51,    79,    -1,    22,   115,
     159,    -1,    25,    80,   111,    -1,    25,    80,   112,    -1,
      25,    80,   111,   112,    -1,     5,   160,    39,   161,    80,
      -1,    12,    80,    -1,    -1,   115,    -1,   115,    -1,     1,
      -1,   119,    -1,   115,    48,   119,    -1,    -1,   117,    -1,
     120,    -1,   117,    48,   120,    -1,    40,    -1,    36,    -1,
     121,    -1,   142,    49,   119,    -1,   142,   118,   119,    -1,
     122,    -1,   142,    49,   120,    -1,   142,   118,   120,    -1,
     123,    -1,   123,    52,   119,    51,   119,    -1,   124,    -1,
     124,    52,   120,    51,   120,    -1,   125,    -1,   123,    32,
     125,    -1,   126,    -1,   124,    32,   126,    -1,   127,    -1,
     125,    31,   127,    -1,   128,    -1,   126,    31,   128,    -1,
     129,    -1,   127,    53,   129,    -1,   130,    -1,   128,    53,
     130,    -1,   131,    -1,   129,    54,   131,    -1,   132,    -1,
     130,    54,   132,    -1,   133,    -1,   131,    55,   133,    -1,
     134,    -1,   132,    55,   134,    -1,   135,    -1,   133,    41,
     135,    -1,   136,    -1,   134,    41,   136,    -1,   137,    -1,
     135,    43,   137,    -1,   135,    15,   137,    -1,   135,    14,
     137,    -1,   137,    -1,   136,    43,   137,    -1,   136,    15,
     137,    -1,   138,    -1,   137,    42,   138,    -1,   139,    -1,
     138,    56,   139,    -1,   138,    57,   139,    -1,   140,    -1,
     139,    58,   140,    -1,   139,    59,   140,    -1,   139,    60,
     140,    -1,   141,    -1,     8,   140,    -1,    28,   140,    -1,
      26,   140,    -1,    33,   140,    -1,    34,   140,    -1,    56,
     140,    -1,    57,   140,    -1,    61,   140,    -1,    62,   140,
      -1,   142,    -1,   142,    33,    -1,   142,    34,    -1,   143,
      -1,   145,    -1,   144,    -1,    16,   143,    -1,   148,    -1,
      72,    -1,   144,    63,   115,    64,    -1,   144,    65,    39,
      -1,    16,   144,   146,    -1,   144,   146,    -1,   145,   146,
      -1,   145,    63,   115,    64,    -1,   145,    65,    39,    -1,
      66,    67,    -1,    66,   147,    67,    -1,   119,    -1,   147,
      48,   119,    -1,    21,    -1,    39,    -1,   157,    -1,   149,
      -1,   153,    -1,    66,   115,    67,    -1,    63,    64,    -1,
      63,   151,    64,    -1,    63,   150,    64,    -1,    63,   150,
      48,   152,    64,    -1,   152,   119,    -1,   150,    48,   152,
     119,    -1,    48,    -1,   151,    48,    -1,    -1,   151,    -1,
      47,    38,    -1,    47,   154,    38,    -1,   155,    51,   119,
      -1,   154,    48,   155,    51,   119,    -1,    39,    -1,    45,
      -1,    44,    -1,    -1,    39,    -1,    17,    -1,    18,    -1,
     158,    -1,    44,    -1,    45,    -1,    59,    -1,    36,    -1,
      23,    -1,    24,    -1,    50,    -1,     1,    -1,    66,    -1,
       1,    -1,    67,    -1,     1,    -1,    50,    -1,     1,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   257,   257,   261,   262,   266,   267,   272,   276,   280,
     284,   285,   290,   291,   295,   296,   297,   298,   299,   300,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   313,
     314,   319,   320,   324,   325,   329,   334,   335,   340,   342,
     347,   352,   357,   358,   362,   367,   368,   372,   377,   381,
     386,   388,   393,   395,   398,   400,   397,   404,   406,   403,
     409,   411,   416,   421,   426,   431,   436,   441,   446,   448,
     453,   454,   458,   459,   464,   469,   474,   479,   480,   481,
     486,   491,   495,   496,   499,   500,   504,   505,   510,   511,
     515,   517,   521,   522,   526,   527,   529,   534,   536,   538,
     543,   544,   549,   551,   556,   557,   562,   564,   569,   570,
     575,   577,   582,   583,   588,   590,   595,   596,   601,   603,
     608,   609,   614,   616,   621,   622,   627,   628,   633,   634,
     636,   638,   643,   644,   646,   651,   652,   657,   659,   661,
     666,   667,   669,   671,   676,   677,   679,   680,   682,   683,
     684,   685,   686,   687,   691,   693,   695,   701,   702,   706,
     707,   711,   712,   713,   715,   717,   722,   724,   726,   728,
     733,   734,   738,   739,   744,   745,   746,   747,   748,   749,
     753,   754,   755,   756,   761,   763,   768,   769,   773,   774,
     778,   779,   784,   786,   791,   792,   793,   797,   798,   802,
     803,   804,   805,   806,   807,   809,   814,   815,   818,   819,
     822,   823,   826,   827,   830,   831
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
  "kDIVEQ", "kFUNCTION", "'}'", "tIdentifier", "tAssignOper", "tEqOper",
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
     285,   286,   287,   288,   289,   290,   291,   292,   125,   293,
     294,   295,   296,   297,   298,   299,   300,   123,    44,    61,
      59,    58,    63,   124,    94,    38,    43,    45,    42,    47,
      37,   126,    33,    91,    93,    46,    40,    41
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    68,    69,    70,    70,    71,    71,    72,    73,    74,
      75,    75,    76,    76,    77,    77,    77,    77,    77,    77,
      77,    77,    77,    77,    77,    77,    77,    77,    77,    78,
      78,    79,    79,    80,    80,    81,    82,    82,    83,    83,
      84,    85,    86,    86,    87,    88,    88,    89,    90,    91,
      92,    92,    93,    93,    94,    95,    93,    96,    97,    93,
      93,    93,    98,    99,   100,   101,   102,   103,   104,   104,
     105,   105,   106,   106,   107,   108,   109,   110,   110,   110,
     111,   112,   113,   113,   114,   114,   115,   115,   116,   116,
     117,   117,   118,   118,   119,   119,   119,   120,   120,   120,
     121,   121,   122,   122,   123,   123,   124,   124,   125,   125,
     126,   126,   127,   127,   128,   128,   129,   129,   130,   130,
     131,   131,   132,   132,   133,   133,   134,   134,   135,   135,
     135,   135,   136,   136,   136,   137,   137,   138,   138,   138,
     139,   139,   139,   139,   140,   140,   140,   140,   140,   140,
     140,   140,   140,   140,   141,   141,   141,   142,   142,   143,
     143,   144,   144,   144,   144,   144,   145,   145,   145,   145,
     146,   146,   147,   147,   148,   148,   148,   148,   148,   148,
     149,   149,   149,   149,   150,   150,   151,   151,   152,   152,
     153,   153,   154,   154,   155,   155,   155,   156,   156,   157,
     157,   157,   157,   157,   157,   157,   158,   158,   159,   159,
     160,   160,   161,   161,   162,   162
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
       1,     1,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       5,     0,     4,     1,   197,   197,     0,     0,     0,     0,
       0,   199,   200,    82,     0,   174,     0,   206,   207,     0,
       0,     0,     0,     0,     0,     0,     0,     3,   205,     8,
     175,   202,   203,     0,    48,     0,     0,   204,     0,     0,
     188,     0,     2,    17,   197,     6,    14,    15,    16,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
       0,    86,    94,   100,   104,   108,   112,   116,   120,   124,
     128,   135,   137,   140,   144,   154,   157,   159,   158,   161,
     177,   178,   176,   201,   198,     0,     0,   175,     0,   162,
     145,   154,     0,   211,   210,     0,    88,   160,   159,     0,
      83,     0,     0,     0,     0,   147,    42,     0,    36,   146,
       0,     0,   148,   149,     0,    34,   175,   202,   203,    29,
       0,     0,     0,   150,   151,   152,   153,   186,   180,     0,
     189,     0,     0,     0,   209,     0,   208,    49,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   155,   156,    93,    92,     0,     0,
       0,     0,     0,   166,     0,     0,   167,    63,    62,   190,
     194,   196,   195,     0,    85,     0,    84,     0,    54,    89,
      90,    97,   102,   106,   110,   114,   118,   122,   126,   132,
     154,   165,    64,     0,    76,    34,     0,     0,    77,    78,
       0,    40,    43,     0,    35,     0,     0,    66,    33,    30,
     191,     0,     0,   188,   182,   187,   181,   184,   179,    12,
      87,   105,     0,   109,   113,   117,   121,   125,   131,   130,
     129,   136,   138,   139,   141,   142,   143,    95,    96,     0,
     164,   170,   172,     0,     0,   169,     0,   213,   212,     0,
      45,    57,    38,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    81,
      79,    44,    37,     0,     0,     0,   192,   189,     0,    10,
      13,     0,     0,   163,     0,   171,   168,     0,    51,     0,
      41,    46,     0,     0,     0,   215,   214,    82,    91,   154,
     107,     0,   111,   115,   119,   123,   127,   134,   133,     0,
      98,    99,    70,    67,     0,    53,    65,     0,   183,   185,
       0,     0,   101,   173,     0,     0,    47,    39,    82,     0,
      55,     0,     0,     0,     0,    71,    72,     0,   193,    11,
       5,    52,    50,    58,     0,     0,   103,    60,     0,     0,
      68,    70,    73,    80,     9,     0,     0,    61,    82,    31,
      31,     0,     7,    82,     0,    32,    74,    75,    69,     0,
       0,     0,    56,    59
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    42,     2,    89,    44,   355,   280,   281,   119,
     120,   366,    46,    47,   107,   251,   108,   252,   201,   202,
     290,   291,    48,    49,    50,    51,   253,   345,   293,   356,
      52,    53,    54,    55,    56,    57,   313,   334,   335,   336,
     351,    58,    59,   198,   199,    99,   175,    60,   178,   179,
     266,    61,   180,    62,   181,    63,   182,    64,   183,    65,
     184,    66,   185,    67,   186,    68,   187,    69,   188,    70,
      71,    72,    73,    74,    75,    76,    77,    78,   163,   243,
      79,    80,   129,   130,   131,    81,   121,   122,    85,    82,
      83,   137,    95,   249,   297
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -273
static const yytype_int16 yypact[] =
{
    -273,    19,   524,  -273,    -7,    -7,   976,   768,    12,    12,
     192,  -273,  -273,   976,    12,  -273,   976,  -273,  -273,    57,
     976,    24,   976,    12,    12,   976,   976,  -273,  -273,  -273,
      -3,  -273,  -273,   585,  -273,   976,   976,  -273,   976,   976,
      22,   976,  -273,   416,    -7,  -273,  -273,  -273,  -273,  -273,
    -273,  -273,  -273,  -273,  -273,  -273,  -273,  -273,  -273,  -273,
      40,  -273,  -273,     8,    30,    49,    54,    61,    84,    21,
      96,   155,    83,  -273,  -273,    88,  -273,    99,   140,  -273,
    -273,  -273,  -273,  -273,  -273,    17,    17,  -273,   115,  -273,
    -273,   184,   118,  -273,  -273,   379,   872,  -273,    99,    17,
     107,   976,    40,   646,   134,  -273,   109,    55,  -273,  -273,
     379,   976,  -273,  -273,   768,   471,    -3,   119,   121,  -273,
     707,    58,   153,  -273,  -273,  -273,  -273,  -273,  -273,    45,
      59,   976,   -45,    12,  -273,   976,  -273,  -273,   976,   976,
     976,   976,   976,   976,   976,   976,   976,   976,   976,   976,
     976,   976,   976,   976,  -273,  -273,  -273,  -273,   976,   976,
     976,   127,   820,  -273,   976,   168,  -273,  -273,  -273,  -273,
    -273,  -273,  -273,    12,  -273,     7,   107,   175,  -273,   171,
    -273,  -273,    14,   194,   174,   176,   177,   197,    34,    96,
      77,  -273,  -273,     9,  -273,  -273,    12,    57,   221,  -273,
     976,  -273,  -273,    24,  -273,     7,     9,  -273,  -273,  -273,
    -273,   112,   976,   187,  -273,  -273,  -273,  -273,  -273,   203,
    -273,    30,   193,    49,    54,    61,    84,    21,    96,    96,
      96,   155,    83,    83,  -273,  -273,  -273,  -273,  -273,    67,
    -273,  -273,  -273,    27,    71,  -273,   379,  -273,  -273,   768,
     196,   195,   232,    37,   976,   976,   976,   976,   976,   976,
     976,   976,   976,   976,   379,   976,   976,   207,   217,  -273,
    -273,  -273,  -273,   768,   768,   206,  -273,   211,   924,  -273,
     212,     7,   976,  -273,   976,  -273,  -273,     7,   251,   976,
    -273,  -273,   175,    37,   379,  -273,  -273,   976,  -273,   100,
     194,   213,   174,   176,   177,   197,    34,    96,    96,     7,
    -273,  -273,   258,  -273,     7,  -273,  -273,   976,  -273,  -273,
     224,   219,  -273,  -273,    17,   768,  -273,  -273,   976,     7,
    -273,   976,   768,   976,    16,   258,  -273,    57,  -273,  -273,
    -273,  -273,  -273,  -273,   768,    37,  -273,  -273,    79,   216,
    -273,   258,  -273,  -273,   768,   231,    37,  -273,   976,   768,
     768,   237,  -273,   976,     7,   768,  -273,  -273,  -273,     7,
     768,   768,  -273,  -273
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -273,  -273,  -273,   -70,    -2,  -273,  -273,  -273,  -273,     0,
    -138,   -84,   -10,  -273,  -273,  -273,    74,   -14,  -273,  -273,
    -273,  -273,  -273,  -273,  -273,  -273,  -273,  -273,  -273,  -273,
    -273,  -273,  -273,  -273,  -273,  -273,  -273,   -72,  -273,   -55,
    -273,  -273,  -273,  -273,    85,  -263,   -96,   -12,  -273,  -273,
     209,  -115,  -239,  -273,  -273,  -273,  -273,   143,    31,   157,
      43,   160,    44,   162,    46,   163,    47,   165,    50,   -66,
     164,    91,   353,  -273,    33,   300,   303,  -273,   -16,  -273,
    -273,  -273,  -273,   101,   102,  -273,  -273,   106,     1,  -273,
    -273,   -74,    28,   -61,  -272
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -197
static const yytype_int16 yytable[] =
{
      43,   100,    45,   135,   102,    43,    86,    92,   247,   104,
     247,   167,   168,    93,   205,   298,   217,   301,   134,     3,
     220,   328,   218,   349,   222,   192,   310,   311,   194,   132,
     189,    43,    84,   204,   330,   145,   146,    96,   295,    91,
     138,   134,   101,   237,   238,   133,   255,   242,   114,   262,
     326,   110,   111,    91,   350,    91,   134,   135,    91,    91,
     139,   140,   166,   106,   147,   343,   256,   136,    91,    91,
     127,    91,    91,   358,   248,   284,   248,   263,    94,   228,
     229,   230,   191,   176,   363,   271,   128,   296,   135,   193,
     136,   264,   346,   213,   285,   364,   210,   276,   176,   206,
     369,    43,   141,   203,   103,   136,   211,   215,   142,   214,
     154,   155,    43,   156,   207,   135,   143,   157,    43,   135,
     209,   154,   155,   216,   156,   144,   265,   135,   157,   190,
     359,   283,   267,   154,   155,   286,   156,   158,   148,   196,
     157,   151,   152,   153,   273,   274,   197,   173,   239,   265,
     287,   170,   244,   169,   170,   135,   171,   172,   200,   171,
     172,   219,   160,   319,   161,   162,   240,   322,   309,   323,
    -196,    91,  -195,    91,    91,    91,    91,    91,    91,    91,
      91,    91,    91,    91,    91,    91,    91,   269,   189,   189,
     189,   189,   189,   189,   189,   189,   307,   308,   329,   189,
     189,   246,   338,   164,   212,   165,   162,   245,    10,    11,
      12,   149,   150,    15,   250,    17,    18,   154,   155,   254,
     321,   365,   365,   189,   268,   257,   324,   258,    28,    29,
     259,    87,   260,   197,   176,   127,    31,    32,   261,    88,
     232,   233,   279,   292,   282,   289,   294,    43,   332,   288,
     341,    37,   176,   337,   312,    40,   314,   317,    41,   215,
     320,   325,   333,   339,   331,   189,   340,   360,   344,   362,
     354,    43,    43,   315,   316,   368,   367,   272,   327,   361,
     352,   221,   176,   270,   159,   100,   300,   299,    91,   299,
      91,    91,    91,    91,    91,    91,    91,   223,   299,   299,
     302,   224,   303,   370,   225,   304,   226,   305,   371,   227,
      97,   306,   231,    98,   277,   278,   100,   275,     0,     0,
       0,   348,   299,    43,     0,   342,     0,   353,     0,     0,
      43,     0,   347,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    43,     0,   357,     0,   100,     0,     0,     0,
       0,   100,    43,     0,    45,     0,     0,    43,    43,    90,
       0,     0,     0,    43,   299,   209,     0,     0,    43,    43,
     372,   373,     0,   105,     0,   109,     0,     0,   112,   113,
     174,     0,     0,     0,     0,     0,     0,     6,   123,   124,
       0,   125,   126,     0,     0,    10,    11,    12,     0,     0,
      15,     0,    17,    18,     0,    20,     0,    22,     0,     0,
       0,     0,    25,    26,     0,    28,    29,  -162,    87,     0,
       0,     0,     0,    31,    32,     0,    88,     0,     0,     0,
    -162,  -162,     0,     0,     0,    35,    36,     0,    37,     0,
      38,    39,    40,     0,     0,    41,     0,  -162,  -162,     0,
       0,     0,     0,     0,     0,     0,  -162,  -162,  -162,  -162,
       0,     0,     0,     0,  -162,  -162,     0,     0,  -162,  -162,
    -162,  -162,  -190,     0,  -162,     0,  -162,     0,     0,     0,
       0,  -162,     0,     0,     0,  -190,  -190,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  -190,  -190,   234,   235,   236,     0,     0,     0,
       0,  -190,  -190,  -190,  -190,     0,     0,     0,     0,  -190,
    -190,     0,     0,  -190,  -190,  -190,  -190,     4,     0,  -190,
       5,  -190,     6,     7,     0,     8,  -190,     9,     0,     0,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,     0,     0,    25,    26,    27,
      28,    29,     0,    30,     0,     0,     0,     0,    31,    32,
       0,    33,     0,     0,    34,     0,     0,     0,     0,     0,
      35,    36,     0,    37,     0,    38,    39,    40,     4,     0,
      41,     5,     0,     6,     7,     0,     8,     0,     9,     0,
       0,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,     0,     0,    25,    26,
       0,    28,    29,   115,   116,     0,     0,     0,     0,   117,
     118,     0,    33,     0,     0,    34,     0,     0,     0,     0,
       0,    35,    36,     0,    37,     0,    38,    39,    40,     4,
       0,    41,     5,     0,     6,     7,     0,     8,     0,     9,
       0,     0,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,     0,     0,    25,
      26,     0,    28,    29,   195,    30,     0,     0,     0,     0,
      31,    32,     0,    33,     0,     0,    34,     0,     0,     0,
       0,     0,    35,    36,     0,    37,     0,    38,    39,    40,
       4,     0,    41,     5,     0,     6,     7,     0,     8,     0,
       9,     0,     0,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,     0,     0,
      25,    26,     0,    28,    29,   208,    30,     0,     0,     0,
       0,    31,    32,     0,    33,     0,     0,    34,     0,     0,
       0,     0,     0,    35,    36,     0,    37,     0,    38,    39,
      40,     4,     0,    41,     5,     0,     6,     7,     0,     8,
       0,     9,     0,     0,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,     0,
       0,    25,    26,     0,    28,    29,     0,    30,     0,     0,
       0,     0,    31,    32,     0,    33,     0,     0,    34,     0,
       0,     0,     0,     0,    35,    36,     0,    37,     6,    38,
      39,    40,     0,     0,    41,     0,    10,    11,    12,     0,
       0,    15,     0,    17,    18,     0,    20,     0,    22,     0,
       0,     0,     0,    25,    26,     0,    28,    29,     0,    87,
       0,     0,     0,     0,    31,    32,     0,    88,     0,     0,
       0,     0,     0,     0,     0,     0,    35,    36,     0,    37,
       6,    38,    39,    40,     0,     0,    41,   241,    10,    11,
      12,     0,     0,    15,     0,    17,    18,     0,    20,   177,
      22,     0,     0,     0,     0,    25,    26,     0,    28,    29,
       0,    87,     0,     0,     0,     0,    31,    32,     0,    88,
       0,     0,     0,     0,     0,     0,     0,     0,    35,    36,
       0,    37,     6,    38,    39,    40,     0,     0,    41,     0,
      10,    11,    12,     0,     0,    15,     0,    17,    18,     0,
      20,     0,    22,     0,     0,     0,     0,    25,    26,     0,
      28,    29,     0,    87,     0,     0,     0,     0,    31,    32,
       0,    88,     0,     0,     0,     0,     0,     0,     0,     0,
      35,    36,     0,    37,     6,    38,    39,    40,   318,     0,
      41,     0,    10,    11,    12,     0,     0,    15,     0,    17,
      18,     0,    20,     0,    22,     0,     0,     0,     0,    25,
      26,     0,    28,    29,     0,    87,     0,     0,     0,     0,
      31,    32,     0,    88,     0,     0,     0,     0,     0,     0,
       0,     0,    35,    36,     0,    37,     0,    38,    39,    40,
       0,     0,    41
};

static const yytype_int16 yycheck[] =
{
       2,    13,     2,    48,    16,     7,     5,     7,     1,    19,
       1,    85,    86,     1,   110,   254,   131,   256,     1,     0,
     135,   293,    67,     7,   139,    99,   265,   266,   102,    41,
      96,    33,    39,   107,   297,    14,    15,     9,     1,     6,
      32,     1,    14,   158,   159,    44,    32,   162,    51,    15,
     289,    23,    24,    20,    38,    22,     1,    48,    25,    26,
      52,    31,    78,    39,    43,   328,    52,    50,    35,    36,
      48,    38,    39,   345,    67,    48,    67,    43,    66,   145,
     146,   147,    98,    95,   356,   200,    64,    50,    48,   101,
      50,    14,   331,    48,    67,   358,    38,   212,   110,   111,
     363,   103,    53,    48,    47,    50,    48,    48,    54,    64,
      33,    34,   114,    36,   114,    48,    55,    40,   120,    48,
     120,    33,    34,    64,    36,    41,    49,    48,    40,    96,
      51,    64,   193,    33,    34,    64,    36,    49,    42,     5,
      40,    58,    59,    60,   205,   206,    12,    29,   160,    49,
     246,    39,   164,    38,    39,    48,    44,    45,    49,    44,
      45,   133,    63,   278,    65,    66,    39,   282,   264,   284,
      51,   138,    51,   140,   141,   142,   143,   144,   145,   146,
     147,   148,   149,   150,   151,   152,   153,   197,   254,   255,
     256,   257,   258,   259,   260,   261,   262,   263,   294,   265,
     266,   173,   317,    63,    51,    65,    66,    39,    16,    17,
      18,    56,    57,    21,    39,    23,    24,    33,    34,    48,
     281,   359,   360,   289,   196,    31,   287,    53,    36,    37,
      54,    39,    55,    12,   246,    48,    44,    45,    41,    47,
     149,   150,    39,    48,    51,    49,    14,   249,   309,   249,
     324,    59,   264,   314,    47,    63,    39,    51,    66,    48,
      48,    10,     4,    39,    51,   331,    47,    51,   329,    38,
     340,   273,   274,   273,   274,    38,   360,   203,   292,   351,
     335,   138,   294,   198,    75,   297,   255,   254,   255,   256,
     257,   258,   259,   260,   261,   262,   263,   140,   265,   266,
     257,   141,   258,   364,   142,   259,   143,   260,   369,   144,
      10,   261,   148,    10,   213,   213,   328,   211,    -1,    -1,
      -1,   333,   289,   325,    -1,   325,    -1,   337,    -1,    -1,
     332,    -1,   332,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   344,    -1,   344,    -1,   358,    -1,    -1,    -1,
      -1,   363,   354,    -1,   354,    -1,    -1,   359,   360,     6,
      -1,    -1,    -1,   365,   331,   365,    -1,    -1,   370,   371,
     370,   371,    -1,    20,    -1,    22,    -1,    -1,    25,    26,
       1,    -1,    -1,    -1,    -1,    -1,    -1,     8,    35,    36,
      -1,    38,    39,    -1,    -1,    16,    17,    18,    -1,    -1,
      21,    -1,    23,    24,    -1,    26,    -1,    28,    -1,    -1,
      -1,    -1,    33,    34,    -1,    36,    37,     1,    39,    -1,
      -1,    -1,    -1,    44,    45,    -1,    47,    -1,    -1,    -1,
      14,    15,    -1,    -1,    -1,    56,    57,    -1,    59,    -1,
      61,    62,    63,    -1,    -1,    66,    -1,    31,    32,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    40,    41,    42,    43,
      -1,    -1,    -1,    -1,    48,    49,    -1,    -1,    52,    53,
      54,    55,     1,    -1,    58,    -1,    60,    -1,    -1,    -1,
      -1,    65,    -1,    -1,    -1,    14,    15,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    31,    32,   151,   152,   153,    -1,    -1,    -1,
      -1,    40,    41,    42,    43,    -1,    -1,    -1,    -1,    48,
      49,    -1,    -1,    52,    53,    54,    55,     3,    -1,    58,
       6,    60,     8,     9,    -1,    11,    65,    13,    -1,    -1,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    -1,    -1,    33,    34,    35,
      36,    37,    -1,    39,    -1,    -1,    -1,    -1,    44,    45,
      -1,    47,    -1,    -1,    50,    -1,    -1,    -1,    -1,    -1,
      56,    57,    -1,    59,    -1,    61,    62,    63,     3,    -1,
      66,     6,    -1,     8,     9,    -1,    11,    -1,    13,    -1,
      -1,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    -1,    -1,    33,    34,
      -1,    36,    37,    38,    39,    -1,    -1,    -1,    -1,    44,
      45,    -1,    47,    -1,    -1,    50,    -1,    -1,    -1,    -1,
      -1,    56,    57,    -1,    59,    -1,    61,    62,    63,     3,
      -1,    66,     6,    -1,     8,     9,    -1,    11,    -1,    13,
      -1,    -1,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    -1,    -1,    33,
      34,    -1,    36,    37,    38,    39,    -1,    -1,    -1,    -1,
      44,    45,    -1,    47,    -1,    -1,    50,    -1,    -1,    -1,
      -1,    -1,    56,    57,    -1,    59,    -1,    61,    62,    63,
       3,    -1,    66,     6,    -1,     8,     9,    -1,    11,    -1,
      13,    -1,    -1,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    -1,    -1,
      33,    34,    -1,    36,    37,    38,    39,    -1,    -1,    -1,
      -1,    44,    45,    -1,    47,    -1,    -1,    50,    -1,    -1,
      -1,    -1,    -1,    56,    57,    -1,    59,    -1,    61,    62,
      63,     3,    -1,    66,     6,    -1,     8,     9,    -1,    11,
      -1,    13,    -1,    -1,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    -1,
      -1,    33,    34,    -1,    36,    37,    -1,    39,    -1,    -1,
      -1,    -1,    44,    45,    -1,    47,    -1,    -1,    50,    -1,
      -1,    -1,    -1,    -1,    56,    57,    -1,    59,     8,    61,
      62,    63,    -1,    -1,    66,    -1,    16,    17,    18,    -1,
      -1,    21,    -1,    23,    24,    -1,    26,    -1,    28,    -1,
      -1,    -1,    -1,    33,    34,    -1,    36,    37,    -1,    39,
      -1,    -1,    -1,    -1,    44,    45,    -1,    47,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    56,    57,    -1,    59,
       8,    61,    62,    63,    -1,    -1,    66,    67,    16,    17,
      18,    -1,    -1,    21,    -1,    23,    24,    -1,    26,    27,
      28,    -1,    -1,    -1,    -1,    33,    34,    -1,    36,    37,
      -1,    39,    -1,    -1,    -1,    -1,    44,    45,    -1,    47,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    56,    57,
      -1,    59,     8,    61,    62,    63,    -1,    -1,    66,    -1,
      16,    17,    18,    -1,    -1,    21,    -1,    23,    24,    -1,
      26,    -1,    28,    -1,    -1,    -1,    -1,    33,    34,    -1,
      36,    37,    -1,    39,    -1,    -1,    -1,    -1,    44,    45,
      -1,    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      56,    57,    -1,    59,     8,    61,    62,    63,    64,    -1,
      66,    -1,    16,    17,    18,    -1,    -1,    21,    -1,    23,
      24,    -1,    26,    -1,    28,    -1,    -1,    -1,    -1,    33,
      34,    -1,    36,    37,    -1,    39,    -1,    -1,    -1,    -1,
      44,    45,    -1,    47,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    56,    57,    -1,    59,    -1,    61,    62,    63,
      -1,    -1,    66
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    69,    71,     0,     3,     6,     8,     9,    11,    13,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    33,    34,    35,    36,    37,
      39,    44,    45,    47,    50,    56,    57,    59,    61,    62,
      63,    66,    70,    72,    73,    77,    80,    81,    90,    91,
      92,    93,    98,    99,   100,   101,   102,   103,   109,   110,
     115,   119,   121,   123,   125,   127,   129,   131,   133,   135,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   148,
     149,   153,   157,   158,    39,   156,   156,    39,    47,    72,
     140,   142,    77,     1,    66,   160,   160,   143,   144,   113,
     115,   160,   115,    47,    80,   140,    39,    82,    84,   140,
     160,   160,   140,   140,    51,    38,    39,    44,    45,    77,
      78,   154,   155,   140,   140,   140,   140,    48,    64,   150,
     151,   152,   115,   156,     1,    48,    50,   159,    32,    52,
      31,    53,    54,    55,    41,    14,    15,    43,    42,    56,
      57,    58,    59,    60,    33,    34,    36,    40,    49,   118,
      63,    65,    66,   146,    63,    65,   146,   159,   159,    38,
      39,    44,    45,    29,     1,   114,   115,    27,   116,   117,
     120,   122,   124,   126,   128,   130,   132,   134,   136,   137,
     142,   146,   159,   115,   159,    38,     5,    12,   111,   112,
      49,    86,    87,    48,   159,   114,   115,    77,    38,    77,
      38,    48,    51,    48,    64,    48,    64,   119,    67,   160,
     119,   125,   119,   127,   129,   131,   133,   135,   137,   137,
     137,   138,   139,   139,   140,   140,   140,   119,   119,   115,
      39,    67,   119,   147,   115,    39,   160,     1,    67,   161,
      39,    83,    85,    94,    48,    32,    52,    31,    53,    54,
      55,    41,    15,    43,    14,    49,   118,   161,   160,    80,
     112,   119,    84,   161,   161,   155,   119,   151,   152,    39,
      75,    76,    51,    64,    48,    67,    64,   114,    77,    49,
      88,    89,    48,    96,    14,     1,    50,   162,   120,   142,
     126,   120,   128,   130,   132,   134,   136,   137,   137,   114,
     120,   120,    47,   104,    39,    77,    77,    51,    64,   119,
      48,   161,   119,   119,   161,    10,   120,    85,   162,   114,
     113,    51,   161,     4,   105,   106,   107,   161,   119,    39,
      47,   159,    77,   113,   161,    95,   120,    77,   115,     7,
      38,   108,   107,    80,    71,    74,    97,    77,   162,    51,
      51,   105,    38,   162,   113,    78,    79,    79,    38,   113,
     161,   161,    77,    77
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
#line 258 "parser.y"
    { program_parsed(ctx, (yyvsp[(1) - (2)].source_elements)); ;}
    break;

  case 3:

/* Line 1455 of yacc.c  */
#line 261 "parser.y"
    {;}
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 262 "parser.y"
    {;}
    break;

  case 5:

/* Line 1455 of yacc.c  */
#line 266 "parser.y"
    { (yyval.source_elements) = new_source_elements(ctx); ;}
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 268 "parser.y"
    { (yyval.source_elements) = source_elements_add_statement((yyvsp[(1) - (2)].source_elements), (yyvsp[(2) - (2)].statement)); ;}
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 273 "parser.y"
    { (yyval.expr) = new_function_expression(ctx, (yyvsp[(2) - (8)].identifier), (yyvsp[(4) - (8)].parameter_list), (yyvsp[(7) - (8)].source_elements), (yyvsp[(1) - (8)].srcptr), (yyvsp[(8) - (8)].srcptr)-(yyvsp[(1) - (8)].srcptr)+1); ;}
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 276 "parser.y"
    { push_func(ctx); (yyval.srcptr) = (yyvsp[(1) - (1)].srcptr); ;}
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 280 "parser.y"
    { (yyval.source_elements) = function_body_parsed(ctx, (yyvsp[(1) - (1)].source_elements)); ;}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 284 "parser.y"
    { (yyval.parameter_list) = new_parameter_list(ctx, (yyvsp[(1) - (1)].identifier)); ;}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 286 "parser.y"
    { (yyval.parameter_list) = parameter_list_add(ctx, (yyvsp[(1) - (3)].parameter_list), (yyvsp[(3) - (3)].identifier)); ;}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 290 "parser.y"
    { (yyval.parameter_list) = NULL; ;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 291 "parser.y"
    { (yyval.parameter_list) = (yyvsp[(1) - (1)].parameter_list); ;}
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 295 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 296 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 297 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 298 "parser.y"
    { (yyval.statement) = new_empty_statement(ctx); ;}
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 299 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 300 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 301 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 302 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 303 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 304 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 305 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 306 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 307 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 308 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 309 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); ;}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 313 "parser.y"
    { (yyval.statement_list) = new_statement_list(ctx, (yyvsp[(1) - (1)].statement)); ;}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 315 "parser.y"
    { (yyval.statement_list) = statement_list_add((yyvsp[(1) - (2)].statement_list), (yyvsp[(2) - (2)].statement)); ;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 319 "parser.y"
    { (yyval.statement_list) = NULL; ;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 320 "parser.y"
    { (yyval.statement_list) = (yyvsp[(1) - (1)].statement_list); ;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 324 "parser.y"
    { (yyval.statement) = new_block_statement(ctx, (yyvsp[(2) - (3)].statement_list)); ;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 325 "parser.y"
    { (yyval.statement) = new_block_statement(ctx, NULL); ;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 330 "parser.y"
    { (yyval.statement) = new_var_statement(ctx, (yyvsp[(2) - (3)].variable_list)); ;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 334 "parser.y"
    { (yyval.variable_list) = new_variable_list(ctx, (yyvsp[(1) - (1)].variable_declaration)); ;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 336 "parser.y"
    { (yyval.variable_list) = variable_list_add(ctx, (yyvsp[(1) - (3)].variable_list), (yyvsp[(3) - (3)].variable_declaration)); ;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 341 "parser.y"
    { (yyval.variable_list) = new_variable_list(ctx, (yyvsp[(1) - (1)].variable_declaration)); ;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 343 "parser.y"
    { (yyval.variable_list) = variable_list_add(ctx, (yyvsp[(1) - (3)].variable_list), (yyvsp[(3) - (3)].variable_declaration)); ;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 348 "parser.y"
    { (yyval.variable_declaration) = new_variable_declaration(ctx, (yyvsp[(1) - (2)].identifier), (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 353 "parser.y"
    { (yyval.variable_declaration) = new_variable_declaration(ctx, (yyvsp[(1) - (2)].identifier), (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 357 "parser.y"
    { (yyval.expr) = NULL; ;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 358 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 363 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (2)].expr); ;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 367 "parser.y"
    { (yyval.expr) = NULL; ;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 368 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 373 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (2)].expr); ;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 377 "parser.y"
    { (yyval.statement) = new_empty_statement(ctx); ;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 382 "parser.y"
    { (yyval.statement) = new_expression_statement(ctx, (yyvsp[(1) - (2)].expr)); ;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 387 "parser.y"
    { (yyval.statement) = new_if_statement(ctx, (yyvsp[(3) - (7)].expr), (yyvsp[(5) - (7)].statement), (yyvsp[(7) - (7)].statement)); ;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 389 "parser.y"
    { (yyval.statement) = new_if_statement(ctx, (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].statement), NULL); ;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 394 "parser.y"
    { (yyval.statement) = new_while_statement(ctx, TRUE, (yyvsp[(5) - (7)].expr), (yyvsp[(2) - (7)].statement)); ;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 396 "parser.y"
    { (yyval.statement) = new_while_statement(ctx, FALSE, (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].statement)); ;}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 398 "parser.y"
    { if(!explicit_error(ctx, (yyvsp[(3) - (3)].expr), ';')) YYABORT; ;}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 400 "parser.y"
    { if(!explicit_error(ctx, (yyvsp[(6) - (6)].expr), ';')) YYABORT; ;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 402 "parser.y"
    { (yyval.statement) = new_for_statement(ctx, NULL, (yyvsp[(3) - (11)].expr), (yyvsp[(6) - (11)].expr), (yyvsp[(9) - (11)].expr), (yyvsp[(11) - (11)].statement)); ;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 404 "parser.y"
    { if(!explicit_error(ctx, (yyvsp[(4) - (4)].variable_list), ';')) YYABORT; ;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 406 "parser.y"
    { if(!explicit_error(ctx, (yyvsp[(7) - (7)].expr), ';')) YYABORT; ;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 408 "parser.y"
    { (yyval.statement) = new_for_statement(ctx, (yyvsp[(4) - (12)].variable_list), NULL, (yyvsp[(7) - (12)].expr), (yyvsp[(10) - (12)].expr), (yyvsp[(12) - (12)].statement)); ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 410 "parser.y"
    { (yyval.statement) = new_forin_statement(ctx, NULL, (yyvsp[(3) - (7)].expr), (yyvsp[(5) - (7)].expr), (yyvsp[(7) - (7)].statement)); ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 412 "parser.y"
    { (yyval.statement) = new_forin_statement(ctx, (yyvsp[(4) - (8)].variable_declaration), NULL, (yyvsp[(6) - (8)].expr), (yyvsp[(8) - (8)].statement)); ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 417 "parser.y"
    { (yyval.statement) = new_continue_statement(ctx, (yyvsp[(2) - (3)].identifier)); ;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 422 "parser.y"
    { (yyval.statement) = new_break_statement(ctx, (yyvsp[(2) - (3)].identifier)); ;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 427 "parser.y"
    { (yyval.statement) = new_return_statement(ctx, (yyvsp[(2) - (3)].expr)); ;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 432 "parser.y"
    { (yyval.statement) = new_with_statement(ctx, (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].statement)); ;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 437 "parser.y"
    { (yyval.statement) = new_labelled_statement(ctx, (yyvsp[(1) - (3)].identifier), (yyvsp[(3) - (3)].statement)); ;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 442 "parser.y"
    { (yyval.statement) = new_switch_statement(ctx, (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].case_clausule)); ;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 447 "parser.y"
    { (yyval.case_clausule) = new_case_block(ctx, (yyvsp[(2) - (3)].case_list), NULL, NULL); ;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 449 "parser.y"
    { (yyval.case_clausule) = new_case_block(ctx, (yyvsp[(2) - (5)].case_list), (yyvsp[(3) - (5)].case_clausule), (yyvsp[(4) - (5)].case_list)); ;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 453 "parser.y"
    { (yyval.case_list) = NULL; ;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 454 "parser.y"
    { (yyval.case_list) = (yyvsp[(1) - (1)].case_list); ;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 458 "parser.y"
    { (yyval.case_list) = new_case_list(ctx, (yyvsp[(1) - (1)].case_clausule)); ;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 460 "parser.y"
    { (yyval.case_list) = case_list_add(ctx, (yyvsp[(1) - (2)].case_list), (yyvsp[(2) - (2)].case_clausule)); ;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 465 "parser.y"
    { (yyval.case_clausule) = new_case_clausule(ctx, (yyvsp[(2) - (4)].expr), (yyvsp[(4) - (4)].statement_list)); ;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 470 "parser.y"
    { (yyval.case_clausule) = new_case_clausule(ctx, NULL, (yyvsp[(3) - (3)].statement_list)); ;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 475 "parser.y"
    { (yyval.statement) = new_throw_statement(ctx, (yyvsp[(2) - (3)].expr)); ;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 479 "parser.y"
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[(2) - (3)].statement), (yyvsp[(3) - (3)].catch_block), NULL); ;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 480 "parser.y"
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[(2) - (3)].statement), NULL, (yyvsp[(3) - (3)].statement)); ;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 482 "parser.y"
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[(2) - (4)].statement), (yyvsp[(3) - (4)].catch_block), (yyvsp[(4) - (4)].statement)); ;}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 487 "parser.y"
    { (yyval.catch_block) = new_catch_block(ctx, (yyvsp[(3) - (5)].identifier), (yyvsp[(5) - (5)].statement)); ;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 491 "parser.y"
    { (yyval.statement) = (yyvsp[(2) - (2)].statement); ;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 495 "parser.y"
    { (yyval.expr) = NULL; ;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 496 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 499 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 500 "parser.y"
    { set_error(ctx, IDS_SYNTAX_ERROR); YYABORT; ;}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 504 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 506 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_COMMA, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 510 "parser.y"
    { (yyval.expr) = NULL; ;}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 511 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 516 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 518 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_COMMA, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 521 "parser.y"
    { (yyval.ival) = (yyvsp[(1) - (1)].ival); ;}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 522 "parser.y"
    { (yyval.ival) = EXPR_ASSIGNDIV; ;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 526 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 528 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ASSIGN, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 530 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 535 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 537 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ASSIGN, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 539 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 543 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 545 "parser.y"
    { (yyval.expr) = new_conditional_expression(ctx, (yyvsp[(1) - (5)].expr), (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].expr)); ;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 550 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 552 "parser.y"
    { (yyval.expr) = new_conditional_expression(ctx, (yyvsp[(1) - (5)].expr), (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].expr)); ;}
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 556 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 558 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_OR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 563 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 565 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_OR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 569 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 571 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_AND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 576 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 578 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_AND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 582 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 584 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BOR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 589 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 591 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BOR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 595 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 597 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BXOR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 602 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 604 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BXOR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 608 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 610 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BAND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 615 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 617 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BAND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 621 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 623 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 627 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 629 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 633 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 635 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 637 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_INSTANCEOF, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 639 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_IN, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 643 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 645 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 647 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_INSTANCEOF, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 651 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 653 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 658 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 660 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ADD, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 662 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_SUB, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 666 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 668 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_MUL, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 670 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_DIV, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 672 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_MOD, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 676 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 678 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_DELETE, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 679 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_VOID, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 681 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_TYPEOF, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 682 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PREINC, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 683 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PREDEC, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 684 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PLUS, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 685 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_MINUS, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 686 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_BITNEG, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 687 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_LOGNEG, (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 692 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 694 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_POSTINC, (yyvsp[(1) - (2)].expr)); ;}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 696 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_POSTDEC, (yyvsp[(1) - (2)].expr)); ;}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 701 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 702 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 706 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 707 "parser.y"
    { (yyval.expr) = new_new_expression(ctx, (yyvsp[(2) - (2)].expr), NULL); ;}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 711 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 712 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 714 "parser.y"
    { (yyval.expr) = new_array_expression(ctx, (yyvsp[(1) - (4)].expr), (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 716 "parser.y"
    { (yyval.expr) = new_member_expression(ctx, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].identifier)); ;}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 718 "parser.y"
    { (yyval.expr) = new_new_expression(ctx, (yyvsp[(2) - (3)].expr), (yyvsp[(3) - (3)].argument_list)); ;}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 723 "parser.y"
    { (yyval.expr) = new_call_expression(ctx, (yyvsp[(1) - (2)].expr), (yyvsp[(2) - (2)].argument_list)); ;}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 725 "parser.y"
    { (yyval.expr) = new_call_expression(ctx, (yyvsp[(1) - (2)].expr), (yyvsp[(2) - (2)].argument_list)); ;}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 727 "parser.y"
    { (yyval.expr) = new_array_expression(ctx, (yyvsp[(1) - (4)].expr), (yyvsp[(3) - (4)].expr)); ;}
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 729 "parser.y"
    { (yyval.expr) = new_member_expression(ctx, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].identifier)); ;}
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 733 "parser.y"
    { (yyval.argument_list) = NULL; ;}
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 734 "parser.y"
    { (yyval.argument_list) = (yyvsp[(2) - (3)].argument_list); ;}
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 738 "parser.y"
    { (yyval.argument_list) = new_argument_list(ctx, (yyvsp[(1) - (1)].expr)); ;}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 740 "parser.y"
    { (yyval.argument_list) = argument_list_add(ctx, (yyvsp[(1) - (3)].argument_list), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 744 "parser.y"
    { (yyval.expr) = new_this_expression(ctx); ;}
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 745 "parser.y"
    { (yyval.expr) = new_identifier_expression(ctx, (yyvsp[(1) - (1)].identifier)); ;}
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 746 "parser.y"
    { (yyval.expr) = new_literal_expression(ctx, (yyvsp[(1) - (1)].literal)); ;}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 747 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 748 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); ;}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 749 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (3)].expr); ;}
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 753 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, NULL, 0); ;}
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 754 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, NULL, (yyvsp[(2) - (3)].ival)+1); ;}
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 755 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, (yyvsp[(2) - (3)].element_list), 0); ;}
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 757 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, (yyvsp[(2) - (5)].element_list), (yyvsp[(4) - (5)].ival)+1); ;}
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 762 "parser.y"
    { (yyval.element_list) = new_element_list(ctx, (yyvsp[(1) - (2)].ival), (yyvsp[(2) - (2)].expr)); ;}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 764 "parser.y"
    { (yyval.element_list) = element_list_add(ctx, (yyvsp[(1) - (4)].element_list), (yyvsp[(3) - (4)].ival), (yyvsp[(4) - (4)].expr)); ;}
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 768 "parser.y"
    { (yyval.ival) = 1; ;}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 769 "parser.y"
    { (yyval.ival) = (yyvsp[(1) - (2)].ival) + 1; ;}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 773 "parser.y"
    { (yyval.ival) = 0; ;}
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 774 "parser.y"
    { (yyval.ival) = (yyvsp[(1) - (1)].ival); ;}
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 778 "parser.y"
    { (yyval.expr) = new_prop_and_value_expression(ctx, NULL); ;}
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 780 "parser.y"
    { (yyval.expr) = new_prop_and_value_expression(ctx, (yyvsp[(2) - (3)].property_list)); ;}
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 785 "parser.y"
    { (yyval.property_list) = new_property_list(ctx, (yyvsp[(1) - (3)].literal), (yyvsp[(3) - (3)].expr)); ;}
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 787 "parser.y"
    { (yyval.property_list) = property_list_add(ctx, (yyvsp[(1) - (5)].property_list), (yyvsp[(3) - (5)].literal), (yyvsp[(5) - (5)].expr)); ;}
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 791 "parser.y"
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[(1) - (1)].identifier)); ;}
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 792 "parser.y"
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[(1) - (1)].wstr)); ;}
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 793 "parser.y"
    { (yyval.literal) = (yyvsp[(1) - (1)].literal); ;}
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 797 "parser.y"
    { (yyval.identifier) = NULL; ;}
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 798 "parser.y"
    { (yyval.identifier) = (yyvsp[(1) - (1)].identifier); ;}
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 802 "parser.y"
    { (yyval.literal) = new_null_literal(ctx); ;}
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 803 "parser.y"
    { (yyval.literal) = new_undefined_literal(ctx); ;}
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 804 "parser.y"
    { (yyval.literal) = (yyvsp[(1) - (1)].literal); ;}
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 805 "parser.y"
    { (yyval.literal) = (yyvsp[(1) - (1)].literal); ;}
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 806 "parser.y"
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[(1) - (1)].wstr)); ;}
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 807 "parser.y"
    { (yyval.literal) = parse_regexp(ctx);
                                  if(!(yyval.literal)) YYABORT; ;}
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 809 "parser.y"
    { (yyval.literal) = parse_regexp(ctx);
                                  if(!(yyval.literal)) YYABORT; ;}
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 814 "parser.y"
    { (yyval.literal) = new_boolean_literal(ctx, VARIANT_TRUE); ;}
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 815 "parser.y"
    { (yyval.literal) = new_boolean_literal(ctx, VARIANT_FALSE); ;}
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 819 "parser.y"
    { if(!allow_auto_semicolon(ctx)) {YYABORT;} ;}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 823 "parser.y"
    { set_error(ctx, IDS_LBRACKET); YYABORT; ;}
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 827 "parser.y"
    { set_error(ctx, IDS_RBRACKET); YYABORT; ;}
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 831 "parser.y"
    { set_error(ctx, IDS_SEMICOLON); YYABORT; ;}
    break;



/* Line 1455 of yacc.c  */
#line 3509 "parser.tab.c"
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
#line 833 "parser.y"


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

