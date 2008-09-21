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
     kFUNCTION = 269,
     kIN = 270,
     kINSTANCEOF = 271,
     kNEW = 272,
     kNULL = 273,
     kUNDEFINED = 274,
     kRETURN = 275,
     kSWITCH = 276,
     kTHIS = 277,
     kTHROW = 278,
     kTRUE = 279,
     kFALSE = 280,
     kTRY = 281,
     kTYPEOF = 282,
     kVAR = 283,
     kVOID = 284,
     kWHILE = 285,
     kWITH = 286,
     tANDAND = 287,
     tOROR = 288,
     tINC = 289,
     tDEC = 290,
     tIdentifier = 291,
     tAssignOper = 292,
     tEqOper = 293,
     tShiftOper = 294,
     tRelOper = 295,
     tNumericLiteral = 296,
     tStringLiteral = 297
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
#define kFUNCTION 269
#define kIN 270
#define kINSTANCEOF 271
#define kNEW 272
#define kNULL 273
#define kUNDEFINED 274
#define kRETURN 275
#define kSWITCH 276
#define kTHIS 277
#define kTHROW 278
#define kTRUE 279
#define kFALSE 280
#define kTRY 281
#define kTYPEOF 282
#define kVAR 283
#define kVOID 284
#define kWHILE 285
#define kWITH 286
#define tANDAND 287
#define tOROR 288
#define tINC 289
#define tDEC 290
#define tIdentifier 291
#define tAssignOper 292
#define tEqOper 293
#define tShiftOper 294
#define tRelOper 295
#define tNumericLiteral 296
#define tStringLiteral 297




/* Copy the first part of user declarations.  */
#line 19 "parser.y"


#include "jscript.h"
#include "engine.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

#define YYLEX_PARAM ctx
#define YYPARSE_PARAM ctx

static int parser_error(const char*);
static BOOL allow_auto_semicolon(parser_ctx_t*);
static void program_parsed(parser_ctx_t*,source_elements_t*);

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

statement_list_t *new_statement_list(parser_ctx_t*,statement_t*);
statement_list_t *statement_list_add(statement_list_t*,statement_t*);

typedef struct _parameter_list_t {
    parameter_t *head;
    parameter_t *tail;
} parameter_list_t;

static parameter_list_t *new_parameter_list(parser_ctx_t*,const WCHAR*);
static parameter_list_t *parameter_list_add(parser_ctx_t*,parameter_list_t*,const WCHAR*);

static expression_t *new_function_expression(parser_ctx_t*,const WCHAR*,parameter_list_t*,source_elements_t*);
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

static function_declaration_t *new_function_declaration(parser_ctx_t*,const WCHAR*,parameter_list_t*,source_elements_t*);
static source_elements_t *new_source_elements(parser_ctx_t*);
static source_elements_t *source_elements_add_statement(source_elements_t*,statement_t*);
static source_elements_t *source_elements_add_function(source_elements_t*,function_declaration_t*);



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
#line 147 "parser.y"
typedef union YYSTYPE {
    int                     ival;
    LPCWSTR                 wstr;
    literal_t               *literal;
    struct _argument_list_t *argument_list;
    case_clausule_t         *case_clausule;
    struct _case_list_t     *case_list;
    catch_block_t           *catch_block;
    struct _element_list_t  *element_list;
    expression_t            *expr;
    const WCHAR            *identifier;
    function_declaration_t  *function_declaration;
    struct _parameter_list_t *parameter_list;
    struct _property_list_t *property_list;
    source_elements_t       *source_elements;
    statement_t             *statement;
    struct _statement_list_t *statement_list;
    struct _variable_list_t *variable_list;
    variable_declaration_t  *variable_declaration;
} YYSTYPE;
/* Line 196 of yacc.c.  */
#line 323 "parser.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */
#line 335 "parser.tab.c"

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
#define YYLAST   773

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  65
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  85
/* YYNRULES -- Number of rules. */
#define YYNRULES  196
/* YYNRULES -- Number of states. */
#define YYNSTATES  360

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   297

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    61,     2,     2,     2,    59,    54,     2,
      43,    44,    57,    55,    47,    56,    64,    58,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    50,    49,
       2,    48,     2,    51,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    62,     2,    63,    53,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    45,    52,    46,    60,     2,     2,     2,
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
      35,    36,    37,    38,    39,    40,    41,    42
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,     5,     6,     9,    12,    21,    30,    32,
      34,    38,    39,    41,    43,    45,    47,    49,    51,    53,
      55,    57,    59,    61,    63,    65,    67,    69,    71,    74,
      75,    77,    81,    85,    87,    91,    93,    97,   100,   103,
     104,   106,   109,   110,   112,   115,   117,   120,   128,   134,
     142,   148,   158,   169,   177,   186,   190,   194,   198,   204,
     208,   214,   218,   224,   225,   227,   229,   232,   237,   241,
     245,   249,   253,   258,   264,   267,   268,   270,   272,   276,
     277,   279,   281,   285,   287,   291,   295,   297,   301,   305,
     307,   313,   315,   321,   323,   327,   329,   333,   335,   339,
     341,   345,   347,   351,   353,   357,   359,   363,   365,   369,
     371,   375,   377,   381,   383,   387,   389,   393,   395,   399,
     403,   407,   409,   413,   417,   419,   423,   425,   429,   433,
     435,   439,   443,   447,   449,   452,   455,   458,   461,   464,
     467,   470,   473,   476,   478,   481,   484,   486,   488,   490,
     493,   495,   497,   502,   506,   510,   513,   516,   521,   525,
     528,   532,   534,   538,   540,   542,   544,   546,   548,   552,
     556,   560,   566,   569,   574,   576,   579,   580,   582,   585,
     589,   593,   599,   601,   603,   605,   606,   608,   610,   612,
     614,   616,   618,   620,   622,   624,   626
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short int yyrhs[] =
{
      66,     0,    -1,    67,    -1,    -1,    67,    73,    -1,    67,
      68,    -1,    14,    36,    43,    72,    44,    45,    70,    46,
      -1,    14,   146,    43,    72,    44,    45,    70,    46,    -1,
      67,    -1,    36,    -1,    71,    47,    36,    -1,    -1,    71,
      -1,    76,    -1,    77,    -1,    86,    -1,    87,    -1,    88,
      -1,    89,    -1,    90,    -1,    91,    -1,    92,    -1,    93,
      -1,    94,    -1,    95,    -1,   101,    -1,   102,    -1,    73,
      -1,    74,    73,    -1,    -1,    74,    -1,    45,    75,    46,
      -1,    28,    78,   149,    -1,    80,    -1,    78,    47,    80,
      -1,    81,    -1,    79,    47,    81,    -1,    36,    82,    -1,
      36,    84,    -1,    -1,    83,    -1,    48,   109,    -1,    -1,
      85,    -1,    48,   110,    -1,    49,    -1,   106,   149,    -1,
      11,    43,   106,    44,    73,    10,    73,    -1,    11,    43,
     106,    44,    73,    -1,     9,    73,    30,    43,   106,    44,
      49,    -1,    30,    43,   106,    44,    73,    -1,    13,    43,
     107,    49,   105,    49,   105,    44,    73,    -1,    13,    43,
      28,    79,    49,   105,    49,   105,    44,    73,    -1,    13,
      43,   132,    15,   106,    44,    73,    -1,    13,    43,    28,
      81,    15,   106,    44,    73,    -1,     6,   146,   149,    -1,
       3,   146,   149,    -1,    20,   105,   149,    -1,    31,    43,
     106,    44,    73,    -1,    36,    50,    73,    -1,    21,    43,
     106,    44,    96,    -1,    45,    97,    46,    -1,    45,    97,
     100,    97,    46,    -1,    -1,    98,    -1,    99,    -1,    98,
      99,    -1,     4,   106,    50,    75,    -1,     7,    50,    75,
      -1,    23,   106,   149,    -1,    26,    76,   103,    -1,    26,
      76,   104,    -1,    26,    76,   103,   104,    -1,     5,    43,
      36,    44,    76,    -1,    12,    76,    -1,    -1,   106,    -1,
     109,    -1,   106,    47,   109,    -1,    -1,   108,    -1,   110,
      -1,   108,    47,   110,    -1,   111,    -1,   132,    48,   109,
      -1,   132,    37,   109,    -1,   112,    -1,   132,    48,   110,
      -1,   132,    37,   110,    -1,   113,    -1,   113,    51,   109,
      50,   109,    -1,   114,    -1,   114,    51,   110,    50,   110,
      -1,   115,    -1,   113,    33,   115,    -1,   116,    -1,   114,
      33,   116,    -1,   117,    -1,   115,    32,   117,    -1,   118,
      -1,   116,    32,   118,    -1,   119,    -1,   117,    52,   119,
      -1,   120,    -1,   118,    52,   120,    -1,   121,    -1,   119,
      53,   121,    -1,   122,    -1,   120,    53,   122,    -1,   123,
      -1,   121,    54,   123,    -1,   124,    -1,   122,    54,   124,
      -1,   125,    -1,   123,    38,   125,    -1,   126,    -1,   124,
      38,   126,    -1,   127,    -1,   125,    40,   127,    -1,   125,
      16,   127,    -1,   125,    15,   127,    -1,   127,    -1,   126,
      40,   127,    -1,   126,    16,   127,    -1,   128,    -1,   127,
      39,   128,    -1,   129,    -1,   128,    55,   129,    -1,   128,
      56,   129,    -1,   130,    -1,   129,    57,   130,    -1,   129,
      58,   130,    -1,   129,    59,   130,    -1,   131,    -1,     8,
     130,    -1,    29,   130,    -1,    27,   130,    -1,    34,   130,
      -1,    35,   130,    -1,    55,   130,    -1,    56,   130,    -1,
      60,   130,    -1,    61,   130,    -1,   132,    -1,   132,    34,
      -1,   132,    35,    -1,   133,    -1,   135,    -1,   134,    -1,
      17,   133,    -1,   138,    -1,    69,    -1,   134,    62,   106,
      63,    -1,   134,    64,    36,    -1,    17,   134,   136,    -1,
     134,   136,    -1,   135,   136,    -1,   135,    62,   106,    63,
      -1,   135,    64,    36,    -1,    43,    44,    -1,    43,   137,
      44,    -1,   109,    -1,   137,    47,   109,    -1,    22,    -1,
      36,    -1,   147,    -1,   139,    -1,   143,    -1,    43,   106,
      44,    -1,    62,   142,    63,    -1,    62,   140,    63,    -1,
      62,   140,    47,   142,    63,    -1,   142,   109,    -1,   140,
      47,   142,   109,    -1,    47,    -1,   141,    47,    -1,    -1,
     141,    -1,    45,    46,    -1,    45,   144,    46,    -1,   145,
      50,   109,    -1,   144,    47,   145,    50,   109,    -1,    36,
      -1,    42,    -1,    41,    -1,    -1,    36,    -1,    18,    -1,
      19,    -1,   148,    -1,    41,    -1,    42,    -1,    58,    -1,
      24,    -1,    25,    -1,    49,    -1,     1,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   247,   247,   251,   252,   254,   259,   264,   269,   273,
     274,   279,   280,   284,   285,   286,   287,   288,   289,   290,
     291,   292,   293,   294,   295,   296,   297,   301,   302,   307,
     308,   312,   317,   322,   323,   328,   330,   335,   340,   345,
     346,   350,   355,   356,   360,   365,   369,   374,   376,   381,
     383,   385,   387,   389,   391,   396,   401,   406,   411,   416,
     421,   426,   428,   433,   434,   438,   439,   444,   449,   454,
     459,   460,   461,   466,   471,   475,   476,   480,   481,   486,
     487,   491,   493,   498,   499,   501,   506,   508,   510,   515,
     516,   521,   523,   528,   529,   534,   536,   541,   542,   547,
     549,   554,   555,   560,   562,   567,   568,   573,   575,   580,
     581,   586,   588,   593,   594,   599,   600,   605,   606,   608,
     610,   615,   616,   618,   623,   624,   629,   631,   633,   638,
     639,   641,   643,   648,   649,   651,   652,   654,   655,   656,
     657,   658,   659,   663,   665,   667,   673,   674,   678,   679,
     683,   684,   685,   687,   689,   694,   696,   698,   700,   705,
     706,   710,   711,   716,   717,   718,   719,   720,   721,   725,
     726,   727,   732,   734,   739,   740,   744,   745,   749,   750,
     755,   757,   762,   763,   764,   768,   769,   773,   774,   775,
     776,   777,   778,   783,   784,   787,   788
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "kBREAK", "kCASE", "kCATCH", "kCONTINUE",
  "kDEFAULT", "kDELETE", "kDO", "kELSE", "kIF", "kFINALLY", "kFOR",
  "kFUNCTION", "kIN", "kINSTANCEOF", "kNEW", "kNULL", "kUNDEFINED",
  "kRETURN", "kSWITCH", "kTHIS", "kTHROW", "kTRUE", "kFALSE", "kTRY",
  "kTYPEOF", "kVAR", "kVOID", "kWHILE", "kWITH", "tANDAND", "tOROR",
  "tINC", "tDEC", "tIdentifier", "tAssignOper", "tEqOper", "tShiftOper",
  "tRelOper", "tNumericLiteral", "tStringLiteral", "'('", "')'", "'{'",
  "'}'", "','", "'='", "';'", "':'", "'?'", "'|'", "'^'", "'&'", "'+'",
  "'-'", "'*'", "'/'", "'%'", "'~'", "'!'", "'['", "']'", "'.'", "$accept",
  "Program", "SourceElements", "FunctionDeclaration", "FunctionExpression",
  "FunctionBody", "FormalParameterList", "FormalParameterList_opt",
  "Statement", "StatementList", "StatementList_opt", "Block",
  "VariableStatement", "VariableDeclarationList",
  "VariableDeclarationListNoIn", "VariableDeclaration",
  "VariableDeclarationNoIn", "Initialiser_opt", "Initialiser",
  "InitialiserNoIn_opt", "InitialiserNoIn", "EmptyStatement",
  "ExpressionStatement", "IfStatement", "IterationStatement",
  "ContinueStatement", "BreakStatement", "ReturnStatement",
  "WithStatement", "LabelledStatement", "SwitchStatement", "CaseBlock",
  "CaseClausules_opt", "CaseClausules", "CaseClausule", "DefaultClausule",
  "ThrowStatement", "TryStatement", "Catch", "Finally", "Expression_opt",
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
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,    40,    41,   123,   125,    44,    61,    59,
      58,    63,   124,    94,    38,    43,    45,    42,    47,    37,
     126,    33,    91,    93,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    65,    66,    67,    67,    67,    68,    69,    70,    71,
      71,    72,    72,    73,    73,    73,    73,    73,    73,    73,
      73,    73,    73,    73,    73,    73,    73,    74,    74,    75,
      75,    76,    77,    78,    78,    79,    79,    80,    81,    82,
      82,    83,    84,    84,    85,    86,    87,    88,    88,    89,
      89,    89,    89,    89,    89,    90,    91,    92,    93,    94,
      95,    96,    96,    97,    97,    98,    98,    99,   100,   101,
     102,   102,   102,   103,   104,   105,   105,   106,   106,   107,
     107,   108,   108,   109,   109,   109,   110,   110,   110,   111,
     111,   112,   112,   113,   113,   114,   114,   115,   115,   116,
     116,   117,   117,   118,   118,   119,   119,   120,   120,   121,
     121,   122,   122,   123,   123,   124,   124,   125,   125,   125,
     125,   126,   126,   126,   127,   127,   128,   128,   128,   129,
     129,   129,   129,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   131,   131,   131,   132,   132,   133,   133,
     134,   134,   134,   134,   134,   135,   135,   135,   135,   136,
     136,   137,   137,   138,   138,   138,   138,   138,   138,   139,
     139,   139,   140,   140,   141,   141,   142,   142,   143,   143,
     144,   144,   145,   145,   145,   146,   146,   147,   147,   147,
     147,   147,   147,   148,   148,   149,   149
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     0,     2,     2,     8,     8,     1,     1,
       3,     0,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     2,     0,
       1,     3,     3,     1,     3,     1,     3,     2,     2,     0,
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
       3,     1,     3,     1,     1,     1,     1,     1,     3,     3,
       3,     5,     2,     4,     1,     2,     0,     1,     2,     3,
       3,     5,     1,     1,     1,     0,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       3,     0,     2,     1,   185,   185,     0,     0,     0,     0,
     185,     0,   187,   188,    75,     0,   163,     0,   193,   194,
       0,     0,     0,     0,     0,     0,     0,     0,   164,   190,
     191,     0,     0,    45,     0,     0,   192,     0,     0,   176,
       5,   151,     4,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,     0,    77,    83,
      89,    93,    97,   101,   105,   109,   113,   117,   124,   126,
     129,   133,   143,   146,   148,   147,   150,   166,   167,   165,
     189,   186,     0,     0,   185,   164,     0,   134,   143,     0,
       0,    79,     0,     0,   149,   148,     0,    76,     0,     0,
      29,     0,   136,    39,     0,    33,   135,     0,     0,   137,
     138,     0,     0,   164,   190,   191,   178,    27,    30,     0,
       0,     0,   139,   140,   141,   142,   174,     0,   177,     0,
     196,     0,   195,    46,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     144,   145,     0,     0,     0,     0,     0,   155,     0,     0,
     156,    56,    55,   182,   184,   183,     0,     0,     0,     0,
      80,    81,    86,    91,    95,    99,   103,   107,   111,   115,
     121,   143,    11,    11,   154,    57,     0,    69,     0,     0,
      70,    71,     0,    37,    40,     0,    32,     0,     0,    59,
     168,    28,    31,   179,     0,     0,   176,   170,   175,   169,
     172,    78,    94,     0,    98,   102,   106,   110,   114,   120,
     119,   118,   125,   127,   128,   130,   131,   132,    85,    84,
     159,   161,     0,     0,   153,     0,   158,     0,     0,    42,
       0,    35,    75,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     9,    12,     0,     0,
       0,     0,    74,    72,    41,    34,     0,     0,     0,   180,
       0,     0,   160,     0,   152,   157,     0,    48,     0,    38,
      43,     0,    75,     0,     0,    82,   143,    96,     0,   100,
     104,   108,   112,   116,   123,   122,     0,    88,    87,     0,
       0,     0,    63,    60,     0,    50,    58,     0,   171,   173,
      90,   162,     0,     0,    44,    36,     0,     0,    75,     0,
       0,    10,     3,     3,     0,     0,    64,    65,     0,   181,
      49,    47,    75,     0,     0,    92,    53,     8,     0,     0,
       0,     0,    61,    63,    66,    73,     0,    54,     0,     6,
       7,    29,    29,     0,     0,    51,    67,    68,    62,    52
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,     1,   337,    40,    41,   338,   257,   258,   117,   118,
     119,    43,    44,   104,   240,   105,   241,   193,   194,   279,
     280,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,   303,   325,   326,   327,   343,    55,    56,   190,   191,
      96,    57,   169,   170,    58,   171,    59,   172,    60,   173,
      61,   174,    62,   175,    63,   176,    64,   177,    65,   178,
      66,   179,    67,    68,    69,    70,    71,    72,    73,    74,
      75,   157,   232,    76,    77,   127,   128,   129,    78,   120,
     121,    93,    79,    80,   133
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -241
static const short int yypact[] =
{
    -241,     8,   402,  -241,   -17,   -17,   711,   462,   -20,   -12,
       2,    88,  -241,  -241,   711,    20,  -241,   711,  -241,  -241,
       1,   711,    37,   711,    50,    55,   711,   711,    36,  -241,
    -241,   711,   342,  -241,   711,   711,  -241,   711,   711,    44,
    -241,  -241,  -241,  -241,  -241,  -241,  -241,  -241,  -241,  -241,
    -241,  -241,  -241,  -241,  -241,  -241,  -241,     0,  -241,  -241,
      21,    72,    66,    67,    54,    89,    19,    93,   103,   153,
    -241,  -241,    91,  -241,   -36,   -25,  -241,  -241,  -241,  -241,
    -241,  -241,     9,     9,   -17,  -241,   115,  -241,   179,   151,
     711,   613,   157,   164,  -241,   -36,     9,   148,   711,     0,
     462,     4,  -241,   161,     3,  -241,  -241,   711,   711,  -241,
    -241,   462,   -30,    36,   174,   175,  -241,  -241,   462,   181,
     169,   178,  -241,  -241,  -241,  -241,  -241,    48,   182,   511,
    -241,   711,  -241,  -241,   711,   711,   711,   711,   711,   711,
     711,   711,   711,   711,   711,   711,   711,   711,   711,   711,
    -241,  -241,   711,   711,   662,   711,   194,  -241,   711,   196,
    -241,  -241,  -241,  -241,  -241,  -241,   190,    94,   198,   186,
     192,  -241,  -241,    43,   205,   188,   191,   187,   207,     5,
      93,    40,   210,   210,  -241,  -241,    96,  -241,   200,     1,
     235,  -241,   711,  -241,  -241,    37,  -241,    98,   105,  -241,
    -241,  -241,  -241,  -241,   156,   711,    44,  -241,  -241,  -241,
    -241,  -241,    72,   199,    66,    67,    54,    89,    19,    93,
      93,    93,   103,   153,   153,  -241,  -241,  -241,  -241,  -241,
    -241,  -241,   116,    52,  -241,    56,  -241,   711,   462,   202,
      22,   233,   711,   711,   711,   711,   711,   711,   711,   711,
     711,   711,   711,   711,   711,   711,  -241,   204,   208,   209,
     211,   219,  -241,  -241,  -241,  -241,   462,   462,   212,  -241,
     562,   711,  -241,   711,  -241,  -241,   118,   244,   711,  -241,
    -241,   198,   711,   711,   214,  -241,   100,   205,   216,   188,
     191,   187,   207,     5,    93,    93,   152,  -241,  -241,   221,
     215,   222,   255,  -241,   217,  -241,  -241,   711,  -241,  -241,
    -241,  -241,   220,   462,  -241,  -241,   234,   159,   711,   711,
     462,  -241,  -241,  -241,   711,    15,   255,  -241,     1,  -241,
    -241,  -241,   711,   462,   224,  -241,  -241,   402,   240,   241,
     158,   238,  -241,   255,  -241,  -241,   226,  -241,   462,  -241,
    -241,   462,   462,   243,   462,  -241,  -241,  -241,  -241,  -241
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -241,  -241,   290,  -241,  -241,   -32,  -241,   109,    -2,  -241,
    -133,    -9,  -241,  -241,  -241,    99,    12,  -241,  -241,  -241,
    -241,  -241,  -241,  -241,  -241,  -241,  -241,  -241,  -241,  -241,
    -241,  -241,   -48,  -241,   -29,  -241,  -241,  -241,  -241,   106,
    -240,   -11,  -241,  -241,   -69,  -230,  -241,  -241,  -241,  -241,
     165,    57,   162,    58,   163,    59,   167,    61,   171,    53,
     172,    64,   -61,   173,    75,     6,  -241,    30,   292,   304,
    -241,    -5,  -241,  -241,  -241,  -241,  -241,   110,  -241,  -241,
     119,   218,  -241,  -241,    18
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -185
static const short int yytable[] =
{
      42,   130,   284,    97,   130,    89,    99,   154,     3,   188,
     130,   101,    87,   285,   200,   288,   189,   131,   154,    81,
     112,   251,   341,    90,   297,   298,   155,   102,   156,   106,
     180,    91,   109,   110,   141,   142,    88,   158,    92,   159,
     122,   123,   316,   124,   125,   252,   100,   131,   314,   132,
     195,    88,   132,    88,   134,   253,    88,    88,   132,   143,
     210,   342,   211,    98,    88,    88,   213,    88,    88,   281,
     160,   282,   135,   103,   150,   151,   244,   254,   334,   167,
     219,   220,   221,   228,   229,   231,   111,   186,   255,   335,
     184,   126,   346,   107,   245,   206,   197,   198,   108,   131,
     161,   162,    84,   131,   136,    11,    12,    13,   139,   199,
      16,   207,    18,    19,   185,   274,   201,   187,   137,   275,
     138,   181,   196,   264,    85,   150,   151,   140,   152,    29,
      30,    31,   144,    86,   150,   151,   269,   254,   238,   153,
     260,   131,   266,   131,   233,   131,    36,   235,   255,   267,
      39,   163,   131,   225,   226,   227,   164,   165,   145,   146,
     272,   116,   312,   273,    88,   131,    88,    88,    88,    88,
      88,    88,    88,    88,    88,    88,    88,    88,    88,    88,
     262,   166,   180,   180,   180,   180,   180,   180,   180,   180,
     294,   295,   163,   180,   180,   131,   320,   164,   165,   131,
     182,   309,   310,   333,   311,   131,   131,   183,   351,   192,
     147,   148,   149,   150,   151,   203,   204,   180,   356,   357,
     223,   224,    82,    83,  -184,  -183,   276,   202,   205,   208,
     234,    97,   236,   237,   239,   242,   277,   246,   329,   243,
     247,   249,   296,   261,   248,   250,   256,   189,   283,   271,
     278,   299,   300,   301,   313,   304,   302,   321,   180,   324,
     322,   328,   307,   318,   305,   306,   319,   323,   348,   330,
     354,    97,   317,   286,    88,   286,    88,    88,    88,    88,
      88,    88,    88,   332,   286,   286,   349,   350,   352,   358,
       2,   339,   259,   315,   265,   353,   263,   344,   214,   212,
     215,   287,   292,    94,   289,   216,   290,    97,   286,   291,
     217,   331,   218,   340,   293,    95,   270,   222,   336,   345,
       0,    97,     0,   268,     0,     0,     0,     0,     0,     0,
       0,   347,     0,     0,     0,    42,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     4,   355,     0,     5,   286,
       6,     7,   359,     8,     0,     9,    84,     0,     0,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,     0,     0,    26,    27,   113,     0,
       0,     0,     0,   114,   115,    31,     0,    32,   116,     0,
       0,    33,     0,     0,     0,     0,     0,    34,    35,     0,
      36,     0,    37,    38,    39,     4,     0,     0,     5,     0,
       6,     7,     0,     8,     0,     9,    10,     0,     0,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,     0,     0,    26,    27,    28,     0,
       0,     0,     0,    29,    30,    31,     0,    32,     0,     0,
       0,    33,     0,     0,     0,     0,     0,    34,    35,     0,
      36,     0,    37,    38,    39,     4,     0,     0,     5,     0,
       6,     7,     0,     8,     0,     9,    84,     0,     0,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,     0,     0,    26,    27,    28,     0,
       0,     0,     0,    29,    30,    31,     0,    32,     0,     0,
       0,    33,     0,     0,     0,     0,     0,    34,    35,     6,
      36,     0,    37,    38,    39,    84,     0,     0,    11,    12,
      13,     0,     0,    16,     0,    18,    19,     0,    21,     0,
      23,     0,     0,     0,     0,    26,    27,    85,     0,     0,
       0,     0,    29,    30,    31,     0,    86,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    34,    35,     0,    36,
       6,    37,    38,    39,   209,     0,    84,     0,     0,    11,
      12,    13,     0,     0,    16,     0,    18,    19,     0,    21,
       0,    23,     0,     0,     0,     0,    26,    27,    85,     0,
       0,     0,     0,    29,    30,    31,     0,    86,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    34,    35,     0,
      36,     6,    37,    38,    39,   308,     0,    84,     0,     0,
      11,    12,    13,     0,     0,    16,     0,    18,    19,     0,
      21,   168,    23,     0,     0,     0,     0,    26,    27,    85,
       0,     0,     0,     0,    29,    30,    31,     0,    86,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    34,    35,
       6,    36,     0,    37,    38,    39,    84,     0,     0,    11,
      12,    13,     0,     0,    16,     0,    18,    19,     0,    21,
       0,    23,     0,     0,     0,     0,    26,    27,    85,     0,
       0,     0,     0,    29,    30,    31,   230,    86,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    34,    35,     6,
      36,     0,    37,    38,    39,    84,     0,     0,    11,    12,
      13,     0,     0,    16,     0,    18,    19,     0,    21,     0,
      23,     0,     0,     0,     0,    26,    27,    85,     0,     0,
       0,     0,    29,    30,    31,     0,    86,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    34,    35,     0,    36,
       0,    37,    38,    39
};

static const short int yycheck[] =
{
       2,     1,   242,    14,     1,     7,    17,    43,     0,     5,
       1,    20,     6,   243,    44,   245,    12,    47,    43,    36,
      31,    16,     7,    43,   254,   255,    62,    21,    64,    23,
      91,    43,    26,    27,    15,    16,     6,    62,    36,    64,
      34,    35,   282,    37,    38,    40,    45,    47,   278,    49,
      47,    21,    49,    23,    33,    15,    26,    27,    49,    40,
     129,    46,   131,    43,    34,    35,   135,    37,    38,    47,
      75,    49,    51,    36,    34,    35,    33,    37,   318,    90,
     141,   142,   143,   152,   153,   154,    50,    98,    48,   319,
      95,    47,   332,    43,    51,    47,   107,   108,    43,    47,
      82,    83,    14,    47,    32,    17,    18,    19,    54,   111,
      22,    63,    24,    25,    96,    63,   118,    99,    52,    63,
      53,    91,   104,   192,    36,    34,    35,    38,    37,    41,
      42,    43,    39,    45,    34,    35,   205,    37,    44,    48,
      44,    47,    44,    47,   155,    47,    58,   158,    48,    44,
      62,    36,    47,   147,   148,   149,    41,    42,    55,    56,
      44,    46,    44,    47,   134,    47,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,   149,
     189,    30,   243,   244,   245,   246,   247,   248,   249,   250,
     251,   252,    36,   254,   255,    47,    44,    41,    42,    47,
      43,   270,   271,    44,   273,    47,    47,    43,    50,    48,
      57,    58,    59,    34,    35,    46,    47,   278,   351,   352,
     145,   146,     4,     5,    50,    50,   237,    46,    50,    47,
      36,   242,    36,    43,    36,    49,   238,    32,   307,    47,
      52,    54,   253,    43,    53,    38,    36,    12,    15,    50,
      48,    47,    44,    44,    10,    36,    45,    36,   319,     4,
      45,    44,    50,    49,   266,   267,    50,    45,    44,    49,
      44,   282,   283,   243,   244,   245,   246,   247,   248,   249,
     250,   251,   252,    49,   254,   255,    46,    46,    50,    46,
       0,   323,   183,   281,   195,   343,   190,   326,   136,   134,
     137,   244,   249,    11,   246,   138,   247,   318,   278,   248,
     139,   313,   140,   324,   250,    11,   206,   144,   320,   328,
      -1,   332,    -1,   204,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   333,    -1,    -1,    -1,   337,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     3,   348,    -1,     6,   319,
       8,     9,   354,    11,    -1,    13,    14,    -1,    -1,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    -1,    -1,    34,    35,    36,    -1,
      -1,    -1,    -1,    41,    42,    43,    -1,    45,    46,    -1,
      -1,    49,    -1,    -1,    -1,    -1,    -1,    55,    56,    -1,
      58,    -1,    60,    61,    62,     3,    -1,    -1,     6,    -1,
       8,     9,    -1,    11,    -1,    13,    14,    -1,    -1,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    -1,    -1,    34,    35,    36,    -1,
      -1,    -1,    -1,    41,    42,    43,    -1,    45,    -1,    -1,
      -1,    49,    -1,    -1,    -1,    -1,    -1,    55,    56,    -1,
      58,    -1,    60,    61,    62,     3,    -1,    -1,     6,    -1,
       8,     9,    -1,    11,    -1,    13,    14,    -1,    -1,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    -1,    -1,    34,    35,    36,    -1,
      -1,    -1,    -1,    41,    42,    43,    -1,    45,    -1,    -1,
      -1,    49,    -1,    -1,    -1,    -1,    -1,    55,    56,     8,
      58,    -1,    60,    61,    62,    14,    -1,    -1,    17,    18,
      19,    -1,    -1,    22,    -1,    24,    25,    -1,    27,    -1,
      29,    -1,    -1,    -1,    -1,    34,    35,    36,    -1,    -1,
      -1,    -1,    41,    42,    43,    -1,    45,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    55,    56,    -1,    58,
       8,    60,    61,    62,    63,    -1,    14,    -1,    -1,    17,
      18,    19,    -1,    -1,    22,    -1,    24,    25,    -1,    27,
      -1,    29,    -1,    -1,    -1,    -1,    34,    35,    36,    -1,
      -1,    -1,    -1,    41,    42,    43,    -1,    45,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    55,    56,    -1,
      58,     8,    60,    61,    62,    63,    -1,    14,    -1,    -1,
      17,    18,    19,    -1,    -1,    22,    -1,    24,    25,    -1,
      27,    28,    29,    -1,    -1,    -1,    -1,    34,    35,    36,
      -1,    -1,    -1,    -1,    41,    42,    43,    -1,    45,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    55,    56,
       8,    58,    -1,    60,    61,    62,    14,    -1,    -1,    17,
      18,    19,    -1,    -1,    22,    -1,    24,    25,    -1,    27,
      -1,    29,    -1,    -1,    -1,    -1,    34,    35,    36,    -1,
      -1,    -1,    -1,    41,    42,    43,    44,    45,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    55,    56,     8,
      58,    -1,    60,    61,    62,    14,    -1,    -1,    17,    18,
      19,    -1,    -1,    22,    -1,    24,    25,    -1,    27,    -1,
      29,    -1,    -1,    -1,    -1,    34,    35,    36,    -1,    -1,
      -1,    -1,    41,    42,    43,    -1,    45,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    55,    56,    -1,    58,
      -1,    60,    61,    62
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    66,    67,     0,     3,     6,     8,     9,    11,    13,
      14,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    34,    35,    36,    41,
      42,    43,    45,    49,    55,    56,    58,    60,    61,    62,
      68,    69,    73,    76,    77,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,   101,   102,   106,   109,   111,
     113,   115,   117,   119,   121,   123,   125,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   138,   139,   143,   147,
     148,    36,   146,   146,    14,    36,    45,   130,   132,    73,
      43,    43,    36,   146,   133,   134,   105,   106,    43,   106,
      45,    76,   130,    36,    78,    80,   130,    43,    43,   130,
     130,    50,   106,    36,    41,    42,    46,    73,    74,    75,
     144,   145,   130,   130,   130,   130,    47,   140,   141,   142,
       1,    47,    49,   149,    33,    51,    32,    52,    53,    54,
      38,    15,    16,    40,    39,    55,    56,    57,    58,    59,
      34,    35,    37,    48,    43,    62,    64,   136,    62,    64,
     136,   149,   149,    36,    41,    42,    30,   106,    28,   107,
     108,   110,   112,   114,   116,   118,   120,   122,   124,   126,
     127,   132,    43,    43,   136,   149,   106,   149,     5,    12,
     103,   104,    48,    82,    83,    47,   149,   106,   106,    73,
      44,    73,    46,    46,    47,    50,    47,    63,    47,    63,
     109,   109,   115,   109,   117,   119,   121,   123,   125,   127,
     127,   127,   128,   129,   129,   130,   130,   130,   109,   109,
      44,   109,   137,   106,    36,   106,    36,    43,    44,    36,
      79,    81,    49,    47,    33,    51,    32,    52,    53,    54,
      38,    16,    40,    15,    37,    48,    36,    71,    72,    72,
      44,    43,    76,   104,   109,    80,    44,    44,   145,   109,
     142,    50,    44,    47,    63,    63,   106,    73,    48,    84,
      85,    47,    49,    15,   105,   110,   132,   116,   110,   118,
     120,   122,   124,   126,   127,   127,   106,   110,   110,    47,
      44,    44,    45,    96,    36,    73,    73,    50,    63,   109,
     109,   109,    44,    10,   110,    81,   105,   106,    49,    50,
      44,    36,    45,    45,     4,    97,    98,    99,    44,   109,
      49,    73,    49,    44,   105,   110,    73,    67,    70,    70,
     106,     7,    46,   100,    99,    76,   105,    73,    44,    46,
      46,    50,    50,    97,    44,    73,    75,    75,    46,    73
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
#line 247 "parser.y"
    { program_parsed(ctx, (yyvsp[0].source_elements)); ;}
    break;

  case 3:
#line 251 "parser.y"
    { (yyval.source_elements) = new_source_elements(ctx); ;}
    break;

  case 4:
#line 253 "parser.y"
    { (yyval.source_elements) = source_elements_add_statement((yyvsp[-1].source_elements), (yyvsp[0].statement)); ;}
    break;

  case 5:
#line 255 "parser.y"
    { (yyval.source_elements) = source_elements_add_function((yyvsp[-1].source_elements), (yyvsp[0].function_declaration)); ;}
    break;

  case 6:
#line 260 "parser.y"
    { (yyval.function_declaration) = new_function_declaration(ctx, (yyvsp[-6].identifier), (yyvsp[-4].parameter_list), (yyvsp[-1].source_elements)); ;}
    break;

  case 7:
#line 265 "parser.y"
    { (yyval.expr) = new_function_expression(ctx, (yyvsp[-6].identifier), (yyvsp[-4].parameter_list), (yyvsp[-1].source_elements)); ;}
    break;

  case 8:
#line 269 "parser.y"
    { (yyval.source_elements) = (yyvsp[0].source_elements); ;}
    break;

  case 9:
#line 273 "parser.y"
    { (yyval.parameter_list) = new_parameter_list(ctx, (yyvsp[0].identifier)); ;}
    break;

  case 10:
#line 275 "parser.y"
    { (yyval.parameter_list) = parameter_list_add(ctx, (yyvsp[-2].parameter_list), (yyvsp[0].identifier)); ;}
    break;

  case 11:
#line 279 "parser.y"
    { (yyval.parameter_list) = NULL; ;}
    break;

  case 12:
#line 280 "parser.y"
    { (yyval.parameter_list) = (yyvsp[0].parameter_list); ;}
    break;

  case 13:
#line 284 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 14:
#line 285 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 15:
#line 286 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 16:
#line 287 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 17:
#line 288 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 18:
#line 289 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 19:
#line 290 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 20:
#line 291 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 21:
#line 292 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 22:
#line 293 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 23:
#line 294 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 24:
#line 295 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 25:
#line 296 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 26:
#line 297 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 27:
#line 301 "parser.y"
    { (yyval.statement_list) = new_statement_list(ctx, (yyvsp[0].statement)); ;}
    break;

  case 28:
#line 303 "parser.y"
    { (yyval.statement_list) = statement_list_add((yyvsp[-1].statement_list), (yyvsp[0].statement)); ;}
    break;

  case 29:
#line 307 "parser.y"
    { (yyval.statement_list) = NULL; ;}
    break;

  case 30:
#line 308 "parser.y"
    { (yyval.statement_list) = (yyvsp[0].statement_list); ;}
    break;

  case 31:
#line 313 "parser.y"
    { (yyval.statement) = new_block_statement(ctx, (yyvsp[-1].statement_list)); ;}
    break;

  case 32:
#line 318 "parser.y"
    { (yyval.statement) = new_var_statement(ctx, (yyvsp[-1].variable_list)); ;}
    break;

  case 33:
#line 322 "parser.y"
    { (yyval.variable_list) = new_variable_list(ctx, (yyvsp[0].variable_declaration)); ;}
    break;

  case 34:
#line 324 "parser.y"
    { (yyval.variable_list) = variable_list_add(ctx, (yyvsp[-2].variable_list), (yyvsp[0].variable_declaration)); ;}
    break;

  case 35:
#line 329 "parser.y"
    { (yyval.variable_list) = new_variable_list(ctx, (yyvsp[0].variable_declaration)); ;}
    break;

  case 36:
#line 331 "parser.y"
    { (yyval.variable_list) = variable_list_add(ctx, (yyvsp[-2].variable_list), (yyvsp[0].variable_declaration)); ;}
    break;

  case 37:
#line 336 "parser.y"
    { (yyval.variable_declaration) = new_variable_declaration(ctx, (yyvsp[-1].identifier), (yyvsp[0].expr)); ;}
    break;

  case 38:
#line 341 "parser.y"
    { (yyval.variable_declaration) = new_variable_declaration(ctx, (yyvsp[-1].identifier), (yyvsp[0].expr)); ;}
    break;

  case 39:
#line 345 "parser.y"
    { (yyval.expr) = NULL; ;}
    break;

  case 40:
#line 346 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 41:
#line 351 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 42:
#line 355 "parser.y"
    { (yyval.expr) = NULL; ;}
    break;

  case 43:
#line 356 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 44:
#line 361 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 45:
#line 365 "parser.y"
    { (yyval.statement) = new_empty_statement(ctx); ;}
    break;

  case 46:
#line 370 "parser.y"
    { (yyval.statement) = new_expression_statement(ctx, (yyvsp[-1].expr)); ;}
    break;

  case 47:
#line 375 "parser.y"
    { (yyval.statement) = new_if_statement(ctx, (yyvsp[-4].expr), (yyvsp[-2].statement), (yyvsp[0].statement)); ;}
    break;

  case 48:
#line 377 "parser.y"
    { (yyval.statement) = new_if_statement(ctx, (yyvsp[-2].expr), (yyvsp[0].statement), NULL); ;}
    break;

  case 49:
#line 382 "parser.y"
    { (yyval.statement) = new_while_statement(ctx, TRUE, (yyvsp[-2].expr), (yyvsp[-5].statement)); ;}
    break;

  case 50:
#line 384 "parser.y"
    { (yyval.statement) = new_while_statement(ctx, FALSE, (yyvsp[-2].expr), (yyvsp[0].statement)); ;}
    break;

  case 51:
#line 386 "parser.y"
    { (yyval.statement) = new_for_statement(ctx, NULL, (yyvsp[-6].expr), (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].statement)); ;}
    break;

  case 52:
#line 388 "parser.y"
    { (yyval.statement) = new_for_statement(ctx, (yyvsp[-6].variable_list), NULL, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].statement)); ;}
    break;

  case 53:
#line 390 "parser.y"
    { (yyval.statement) = new_forin_statement(ctx, NULL, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].statement)); ;}
    break;

  case 54:
#line 392 "parser.y"
    { (yyval.statement) = new_forin_statement(ctx, (yyvsp[-4].variable_declaration), NULL, (yyvsp[-2].expr), (yyvsp[0].statement)); ;}
    break;

  case 55:
#line 397 "parser.y"
    { (yyval.statement) = new_continue_statement(ctx, (yyvsp[-1].identifier)); ;}
    break;

  case 56:
#line 402 "parser.y"
    { (yyval.statement) = new_break_statement(ctx, (yyvsp[-1].identifier)); ;}
    break;

  case 57:
#line 407 "parser.y"
    { (yyval.statement) = new_return_statement(ctx, (yyvsp[-1].expr)); ;}
    break;

  case 58:
#line 412 "parser.y"
    { (yyval.statement) = new_with_statement(ctx, (yyvsp[-2].expr), (yyvsp[0].statement)); ;}
    break;

  case 59:
#line 417 "parser.y"
    { (yyval.statement) = new_labelled_statement(ctx, (yyvsp[-2].identifier), (yyvsp[0].statement)); ;}
    break;

  case 60:
#line 422 "parser.y"
    { (yyval.statement) = new_switch_statement(ctx, (yyvsp[-2].expr), (yyvsp[0].case_clausule)); ;}
    break;

  case 61:
#line 427 "parser.y"
    { (yyval.case_clausule) = new_case_block(ctx, (yyvsp[-1].case_list), NULL, NULL); ;}
    break;

  case 62:
#line 429 "parser.y"
    { (yyval.case_clausule) = new_case_block(ctx, (yyvsp[-3].case_list), (yyvsp[-2].case_clausule), (yyvsp[-1].case_list)); ;}
    break;

  case 63:
#line 433 "parser.y"
    { (yyval.case_list) = NULL; ;}
    break;

  case 64:
#line 434 "parser.y"
    { (yyval.case_list) = (yyvsp[0].case_list); ;}
    break;

  case 65:
#line 438 "parser.y"
    { (yyval.case_list) = new_case_list(ctx, (yyvsp[0].case_clausule)); ;}
    break;

  case 66:
#line 440 "parser.y"
    { (yyval.case_list) = case_list_add(ctx, (yyvsp[-1].case_list), (yyvsp[0].case_clausule)); ;}
    break;

  case 67:
#line 445 "parser.y"
    { (yyval.case_clausule) = new_case_clausule(ctx, (yyvsp[-2].expr), (yyvsp[0].statement_list)); ;}
    break;

  case 68:
#line 450 "parser.y"
    { (yyval.case_clausule) = new_case_clausule(ctx, NULL, (yyvsp[0].statement_list)); ;}
    break;

  case 69:
#line 455 "parser.y"
    { (yyval.statement) = new_throw_statement(ctx, (yyvsp[-1].expr)); ;}
    break;

  case 70:
#line 459 "parser.y"
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[-1].statement), (yyvsp[0].catch_block), NULL); ;}
    break;

  case 71:
#line 460 "parser.y"
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[-1].statement), NULL, (yyvsp[0].statement)); ;}
    break;

  case 72:
#line 462 "parser.y"
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[-2].statement), (yyvsp[-1].catch_block), (yyvsp[0].statement)); ;}
    break;

  case 73:
#line 467 "parser.y"
    { (yyval.catch_block) = new_catch_block(ctx, (yyvsp[-2].identifier), (yyvsp[0].statement)); ;}
    break;

  case 74:
#line 471 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); ;}
    break;

  case 75:
#line 475 "parser.y"
    { (yyval.expr) = NULL; ;}
    break;

  case 76:
#line 476 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 77:
#line 480 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 78:
#line 482 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_COMMA, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 79:
#line 486 "parser.y"
    { (yyval.expr) = NULL; ;}
    break;

  case 80:
#line 487 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 81:
#line 492 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 82:
#line 494 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_COMMA, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 83:
#line 498 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 84:
#line 500 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ASSIGN, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 85:
#line 502 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 86:
#line 507 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 87:
#line 509 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ASSIGN, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 88:
#line 511 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 89:
#line 515 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 90:
#line 517 "parser.y"
    { (yyval.expr) = new_conditional_expression(ctx, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 91:
#line 522 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 92:
#line 524 "parser.y"
    { (yyval.expr) = new_conditional_expression(ctx, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 93:
#line 528 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 94:
#line 530 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_OR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 95:
#line 535 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 96:
#line 537 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_OR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 97:
#line 541 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 98:
#line 543 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_AND, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 99:
#line 548 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 100:
#line 550 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_AND, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 101:
#line 554 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 102:
#line 556 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BOR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 103:
#line 561 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 104:
#line 563 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BOR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 105:
#line 567 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 106:
#line 569 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BXOR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 107:
#line 574 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 108:
#line 576 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BXOR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 109:
#line 580 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 110:
#line 582 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BAND, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 111:
#line 587 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 112:
#line 589 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BAND, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 113:
#line 593 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 114:
#line 595 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 115:
#line 599 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 116:
#line 601 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 117:
#line 605 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 118:
#line 607 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 119:
#line 609 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_INSTANCEOF, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 120:
#line 611 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_IN, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 121:
#line 615 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 122:
#line 617 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 123:
#line 619 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_INSTANCEOF, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 124:
#line 623 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 125:
#line 625 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 126:
#line 630 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 127:
#line 632 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ADD, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 128:
#line 634 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_SUB, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 129:
#line 638 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 130:
#line 640 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_MUL, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 131:
#line 642 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_DIV, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 132:
#line 644 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_MOD, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 133:
#line 648 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 134:
#line 650 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_DELETE, (yyvsp[0].expr)); ;}
    break;

  case 135:
#line 651 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_VOID, (yyvsp[0].expr)); ;}
    break;

  case 136:
#line 653 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_TYPEOF, (yyvsp[0].expr)); ;}
    break;

  case 137:
#line 654 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PREINC, (yyvsp[0].expr)); ;}
    break;

  case 138:
#line 655 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PREDEC, (yyvsp[0].expr)); ;}
    break;

  case 139:
#line 656 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PLUS, (yyvsp[0].expr)); ;}
    break;

  case 140:
#line 657 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_MINUS, (yyvsp[0].expr)); ;}
    break;

  case 141:
#line 658 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_BITNEG, (yyvsp[0].expr)); ;}
    break;

  case 142:
#line 659 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_LOGNEG, (yyvsp[0].expr)); ;}
    break;

  case 143:
#line 664 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 144:
#line 666 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_POSTINC, (yyvsp[-1].expr)); ;}
    break;

  case 145:
#line 668 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_POSTDEC, (yyvsp[-1].expr)); ;}
    break;

  case 146:
#line 673 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 147:
#line 674 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 148:
#line 678 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 149:
#line 679 "parser.y"
    { (yyval.expr) = new_new_expression(ctx, (yyvsp[0].expr), NULL); ;}
    break;

  case 150:
#line 683 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 151:
#line 684 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 152:
#line 686 "parser.y"
    { (yyval.expr) = new_array_expression(ctx, (yyvsp[-3].expr), (yyvsp[-1].expr)); ;}
    break;

  case 153:
#line 688 "parser.y"
    { (yyval.expr) = new_member_expression(ctx, (yyvsp[-2].expr), (yyvsp[0].identifier)); ;}
    break;

  case 154:
#line 690 "parser.y"
    { (yyval.expr) = new_new_expression(ctx, (yyvsp[-1].expr), (yyvsp[0].argument_list)); ;}
    break;

  case 155:
#line 695 "parser.y"
    { (yyval.expr) = new_call_expression(ctx, (yyvsp[-1].expr), (yyvsp[0].argument_list)); ;}
    break;

  case 156:
#line 697 "parser.y"
    { (yyval.expr) = new_call_expression(ctx, (yyvsp[-1].expr), (yyvsp[0].argument_list)); ;}
    break;

  case 157:
#line 699 "parser.y"
    { (yyval.expr) = new_array_expression(ctx, (yyvsp[-3].expr), (yyvsp[-1].expr)); ;}
    break;

  case 158:
#line 701 "parser.y"
    { (yyval.expr) = new_member_expression(ctx, (yyvsp[-2].expr), (yyvsp[0].identifier)); ;}
    break;

  case 159:
#line 705 "parser.y"
    { (yyval.argument_list) = NULL; ;}
    break;

  case 160:
#line 706 "parser.y"
    { (yyval.argument_list) = (yyvsp[-1].argument_list); ;}
    break;

  case 161:
#line 710 "parser.y"
    { (yyval.argument_list) = new_argument_list(ctx, (yyvsp[0].expr)); ;}
    break;

  case 162:
#line 712 "parser.y"
    { (yyval.argument_list) = argument_list_add(ctx, (yyvsp[-2].argument_list), (yyvsp[0].expr)); ;}
    break;

  case 163:
#line 716 "parser.y"
    { (yyval.expr) = new_this_expression(ctx); ;}
    break;

  case 164:
#line 717 "parser.y"
    { (yyval.expr) = new_identifier_expression(ctx, (yyvsp[0].identifier)); ;}
    break;

  case 165:
#line 718 "parser.y"
    { (yyval.expr) = new_literal_expression(ctx, (yyvsp[0].literal)); ;}
    break;

  case 166:
#line 719 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 167:
#line 720 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 168:
#line 721 "parser.y"
    { (yyval.expr) = (yyvsp[-1].expr); ;}
    break;

  case 169:
#line 725 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, NULL, (yyvsp[-1].ival)); ;}
    break;

  case 170:
#line 726 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, (yyvsp[-1].element_list), 0); ;}
    break;

  case 171:
#line 728 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, (yyvsp[-3].element_list), (yyvsp[-1].ival)); ;}
    break;

  case 172:
#line 733 "parser.y"
    { (yyval.element_list) = new_element_list(ctx, (yyvsp[-1].ival), (yyvsp[0].expr)); ;}
    break;

  case 173:
#line 735 "parser.y"
    { (yyval.element_list) = element_list_add(ctx, (yyvsp[-3].element_list), (yyvsp[-1].ival), (yyvsp[0].expr)); ;}
    break;

  case 174:
#line 739 "parser.y"
    { (yyval.ival) = 1; ;}
    break;

  case 175:
#line 740 "parser.y"
    { (yyval.ival) = (yyvsp[-1].ival) + 1; ;}
    break;

  case 176:
#line 744 "parser.y"
    { (yyval.ival) = 0; ;}
    break;

  case 177:
#line 745 "parser.y"
    { (yyval.ival) = (yyvsp[0].ival); ;}
    break;

  case 178:
#line 749 "parser.y"
    { (yyval.expr) = new_prop_and_value_expression(ctx, NULL); ;}
    break;

  case 179:
#line 751 "parser.y"
    { (yyval.expr) = new_prop_and_value_expression(ctx, (yyvsp[-1].property_list)); ;}
    break;

  case 180:
#line 756 "parser.y"
    { (yyval.property_list) = new_property_list(ctx, (yyvsp[-2].literal), (yyvsp[0].expr)); ;}
    break;

  case 181:
#line 758 "parser.y"
    { (yyval.property_list) = property_list_add(ctx, (yyvsp[-4].property_list), (yyvsp[-2].literal), (yyvsp[0].expr)); ;}
    break;

  case 182:
#line 762 "parser.y"
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[0].identifier)); ;}
    break;

  case 183:
#line 763 "parser.y"
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[0].wstr)); ;}
    break;

  case 184:
#line 764 "parser.y"
    { (yyval.literal) = (yyvsp[0].literal); ;}
    break;

  case 185:
#line 768 "parser.y"
    { (yyval.identifier) = NULL; ;}
    break;

  case 186:
#line 769 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); ;}
    break;

  case 187:
#line 773 "parser.y"
    { (yyval.literal) = new_null_literal(ctx); ;}
    break;

  case 188:
#line 774 "parser.y"
    { (yyval.literal) = new_undefined_literal(ctx); ;}
    break;

  case 189:
#line 775 "parser.y"
    { (yyval.literal) = (yyvsp[0].literal); ;}
    break;

  case 190:
#line 776 "parser.y"
    { (yyval.literal) = (yyvsp[0].literal); ;}
    break;

  case 191:
#line 777 "parser.y"
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[0].wstr)); ;}
    break;

  case 192:
#line 778 "parser.y"
    { (yyval.literal) = parse_regexp(ctx);
                                  if(!(yyval.literal)) YYABORT; ;}
    break;

  case 193:
#line 783 "parser.y"
    { (yyval.literal) = new_boolean_literal(ctx, TRUE); ;}
    break;

  case 194:
#line 784 "parser.y"
    { (yyval.literal) = new_boolean_literal(ctx, FALSE); ;}
    break;

  case 196:
#line 788 "parser.y"
    { if(!allow_auto_semicolon(ctx)) {YYABORT;} ;}
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
#line 2737 "parser.tab.c"

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


#line 790 "parser.y"


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

    ret->identifier = identifier;
    ret->expr = expr;
    ret->next = NULL;

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
       parameter_list_t *parameter_list, source_elements_t *source_elements)
{
    function_expression_t *ret = parser_alloc(ctx, sizeof(function_expression_t));

    ret->expr.eval = function_expression_eval;
    ret->identifier = identifier;
    ret->parameter_list = parameter_list ? parameter_list->head : NULL;
    ret->source_elements = source_elements;

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

static function_declaration_t *new_function_declaration(parser_ctx_t *ctx, const WCHAR *identifier,
       parameter_list_t *parameter_list, source_elements_t *source_elements)
{
    function_declaration_t *ret = parser_alloc(ctx, sizeof(function_declaration_t));

    ret->identifier = identifier;
    ret->parameter_list = parameter_list ? parameter_list->head : NULL;
    ret->source_elements = source_elements;
    ret->next = NULL;

    return ret;
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

static source_elements_t *source_elements_add_function(source_elements_t *source_elements,
       function_declaration_t *function_declaration)
{
    if(source_elements->functions_tail)
        source_elements->functions_tail = source_elements->functions_tail->next = function_declaration;
    else
        source_elements->functions = source_elements->functions_tail = function_declaration;

    return source_elements;
}

statement_list_t *new_statement_list(parser_ctx_t *ctx, statement_t *statement)
{
    statement_list_t *ret =  parser_alloc_tmp(ctx, sizeof(statement_list_t));

    ret->head = ret->tail = statement;

    return ret;
}

statement_list_t *statement_list_add(statement_list_t *list, statement_t *statement)
{
    list->tail = list->tail->next = statement;

    return list;
}

static void program_parsed(parser_ctx_t *ctx, source_elements_t *source)
{
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

