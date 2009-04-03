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
#define YYPURE 1

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
     kFUNCTION = 290,
     tIdentifier = 291,
     tAssignOper = 292,
     tEqOper = 293,
     tShiftOper = 294,
     tRelOper = 295,
     tNumericLiteral = 296,
     tStringLiteral = 297,
     LOWER_THAN_ELSE = 298
   };
#endif
/* Tokens.  */
#define kBREAK 258
#define kCASE 259
#define kCATCH 260
#define kCONTINUE 261
#define kDEFAULT 262
#define kDELETE 263
#define kDO 264
#define kELSE 265
#define kIF 266
#define kFINALLY 267
#define kFOR 268
#define kIN 269
#define kINSTANCEOF 270
#define kNEW 271
#define kNULL 272
#define kUNDEFINED 273
#define kRETURN 274
#define kSWITCH 275
#define kTHIS 276
#define kTHROW 277
#define kTRUE 278
#define kFALSE 279
#define kTRY 280
#define kTYPEOF 281
#define kVAR 282
#define kVOID 283
#define kWHILE 284
#define kWITH 285
#define tANDAND 286
#define tOROR 287
#define tINC 288
#define tDEC 289
#define kFUNCTION 290
#define tIdentifier 291
#define tAssignOper 292
#define tEqOper 293
#define tShiftOper 294
#define tRelOper 295
#define tNumericLiteral 296
#define tStringLiteral 297
#define LOWER_THAN_ELSE 298




/* Copy the first part of user declarations.  */
#line 19 "parser.y"


#include "jscript.h"
#include "engine.h"

#define YYLEX_PARAM ctx
#define YYPARSE_PARAM ctx

static int parser_error(const char*);
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
#line 149 "parser.y"
typedef union YYSTYPE {
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
} YYSTYPE;
/* Line 196 of yacc.c.  */
#line 327 "parser.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */
#line 339 "parser.tab.c"

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
#define YYLAST   818

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  66
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  85
/* YYNRULES -- Number of rules. */
#define YYNRULES  197
/* YYNRULES -- Number of states. */
#define YYNSTATES  356

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   298

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    62,     2,     2,     2,    60,    55,     2,
      45,    46,    58,    56,    48,    57,    65,    59,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    51,    50,
       2,    49,     2,    52,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    63,     2,    64,    54,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    47,    53,    35,    61,     2,     2,     2,
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
      36,    37,    38,    39,    40,    41,    42,    43,    44
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,     5,     6,     9,    18,    20,    22,    24,
      28,    29,    31,    33,    35,    37,    39,    41,    43,    45,
      47,    49,    51,    53,    55,    57,    59,    61,    64,    65,
      67,    71,    74,    78,    80,    84,    86,    90,    93,    96,
      97,    99,   102,   103,   105,   108,   110,   113,   121,   127,
     135,   141,   151,   162,   170,   179,   183,   187,   191,   197,
     201,   207,   211,   217,   218,   220,   222,   225,   230,   234,
     238,   242,   246,   251,   257,   260,   261,   263,   265,   269,
     270,   272,   274,   278,   280,   284,   288,   290,   294,   298,
     300,   306,   308,   314,   316,   320,   322,   326,   328,   332,
     334,   338,   340,   344,   346,   350,   352,   356,   358,   362,
     364,   368,   370,   374,   376,   380,   382,   386,   388,   392,
     396,   400,   402,   406,   410,   412,   416,   418,   422,   426,
     428,   432,   436,   440,   442,   445,   448,   451,   454,   457,
     460,   463,   466,   469,   471,   474,   477,   479,   481,   483,
     486,   488,   490,   495,   499,   503,   506,   509,   514,   518,
     521,   525,   527,   531,   533,   535,   537,   539,   541,   545,
     548,   552,   556,   562,   565,   570,   572,   575,   576,   578,
     581,   585,   589,   595,   597,   599,   601,   602,   604,   606,
     608,   610,   612,   614,   616,   618,   620,   622
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short int yyrhs[] =
{
      67,     0,    -1,    68,    -1,    -1,    68,    74,    -1,    70,
     147,    45,    73,    46,    47,    71,    35,    -1,    36,    -1,
      68,    -1,    37,    -1,    72,    48,    37,    -1,    -1,    72,
      -1,    77,    -1,    78,    -1,    87,    -1,    88,    -1,    89,
      -1,    90,    -1,    91,    -1,    92,    -1,    93,    -1,    94,
      -1,    95,    -1,    96,    -1,   102,    -1,   103,    -1,    74,
      -1,    75,    74,    -1,    -1,    75,    -1,    47,    75,    35,
      -1,    47,    35,    -1,    27,    79,   150,    -1,    81,    -1,
      79,    48,    81,    -1,    82,    -1,    80,    48,    82,    -1,
      37,    83,    -1,    37,    85,    -1,    -1,    84,    -1,    49,
     110,    -1,    -1,    86,    -1,    49,   111,    -1,    50,    -1,
     107,   150,    -1,    11,    45,   107,    46,    74,    10,    74,
      -1,    11,    45,   107,    46,    74,    -1,     9,    74,    29,
      45,   107,    46,    50,    -1,    29,    45,   107,    46,    74,
      -1,    13,    45,   108,    50,   106,    50,   106,    46,    74,
      -1,    13,    45,    27,    80,    50,   106,    50,   106,    46,
      74,    -1,    13,    45,   133,    14,   107,    46,    74,    -1,
      13,    45,    27,    82,    14,   107,    46,    74,    -1,     6,
     147,   150,    -1,     3,   147,   150,    -1,    19,   106,   150,
      -1,    30,    45,   107,    46,    74,    -1,    37,    51,    74,
      -1,    20,    45,   107,    46,    97,    -1,    47,    98,    35,
      -1,    47,    98,   101,    98,    35,    -1,    -1,    99,    -1,
     100,    -1,    99,   100,    -1,     4,   107,    51,    76,    -1,
       7,    51,    76,    -1,    22,   107,   150,    -1,    25,    77,
     104,    -1,    25,    77,   105,    -1,    25,    77,   104,   105,
      -1,     5,    45,    37,    46,    77,    -1,    12,    77,    -1,
      -1,   107,    -1,   110,    -1,   107,    48,   110,    -1,    -1,
     109,    -1,   111,    -1,   109,    48,   111,    -1,   112,    -1,
     133,    49,   110,    -1,   133,    38,   110,    -1,   113,    -1,
     133,    49,   111,    -1,   133,    38,   111,    -1,   114,    -1,
     114,    52,   110,    51,   110,    -1,   115,    -1,   115,    52,
     111,    51,   111,    -1,   116,    -1,   114,    32,   116,    -1,
     117,    -1,   115,    32,   117,    -1,   118,    -1,   116,    31,
     118,    -1,   119,    -1,   117,    31,   119,    -1,   120,    -1,
     118,    53,   120,    -1,   121,    -1,   119,    53,   121,    -1,
     122,    -1,   120,    54,   122,    -1,   123,    -1,   121,    54,
     123,    -1,   124,    -1,   122,    55,   124,    -1,   125,    -1,
     123,    55,   125,    -1,   126,    -1,   124,    39,   126,    -1,
     127,    -1,   125,    39,   127,    -1,   128,    -1,   126,    41,
     128,    -1,   126,    15,   128,    -1,   126,    14,   128,    -1,
     128,    -1,   127,    41,   128,    -1,   127,    15,   128,    -1,
     129,    -1,   128,    40,   129,    -1,   130,    -1,   129,    56,
     130,    -1,   129,    57,   130,    -1,   131,    -1,   130,    58,
     131,    -1,   130,    59,   131,    -1,   130,    60,   131,    -1,
     132,    -1,     8,   131,    -1,    28,   131,    -1,    26,   131,
      -1,    33,   131,    -1,    34,   131,    -1,    56,   131,    -1,
      57,   131,    -1,    61,   131,    -1,    62,   131,    -1,   133,
      -1,   133,    33,    -1,   133,    34,    -1,   134,    -1,   136,
      -1,   135,    -1,    16,   134,    -1,   139,    -1,    69,    -1,
     135,    63,   107,    64,    -1,   135,    65,    37,    -1,    16,
     135,   137,    -1,   135,   137,    -1,   136,   137,    -1,   136,
      63,   107,    64,    -1,   136,    65,    37,    -1,    45,    46,
      -1,    45,   138,    46,    -1,   110,    -1,   138,    48,   110,
      -1,    21,    -1,    37,    -1,   148,    -1,   140,    -1,   144,
      -1,    45,   107,    46,    -1,    63,    64,    -1,    63,   142,
      64,    -1,    63,   141,    64,    -1,    63,   141,    48,   143,
      64,    -1,   143,   110,    -1,   141,    48,   143,   110,    -1,
      48,    -1,   142,    48,    -1,    -1,   142,    -1,    47,    35,
      -1,    47,   145,    35,    -1,   146,    51,   110,    -1,   145,
      48,   146,    51,   110,    -1,    37,    -1,    43,    -1,    42,
      -1,    -1,    37,    -1,    17,    -1,    18,    -1,   149,    -1,
      42,    -1,    43,    -1,    59,    -1,    23,    -1,    24,    -1,
      50,    -1,     1,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   254,   254,   258,   259,   264,   268,   272,   276,   277,
     282,   283,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   304,   305,   310,   311,
     315,   316,   320,   325,   326,   331,   333,   338,   343,   348,
     349,   353,   358,   359,   363,   368,   372,   377,   379,   384,
     386,   388,   390,   392,   394,   399,   404,   409,   414,   419,
     424,   429,   431,   436,   437,   441,   442,   447,   452,   457,
     462,   463,   464,   469,   474,   478,   479,   483,   484,   489,
     490,   494,   496,   501,   502,   504,   509,   511,   513,   518,
     519,   524,   526,   531,   532,   537,   539,   544,   545,   550,
     552,   557,   558,   563,   565,   570,   571,   576,   578,   583,
     584,   589,   591,   596,   597,   602,   603,   608,   609,   611,
     613,   618,   619,   621,   626,   627,   632,   634,   636,   641,
     642,   644,   646,   651,   652,   654,   655,   657,   658,   659,
     660,   661,   662,   666,   668,   670,   676,   677,   681,   682,
     686,   687,   688,   690,   692,   697,   699,   701,   703,   708,
     709,   713,   714,   719,   720,   721,   722,   723,   724,   728,
     729,   730,   731,   736,   738,   743,   744,   748,   749,   753,
     754,   759,   761,   766,   767,   768,   772,   773,   777,   778,
     779,   780,   781,   782,   787,   788,   791,   792
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "kBREAK", "kCASE", "kCATCH", "kCONTINUE",
  "kDEFAULT", "kDELETE", "kDO", "kELSE", "kIF", "kFINALLY", "kFOR", "kIN",
  "kINSTANCEOF", "kNEW", "kNULL", "kUNDEFINED", "kRETURN", "kSWITCH",
  "kTHIS", "kTHROW", "kTRUE", "kFALSE", "kTRY", "kTYPEOF", "kVAR", "kVOID",
  "kWHILE", "kWITH", "tANDAND", "tOROR", "tINC", "tDEC", "'}'",
  "kFUNCTION", "tIdentifier", "tAssignOper", "tEqOper", "tShiftOper",
  "tRelOper", "tNumericLiteral", "tStringLiteral", "LOWER_THAN_ELSE",
  "'('", "')'", "'{'", "','", "'='", "';'", "':'", "'?'", "'|'", "'^'",
  "'&'", "'+'", "'-'", "'*'", "'/'", "'%'", "'~'", "'!'", "'['", "']'",
  "'.'", "$accept", "Program", "SourceElements", "FunctionExpression",
  "KFunction", "FunctionBody", "FormalParameterList",
  "FormalParameterList_opt", "Statement", "StatementList",
  "StatementList_opt", "Block", "VariableStatement",
  "VariableDeclarationList", "VariableDeclarationListNoIn",
  "VariableDeclaration", "VariableDeclarationNoIn", "Initialiser_opt",
  "Initialiser", "InitialiserNoIn_opt", "InitialiserNoIn",
  "EmptyStatement", "ExpressionStatement", "IfStatement",
  "IterationStatement", "ContinueStatement", "BreakStatement",
  "ReturnStatement", "WithStatement", "LabelledStatement",
  "SwitchStatement", "CaseBlock", "CaseClausules_opt", "CaseClausules",
  "CaseClausule", "DefaultClausule", "ThrowStatement", "TryStatement",
  "Catch", "Finally", "Expression_opt", "Expression", "ExpressionNoIn_opt",
  "ExpressionNoIn", "AssignmentExpression", "AssignmentExpressionNoIn",
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
  "BooleanLiteral", "semicolon_opt", 0
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
     285,   286,   287,   288,   289,   125,   290,   291,   292,   293,
     294,   295,   296,   297,   298,    40,    41,   123,    44,    61,
      59,    58,    63,   124,    94,    38,    43,    45,    42,    47,
      37,   126,    33,    91,    93,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    66,    67,    68,    68,    69,    70,    71,    72,    72,
      73,    73,    74,    74,    74,    74,    74,    74,    74,    74,
      74,    74,    74,    74,    74,    74,    75,    75,    76,    76,
      77,    77,    78,    79,    79,    80,    80,    81,    82,    83,
      83,    84,    85,    85,    86,    87,    88,    89,    89,    90,
      90,    90,    90,    90,    90,    91,    92,    93,    94,    95,
      96,    97,    97,    98,    98,    99,    99,   100,   101,   102,
     103,   103,   103,   104,   105,   106,   106,   107,   107,   108,
     108,   109,   109,   110,   110,   110,   111,   111,   111,   112,
     112,   113,   113,   114,   114,   115,   115,   116,   116,   117,
     117,   118,   118,   119,   119,   120,   120,   121,   121,   122,
     122,   123,   123,   124,   124,   125,   125,   126,   126,   126,
     126,   127,   127,   127,   128,   128,   129,   129,   129,   130,
     130,   130,   130,   131,   131,   131,   131,   131,   131,   131,
     131,   131,   131,   132,   132,   132,   133,   133,   134,   134,
     135,   135,   135,   135,   135,   136,   136,   136,   136,   137,
     137,   138,   138,   139,   139,   139,   139,   139,   139,   140,
     140,   140,   140,   141,   141,   142,   142,   143,   143,   144,
     144,   145,   145,   146,   146,   146,   147,   147,   148,   148,
     148,   148,   148,   148,   149,   149,   150,   150
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     0,     2,     8,     1,     1,     1,     3,
       0,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     0,     1,
       3,     2,     3,     1,     3,     1,     3,     2,     2,     0,
       1,     2,     0,     1,     2,     1,     2,     7,     5,     7,
       5,     9,    10,     7,     8,     3,     3,     3,     5,     3,
       5,     3,     5,     0,     1,     1,     2,     4,     3,     3,
       3,     3,     4,     5,     2,     0,     1,     1,     3,     0,
       1,     1,     3,     1,     3,     3,     1,     3,     3,     1,
       5,     1,     5,     1,     3,     1,     3,     1,     3,     1,
       3,     1,     3,     1,     3,     1,     3,     1,     3,     1,
       3,     1,     3,     1,     3,     1,     3,     1,     3,     3,
       3,     1,     3,     3,     1,     3,     1,     3,     3,     1,
       3,     3,     3,     1,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     1,     2,     2,     1,     1,     1,     2,
       1,     1,     4,     3,     3,     2,     2,     4,     3,     2,
       3,     1,     3,     1,     1,     1,     1,     1,     3,     2,
       3,     3,     5,     2,     4,     1,     2,     0,     1,     2,
       3,     3,     5,     1,     1,     1,     0,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       3,     0,     2,     1,   186,   186,     0,     0,     0,     0,
       0,   188,   189,    75,     0,   163,     0,   194,   195,     0,
       0,     0,     0,     0,     0,     0,     0,     6,   164,   191,
     192,     0,     0,    45,     0,     0,   193,     0,     0,   177,
     151,   186,     4,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,     0,    77,    83,
      89,    93,    97,   101,   105,   109,   113,   117,   124,   126,
     129,   133,   143,   146,   148,   147,   150,   166,   167,   165,
     190,   187,     0,     0,   164,     0,   134,   143,     0,     0,
      79,   149,   148,     0,    76,     0,     0,     0,     0,   136,
      39,     0,    33,   135,     0,     0,   137,   138,     0,     0,
      31,   164,   191,   192,    26,     0,     0,     0,   139,   140,
     141,   142,   175,   169,     0,   178,     0,     0,   197,     0,
     196,    46,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   144,   145,
       0,     0,     0,     0,     0,   155,     0,     0,   156,    56,
      55,   179,   183,   185,   184,     0,     0,     0,     0,    80,
      81,    86,    91,    95,    99,   103,   107,   111,   115,   121,
     143,   154,    57,     0,    69,    31,     0,     0,    70,    71,
       0,    37,    40,     0,    32,     0,     0,    59,   168,    30,
      27,   180,     0,     0,   177,   171,   176,   170,   173,    10,
      78,    94,     0,    98,   102,   106,   110,   114,   120,   119,
     118,   125,   127,   128,   130,   131,   132,    85,    84,   159,
     161,     0,     0,   153,     0,   158,     0,     0,    42,     0,
      35,    75,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    74,    72,    41,
      34,     0,     0,     0,   181,   178,     0,     8,    11,     0,
       0,   160,     0,   152,   157,     0,    48,     0,    38,    43,
       0,    75,     0,     0,    82,   143,    96,     0,   100,   104,
     108,   112,   116,   123,   122,     0,    88,    87,    63,    60,
       0,    50,    58,     0,   172,   174,     0,     0,    90,   162,
       0,     0,    44,    36,     0,     0,    75,     0,     0,     0,
       0,    64,    65,     0,   182,     9,     3,    49,    47,    75,
       0,     0,    92,    53,     0,     0,    61,    63,    66,    73,
       7,     0,     0,    54,     0,    28,    28,     0,     5,     0,
      51,    29,    67,    68,    62,    52
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,     1,     2,    40,    41,   341,   268,   269,   114,   115,
     352,    43,    44,   101,   239,   102,   240,   191,   192,   278,
     279,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,   299,   320,   321,   322,   337,    55,    56,   188,   189,
      93,    57,   168,   169,    58,   170,    59,   171,    60,   172,
      61,   173,    62,   174,    63,   175,    64,   176,    65,   177,
      66,   178,    67,    68,    69,    70,    71,    72,    73,    74,
      75,   155,   231,    76,    77,   124,   125,   126,    78,   116,
     117,    82,    79,    80,   131
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -236
static const short int yypact[] =
{
    -236,    13,   555,  -236,   -19,   -19,   755,   555,    -1,    31,
      87,  -236,  -236,   755,    37,  -236,   755,  -236,  -236,   -21,
     755,    34,   755,    54,    62,   755,   755,  -236,    77,  -236,
    -236,   755,   372,  -236,   755,   755,  -236,   755,   755,    26,
    -236,   -19,  -236,  -236,  -236,  -236,  -236,  -236,  -236,  -236,
    -236,  -236,  -236,  -236,  -236,  -236,  -236,     9,  -236,  -236,
      20,    16,   100,   104,   107,   140,     0,   182,   138,    97,
    -236,  -236,    51,  -236,   -20,   -15,  -236,  -236,  -236,  -236,
    -236,  -236,    11,    11,  -236,   117,  -236,   181,   175,   755,
     655,  -236,   -20,    11,   173,   755,     9,   433,   126,  -236,
     174,    10,  -236,  -236,   755,   755,  -236,  -236,   555,    22,
     319,    77,   176,   177,  -236,   494,   -28,   179,  -236,  -236,
    -236,  -236,  -236,  -236,    27,    61,   755,   180,  -236,   755,
    -236,  -236,   755,   755,   755,   755,   755,   755,   755,   755,
     755,   755,   755,   755,   755,   755,   755,   755,  -236,  -236,
     755,   755,   705,   755,   189,  -236,   755,   194,  -236,  -236,
    -236,  -236,  -236,  -236,  -236,   187,    49,   196,   184,   188,
    -236,  -236,    21,   206,   185,   190,   186,   203,     1,   182,
      84,  -236,  -236,    68,  -236,  -236,   198,   -21,   233,  -236,
     755,  -236,  -236,    34,  -236,   132,   152,  -236,  -236,  -236,
    -236,  -236,   154,   755,   199,  -236,  -236,  -236,  -236,   209,
    -236,    16,   197,   100,   104,   107,   140,     0,   182,   182,
     182,   138,    97,    97,  -236,  -236,  -236,  -236,  -236,  -236,
    -236,   153,    73,  -236,    79,  -236,   755,   555,   200,   155,
     236,   755,   755,   755,   755,   755,   755,   755,   755,   755,
     755,   755,   755,   755,   755,   204,   215,  -236,  -236,  -236,
    -236,   555,   555,   202,  -236,   207,   605,  -236,   210,   208,
     755,  -236,   755,  -236,  -236,   161,   247,   755,  -236,  -236,
     196,   755,   755,   211,  -236,   102,   206,   212,   185,   190,
     186,   203,     1,   182,   182,   164,  -236,  -236,   258,  -236,
     218,  -236,  -236,   755,  -236,  -236,   228,   219,  -236,  -236,
     217,   555,  -236,  -236,   231,   165,   755,   755,   555,   755,
      -4,   258,  -236,   -21,  -236,  -236,  -236,  -236,  -236,   755,
     555,   222,  -236,  -236,    94,   234,  -236,   258,  -236,  -236,
     555,   249,   240,  -236,   555,   555,   555,   252,  -236,   555,
    -236,   555,  -236,  -236,  -236,  -236
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -236,  -236,   -38,  -236,  -236,  -236,  -236,  -236,    -2,  -128,
     -57,   -10,  -236,  -236,  -236,    98,    12,  -236,  -236,  -236,
    -236,  -236,  -236,  -236,  -236,  -236,  -236,  -236,  -236,  -236,
    -236,  -236,   -47,  -236,   -27,  -236,  -236,  -236,  -236,   105,
    -235,   -12,  -236,  -236,   -64,  -221,  -236,  -236,  -236,  -236,
     163,    53,   166,    52,   167,    55,   162,    56,   168,    60,
     172,    50,   -61,   169,    76,     2,  -236,    29,   302,   304,
    -236,   -58,  -236,  -236,  -236,  -236,   111,   114,  -236,  -236,
     119,    -3,  -236,  -236,    19
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -186
static const short int yytable[] =
{
      42,    94,    83,   335,    96,    88,   283,   201,    86,    98,
     128,   128,   128,     3,   139,   140,   250,   158,    81,   109,
     202,   284,    99,   287,   103,   152,    97,   106,   107,   179,
     152,   336,   296,   297,   181,    87,   118,   119,   127,   120,
     121,   141,   251,   153,    89,   154,   314,   134,   156,    87,
     157,    87,   132,   243,    87,    87,   312,   129,   193,   130,
     130,   130,   208,    87,    87,   210,    87,    87,   198,   212,
     129,   100,   133,   244,   122,   204,    90,   166,   218,   219,
     220,   331,    95,   183,   148,   149,   227,   228,   230,   150,
     123,   205,   195,   196,   342,   237,   332,   129,   252,   104,
     151,   159,   160,    10,    11,    12,   197,   105,    15,   206,
      17,    18,   182,   200,   255,   184,   129,   148,   149,   180,
     194,   129,   253,    27,    84,   207,   259,   129,   108,    29,
      30,   186,    31,   254,    85,   148,   149,   273,   187,   264,
     253,   232,   129,   274,   234,   345,    36,   224,   225,   226,
      39,   254,   161,   135,   162,   145,   146,   147,   136,   163,
     164,    87,   137,    87,    87,    87,    87,    87,    87,    87,
      87,    87,    87,    87,    87,    87,    87,   257,   261,   138,
     129,   179,   179,   179,   179,   179,   179,   179,   179,   293,
     294,   162,   179,   179,   143,   144,   163,   164,   262,   271,
     129,   272,   305,   280,   165,   281,   308,   310,   309,   129,
     318,   330,   129,   129,   148,   149,   179,   351,   351,   222,
     223,   129,   142,   190,   275,   209,   233,  -185,  -184,    94,
     203,   235,   236,   238,   241,   276,   242,   245,   246,   324,
     295,   248,   249,   256,   247,   187,   267,   122,   270,   277,
     282,   298,   300,   303,   307,   206,   179,   311,   306,   301,
     302,   316,   319,   317,   323,   325,   326,   327,   344,    94,
     315,   285,    87,   285,    87,    87,    87,    87,    87,    87,
      87,   329,   285,   285,   348,   346,   349,   354,   340,   353,
     347,   260,   313,   258,   338,   211,   286,   288,   215,   292,
     213,   289,   214,   290,    94,   216,   285,   334,   291,   328,
     217,   221,    91,   339,    92,   265,   333,    94,   266,     0,
    -179,   263,     0,     0,     0,     0,     0,     0,   343,     0,
       0,     0,     0,  -179,  -179,     0,     0,     0,    42,     0,
       0,     0,   350,     0,     0,     0,   285,   355,     0,   200,
    -179,  -179,     0,     0,     0,     0,     0,  -179,  -179,  -179,
    -179,     0,     0,     0,     0,     0,     0,  -179,  -179,     0,
       0,  -179,  -179,  -179,  -179,     4,     0,  -179,     5,  -179,
       6,     7,     0,     8,  -179,     9,     0,     0,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,     0,     0,    25,    26,   110,    27,   111,
       0,     0,     0,     0,   112,   113,     0,    31,     0,    32,
       0,     0,    33,     0,     0,     0,     0,     0,    34,    35,
       0,    36,     0,    37,    38,    39,     4,     0,     0,     5,
       0,     6,     7,     0,     8,     0,     9,     0,     0,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,     0,     0,    25,    26,   185,    27,
      28,     0,     0,     0,     0,    29,    30,     0,    31,     0,
      32,     0,     0,    33,     0,     0,     0,     0,     0,    34,
      35,     0,    36,     0,    37,    38,    39,     4,     0,     0,
       5,     0,     6,     7,     0,     8,     0,     9,     0,     0,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,     0,     0,    25,    26,   199,
      27,    28,     0,     0,     0,     0,    29,    30,     0,    31,
       0,    32,     0,     0,    33,     0,     0,     0,     0,     0,
      34,    35,     0,    36,     0,    37,    38,    39,     4,     0,
       0,     5,     0,     6,     7,     0,     8,     0,     9,     0,
       0,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,     0,     0,    25,    26,
       0,    27,    28,     0,     0,     0,     0,    29,    30,     0,
      31,     0,    32,     0,     0,    33,     0,     0,     0,     0,
       0,    34,    35,     6,    36,     0,    37,    38,    39,     0,
       0,    10,    11,    12,     0,     0,    15,     0,    17,    18,
       0,    20,     0,    22,     0,     0,     0,     0,    25,    26,
       0,    27,    84,     0,     0,     0,     0,    29,    30,     0,
      31,     0,    85,     0,     0,     0,     0,     0,     0,     0,
       0,    34,    35,     6,    36,     0,    37,    38,    39,   304,
       0,    10,    11,    12,     0,     0,    15,     0,    17,    18,
       0,    20,   167,    22,     0,     0,     0,     0,    25,    26,
       0,    27,    84,     0,     0,     0,     0,    29,    30,     0,
      31,     0,    85,     0,     0,     0,     0,     0,     0,     0,
       0,    34,    35,     6,    36,     0,    37,    38,    39,     0,
       0,    10,    11,    12,     0,     0,    15,     0,    17,    18,
       0,    20,     0,    22,     0,     0,     0,     0,    25,    26,
       0,    27,    84,     0,     0,     0,     0,    29,    30,     0,
      31,   229,    85,     0,     0,     0,     0,     0,     0,     0,
       0,    34,    35,     6,    36,     0,    37,    38,    39,     0,
       0,    10,    11,    12,     0,     0,    15,     0,    17,    18,
       0,    20,     0,    22,     0,     0,     0,     0,    25,    26,
       0,    27,    84,     0,     0,     0,     0,    29,    30,     0,
      31,     0,    85,     0,     0,     0,     0,     0,     0,     0,
       0,    34,    35,     0,    36,     0,    37,    38,    39
};

static const short int yycheck[] =
{
       2,    13,     5,     7,    16,     7,   241,    35,     6,    19,
       1,     1,     1,     0,    14,    15,    15,    75,    37,    31,
      48,   242,    20,   244,    22,    45,    47,    25,    26,    90,
      45,    35,   253,   254,    92,     6,    34,    35,    41,    37,
      38,    41,    41,    63,    45,    65,   281,    31,    63,    20,
      65,    22,    32,    32,    25,    26,   277,    48,    48,    50,
      50,    50,   126,    34,    35,   129,    37,    38,    46,   133,
      48,    37,    52,    52,    48,    48,    45,    89,   139,   140,
     141,   316,    45,    95,    33,    34,   150,   151,   152,    38,
      64,    64,   104,   105,   329,    46,   317,    48,    14,    45,
      49,    82,    83,    16,    17,    18,   108,    45,    21,    48,
      23,    24,    93,   115,    46,    96,    48,    33,    34,    90,
     101,    48,    38,    36,    37,    64,   190,    48,    51,    42,
      43,     5,    45,    49,    47,    33,    34,    64,    12,   203,
      38,   153,    48,    64,   156,    51,    59,   145,   146,   147,
      63,    49,    35,    53,    37,    58,    59,    60,    54,    42,
      43,   132,    55,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   147,   187,    46,    39,
      48,   242,   243,   244,   245,   246,   247,   248,   249,   250,
     251,    37,   253,   254,    56,    57,    42,    43,    46,    46,
      48,    48,   266,    48,    29,    50,   270,    46,   272,    48,
      46,    46,    48,    48,    33,    34,   277,   345,   346,   143,
     144,    48,    40,    49,   236,    45,    37,    51,    51,   241,
      51,    37,    45,    37,    50,   237,    48,    31,    53,   303,
     252,    55,    39,    45,    54,    12,    37,    48,    51,    49,
      14,    47,    37,    51,    46,    48,   317,    10,    48,   261,
     262,    50,     4,    51,    46,    37,    47,    50,    46,   281,
     282,   242,   243,   244,   245,   246,   247,   248,   249,   250,
     251,    50,   253,   254,    35,    51,    46,    35,   326,   346,
     337,   193,   280,   188,   321,   132,   243,   245,   136,   249,
     134,   246,   135,   247,   316,   137,   277,   319,   248,   311,
     138,   142,    10,   323,    10,   204,   318,   329,   204,    -1,
       1,   202,    -1,    -1,    -1,    -1,    -1,    -1,   330,    -1,
      -1,    -1,    -1,    14,    15,    -1,    -1,    -1,   340,    -1,
      -1,    -1,   344,    -1,    -1,    -1,   317,   349,    -1,   351,
      31,    32,    -1,    -1,    -1,    -1,    -1,    38,    39,    40,
      41,    -1,    -1,    -1,    -1,    -1,    -1,    48,    49,    -1,
      -1,    52,    53,    54,    55,     3,    -1,    58,     6,    60,
       8,     9,    -1,    11,    65,    13,    -1,    -1,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    -1,    -1,    33,    34,    35,    36,    37,
      -1,    -1,    -1,    -1,    42,    43,    -1,    45,    -1,    47,
      -1,    -1,    50,    -1,    -1,    -1,    -1,    -1,    56,    57,
      -1,    59,    -1,    61,    62,    63,     3,    -1,    -1,     6,
      -1,     8,     9,    -1,    11,    -1,    13,    -1,    -1,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    -1,    -1,    33,    34,    35,    36,
      37,    -1,    -1,    -1,    -1,    42,    43,    -1,    45,    -1,
      47,    -1,    -1,    50,    -1,    -1,    -1,    -1,    -1,    56,
      57,    -1,    59,    -1,    61,    62,    63,     3,    -1,    -1,
       6,    -1,     8,     9,    -1,    11,    -1,    13,    -1,    -1,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    -1,    -1,    33,    34,    35,
      36,    37,    -1,    -1,    -1,    -1,    42,    43,    -1,    45,
      -1,    47,    -1,    -1,    50,    -1,    -1,    -1,    -1,    -1,
      56,    57,    -1,    59,    -1,    61,    62,    63,     3,    -1,
      -1,     6,    -1,     8,     9,    -1,    11,    -1,    13,    -1,
      -1,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    -1,    -1,    33,    34,
      -1,    36,    37,    -1,    -1,    -1,    -1,    42,    43,    -1,
      45,    -1,    47,    -1,    -1,    50,    -1,    -1,    -1,    -1,
      -1,    56,    57,     8,    59,    -1,    61,    62,    63,    -1,
      -1,    16,    17,    18,    -1,    -1,    21,    -1,    23,    24,
      -1,    26,    -1,    28,    -1,    -1,    -1,    -1,    33,    34,
      -1,    36,    37,    -1,    -1,    -1,    -1,    42,    43,    -1,
      45,    -1,    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    56,    57,     8,    59,    -1,    61,    62,    63,    64,
      -1,    16,    17,    18,    -1,    -1,    21,    -1,    23,    24,
      -1,    26,    27,    28,    -1,    -1,    -1,    -1,    33,    34,
      -1,    36,    37,    -1,    -1,    -1,    -1,    42,    43,    -1,
      45,    -1,    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    56,    57,     8,    59,    -1,    61,    62,    63,    -1,
      -1,    16,    17,    18,    -1,    -1,    21,    -1,    23,    24,
      -1,    26,    -1,    28,    -1,    -1,    -1,    -1,    33,    34,
      -1,    36,    37,    -1,    -1,    -1,    -1,    42,    43,    -1,
      45,    46,    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    56,    57,     8,    59,    -1,    61,    62,    63,    -1,
      -1,    16,    17,    18,    -1,    -1,    21,    -1,    23,    24,
      -1,    26,    -1,    28,    -1,    -1,    -1,    -1,    33,    34,
      -1,    36,    37,    -1,    -1,    -1,    -1,    42,    43,    -1,
      45,    -1,    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    56,    57,    -1,    59,    -1,    61,    62,    63
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    67,    68,     0,     3,     6,     8,     9,    11,    13,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    33,    34,    36,    37,    42,
      43,    45,    47,    50,    56,    57,    59,    61,    62,    63,
      69,    70,    74,    77,    78,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,   102,   103,   107,   110,   112,
     114,   116,   118,   120,   122,   124,   126,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   139,   140,   144,   148,
     149,    37,   147,   147,    37,    47,   131,   133,    74,    45,
      45,   134,   135,   106,   107,    45,   107,    47,    77,   131,
      37,    79,    81,   131,    45,    45,   131,   131,    51,   107,
      35,    37,    42,    43,    74,    75,   145,   146,   131,   131,
     131,   131,    48,    64,   141,   142,   143,   147,     1,    48,
      50,   150,    32,    52,    31,    53,    54,    55,    39,    14,
      15,    41,    40,    56,    57,    58,    59,    60,    33,    34,
      38,    49,    45,    63,    65,   137,    63,    65,   137,   150,
     150,    35,    37,    42,    43,    29,   107,    27,   108,   109,
     111,   113,   115,   117,   119,   121,   123,   125,   127,   128,
     133,   137,   150,   107,   150,    35,     5,    12,   104,   105,
      49,    83,    84,    48,   150,   107,   107,    74,    46,    35,
      74,    35,    48,    51,    48,    64,    48,    64,   110,    45,
     110,   116,   110,   118,   120,   122,   124,   126,   128,   128,
     128,   129,   130,   130,   131,   131,   131,   110,   110,    46,
     110,   138,   107,    37,   107,    37,    45,    46,    37,    80,
      82,    50,    48,    32,    52,    31,    53,    54,    55,    39,
      15,    41,    14,    38,    49,    46,    45,    77,   105,   110,
      81,    46,    46,   146,   110,   142,   143,    37,    72,    73,
      51,    46,    48,    64,    64,   107,    74,    49,    85,    86,
      48,    50,    14,   106,   111,   133,   117,   111,   119,   121,
     123,   125,   127,   128,   128,   107,   111,   111,    47,    97,
      37,    74,    74,    51,    64,   110,    48,    46,   110,   110,
      46,    10,   111,    82,   106,   107,    50,    51,    46,     4,
      98,    99,   100,    46,   110,    37,    47,    50,    74,    50,
      46,   106,   111,    74,   107,     7,    35,   101,   100,    77,
      68,    71,   106,    74,    46,    51,    51,    98,    35,    46,
      74,    75,    76,    76,    35,    74
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
  /* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;

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
#line 254 "parser.y"
    { program_parsed(ctx, (yyvsp[0].source_elements)); ;}
    break;

  case 3:
#line 258 "parser.y"
    { (yyval.source_elements) = new_source_elements(ctx); ;}
    break;

  case 4:
#line 260 "parser.y"
    { (yyval.source_elements) = source_elements_add_statement((yyvsp[-1].source_elements), (yyvsp[0].statement)); ;}
    break;

  case 5:
#line 265 "parser.y"
    { (yyval.expr) = new_function_expression(ctx, (yyvsp[-6].identifier), (yyvsp[-4].parameter_list), (yyvsp[-1].source_elements), (yyvsp[-7].srcptr), (yyvsp[0].srcptr)-(yyvsp[-7].srcptr)+1); ;}
    break;

  case 6:
#line 268 "parser.y"
    { push_func(ctx); (yyval.srcptr) = (yyvsp[0].srcptr); ;}
    break;

  case 7:
#line 272 "parser.y"
    { (yyval.source_elements) = function_body_parsed(ctx, (yyvsp[0].source_elements)); ;}
    break;

  case 8:
#line 276 "parser.y"
    { (yyval.parameter_list) = new_parameter_list(ctx, (yyvsp[0].identifier)); ;}
    break;

  case 9:
#line 278 "parser.y"
    { (yyval.parameter_list) = parameter_list_add(ctx, (yyvsp[-2].parameter_list), (yyvsp[0].identifier)); ;}
    break;

  case 10:
#line 282 "parser.y"
    { (yyval.parameter_list) = NULL; ;}
    break;

  case 11:
#line 283 "parser.y"
    { (yyval.parameter_list) = (yyvsp[0].parameter_list); ;}
    break;

  case 12:
#line 287 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 13:
#line 288 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 14:
#line 289 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 15:
#line 290 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 16:
#line 291 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 17:
#line 292 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 18:
#line 293 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 19:
#line 294 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 20:
#line 295 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 21:
#line 296 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 22:
#line 297 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 23:
#line 298 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 24:
#line 299 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 25:
#line 300 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 26:
#line 304 "parser.y"
    { (yyval.statement_list) = new_statement_list(ctx, (yyvsp[0].statement)); ;}
    break;

  case 27:
#line 306 "parser.y"
    { (yyval.statement_list) = statement_list_add((yyvsp[-1].statement_list), (yyvsp[0].statement)); ;}
    break;

  case 28:
#line 310 "parser.y"
    { (yyval.statement_list) = NULL; ;}
    break;

  case 29:
#line 311 "parser.y"
    { (yyval.statement_list) = (yyvsp[0].statement_list); ;}
    break;

  case 30:
#line 315 "parser.y"
    { (yyval.statement) = new_block_statement(ctx, (yyvsp[-1].statement_list)); ;}
    break;

  case 31:
#line 316 "parser.y"
    { (yyval.statement) = new_block_statement(ctx, NULL); ;}
    break;

  case 32:
#line 321 "parser.y"
    { (yyval.statement) = new_var_statement(ctx, (yyvsp[-1].variable_list)); ;}
    break;

  case 33:
#line 325 "parser.y"
    { (yyval.variable_list) = new_variable_list(ctx, (yyvsp[0].variable_declaration)); ;}
    break;

  case 34:
#line 327 "parser.y"
    { (yyval.variable_list) = variable_list_add(ctx, (yyvsp[-2].variable_list), (yyvsp[0].variable_declaration)); ;}
    break;

  case 35:
#line 332 "parser.y"
    { (yyval.variable_list) = new_variable_list(ctx, (yyvsp[0].variable_declaration)); ;}
    break;

  case 36:
#line 334 "parser.y"
    { (yyval.variable_list) = variable_list_add(ctx, (yyvsp[-2].variable_list), (yyvsp[0].variable_declaration)); ;}
    break;

  case 37:
#line 339 "parser.y"
    { (yyval.variable_declaration) = new_variable_declaration(ctx, (yyvsp[-1].identifier), (yyvsp[0].expr)); ;}
    break;

  case 38:
#line 344 "parser.y"
    { (yyval.variable_declaration) = new_variable_declaration(ctx, (yyvsp[-1].identifier), (yyvsp[0].expr)); ;}
    break;

  case 39:
#line 348 "parser.y"
    { (yyval.expr) = NULL; ;}
    break;

  case 40:
#line 349 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 41:
#line 354 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 42:
#line 358 "parser.y"
    { (yyval.expr) = NULL; ;}
    break;

  case 43:
#line 359 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 44:
#line 364 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 45:
#line 368 "parser.y"
    { (yyval.statement) = new_empty_statement(ctx); ;}
    break;

  case 46:
#line 373 "parser.y"
    { (yyval.statement) = new_expression_statement(ctx, (yyvsp[-1].expr)); ;}
    break;

  case 47:
#line 378 "parser.y"
    { (yyval.statement) = new_if_statement(ctx, (yyvsp[-4].expr), (yyvsp[-2].statement), (yyvsp[0].statement)); ;}
    break;

  case 48:
#line 380 "parser.y"
    { (yyval.statement) = new_if_statement(ctx, (yyvsp[-2].expr), (yyvsp[0].statement), NULL); ;}
    break;

  case 49:
#line 385 "parser.y"
    { (yyval.statement) = new_while_statement(ctx, TRUE, (yyvsp[-2].expr), (yyvsp[-5].statement)); ;}
    break;

  case 50:
#line 387 "parser.y"
    { (yyval.statement) = new_while_statement(ctx, FALSE, (yyvsp[-2].expr), (yyvsp[0].statement)); ;}
    break;

  case 51:
#line 389 "parser.y"
    { (yyval.statement) = new_for_statement(ctx, NULL, (yyvsp[-6].expr), (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].statement)); ;}
    break;

  case 52:
#line 391 "parser.y"
    { (yyval.statement) = new_for_statement(ctx, (yyvsp[-6].variable_list), NULL, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].statement)); ;}
    break;

  case 53:
#line 393 "parser.y"
    { (yyval.statement) = new_forin_statement(ctx, NULL, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].statement)); ;}
    break;

  case 54:
#line 395 "parser.y"
    { (yyval.statement) = new_forin_statement(ctx, (yyvsp[-4].variable_declaration), NULL, (yyvsp[-2].expr), (yyvsp[0].statement)); ;}
    break;

  case 55:
#line 400 "parser.y"
    { (yyval.statement) = new_continue_statement(ctx, (yyvsp[-1].identifier)); ;}
    break;

  case 56:
#line 405 "parser.y"
    { (yyval.statement) = new_break_statement(ctx, (yyvsp[-1].identifier)); ;}
    break;

  case 57:
#line 410 "parser.y"
    { (yyval.statement) = new_return_statement(ctx, (yyvsp[-1].expr)); ;}
    break;

  case 58:
#line 415 "parser.y"
    { (yyval.statement) = new_with_statement(ctx, (yyvsp[-2].expr), (yyvsp[0].statement)); ;}
    break;

  case 59:
#line 420 "parser.y"
    { (yyval.statement) = new_labelled_statement(ctx, (yyvsp[-2].identifier), (yyvsp[0].statement)); ;}
    break;

  case 60:
#line 425 "parser.y"
    { (yyval.statement) = new_switch_statement(ctx, (yyvsp[-2].expr), (yyvsp[0].case_clausule)); ;}
    break;

  case 61:
#line 430 "parser.y"
    { (yyval.case_clausule) = new_case_block(ctx, (yyvsp[-1].case_list), NULL, NULL); ;}
    break;

  case 62:
#line 432 "parser.y"
    { (yyval.case_clausule) = new_case_block(ctx, (yyvsp[-3].case_list), (yyvsp[-2].case_clausule), (yyvsp[-1].case_list)); ;}
    break;

  case 63:
#line 436 "parser.y"
    { (yyval.case_list) = NULL; ;}
    break;

  case 64:
#line 437 "parser.y"
    { (yyval.case_list) = (yyvsp[0].case_list); ;}
    break;

  case 65:
#line 441 "parser.y"
    { (yyval.case_list) = new_case_list(ctx, (yyvsp[0].case_clausule)); ;}
    break;

  case 66:
#line 443 "parser.y"
    { (yyval.case_list) = case_list_add(ctx, (yyvsp[-1].case_list), (yyvsp[0].case_clausule)); ;}
    break;

  case 67:
#line 448 "parser.y"
    { (yyval.case_clausule) = new_case_clausule(ctx, (yyvsp[-2].expr), (yyvsp[0].statement_list)); ;}
    break;

  case 68:
#line 453 "parser.y"
    { (yyval.case_clausule) = new_case_clausule(ctx, NULL, (yyvsp[0].statement_list)); ;}
    break;

  case 69:
#line 458 "parser.y"
    { (yyval.statement) = new_throw_statement(ctx, (yyvsp[-1].expr)); ;}
    break;

  case 70:
#line 462 "parser.y"
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[-1].statement), (yyvsp[0].catch_block), NULL); ;}
    break;

  case 71:
#line 463 "parser.y"
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[-1].statement), NULL, (yyvsp[0].statement)); ;}
    break;

  case 72:
#line 465 "parser.y"
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[-2].statement), (yyvsp[-1].catch_block), (yyvsp[0].statement)); ;}
    break;

  case 73:
#line 470 "parser.y"
    { (yyval.catch_block) = new_catch_block(ctx, (yyvsp[-2].identifier), (yyvsp[0].statement)); ;}
    break;

  case 74:
#line 474 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 75:
#line 478 "parser.y"
    { (yyval.expr) = NULL; ;}
    break;

  case 76:
#line 479 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 77:
#line 483 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 78:
#line 485 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_COMMA, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 79:
#line 489 "parser.y"
    { (yyval.expr) = NULL; ;}
    break;

  case 80:
#line 490 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 81:
#line 495 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 82:
#line 497 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_COMMA, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 83:
#line 501 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 84:
#line 503 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ASSIGN, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 85:
#line 505 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 86:
#line 510 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 87:
#line 512 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ASSIGN, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 88:
#line 514 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 89:
#line 518 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 90:
#line 520 "parser.y"
    { (yyval.expr) = new_conditional_expression(ctx, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 91:
#line 525 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 92:
#line 527 "parser.y"
    { (yyval.expr) = new_conditional_expression(ctx, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 93:
#line 531 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 94:
#line 533 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_OR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 95:
#line 538 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 96:
#line 540 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_OR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 97:
#line 544 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 98:
#line 546 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_AND, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 99:
#line 551 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 100:
#line 553 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_AND, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 101:
#line 557 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 102:
#line 559 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BOR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 103:
#line 564 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 104:
#line 566 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BOR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 105:
#line 570 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 106:
#line 572 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BXOR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 107:
#line 577 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 108:
#line 579 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BXOR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 109:
#line 583 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 110:
#line 585 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BAND, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 111:
#line 590 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 112:
#line 592 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BAND, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 113:
#line 596 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 114:
#line 598 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 115:
#line 602 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 116:
#line 604 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 117:
#line 608 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 118:
#line 610 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 119:
#line 612 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_INSTANCEOF, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 120:
#line 614 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_IN, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 121:
#line 618 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 122:
#line 620 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 123:
#line 622 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_INSTANCEOF, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 124:
#line 626 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 125:
#line 628 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 126:
#line 633 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 127:
#line 635 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ADD, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 128:
#line 637 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_SUB, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 129:
#line 641 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 130:
#line 643 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_MUL, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 131:
#line 645 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_DIV, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 132:
#line 647 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_MOD, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 133:
#line 651 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 134:
#line 653 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_DELETE, (yyvsp[0].expr)); ;}
    break;

  case 135:
#line 654 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_VOID, (yyvsp[0].expr)); ;}
    break;

  case 136:
#line 656 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_TYPEOF, (yyvsp[0].expr)); ;}
    break;

  case 137:
#line 657 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PREINC, (yyvsp[0].expr)); ;}
    break;

  case 138:
#line 658 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PREDEC, (yyvsp[0].expr)); ;}
    break;

  case 139:
#line 659 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PLUS, (yyvsp[0].expr)); ;}
    break;

  case 140:
#line 660 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_MINUS, (yyvsp[0].expr)); ;}
    break;

  case 141:
#line 661 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_BITNEG, (yyvsp[0].expr)); ;}
    break;

  case 142:
#line 662 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_LOGNEG, (yyvsp[0].expr)); ;}
    break;

  case 143:
#line 667 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 144:
#line 669 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_POSTINC, (yyvsp[-1].expr)); ;}
    break;

  case 145:
#line 671 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_POSTDEC, (yyvsp[-1].expr)); ;}
    break;

  case 146:
#line 676 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 147:
#line 677 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 148:
#line 681 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 149:
#line 682 "parser.y"
    { (yyval.expr) = new_new_expression(ctx, (yyvsp[0].expr), NULL); ;}
    break;

  case 150:
#line 686 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 151:
#line 687 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 152:
#line 689 "parser.y"
    { (yyval.expr) = new_array_expression(ctx, (yyvsp[-3].expr), (yyvsp[-1].expr)); ;}
    break;

  case 153:
#line 691 "parser.y"
    { (yyval.expr) = new_member_expression(ctx, (yyvsp[-2].expr), (yyvsp[0].identifier)); ;}
    break;

  case 154:
#line 693 "parser.y"
    { (yyval.expr) = new_new_expression(ctx, (yyvsp[-1].expr), (yyvsp[0].argument_list)); ;}
    break;

  case 155:
#line 698 "parser.y"
    { (yyval.expr) = new_call_expression(ctx, (yyvsp[-1].expr), (yyvsp[0].argument_list)); ;}
    break;

  case 156:
#line 700 "parser.y"
    { (yyval.expr) = new_call_expression(ctx, (yyvsp[-1].expr), (yyvsp[0].argument_list)); ;}
    break;

  case 157:
#line 702 "parser.y"
    { (yyval.expr) = new_array_expression(ctx, (yyvsp[-3].expr), (yyvsp[-1].expr)); ;}
    break;

  case 158:
#line 704 "parser.y"
    { (yyval.expr) = new_member_expression(ctx, (yyvsp[-2].expr), (yyvsp[0].identifier)); ;}
    break;

  case 159:
#line 708 "parser.y"
    { (yyval.argument_list) = NULL; ;}
    break;

  case 160:
#line 709 "parser.y"
    { (yyval.argument_list) = (yyvsp[-1].argument_list); ;}
    break;

  case 161:
#line 713 "parser.y"
    { (yyval.argument_list) = new_argument_list(ctx, (yyvsp[0].expr)); ;}
    break;

  case 162:
#line 715 "parser.y"
    { (yyval.argument_list) = argument_list_add(ctx, (yyvsp[-2].argument_list), (yyvsp[0].expr)); ;}
    break;

  case 163:
#line 719 "parser.y"
    { (yyval.expr) = new_this_expression(ctx); ;}
    break;

  case 164:
#line 720 "parser.y"
    { (yyval.expr) = new_identifier_expression(ctx, (yyvsp[0].identifier)); ;}
    break;

  case 165:
#line 721 "parser.y"
    { (yyval.expr) = new_literal_expression(ctx, (yyvsp[0].literal)); ;}
    break;

  case 166:
#line 722 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 167:
#line 723 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 168:
#line 724 "parser.y"
    { (yyval.expr) = (yyvsp[-1].expr); ;}
    break;

  case 169:
#line 728 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, NULL, 0); ;}
    break;

  case 170:
#line 729 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, NULL, (yyvsp[-1].ival)+1); ;}
    break;

  case 171:
#line 730 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, (yyvsp[-1].element_list), 0); ;}
    break;

  case 172:
#line 732 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, (yyvsp[-3].element_list), (yyvsp[-1].ival)+1); ;}
    break;

  case 173:
#line 737 "parser.y"
    { (yyval.element_list) = new_element_list(ctx, (yyvsp[-1].ival), (yyvsp[0].expr)); ;}
    break;

  case 174:
#line 739 "parser.y"
    { (yyval.element_list) = element_list_add(ctx, (yyvsp[-3].element_list), (yyvsp[-1].ival), (yyvsp[0].expr)); ;}
    break;

  case 175:
#line 743 "parser.y"
    { (yyval.ival) = 1; ;}
    break;

  case 176:
#line 744 "parser.y"
    { (yyval.ival) = (yyvsp[-1].ival) + 1; ;}
    break;

  case 177:
#line 748 "parser.y"
    { (yyval.ival) = 0; ;}
    break;

  case 178:
#line 749 "parser.y"
    { (yyval.ival) = (yyvsp[0].ival); ;}
    break;

  case 179:
#line 753 "parser.y"
    { (yyval.expr) = new_prop_and_value_expression(ctx, NULL); ;}
    break;

  case 180:
#line 755 "parser.y"
    { (yyval.expr) = new_prop_and_value_expression(ctx, (yyvsp[-1].property_list)); ;}
    break;

  case 181:
#line 760 "parser.y"
    { (yyval.property_list) = new_property_list(ctx, (yyvsp[-2].literal), (yyvsp[0].expr)); ;}
    break;

  case 182:
#line 762 "parser.y"
    { (yyval.property_list) = property_list_add(ctx, (yyvsp[-4].property_list), (yyvsp[-2].literal), (yyvsp[0].expr)); ;}
    break;

  case 183:
#line 766 "parser.y"
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[0].identifier)); ;}
    break;

  case 184:
#line 767 "parser.y"
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[0].wstr)); ;}
    break;

  case 185:
#line 768 "parser.y"
    { (yyval.literal) = (yyvsp[0].literal); ;}
    break;

  case 186:
#line 772 "parser.y"
    { (yyval.identifier) = NULL; ;}
    break;

  case 187:
#line 773 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); ;}
    break;

  case 188:
#line 777 "parser.y"
    { (yyval.literal) = new_null_literal(ctx); ;}
    break;

  case 189:
#line 778 "parser.y"
    { (yyval.literal) = new_undefined_literal(ctx); ;}
    break;

  case 190:
#line 779 "parser.y"
    { (yyval.literal) = (yyvsp[0].literal); ;}
    break;

  case 191:
#line 780 "parser.y"
    { (yyval.literal) = (yyvsp[0].literal); ;}
    break;

  case 192:
#line 781 "parser.y"
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[0].wstr)); ;}
    break;

  case 193:
#line 782 "parser.y"
    { (yyval.literal) = parse_regexp(ctx);
                                  if(!(yyval.literal)) YYABORT; ;}
    break;

  case 194:
#line 787 "parser.y"
    { (yyval.literal) = new_boolean_literal(ctx, TRUE); ;}
    break;

  case 195:
#line 788 "parser.y"
    { (yyval.literal) = new_boolean_literal(ctx, FALSE); ;}
    break;

  case 197:
#line 792 "parser.y"
    { if(!allow_auto_semicolon(ctx)) {YYABORT;} ;}
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
#line 2754 "parser.tab.c"

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


#line 794 "parser.y"


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

HRESULT script_parse(script_ctx_t *ctx, const WCHAR *code, parser_ctx_t **ret)
{
    parser_ctx_t *parser_ctx;
    jsheap_t *mark;
    HRESULT hres;

    parser_ctx = heap_alloc_zero(sizeof(parser_ctx_t));
    if(!parser_ctx)
        return E_OUTOFMEMORY;

    parser_ctx->ref = 1;
    parser_ctx->hres = E_FAIL;

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

