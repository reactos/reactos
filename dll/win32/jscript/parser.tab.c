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

/* Line 268 of yacc.c  */
#line 19 "parser.y"

#include "jscript.h"

static int parser_error(parser_ctx_t*,const char*);
static void set_error(parser_ctx_t*,UINT);
static BOOL explicit_error(parser_ctx_t*,void*,WCHAR);
static BOOL allow_auto_semicolon(parser_ctx_t*);
static void program_parsed(parser_ctx_t*,source_elements_t*);

typedef struct _statement_list_t {
    statement_t *head;
    statement_t *tail;
} statement_list_t;

static literal_t *new_string_literal(parser_ctx_t*,const WCHAR*);
static literal_t *new_null_literal(parser_ctx_t*);

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

static void *new_statement(parser_ctx_t*,statement_type_t,size_t);
static statement_t *new_block_statement(parser_ctx_t*,statement_list_t*);
static statement_t *new_var_statement(parser_ctx_t*,variable_list_t*);
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

static void *new_expression(parser_ctx_t *ctx,expression_type_t,size_t);
static expression_t *new_function_expression(parser_ctx_t*,const WCHAR*,parameter_list_t*,
        source_elements_t*,const WCHAR*,DWORD);
static expression_t *new_binary_expression(parser_ctx_t*,expression_type_t,expression_t*,expression_t*);
static expression_t *new_unary_expression(parser_ctx_t*,expression_type_t,expression_t*);
static expression_t *new_conditional_expression(parser_ctx_t*,expression_t*,expression_t*,expression_t*);
static expression_t *new_member_expression(parser_ctx_t*,expression_t*,const WCHAR*);
static expression_t *new_new_expression(parser_ctx_t*,expression_t*,argument_list_t*);
static expression_t *new_call_expression(parser_ctx_t*,expression_t*,argument_list_t*);
static expression_t *new_identifier_expression(parser_ctx_t*,const WCHAR*);
static expression_t *new_literal_expression(parser_ctx_t*,literal_t*);
static expression_t *new_array_literal_expression(parser_ctx_t*,element_list_t*,int);
static expression_t *new_prop_and_value_expression(parser_ctx_t*,property_list_t*);

static source_elements_t *new_source_elements(parser_ctx_t*);
static source_elements_t *source_elements_add_statement(source_elements_t*,statement_t*);



/* Line 268 of yacc.c  */
#line 200 "parser.tab.c"

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
     tBooleanLiteral = 298,
     tStringLiteral = 299,
     tEOF = 300,
     LOWER_THAN_ELSE = 301
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 293 of yacc.c  */
#line 144 "parser.y"

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



/* Line 293 of yacc.c  */
#line 305 "parser.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 343 of yacc.c  */
#line 317 "parser.tab.c"

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
#define YYLAST   1053

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  69
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  95
/* YYNRULES -- Number of rules.  */
#define YYNRULES  215
/* YYNRULES -- Number of states.  */
#define YYNSTATES  375

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   301

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    63,     2,     2,     2,    61,    56,     2,
      67,    68,    59,    57,    49,    58,    66,    60,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    52,    51,
       2,    50,     2,    53,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    64,     2,    65,    55,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    48,    54,    37,    62,     2,     2,     2,
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
      35,    36,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     7,     9,    10,    11,    14,    23,    25,
      27,    29,    33,    34,    36,    38,    40,    42,    44,    46,
      48,    50,    52,    54,    56,    58,    60,    62,    64,    66,
      68,    71,    72,    74,    78,    81,    85,    87,    91,    93,
      97,   100,   103,   104,   106,   109,   110,   112,   115,   117,
     120,   128,   134,   142,   148,   149,   150,   162,   163,   164,
     177,   185,   194,   198,   202,   206,   212,   216,   222,   226,
     232,   233,   235,   237,   240,   245,   249,   253,   257,   261,
     266,   272,   275,   276,   278,   280,   282,   284,   288,   289,
     291,   293,   297,   299,   301,   303,   307,   311,   313,   317,
     321,   323,   329,   331,   337,   339,   343,   345,   349,   351,
     355,   357,   361,   363,   367,   369,   373,   375,   379,   381,
     385,   387,   391,   393,   397,   399,   403,   405,   409,   411,
     415,   419,   423,   425,   429,   433,   435,   439,   441,   445,
     449,   451,   455,   459,   463,   465,   468,   471,   474,   477,
     480,   483,   486,   489,   492,   494,   497,   500,   502,   504,
     506,   509,   511,   513,   518,   522,   526,   529,   532,   537,
     541,   544,   548,   550,   554,   556,   558,   560,   562,   564,
     568,   571,   575,   579,   585,   588,   593,   595,   598,   599,
     601,   604,   608,   612,   618,   620,   622,   624,   625,   627,
     629,   631,   633,   635,   637,   639,   641,   643,   645,   647,
     649,   651,   653,   655,   657,   659
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      70,     0,    -1,    72,    71,    46,    -1,    34,    -1,    -1,
      -1,    72,    78,    -1,    74,   157,   161,    77,   162,    48,
      75,    37,    -1,    36,    -1,    72,    -1,    38,    -1,    76,
      49,    38,    -1,    -1,    76,    -1,    81,    -1,    82,    -1,
      91,    -1,    73,    -1,    92,    -1,    93,    -1,    94,    -1,
      99,    -1,   100,    -1,   101,    -1,   102,    -1,   103,    -1,
     104,    -1,   110,    -1,   111,    -1,    78,    -1,    79,    78,
      -1,    -1,    79,    -1,    48,    79,    37,    -1,    48,    37,
      -1,    26,    83,   160,    -1,    85,    -1,    83,    49,    85,
      -1,    86,    -1,    84,    49,    86,    -1,    38,    87,    -1,
      38,    89,    -1,    -1,    88,    -1,    50,   120,    -1,    -1,
      90,    -1,    50,   121,    -1,    51,    -1,   116,   160,    -1,
      11,   161,   115,   162,    78,    10,    78,    -1,    11,   161,
     115,   162,    78,    -1,     9,    78,    28,   161,   115,   162,
     160,    -1,    28,   161,   115,   162,    78,    -1,    -1,    -1,
      13,   161,   117,    95,   163,   114,    96,   163,   114,   162,
      78,    -1,    -1,    -1,    13,   161,    26,    84,    97,   163,
     114,    98,   163,   114,   162,    78,    -1,    13,   161,   143,
      14,   115,   162,    78,    -1,    13,   161,    26,    86,    14,
     115,   162,    78,    -1,     6,   157,   160,    -1,     3,   157,
     160,    -1,    18,   114,   160,    -1,    29,   161,   116,   162,
      78,    -1,    38,    52,    78,    -1,    19,   161,   116,   162,
     105,    -1,    48,   106,    37,    -1,    48,   106,   109,   106,
      37,    -1,    -1,   107,    -1,   108,    -1,   107,   108,    -1,
       4,   116,    52,    80,    -1,     7,    52,    80,    -1,    21,
     116,   160,    -1,    24,    81,   112,    -1,    24,    81,   113,
      -1,    24,    81,   112,   113,    -1,     5,   161,    38,   162,
      81,    -1,    12,    81,    -1,    -1,   116,    -1,   116,    -1,
       1,    -1,   120,    -1,   116,    49,   120,    -1,    -1,   118,
      -1,   121,    -1,   118,    49,   121,    -1,    39,    -1,    35,
      -1,   122,    -1,   143,    50,   120,    -1,   143,   119,   120,
      -1,   123,    -1,   143,    50,   121,    -1,   143,   119,   121,
      -1,   124,    -1,   124,    53,   120,    52,   120,    -1,   125,
      -1,   125,    53,   121,    52,   121,    -1,   126,    -1,   124,
      31,   126,    -1,   127,    -1,   125,    31,   127,    -1,   128,
      -1,   126,    30,   128,    -1,   129,    -1,   127,    30,   129,
      -1,   130,    -1,   128,    54,   130,    -1,   131,    -1,   129,
      54,   131,    -1,   132,    -1,   130,    55,   132,    -1,   133,
      -1,   131,    55,   133,    -1,   134,    -1,   132,    56,   134,
      -1,   135,    -1,   133,    56,   135,    -1,   136,    -1,   134,
      40,   136,    -1,   137,    -1,   135,    40,   137,    -1,   138,
      -1,   136,    42,   138,    -1,   136,    15,   138,    -1,   136,
      14,   138,    -1,   138,    -1,   137,    42,   138,    -1,   137,
      15,   138,    -1,   139,    -1,   138,    41,   139,    -1,   140,
      -1,   139,    57,   140,    -1,   139,    58,   140,    -1,   141,
      -1,   140,    59,   141,    -1,   140,    60,   141,    -1,   140,
      61,   141,    -1,   142,    -1,     8,   141,    -1,    27,   141,
      -1,    25,   141,    -1,    32,   141,    -1,    33,   141,    -1,
      57,   141,    -1,    58,   141,    -1,    62,   141,    -1,    63,
     141,    -1,   143,    -1,   143,    32,    -1,   143,    33,    -1,
     144,    -1,   146,    -1,   145,    -1,    16,   144,    -1,   149,
      -1,    73,    -1,   145,    64,   116,    65,    -1,   145,    66,
      38,    -1,    16,   145,   147,    -1,   145,   147,    -1,   146,
     147,    -1,   146,    64,   116,    65,    -1,   146,    66,    38,
      -1,    67,    68,    -1,    67,   148,    68,    -1,   120,    -1,
     148,    49,   120,    -1,    20,    -1,    38,    -1,   158,    -1,
     150,    -1,   154,    -1,    67,   116,    68,    -1,    64,    65,
      -1,    64,   152,    65,    -1,    64,   151,    65,    -1,    64,
     151,    49,   153,    65,    -1,   153,   120,    -1,   151,    49,
     153,   120,    -1,    49,    -1,   152,    49,    -1,    -1,   152,
      -1,    48,    37,    -1,    48,   155,    37,    -1,   156,    52,
     120,    -1,   155,    49,   156,    52,   120,    -1,    38,    -1,
      45,    -1,    43,    -1,    -1,    38,    -1,    17,    -1,   159,
      -1,    43,    -1,    45,    -1,    60,    -1,    35,    -1,    22,
      -1,    23,    -1,    44,    -1,    51,    -1,     1,    -1,    67,
      -1,     1,    -1,    68,    -1,     1,    -1,    51,    -1,     1,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   251,   251,   255,   256,   260,   261,   266,   270,   274,
     278,   279,   284,   285,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   307,
     308,   313,   314,   318,   319,   323,   328,   329,   334,   336,
     341,   346,   351,   352,   356,   361,   362,   366,   371,   375,
     380,   382,   387,   389,   392,   394,   391,   398,   400,   397,
     403,   405,   410,   415,   420,   425,   430,   435,   440,   442,
     447,   448,   452,   453,   458,   463,   468,   473,   474,   475,
     480,   485,   489,   490,   493,   494,   498,   499,   504,   505,
     509,   511,   515,   516,   520,   521,   523,   528,   530,   532,
     537,   538,   543,   545,   550,   551,   556,   558,   563,   564,
     569,   571,   576,   577,   582,   584,   589,   590,   595,   597,
     602,   603,   608,   610,   615,   616,   621,   622,   627,   628,
     630,   632,   637,   638,   640,   645,   646,   651,   653,   655,
     660,   661,   663,   665,   670,   671,   673,   674,   676,   677,
     678,   679,   680,   681,   685,   687,   689,   695,   696,   700,
     701,   705,   706,   707,   709,   711,   716,   718,   720,   722,
     727,   728,   732,   733,   738,   739,   740,   741,   742,   743,
     747,   748,   749,   750,   755,   757,   762,   763,   767,   768,
     772,   773,   778,   780,   785,   786,   787,   791,   792,   796,
     797,   798,   799,   800,   802,   807,   808,   809,   812,   813,
     816,   817,   820,   821,   824,   825
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
  "tShiftOper", "tRelOper", "tNumericLiteral", "tBooleanLiteral",
  "tStringLiteral", "tEOF", "LOWER_THAN_ELSE", "'{'", "','", "'='", "';'",
  "':'", "'?'", "'|'", "'^'", "'&'", "'+'", "'-'", "'*'", "'/'", "'%'",
  "'~'", "'!'", "'['", "']'", "'.'", "'('", "')'", "$accept", "Program",
  "HtmlComment", "SourceElements", "FunctionExpression", "KFunction",
  "FunctionBody", "FormalParameterList", "FormalParameterList_opt",
  "Statement", "StatementList", "StatementList_opt", "Block",
  "VariableStatement", "VariableDeclarationList",
  "VariableDeclarationListNoIn", "VariableDeclaration",
  "VariableDeclarationNoIn", "Initialiser_opt", "Initialiser",
  "InitialiserNoIn_opt", "InitialiserNoIn", "EmptyStatement",
  "ExpressionStatement", "IfStatement", "IterationStatement", "$@1", "$@2",
  "$@3", "$@4", "ContinueStatement", "BreakStatement", "ReturnStatement",
  "WithStatement", "LabelledStatement", "SwitchStatement", "CaseBlock",
  "CaseClausules_opt", "CaseClausules", "CaseClausule", "DefaultClausule",
  "ThrowStatement", "TryStatement", "Catch", "Finally", "Expression_opt",
  "Expression_err", "Expression", "ExpressionNoIn_opt", "ExpressionNoIn",
  "AssignOper", "AssignmentExpression", "AssignmentExpressionNoIn",
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
     294,   295,   296,   297,   298,   299,   300,   301,   123,    44,
      61,    59,    58,    63,   124,    94,    38,    43,    45,    42,
      47,    37,   126,    33,    91,    93,    46,    40,    41
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    69,    70,    71,    71,    72,    72,    73,    74,    75,
      76,    76,    77,    77,    78,    78,    78,    78,    78,    78,
      78,    78,    78,    78,    78,    78,    78,    78,    78,    79,
      79,    80,    80,    81,    81,    82,    83,    83,    84,    84,
      85,    86,    87,    87,    88,    89,    89,    90,    91,    92,
      93,    93,    94,    94,    95,    96,    94,    97,    98,    94,
      94,    94,    99,   100,   101,   102,   103,   104,   105,   105,
     106,   106,   107,   107,   108,   109,   110,   111,   111,   111,
     112,   113,   114,   114,   115,   115,   116,   116,   117,   117,
     118,   118,   119,   119,   120,   120,   120,   121,   121,   121,
     122,   122,   123,   123,   124,   124,   125,   125,   126,   126,
     127,   127,   128,   128,   129,   129,   130,   130,   131,   131,
     132,   132,   133,   133,   134,   134,   135,   135,   136,   136,
     136,   136,   137,   137,   137,   138,   138,   139,   139,   139,
     140,   140,   140,   140,   141,   141,   141,   141,   141,   141,
     141,   141,   141,   141,   142,   142,   142,   143,   143,   144,
     144,   145,   145,   145,   145,   145,   146,   146,   146,   146,
     147,   147,   148,   148,   149,   149,   149,   149,   149,   149,
     150,   150,   150,   150,   151,   151,   152,   152,   153,   153,
     154,   154,   155,   155,   156,   156,   156,   157,   157,   158,
     158,   158,   158,   158,   158,   159,   159,   159,   160,   160,
     161,   161,   162,   162,   163,   163
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     3,     1,     0,     0,     2,     8,     1,     1,
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

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       5,     0,     4,     1,   197,   197,     0,     0,     0,     0,
       0,   199,    82,     0,   174,     0,   205,   206,     0,     0,
       0,     0,     0,     0,     0,     0,     3,   204,     8,   175,
     201,   207,   202,     0,    48,     0,     0,   203,     0,     0,
     188,     0,     0,    17,   197,     6,    14,    15,    16,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
       0,    86,    94,   100,   104,   108,   112,   116,   120,   124,
     128,   135,   137,   140,   144,   154,   157,   159,   158,   161,
     177,   178,   176,   200,   198,     0,     0,   175,     0,   162,
     145,   154,     0,   211,   210,     0,    88,   160,   159,     0,
      83,     0,     0,     0,     0,   147,    42,     0,    36,   146,
       0,     0,   148,   149,     0,    34,   175,   201,   202,    29,
       0,     0,     0,   150,   151,   152,   153,   186,   180,     0,
     189,     0,     0,     2,     0,   209,     0,   208,    49,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   155,   156,    93,    92,     0,
       0,     0,     0,     0,   166,     0,     0,   167,    63,    62,
     190,   194,   196,   195,     0,    85,     0,    84,     0,    54,
      89,    90,    97,   102,   106,   110,   114,   118,   122,   126,
     132,   154,   165,    64,     0,    76,    34,     0,     0,    77,
      78,     0,    40,    43,     0,    35,     0,     0,    66,    33,
      30,   191,     0,     0,   188,   182,   187,   181,   184,   179,
      12,    87,   105,     0,   109,   113,   117,   121,   125,   131,
     130,   129,   136,   138,   139,   141,   142,   143,    95,    96,
       0,   164,   170,   172,     0,     0,   169,     0,   213,   212,
       0,    45,    57,    38,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      81,    79,    44,    37,     0,     0,     0,   192,   189,     0,
      10,    13,     0,     0,   163,     0,   171,   168,     0,    51,
       0,    41,    46,     0,     0,     0,   215,   214,    82,    91,
     154,   107,     0,   111,   115,   119,   123,   127,   134,   133,
       0,    98,    99,    70,    67,     0,    53,    65,     0,   183,
     185,     0,     0,   101,   173,     0,     0,    47,    39,    82,
       0,    55,     0,     0,     0,     0,    71,    72,     0,   193,
      11,     5,    52,    50,    58,     0,     0,   103,    60,     0,
       0,    68,    70,    73,    80,     9,     0,     0,    61,    82,
      31,    31,     0,     7,    82,     0,    32,    74,    75,    69,
       0,     0,     0,    56,    59
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    42,     2,    89,    44,   356,   281,   282,   119,
     120,   367,    46,    47,   107,   252,   108,   253,   202,   203,
     291,   292,    48,    49,    50,    51,   254,   346,   294,   357,
      52,    53,    54,    55,    56,    57,   314,   335,   336,   337,
     352,    58,    59,   199,   200,    99,   176,    60,   179,   180,
     267,    61,   181,    62,   182,    63,   183,    64,   184,    65,
     185,    66,   186,    67,   187,    68,   188,    69,   189,    70,
      71,    72,    73,    74,    75,    76,    77,    78,   164,   244,
      79,    80,   129,   130,   131,    81,   121,   122,    85,    82,
      83,   138,    95,   250,   298
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -293
static const yytype_int16 yypact[] =
{
    -293,    53,   496,  -293,    26,    26,   956,   744,     8,     8,
     986,  -293,   956,     8,  -293,   956,  -293,  -293,    39,   956,
      71,   956,     8,     8,   956,   956,  -293,  -293,  -293,    67,
    -293,  -293,  -293,   558,  -293,   956,   956,  -293,   956,   956,
      41,   956,    76,   410,    26,  -293,  -293,  -293,  -293,  -293,
    -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,
      19,  -293,  -293,    -7,   102,   100,   108,   111,   135,    36,
     142,   -43,    97,  -293,  -293,   129,  -293,   105,   118,  -293,
    -293,  -293,  -293,  -293,  -293,     7,     7,  -293,   176,  -293,
    -293,    49,   158,  -293,  -293,   372,   850,  -293,   105,     7,
     140,   956,    19,   620,   125,  -293,   153,    47,  -293,  -293,
     372,   956,  -293,  -293,   744,   442,    67,   155,   163,  -293,
     682,    80,   165,  -293,  -293,  -293,  -293,  -293,  -293,    42,
      48,   956,    31,  -293,     8,  -293,   956,  -293,  -293,   956,
     956,   956,   956,   956,   956,   956,   956,   956,   956,   956,
     956,   956,   956,   956,   956,  -293,  -293,  -293,  -293,   956,
     956,   956,   180,   797,  -293,   956,   182,  -293,  -293,  -293,
    -293,  -293,  -293,  -293,     8,  -293,     3,   140,   184,  -293,
     181,  -293,  -293,    35,   195,   172,   178,   175,   192,    23,
     142,   120,  -293,  -293,    11,  -293,  -293,     8,    39,   222,
    -293,   956,  -293,  -293,    71,  -293,     3,    11,  -293,  -293,
    -293,  -293,    90,   956,   186,  -293,  -293,  -293,  -293,  -293,
     198,  -293,   102,   185,   100,   108,   111,   135,    36,   142,
     142,   142,   -43,    97,    97,  -293,  -293,  -293,  -293,  -293,
      56,  -293,  -293,  -293,    34,    66,  -293,   372,  -293,  -293,
     744,   188,   190,   226,    10,   956,   956,   956,   956,   956,
     956,   956,   956,   956,   956,   372,   956,   956,   193,   204,
    -293,  -293,  -293,  -293,   744,   744,   194,  -293,   196,   903,
    -293,   202,     3,   956,  -293,   956,  -293,  -293,     3,   234,
     956,  -293,  -293,   184,    10,   372,  -293,  -293,   956,  -293,
     177,   195,   214,   172,   178,   175,   192,    23,   142,   142,
       3,  -293,  -293,   263,  -293,     3,  -293,  -293,   956,  -293,
    -293,   230,   221,  -293,  -293,     7,   744,  -293,  -293,   956,
       3,  -293,   956,   744,   956,     6,   263,  -293,    39,  -293,
    -293,  -293,  -293,  -293,  -293,   744,    10,  -293,  -293,    59,
     219,  -293,   263,  -293,  -293,   744,   239,    10,  -293,   956,
     744,   744,   240,  -293,   956,     3,   744,  -293,  -293,  -293,
       3,   744,   744,  -293,  -293
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -293,  -293,  -293,   -63,    -2,  -293,  -293,  -293,  -293,     0,
    -187,   -82,   -17,  -293,  -293,  -293,    77,   -10,  -293,  -293,
    -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,
    -293,  -293,  -293,  -293,  -293,  -293,  -293,   -68,  -293,   -51,
    -293,  -293,  -293,  -293,    87,  -292,   -87,    15,  -293,  -293,
     213,   -36,  -238,  -293,  -293,  -293,  -293,   150,    40,   149,
      33,   151,    43,   152,    44,   148,    45,   156,    46,   -62,
     145,    37,    38,  -293,    -3,   287,   288,  -293,    -9,  -293,
    -293,  -293,  -293,    86,    89,  -293,  -293,    95,     5,  -293,
    -293,   -60,    32,    17,  -254
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -197
static const yytype_int16 yytable[] =
{
      43,   104,    45,    91,   248,    43,   331,    92,   135,    93,
      86,   296,   248,   350,   150,   151,    91,   299,    91,   302,
     135,    91,    91,   206,   139,   168,   169,   100,   311,   312,
     102,    43,    91,    91,   190,    91,    91,   344,   263,   193,
     329,    96,   195,   351,    90,   101,   140,   205,   135,   134,
     146,   147,   327,     3,   110,   111,   132,   105,   137,   109,
     136,   297,   112,   113,    84,   264,   256,   365,   136,   167,
     137,   249,   370,   123,   124,    94,   125,   126,   148,   249,
     136,   155,   156,   285,   229,   230,   231,   103,   257,   192,
     127,   214,   359,   191,   347,   218,   204,   216,   137,   219,
     221,    43,   286,   364,   223,   136,   128,   215,   136,   106,
     177,   360,    43,   217,   208,   136,   194,   211,    43,   114,
     210,   284,   133,   238,   239,   177,   207,   243,   171,   212,
     197,   287,   141,   172,   265,   173,    91,   198,    91,    91,
      91,    91,    91,    91,    91,    91,    91,    91,    91,    91,
      91,    91,   155,   156,   142,   157,   152,   153,   154,   158,
     288,   155,   156,   143,   157,   272,   220,   144,   158,   161,
     266,   162,   163,   366,   366,   145,   240,   277,   310,   159,
     245,   270,   165,   149,   166,   163,   174,   233,   234,   136,
     235,   236,   237,   190,   190,   190,   190,   190,   190,   190,
     190,   308,   309,   201,   190,   190,   247,  -196,   330,   155,
     156,   268,   157,   170,   171,  -195,   158,   213,   241,   172,
     246,   173,   251,   274,   275,   258,   259,   266,   190,   269,
     255,   261,   262,   260,   198,   127,   280,   283,   290,   293,
     295,   313,   315,   320,   326,   216,   318,   323,    43,   324,
     289,   321,   300,    91,   300,    91,    91,    91,    91,    91,
      91,    91,   177,   300,   300,   342,   332,   334,   340,   341,
     190,   361,    43,    43,   316,   317,   363,   369,   355,   368,
     177,   273,   339,   328,   362,   353,   271,   300,   160,   222,
     224,   303,   227,   225,   232,   226,   301,    97,    98,   322,
     278,   228,   304,   279,   305,   325,   306,   276,   307,     0,
     177,     0,     0,   100,     0,     0,     0,     0,     0,     0,
       0,   354,     0,     0,    43,     0,   343,   333,     0,   300,
       0,    43,   338,   348,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    43,   100,   358,     0,   345,     0,   349,
       0,     0,     0,    43,     0,    45,     0,     0,    43,    43,
       0,     0,     0,     0,    43,     0,   210,     0,     0,    43,
      43,   373,   374,   175,   100,     0,     0,     0,     0,   100,
       6,     0,   371,     0,     0,     0,     0,   372,    10,    11,
       0,     0,    14,     0,    16,    17,     0,    19,     0,    21,
       0,     0,     0,     0,    24,    25,     0,    27,    28,     0,
      87,  -162,     0,     0,     0,    30,    31,    32,     0,     0,
      88,     0,     0,     0,  -162,  -162,     0,     0,     0,    35,
      36,     0,    37,     0,    38,    39,    40,     0,     0,    41,
    -162,  -162,     0,  -190,     0,     0,     0,     0,     0,  -162,
    -162,  -162,  -162,     0,     0,     0,  -190,  -190,     0,  -162,
    -162,     0,     0,  -162,  -162,  -162,  -162,     0,     0,  -162,
       0,  -162,  -190,  -190,     0,     0,  -162,     0,     0,     0,
       0,  -190,  -190,  -190,  -190,     0,     0,     0,     0,     0,
       0,  -190,  -190,     0,     0,  -190,  -190,  -190,  -190,     4,
       0,  -190,     5,  -190,     6,     7,     0,     8,  -190,     9,
       0,     0,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,     0,     0,    24,    25,
      26,    27,    28,     0,    29,     0,     0,     0,     0,    30,
      31,    32,     0,     0,    33,     0,     0,    34,     0,     0,
       0,     0,     0,    35,    36,     0,    37,     0,    38,    39,
      40,     4,     0,    41,     5,     0,     6,     7,     0,     8,
       0,     9,     0,     0,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,     0,     0,
      24,    25,     0,    27,    28,   115,   116,     0,     0,     0,
       0,   117,    31,   118,     0,     0,    33,     0,     0,    34,
       0,     0,     0,     0,     0,    35,    36,     0,    37,     0,
      38,    39,    40,     4,     0,    41,     5,     0,     6,     7,
       0,     8,     0,     9,     0,     0,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
       0,     0,    24,    25,     0,    27,    28,   196,    29,     0,
       0,     0,     0,    30,    31,    32,     0,     0,    33,     0,
       0,    34,     0,     0,     0,     0,     0,    35,    36,     0,
      37,     0,    38,    39,    40,     4,     0,    41,     5,     0,
       6,     7,     0,     8,     0,     9,     0,     0,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,     0,     0,    24,    25,     0,    27,    28,   209,
      29,     0,     0,     0,     0,    30,    31,    32,     0,     0,
      33,     0,     0,    34,     0,     0,     0,     0,     0,    35,
      36,     0,    37,     0,    38,    39,    40,     4,     0,    41,
       5,     0,     6,     7,     0,     8,     0,     9,     0,     0,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,     0,     0,    24,    25,     0,    27,
      28,     0,    29,     0,     0,     0,     0,    30,    31,    32,
       0,     0,    33,     0,     0,    34,     0,     0,     0,     0,
       0,    35,    36,     0,    37,     6,    38,    39,    40,     0,
       0,    41,     0,    10,    11,     0,     0,    14,     0,    16,
      17,     0,    19,     0,    21,     0,     0,     0,     0,    24,
      25,     0,    27,    28,     0,    87,     0,     0,     0,     0,
      30,    31,    32,     0,     0,    88,     0,     0,     0,     0,
       0,     0,     0,     0,    35,    36,     0,    37,     6,    38,
      39,    40,     0,     0,    41,   242,    10,    11,     0,     0,
      14,     0,    16,    17,     0,    19,   178,    21,     0,     0,
       0,     0,    24,    25,     0,    27,    28,     0,    87,     0,
       0,     0,     0,    30,    31,    32,     0,     0,    88,     0,
       0,     0,     0,     0,     0,     0,     0,    35,    36,     0,
      37,     6,    38,    39,    40,     0,     0,    41,     0,    10,
      11,     0,     0,    14,     0,    16,    17,     0,    19,     0,
      21,     0,     0,     0,     0,    24,    25,     0,    27,    28,
       0,    87,     0,     0,     0,     0,    30,    31,    32,     0,
       0,    88,     0,     0,     0,     0,     0,     0,     0,     0,
      35,    36,     0,    37,     6,    38,    39,    40,   319,     0,
      41,     0,    10,    11,     0,     0,    14,     0,    16,    17,
       0,    19,     0,    21,     0,     0,     0,     0,    24,    25,
       0,    27,    28,     0,    87,     0,     0,     0,     0,    30,
      31,    32,    10,    11,    88,     0,    14,     0,    16,    17,
       0,     0,     0,    35,    36,     0,    37,     0,    38,    39,
      40,    27,    28,    41,    87,     0,     0,     0,     0,    30,
      31,    32,     0,     0,    88,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    37,     0,     0,     0,
      40,     0,     0,    41
};

#define yypact_value_is_default(yystate) \
  ((yystate) == (-293))

#define yytable_value_is_error(yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
       2,    18,     2,     6,     1,     7,   298,     7,     1,     1,
       5,     1,     1,     7,    57,    58,    19,   255,    21,   257,
       1,    24,    25,   110,    31,    85,    86,    12,   266,   267,
      15,    33,    35,    36,    96,    38,    39,   329,    15,    99,
     294,     9,   102,    37,     6,    13,    53,   107,     1,    44,
      14,    15,   290,     0,    22,    23,    41,    19,    51,    21,
      49,    51,    24,    25,    38,    42,    31,   359,    49,    78,
      51,    68,   364,    35,    36,    67,    38,    39,    42,    68,
      49,    32,    33,    49,   146,   147,   148,    48,    53,    98,
      49,    49,   346,    96,   332,   131,    49,    49,    51,    68,
     136,   103,    68,   357,   140,    49,    65,    65,    49,    38,
      95,    52,   114,    65,   114,    49,   101,    37,   120,    52,
     120,    65,    46,   159,   160,   110,   111,   163,    38,    49,
       5,    65,    30,    43,    14,    45,   139,    12,   141,   142,
     143,   144,   145,   146,   147,   148,   149,   150,   151,   152,
     153,   154,    32,    33,    54,    35,    59,    60,    61,    39,
     247,    32,    33,    55,    35,   201,   134,    56,    39,    64,
      50,    66,    67,   360,   361,    40,   161,   213,   265,    50,
     165,   198,    64,    41,    66,    67,    28,   150,   151,    49,
     152,   153,   154,   255,   256,   257,   258,   259,   260,   261,
     262,   263,   264,    50,   266,   267,   174,    52,   295,    32,
      33,   194,    35,    37,    38,    52,    39,    52,    38,    43,
      38,    45,    38,   206,   207,    30,    54,    50,   290,   197,
      49,    56,    40,    55,    12,    49,    38,    52,    50,    49,
      14,    48,    38,   279,    10,    49,    52,   283,   250,   285,
     250,    49,   255,   256,   257,   258,   259,   260,   261,   262,
     263,   264,   247,   266,   267,   325,    52,     4,    38,    48,
     332,    52,   274,   275,   274,   275,    37,    37,   341,   361,
     265,   204,   318,   293,   352,   336,   199,   290,    75,   139,
     141,   258,   144,   142,   149,   143,   256,    10,    10,   282,
     214,   145,   259,   214,   260,   288,   261,   212,   262,    -1,
     295,    -1,    -1,   298,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   338,    -1,    -1,   326,    -1,   326,   310,    -1,   332,
      -1,   333,   315,   333,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   345,   329,   345,    -1,   330,    -1,   334,
      -1,    -1,    -1,   355,    -1,   355,    -1,    -1,   360,   361,
      -1,    -1,    -1,    -1,   366,    -1,   366,    -1,    -1,   371,
     372,   371,   372,     1,   359,    -1,    -1,    -1,    -1,   364,
       8,    -1,   365,    -1,    -1,    -1,    -1,   370,    16,    17,
      -1,    -1,    20,    -1,    22,    23,    -1,    25,    -1,    27,
      -1,    -1,    -1,    -1,    32,    33,    -1,    35,    36,    -1,
      38,     1,    -1,    -1,    -1,    43,    44,    45,    -1,    -1,
      48,    -1,    -1,    -1,    14,    15,    -1,    -1,    -1,    57,
      58,    -1,    60,    -1,    62,    63,    64,    -1,    -1,    67,
      30,    31,    -1,     1,    -1,    -1,    -1,    -1,    -1,    39,
      40,    41,    42,    -1,    -1,    -1,    14,    15,    -1,    49,
      50,    -1,    -1,    53,    54,    55,    56,    -1,    -1,    59,
      -1,    61,    30,    31,    -1,    -1,    66,    -1,    -1,    -1,
      -1,    39,    40,    41,    42,    -1,    -1,    -1,    -1,    -1,
      -1,    49,    50,    -1,    -1,    53,    54,    55,    56,     3,
      -1,    59,     6,    61,     8,     9,    -1,    11,    66,    13,
      -1,    -1,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    -1,    -1,    32,    33,
      34,    35,    36,    -1,    38,    -1,    -1,    -1,    -1,    43,
      44,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    -1,    -1,    57,    58,    -1,    60,    -1,    62,    63,
      64,     3,    -1,    67,     6,    -1,     8,     9,    -1,    11,
      -1,    13,    -1,    -1,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    -1,    -1,
      32,    33,    -1,    35,    36,    37,    38,    -1,    -1,    -1,
      -1,    43,    44,    45,    -1,    -1,    48,    -1,    -1,    51,
      -1,    -1,    -1,    -1,    -1,    57,    58,    -1,    60,    -1,
      62,    63,    64,     3,    -1,    67,     6,    -1,     8,     9,
      -1,    11,    -1,    13,    -1,    -1,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      -1,    -1,    32,    33,    -1,    35,    36,    37,    38,    -1,
      -1,    -1,    -1,    43,    44,    45,    -1,    -1,    48,    -1,
      -1,    51,    -1,    -1,    -1,    -1,    -1,    57,    58,    -1,
      60,    -1,    62,    63,    64,     3,    -1,    67,     6,    -1,
       8,     9,    -1,    11,    -1,    13,    -1,    -1,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    -1,    -1,    32,    33,    -1,    35,    36,    37,
      38,    -1,    -1,    -1,    -1,    43,    44,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    -1,    -1,    57,
      58,    -1,    60,    -1,    62,    63,    64,     3,    -1,    67,
       6,    -1,     8,     9,    -1,    11,    -1,    13,    -1,    -1,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    -1,    -1,    32,    33,    -1,    35,
      36,    -1,    38,    -1,    -1,    -1,    -1,    43,    44,    45,
      -1,    -1,    48,    -1,    -1,    51,    -1,    -1,    -1,    -1,
      -1,    57,    58,    -1,    60,     8,    62,    63,    64,    -1,
      -1,    67,    -1,    16,    17,    -1,    -1,    20,    -1,    22,
      23,    -1,    25,    -1,    27,    -1,    -1,    -1,    -1,    32,
      33,    -1,    35,    36,    -1,    38,    -1,    -1,    -1,    -1,
      43,    44,    45,    -1,    -1,    48,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    57,    58,    -1,    60,     8,    62,
      63,    64,    -1,    -1,    67,    68,    16,    17,    -1,    -1,
      20,    -1,    22,    23,    -1,    25,    26,    27,    -1,    -1,
      -1,    -1,    32,    33,    -1,    35,    36,    -1,    38,    -1,
      -1,    -1,    -1,    43,    44,    45,    -1,    -1,    48,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    57,    58,    -1,
      60,     8,    62,    63,    64,    -1,    -1,    67,    -1,    16,
      17,    -1,    -1,    20,    -1,    22,    23,    -1,    25,    -1,
      27,    -1,    -1,    -1,    -1,    32,    33,    -1,    35,    36,
      -1,    38,    -1,    -1,    -1,    -1,    43,    44,    45,    -1,
      -1,    48,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      57,    58,    -1,    60,     8,    62,    63,    64,    65,    -1,
      67,    -1,    16,    17,    -1,    -1,    20,    -1,    22,    23,
      -1,    25,    -1,    27,    -1,    -1,    -1,    -1,    32,    33,
      -1,    35,    36,    -1,    38,    -1,    -1,    -1,    -1,    43,
      44,    45,    16,    17,    48,    -1,    20,    -1,    22,    23,
      -1,    -1,    -1,    57,    58,    -1,    60,    -1,    62,    63,
      64,    35,    36,    67,    38,    -1,    -1,    -1,    -1,    43,
      44,    45,    -1,    -1,    48,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    60,    -1,    -1,    -1,
      64,    -1,    -1,    67
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    70,    72,     0,     3,     6,     8,     9,    11,    13,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    32,    33,    34,    35,    36,    38,
      43,    44,    45,    48,    51,    57,    58,    60,    62,    63,
      64,    67,    71,    73,    74,    78,    81,    82,    91,    92,
      93,    94,    99,   100,   101,   102,   103,   104,   110,   111,
     116,   120,   122,   124,   126,   128,   130,   132,   134,   136,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   149,
     150,   154,   158,   159,    38,   157,   157,    38,    48,    73,
     141,   143,    78,     1,    67,   161,   161,   144,   145,   114,
     116,   161,   116,    48,    81,   141,    38,    83,    85,   141,
     161,   161,   141,   141,    52,    37,    38,    43,    45,    78,
      79,   155,   156,   141,   141,   141,   141,    49,    65,   151,
     152,   153,   116,    46,   157,     1,    49,    51,   160,    31,
      53,    30,    54,    55,    56,    40,    14,    15,    42,    41,
      57,    58,    59,    60,    61,    32,    33,    35,    39,    50,
     119,    64,    66,    67,   147,    64,    66,   147,   160,   160,
      37,    38,    43,    45,    28,     1,   115,   116,    26,   117,
     118,   121,   123,   125,   127,   129,   131,   133,   135,   137,
     138,   143,   147,   160,   116,   160,    37,     5,    12,   112,
     113,    50,    87,    88,    49,   160,   115,   116,    78,    37,
      78,    37,    49,    52,    49,    65,    49,    65,   120,    68,
     161,   120,   126,   120,   128,   130,   132,   134,   136,   138,
     138,   138,   139,   140,   140,   141,   141,   141,   120,   120,
     116,    38,    68,   120,   148,   116,    38,   161,     1,    68,
     162,    38,    84,    86,    95,    49,    31,    53,    30,    54,
      55,    56,    40,    15,    42,    14,    50,   119,   162,   161,
      81,   113,   120,    85,   162,   162,   156,   120,   152,   153,
      38,    76,    77,    52,    65,    49,    68,    65,   115,    78,
      50,    89,    90,    49,    97,    14,     1,    51,   163,   121,
     143,   127,   121,   129,   131,   133,   135,   137,   138,   138,
     115,   121,   121,    48,   105,    38,    78,    78,    52,    65,
     120,    49,   162,   120,   120,   162,    10,   121,    86,   163,
     115,   114,    52,   162,     4,   106,   107,   108,   162,   120,
      38,    48,   160,    78,   114,   162,    96,   121,    78,   116,
       7,    37,   109,   108,    81,    72,    75,    98,    78,   163,
      52,    52,   106,    37,   163,   114,    79,    80,    80,    37,
     114,   162,   162,    78,    78
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
      yyerror (ctx, YY_("syntax error: cannot back up")); \
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
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, ctx)
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
		  Type, Value, ctx); \
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
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, parser_ctx_t *ctx)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, ctx)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    parser_ctx_t *ctx;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (ctx);
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
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, parser_ctx_t *ctx)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, ctx)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    parser_ctx_t *ctx;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, ctx);
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
yy_reduce_print (YYSTYPE *yyvsp, int yyrule, parser_ctx_t *ctx)
#else
static void
yy_reduce_print (yyvsp, yyrule, ctx)
    YYSTYPE *yyvsp;
    int yyrule;
    parser_ctx_t *ctx;
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
		       		       , ctx);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule, ctx); \
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
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, parser_ctx_t *ctx)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, ctx)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    parser_ctx_t *ctx;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (ctx);

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
int yyparse (parser_ctx_t *ctx);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


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
yyparse (parser_ctx_t *ctx)
#else
int
yyparse (ctx)
    parser_ctx_t *ctx;
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
#line 252 "parser.y"
    { program_parsed(ctx, (yyvsp[(1) - (3)].source_elements)); }
    break;

  case 3:

/* Line 1806 of yacc.c  */
#line 255 "parser.y"
    {}
    break;

  case 4:

/* Line 1806 of yacc.c  */
#line 256 "parser.y"
    {}
    break;

  case 5:

/* Line 1806 of yacc.c  */
#line 260 "parser.y"
    { (yyval.source_elements) = new_source_elements(ctx); }
    break;

  case 6:

/* Line 1806 of yacc.c  */
#line 262 "parser.y"
    { (yyval.source_elements) = source_elements_add_statement((yyvsp[(1) - (2)].source_elements), (yyvsp[(2) - (2)].statement)); }
    break;

  case 7:

/* Line 1806 of yacc.c  */
#line 267 "parser.y"
    { (yyval.expr) = new_function_expression(ctx, (yyvsp[(2) - (8)].identifier), (yyvsp[(4) - (8)].parameter_list), (yyvsp[(7) - (8)].source_elements), (yyvsp[(1) - (8)].srcptr), (yyvsp[(8) - (8)].srcptr)-(yyvsp[(1) - (8)].srcptr)+1); }
    break;

  case 8:

/* Line 1806 of yacc.c  */
#line 270 "parser.y"
    { (yyval.srcptr) = (yyvsp[(1) - (1)].srcptr); }
    break;

  case 9:

/* Line 1806 of yacc.c  */
#line 274 "parser.y"
    { (yyval.source_elements) = (yyvsp[(1) - (1)].source_elements); }
    break;

  case 10:

/* Line 1806 of yacc.c  */
#line 278 "parser.y"
    { (yyval.parameter_list) = new_parameter_list(ctx, (yyvsp[(1) - (1)].identifier)); }
    break;

  case 11:

/* Line 1806 of yacc.c  */
#line 280 "parser.y"
    { (yyval.parameter_list) = parameter_list_add(ctx, (yyvsp[(1) - (3)].parameter_list), (yyvsp[(3) - (3)].identifier)); }
    break;

  case 12:

/* Line 1806 of yacc.c  */
#line 284 "parser.y"
    { (yyval.parameter_list) = NULL; }
    break;

  case 13:

/* Line 1806 of yacc.c  */
#line 285 "parser.y"
    { (yyval.parameter_list) = (yyvsp[(1) - (1)].parameter_list); }
    break;

  case 14:

/* Line 1806 of yacc.c  */
#line 289 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); }
    break;

  case 15:

/* Line 1806 of yacc.c  */
#line 290 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); }
    break;

  case 16:

/* Line 1806 of yacc.c  */
#line 291 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); }
    break;

  case 17:

/* Line 1806 of yacc.c  */
#line 292 "parser.y"
    { (yyval.statement) = new_expression_statement(ctx, (yyvsp[(1) - (1)].expr)); }
    break;

  case 18:

/* Line 1806 of yacc.c  */
#line 293 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); }
    break;

  case 19:

/* Line 1806 of yacc.c  */
#line 294 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); }
    break;

  case 20:

/* Line 1806 of yacc.c  */
#line 295 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); }
    break;

  case 21:

/* Line 1806 of yacc.c  */
#line 296 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); }
    break;

  case 22:

/* Line 1806 of yacc.c  */
#line 297 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); }
    break;

  case 23:

/* Line 1806 of yacc.c  */
#line 298 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); }
    break;

  case 24:

/* Line 1806 of yacc.c  */
#line 299 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); }
    break;

  case 25:

/* Line 1806 of yacc.c  */
#line 300 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); }
    break;

  case 26:

/* Line 1806 of yacc.c  */
#line 301 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); }
    break;

  case 27:

/* Line 1806 of yacc.c  */
#line 302 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); }
    break;

  case 28:

/* Line 1806 of yacc.c  */
#line 303 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); }
    break;

  case 29:

/* Line 1806 of yacc.c  */
#line 307 "parser.y"
    { (yyval.statement_list) = new_statement_list(ctx, (yyvsp[(1) - (1)].statement)); }
    break;

  case 30:

/* Line 1806 of yacc.c  */
#line 309 "parser.y"
    { (yyval.statement_list) = statement_list_add((yyvsp[(1) - (2)].statement_list), (yyvsp[(2) - (2)].statement)); }
    break;

  case 31:

/* Line 1806 of yacc.c  */
#line 313 "parser.y"
    { (yyval.statement_list) = NULL; }
    break;

  case 32:

/* Line 1806 of yacc.c  */
#line 314 "parser.y"
    { (yyval.statement_list) = (yyvsp[(1) - (1)].statement_list); }
    break;

  case 33:

/* Line 1806 of yacc.c  */
#line 318 "parser.y"
    { (yyval.statement) = new_block_statement(ctx, (yyvsp[(2) - (3)].statement_list)); }
    break;

  case 34:

/* Line 1806 of yacc.c  */
#line 319 "parser.y"
    { (yyval.statement) = new_block_statement(ctx, NULL); }
    break;

  case 35:

/* Line 1806 of yacc.c  */
#line 324 "parser.y"
    { (yyval.statement) = new_var_statement(ctx, (yyvsp[(2) - (3)].variable_list)); }
    break;

  case 36:

/* Line 1806 of yacc.c  */
#line 328 "parser.y"
    { (yyval.variable_list) = new_variable_list(ctx, (yyvsp[(1) - (1)].variable_declaration)); }
    break;

  case 37:

/* Line 1806 of yacc.c  */
#line 330 "parser.y"
    { (yyval.variable_list) = variable_list_add(ctx, (yyvsp[(1) - (3)].variable_list), (yyvsp[(3) - (3)].variable_declaration)); }
    break;

  case 38:

/* Line 1806 of yacc.c  */
#line 335 "parser.y"
    { (yyval.variable_list) = new_variable_list(ctx, (yyvsp[(1) - (1)].variable_declaration)); }
    break;

  case 39:

/* Line 1806 of yacc.c  */
#line 337 "parser.y"
    { (yyval.variable_list) = variable_list_add(ctx, (yyvsp[(1) - (3)].variable_list), (yyvsp[(3) - (3)].variable_declaration)); }
    break;

  case 40:

/* Line 1806 of yacc.c  */
#line 342 "parser.y"
    { (yyval.variable_declaration) = new_variable_declaration(ctx, (yyvsp[(1) - (2)].identifier), (yyvsp[(2) - (2)].expr)); }
    break;

  case 41:

/* Line 1806 of yacc.c  */
#line 347 "parser.y"
    { (yyval.variable_declaration) = new_variable_declaration(ctx, (yyvsp[(1) - (2)].identifier), (yyvsp[(2) - (2)].expr)); }
    break;

  case 42:

/* Line 1806 of yacc.c  */
#line 351 "parser.y"
    { (yyval.expr) = NULL; }
    break;

  case 43:

/* Line 1806 of yacc.c  */
#line 352 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 44:

/* Line 1806 of yacc.c  */
#line 357 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (2)].expr); }
    break;

  case 45:

/* Line 1806 of yacc.c  */
#line 361 "parser.y"
    { (yyval.expr) = NULL; }
    break;

  case 46:

/* Line 1806 of yacc.c  */
#line 362 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 47:

/* Line 1806 of yacc.c  */
#line 367 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (2)].expr); }
    break;

  case 48:

/* Line 1806 of yacc.c  */
#line 371 "parser.y"
    { (yyval.statement) = new_statement(ctx, STAT_EMPTY, 0); }
    break;

  case 49:

/* Line 1806 of yacc.c  */
#line 376 "parser.y"
    { (yyval.statement) = new_expression_statement(ctx, (yyvsp[(1) - (2)].expr)); }
    break;

  case 50:

/* Line 1806 of yacc.c  */
#line 381 "parser.y"
    { (yyval.statement) = new_if_statement(ctx, (yyvsp[(3) - (7)].expr), (yyvsp[(5) - (7)].statement), (yyvsp[(7) - (7)].statement)); }
    break;

  case 51:

/* Line 1806 of yacc.c  */
#line 383 "parser.y"
    { (yyval.statement) = new_if_statement(ctx, (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].statement), NULL); }
    break;

  case 52:

/* Line 1806 of yacc.c  */
#line 388 "parser.y"
    { (yyval.statement) = new_while_statement(ctx, TRUE, (yyvsp[(5) - (7)].expr), (yyvsp[(2) - (7)].statement)); }
    break;

  case 53:

/* Line 1806 of yacc.c  */
#line 390 "parser.y"
    { (yyval.statement) = new_while_statement(ctx, FALSE, (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].statement)); }
    break;

  case 54:

/* Line 1806 of yacc.c  */
#line 392 "parser.y"
    { if(!explicit_error(ctx, (yyvsp[(3) - (3)].expr), ';')) YYABORT; }
    break;

  case 55:

/* Line 1806 of yacc.c  */
#line 394 "parser.y"
    { if(!explicit_error(ctx, (yyvsp[(6) - (6)].expr), ';')) YYABORT; }
    break;

  case 56:

/* Line 1806 of yacc.c  */
#line 396 "parser.y"
    { (yyval.statement) = new_for_statement(ctx, NULL, (yyvsp[(3) - (11)].expr), (yyvsp[(6) - (11)].expr), (yyvsp[(9) - (11)].expr), (yyvsp[(11) - (11)].statement)); }
    break;

  case 57:

/* Line 1806 of yacc.c  */
#line 398 "parser.y"
    { if(!explicit_error(ctx, (yyvsp[(4) - (4)].variable_list), ';')) YYABORT; }
    break;

  case 58:

/* Line 1806 of yacc.c  */
#line 400 "parser.y"
    { if(!explicit_error(ctx, (yyvsp[(7) - (7)].expr), ';')) YYABORT; }
    break;

  case 59:

/* Line 1806 of yacc.c  */
#line 402 "parser.y"
    { (yyval.statement) = new_for_statement(ctx, (yyvsp[(4) - (12)].variable_list), NULL, (yyvsp[(7) - (12)].expr), (yyvsp[(10) - (12)].expr), (yyvsp[(12) - (12)].statement)); }
    break;

  case 60:

/* Line 1806 of yacc.c  */
#line 404 "parser.y"
    { (yyval.statement) = new_forin_statement(ctx, NULL, (yyvsp[(3) - (7)].expr), (yyvsp[(5) - (7)].expr), (yyvsp[(7) - (7)].statement)); }
    break;

  case 61:

/* Line 1806 of yacc.c  */
#line 406 "parser.y"
    { (yyval.statement) = new_forin_statement(ctx, (yyvsp[(4) - (8)].variable_declaration), NULL, (yyvsp[(6) - (8)].expr), (yyvsp[(8) - (8)].statement)); }
    break;

  case 62:

/* Line 1806 of yacc.c  */
#line 411 "parser.y"
    { (yyval.statement) = new_continue_statement(ctx, (yyvsp[(2) - (3)].identifier)); }
    break;

  case 63:

/* Line 1806 of yacc.c  */
#line 416 "parser.y"
    { (yyval.statement) = new_break_statement(ctx, (yyvsp[(2) - (3)].identifier)); }
    break;

  case 64:

/* Line 1806 of yacc.c  */
#line 421 "parser.y"
    { (yyval.statement) = new_return_statement(ctx, (yyvsp[(2) - (3)].expr)); }
    break;

  case 65:

/* Line 1806 of yacc.c  */
#line 426 "parser.y"
    { (yyval.statement) = new_with_statement(ctx, (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].statement)); }
    break;

  case 66:

/* Line 1806 of yacc.c  */
#line 431 "parser.y"
    { (yyval.statement) = new_labelled_statement(ctx, (yyvsp[(1) - (3)].identifier), (yyvsp[(3) - (3)].statement)); }
    break;

  case 67:

/* Line 1806 of yacc.c  */
#line 436 "parser.y"
    { (yyval.statement) = new_switch_statement(ctx, (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].case_clausule)); }
    break;

  case 68:

/* Line 1806 of yacc.c  */
#line 441 "parser.y"
    { (yyval.case_clausule) = new_case_block(ctx, (yyvsp[(2) - (3)].case_list), NULL, NULL); }
    break;

  case 69:

/* Line 1806 of yacc.c  */
#line 443 "parser.y"
    { (yyval.case_clausule) = new_case_block(ctx, (yyvsp[(2) - (5)].case_list), (yyvsp[(3) - (5)].case_clausule), (yyvsp[(4) - (5)].case_list)); }
    break;

  case 70:

/* Line 1806 of yacc.c  */
#line 447 "parser.y"
    { (yyval.case_list) = NULL; }
    break;

  case 71:

/* Line 1806 of yacc.c  */
#line 448 "parser.y"
    { (yyval.case_list) = (yyvsp[(1) - (1)].case_list); }
    break;

  case 72:

/* Line 1806 of yacc.c  */
#line 452 "parser.y"
    { (yyval.case_list) = new_case_list(ctx, (yyvsp[(1) - (1)].case_clausule)); }
    break;

  case 73:

/* Line 1806 of yacc.c  */
#line 454 "parser.y"
    { (yyval.case_list) = case_list_add(ctx, (yyvsp[(1) - (2)].case_list), (yyvsp[(2) - (2)].case_clausule)); }
    break;

  case 74:

/* Line 1806 of yacc.c  */
#line 459 "parser.y"
    { (yyval.case_clausule) = new_case_clausule(ctx, (yyvsp[(2) - (4)].expr), (yyvsp[(4) - (4)].statement_list)); }
    break;

  case 75:

/* Line 1806 of yacc.c  */
#line 464 "parser.y"
    { (yyval.case_clausule) = new_case_clausule(ctx, NULL, (yyvsp[(3) - (3)].statement_list)); }
    break;

  case 76:

/* Line 1806 of yacc.c  */
#line 469 "parser.y"
    { (yyval.statement) = new_throw_statement(ctx, (yyvsp[(2) - (3)].expr)); }
    break;

  case 77:

/* Line 1806 of yacc.c  */
#line 473 "parser.y"
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[(2) - (3)].statement), (yyvsp[(3) - (3)].catch_block), NULL); }
    break;

  case 78:

/* Line 1806 of yacc.c  */
#line 474 "parser.y"
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[(2) - (3)].statement), NULL, (yyvsp[(3) - (3)].statement)); }
    break;

  case 79:

/* Line 1806 of yacc.c  */
#line 476 "parser.y"
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[(2) - (4)].statement), (yyvsp[(3) - (4)].catch_block), (yyvsp[(4) - (4)].statement)); }
    break;

  case 80:

/* Line 1806 of yacc.c  */
#line 481 "parser.y"
    { (yyval.catch_block) = new_catch_block(ctx, (yyvsp[(3) - (5)].identifier), (yyvsp[(5) - (5)].statement)); }
    break;

  case 81:

/* Line 1806 of yacc.c  */
#line 485 "parser.y"
    { (yyval.statement) = (yyvsp[(2) - (2)].statement); }
    break;

  case 82:

/* Line 1806 of yacc.c  */
#line 489 "parser.y"
    { (yyval.expr) = NULL; }
    break;

  case 83:

/* Line 1806 of yacc.c  */
#line 490 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 84:

/* Line 1806 of yacc.c  */
#line 493 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 85:

/* Line 1806 of yacc.c  */
#line 494 "parser.y"
    { set_error(ctx, JS_E_SYNTAX); YYABORT; }
    break;

  case 86:

/* Line 1806 of yacc.c  */
#line 498 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 87:

/* Line 1806 of yacc.c  */
#line 500 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_COMMA, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 88:

/* Line 1806 of yacc.c  */
#line 504 "parser.y"
    { (yyval.expr) = NULL; }
    break;

  case 89:

/* Line 1806 of yacc.c  */
#line 505 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 90:

/* Line 1806 of yacc.c  */
#line 510 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 91:

/* Line 1806 of yacc.c  */
#line 512 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_COMMA, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 92:

/* Line 1806 of yacc.c  */
#line 515 "parser.y"
    { (yyval.ival) = (yyvsp[(1) - (1)].ival); }
    break;

  case 93:

/* Line 1806 of yacc.c  */
#line 516 "parser.y"
    { (yyval.ival) = EXPR_ASSIGNDIV; }
    break;

  case 94:

/* Line 1806 of yacc.c  */
#line 520 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 95:

/* Line 1806 of yacc.c  */
#line 522 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ASSIGN, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 96:

/* Line 1806 of yacc.c  */
#line 524 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 97:

/* Line 1806 of yacc.c  */
#line 529 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 98:

/* Line 1806 of yacc.c  */
#line 531 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ASSIGN, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 99:

/* Line 1806 of yacc.c  */
#line 533 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 100:

/* Line 1806 of yacc.c  */
#line 537 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 101:

/* Line 1806 of yacc.c  */
#line 539 "parser.y"
    { (yyval.expr) = new_conditional_expression(ctx, (yyvsp[(1) - (5)].expr), (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].expr)); }
    break;

  case 102:

/* Line 1806 of yacc.c  */
#line 544 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 103:

/* Line 1806 of yacc.c  */
#line 546 "parser.y"
    { (yyval.expr) = new_conditional_expression(ctx, (yyvsp[(1) - (5)].expr), (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].expr)); }
    break;

  case 104:

/* Line 1806 of yacc.c  */
#line 550 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 105:

/* Line 1806 of yacc.c  */
#line 552 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_OR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 106:

/* Line 1806 of yacc.c  */
#line 557 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 107:

/* Line 1806 of yacc.c  */
#line 559 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_OR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 108:

/* Line 1806 of yacc.c  */
#line 563 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 109:

/* Line 1806 of yacc.c  */
#line 565 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_AND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 110:

/* Line 1806 of yacc.c  */
#line 570 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 111:

/* Line 1806 of yacc.c  */
#line 572 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_AND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 112:

/* Line 1806 of yacc.c  */
#line 576 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 113:

/* Line 1806 of yacc.c  */
#line 578 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BOR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 114:

/* Line 1806 of yacc.c  */
#line 583 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 115:

/* Line 1806 of yacc.c  */
#line 585 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BOR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 116:

/* Line 1806 of yacc.c  */
#line 589 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 117:

/* Line 1806 of yacc.c  */
#line 591 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BXOR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 118:

/* Line 1806 of yacc.c  */
#line 596 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 119:

/* Line 1806 of yacc.c  */
#line 598 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BXOR, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 120:

/* Line 1806 of yacc.c  */
#line 602 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 121:

/* Line 1806 of yacc.c  */
#line 604 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BAND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 122:

/* Line 1806 of yacc.c  */
#line 609 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 123:

/* Line 1806 of yacc.c  */
#line 611 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BAND, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 124:

/* Line 1806 of yacc.c  */
#line 615 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 125:

/* Line 1806 of yacc.c  */
#line 617 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 126:

/* Line 1806 of yacc.c  */
#line 621 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 127:

/* Line 1806 of yacc.c  */
#line 623 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 128:

/* Line 1806 of yacc.c  */
#line 627 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 129:

/* Line 1806 of yacc.c  */
#line 629 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 130:

/* Line 1806 of yacc.c  */
#line 631 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_INSTANCEOF, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 131:

/* Line 1806 of yacc.c  */
#line 633 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_IN, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 132:

/* Line 1806 of yacc.c  */
#line 637 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 133:

/* Line 1806 of yacc.c  */
#line 639 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 134:

/* Line 1806 of yacc.c  */
#line 641 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_INSTANCEOF, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 135:

/* Line 1806 of yacc.c  */
#line 645 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 136:

/* Line 1806 of yacc.c  */
#line 647 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[(2) - (3)].ival), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 137:

/* Line 1806 of yacc.c  */
#line 652 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 138:

/* Line 1806 of yacc.c  */
#line 654 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ADD, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 139:

/* Line 1806 of yacc.c  */
#line 656 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_SUB, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 140:

/* Line 1806 of yacc.c  */
#line 660 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 141:

/* Line 1806 of yacc.c  */
#line 662 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_MUL, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 142:

/* Line 1806 of yacc.c  */
#line 664 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_DIV, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 143:

/* Line 1806 of yacc.c  */
#line 666 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_MOD, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)); }
    break;

  case 144:

/* Line 1806 of yacc.c  */
#line 670 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 145:

/* Line 1806 of yacc.c  */
#line 672 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_DELETE, (yyvsp[(2) - (2)].expr)); }
    break;

  case 146:

/* Line 1806 of yacc.c  */
#line 673 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_VOID, (yyvsp[(2) - (2)].expr)); }
    break;

  case 147:

/* Line 1806 of yacc.c  */
#line 675 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_TYPEOF, (yyvsp[(2) - (2)].expr)); }
    break;

  case 148:

/* Line 1806 of yacc.c  */
#line 676 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PREINC, (yyvsp[(2) - (2)].expr)); }
    break;

  case 149:

/* Line 1806 of yacc.c  */
#line 677 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PREDEC, (yyvsp[(2) - (2)].expr)); }
    break;

  case 150:

/* Line 1806 of yacc.c  */
#line 678 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PLUS, (yyvsp[(2) - (2)].expr)); }
    break;

  case 151:

/* Line 1806 of yacc.c  */
#line 679 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_MINUS, (yyvsp[(2) - (2)].expr)); }
    break;

  case 152:

/* Line 1806 of yacc.c  */
#line 680 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_BITNEG, (yyvsp[(2) - (2)].expr)); }
    break;

  case 153:

/* Line 1806 of yacc.c  */
#line 681 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_LOGNEG, (yyvsp[(2) - (2)].expr)); }
    break;

  case 154:

/* Line 1806 of yacc.c  */
#line 686 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 155:

/* Line 1806 of yacc.c  */
#line 688 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_POSTINC, (yyvsp[(1) - (2)].expr)); }
    break;

  case 156:

/* Line 1806 of yacc.c  */
#line 690 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_POSTDEC, (yyvsp[(1) - (2)].expr)); }
    break;

  case 157:

/* Line 1806 of yacc.c  */
#line 695 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 158:

/* Line 1806 of yacc.c  */
#line 696 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 159:

/* Line 1806 of yacc.c  */
#line 700 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 160:

/* Line 1806 of yacc.c  */
#line 701 "parser.y"
    { (yyval.expr) = new_new_expression(ctx, (yyvsp[(2) - (2)].expr), NULL); }
    break;

  case 161:

/* Line 1806 of yacc.c  */
#line 705 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 162:

/* Line 1806 of yacc.c  */
#line 706 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 163:

/* Line 1806 of yacc.c  */
#line 708 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ARRAY, (yyvsp[(1) - (4)].expr), (yyvsp[(3) - (4)].expr)); }
    break;

  case 164:

/* Line 1806 of yacc.c  */
#line 710 "parser.y"
    { (yyval.expr) = new_member_expression(ctx, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].identifier)); }
    break;

  case 165:

/* Line 1806 of yacc.c  */
#line 712 "parser.y"
    { (yyval.expr) = new_new_expression(ctx, (yyvsp[(2) - (3)].expr), (yyvsp[(3) - (3)].argument_list)); }
    break;

  case 166:

/* Line 1806 of yacc.c  */
#line 717 "parser.y"
    { (yyval.expr) = new_call_expression(ctx, (yyvsp[(1) - (2)].expr), (yyvsp[(2) - (2)].argument_list)); }
    break;

  case 167:

/* Line 1806 of yacc.c  */
#line 719 "parser.y"
    { (yyval.expr) = new_call_expression(ctx, (yyvsp[(1) - (2)].expr), (yyvsp[(2) - (2)].argument_list)); }
    break;

  case 168:

/* Line 1806 of yacc.c  */
#line 721 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ARRAY, (yyvsp[(1) - (4)].expr), (yyvsp[(3) - (4)].expr)); }
    break;

  case 169:

/* Line 1806 of yacc.c  */
#line 723 "parser.y"
    { (yyval.expr) = new_member_expression(ctx, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].identifier)); }
    break;

  case 170:

/* Line 1806 of yacc.c  */
#line 727 "parser.y"
    { (yyval.argument_list) = NULL; }
    break;

  case 171:

/* Line 1806 of yacc.c  */
#line 728 "parser.y"
    { (yyval.argument_list) = (yyvsp[(2) - (3)].argument_list); }
    break;

  case 172:

/* Line 1806 of yacc.c  */
#line 732 "parser.y"
    { (yyval.argument_list) = new_argument_list(ctx, (yyvsp[(1) - (1)].expr)); }
    break;

  case 173:

/* Line 1806 of yacc.c  */
#line 734 "parser.y"
    { (yyval.argument_list) = argument_list_add(ctx, (yyvsp[(1) - (3)].argument_list), (yyvsp[(3) - (3)].expr)); }
    break;

  case 174:

/* Line 1806 of yacc.c  */
#line 738 "parser.y"
    { (yyval.expr) = new_expression(ctx, EXPR_THIS, 0); }
    break;

  case 175:

/* Line 1806 of yacc.c  */
#line 739 "parser.y"
    { (yyval.expr) = new_identifier_expression(ctx, (yyvsp[(1) - (1)].identifier)); }
    break;

  case 176:

/* Line 1806 of yacc.c  */
#line 740 "parser.y"
    { (yyval.expr) = new_literal_expression(ctx, (yyvsp[(1) - (1)].literal)); }
    break;

  case 177:

/* Line 1806 of yacc.c  */
#line 741 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 178:

/* Line 1806 of yacc.c  */
#line 742 "parser.y"
    { (yyval.expr) = (yyvsp[(1) - (1)].expr); }
    break;

  case 179:

/* Line 1806 of yacc.c  */
#line 743 "parser.y"
    { (yyval.expr) = (yyvsp[(2) - (3)].expr); }
    break;

  case 180:

/* Line 1806 of yacc.c  */
#line 747 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, NULL, 0); }
    break;

  case 181:

/* Line 1806 of yacc.c  */
#line 748 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, NULL, (yyvsp[(2) - (3)].ival)+1); }
    break;

  case 182:

/* Line 1806 of yacc.c  */
#line 749 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, (yyvsp[(2) - (3)].element_list), 0); }
    break;

  case 183:

/* Line 1806 of yacc.c  */
#line 751 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, (yyvsp[(2) - (5)].element_list), (yyvsp[(4) - (5)].ival)+1); }
    break;

  case 184:

/* Line 1806 of yacc.c  */
#line 756 "parser.y"
    { (yyval.element_list) = new_element_list(ctx, (yyvsp[(1) - (2)].ival), (yyvsp[(2) - (2)].expr)); }
    break;

  case 185:

/* Line 1806 of yacc.c  */
#line 758 "parser.y"
    { (yyval.element_list) = element_list_add(ctx, (yyvsp[(1) - (4)].element_list), (yyvsp[(3) - (4)].ival), (yyvsp[(4) - (4)].expr)); }
    break;

  case 186:

/* Line 1806 of yacc.c  */
#line 762 "parser.y"
    { (yyval.ival) = 1; }
    break;

  case 187:

/* Line 1806 of yacc.c  */
#line 763 "parser.y"
    { (yyval.ival) = (yyvsp[(1) - (2)].ival) + 1; }
    break;

  case 188:

/* Line 1806 of yacc.c  */
#line 767 "parser.y"
    { (yyval.ival) = 0; }
    break;

  case 189:

/* Line 1806 of yacc.c  */
#line 768 "parser.y"
    { (yyval.ival) = (yyvsp[(1) - (1)].ival); }
    break;

  case 190:

/* Line 1806 of yacc.c  */
#line 772 "parser.y"
    { (yyval.expr) = new_prop_and_value_expression(ctx, NULL); }
    break;

  case 191:

/* Line 1806 of yacc.c  */
#line 774 "parser.y"
    { (yyval.expr) = new_prop_and_value_expression(ctx, (yyvsp[(2) - (3)].property_list)); }
    break;

  case 192:

/* Line 1806 of yacc.c  */
#line 779 "parser.y"
    { (yyval.property_list) = new_property_list(ctx, (yyvsp[(1) - (3)].literal), (yyvsp[(3) - (3)].expr)); }
    break;

  case 193:

/* Line 1806 of yacc.c  */
#line 781 "parser.y"
    { (yyval.property_list) = property_list_add(ctx, (yyvsp[(1) - (5)].property_list), (yyvsp[(3) - (5)].literal), (yyvsp[(5) - (5)].expr)); }
    break;

  case 194:

/* Line 1806 of yacc.c  */
#line 785 "parser.y"
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[(1) - (1)].identifier)); }
    break;

  case 195:

/* Line 1806 of yacc.c  */
#line 786 "parser.y"
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[(1) - (1)].wstr)); }
    break;

  case 196:

/* Line 1806 of yacc.c  */
#line 787 "parser.y"
    { (yyval.literal) = (yyvsp[(1) - (1)].literal); }
    break;

  case 197:

/* Line 1806 of yacc.c  */
#line 791 "parser.y"
    { (yyval.identifier) = NULL; }
    break;

  case 198:

/* Line 1806 of yacc.c  */
#line 792 "parser.y"
    { (yyval.identifier) = (yyvsp[(1) - (1)].identifier); }
    break;

  case 199:

/* Line 1806 of yacc.c  */
#line 796 "parser.y"
    { (yyval.literal) = new_null_literal(ctx); }
    break;

  case 200:

/* Line 1806 of yacc.c  */
#line 797 "parser.y"
    { (yyval.literal) = (yyvsp[(1) - (1)].literal); }
    break;

  case 201:

/* Line 1806 of yacc.c  */
#line 798 "parser.y"
    { (yyval.literal) = (yyvsp[(1) - (1)].literal); }
    break;

  case 202:

/* Line 1806 of yacc.c  */
#line 799 "parser.y"
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[(1) - (1)].wstr)); }
    break;

  case 203:

/* Line 1806 of yacc.c  */
#line 800 "parser.y"
    { (yyval.literal) = parse_regexp(ctx);
                                  if(!(yyval.literal)) YYABORT; }
    break;

  case 204:

/* Line 1806 of yacc.c  */
#line 802 "parser.y"
    { (yyval.literal) = parse_regexp(ctx);
                                  if(!(yyval.literal)) YYABORT; }
    break;

  case 205:

/* Line 1806 of yacc.c  */
#line 807 "parser.y"
    { (yyval.literal) = new_boolean_literal(ctx, VARIANT_TRUE); }
    break;

  case 206:

/* Line 1806 of yacc.c  */
#line 808 "parser.y"
    { (yyval.literal) = new_boolean_literal(ctx, VARIANT_FALSE); }
    break;

  case 207:

/* Line 1806 of yacc.c  */
#line 809 "parser.y"
    { (yyval.literal) = (yyvsp[(1) - (1)].literal); }
    break;

  case 209:

/* Line 1806 of yacc.c  */
#line 813 "parser.y"
    { if(!allow_auto_semicolon(ctx)) {YYABORT;} }
    break;

  case 211:

/* Line 1806 of yacc.c  */
#line 817 "parser.y"
    { set_error(ctx, JS_E_MISSING_LBRACKET); YYABORT; }
    break;

  case 213:

/* Line 1806 of yacc.c  */
#line 821 "parser.y"
    { set_error(ctx, JS_E_MISSING_RBRACKET); YYABORT; }
    break;

  case 215:

/* Line 1806 of yacc.c  */
#line 825 "parser.y"
    { set_error(ctx, JS_E_MISSING_SEMICOLON); YYABORT; }
    break;



/* Line 1806 of yacc.c  */
#line 3540 "parser.tab.c"
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
      yyerror (ctx, YY_("syntax error"));
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
        yyerror (ctx, yymsgp);
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
		      yytoken, &yylval, ctx);
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
		  yystos[yystate], yyvsp, ctx);
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
  yyerror (ctx, YY_("memory exhausted"));
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
                  yytoken, &yylval, ctx);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, ctx);
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
#line 827 "parser.y"


static BOOL allow_auto_semicolon(parser_ctx_t *ctx)
{
    return ctx->nl || ctx->ptr == ctx->end || *(ctx->ptr-1) == '}';
}

static void *new_statement(parser_ctx_t *ctx, statement_type_t type, size_t size)
{
    statement_t *stat;

    stat = parser_alloc(ctx, size ? size : sizeof(*stat));
    if(!stat)
        return NULL;

    stat->type = type;
    stat->next = NULL;

    return stat;
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
    block_statement_t *ret;

    ret = new_statement(ctx, STAT_BLOCK, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->stat_list = list ? list->head : NULL;

    return &ret->stat;
}

static variable_declaration_t *new_variable_declaration(parser_ctx_t *ctx, const WCHAR *identifier, expression_t *expr)
{
    variable_declaration_t *ret = parser_alloc(ctx, sizeof(variable_declaration_t));

    ret->identifier = identifier;
    ret->expr = expr;
    ret->next = NULL;
    ret->global_next = NULL;

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
    var_statement_t *ret;

    ret = new_statement(ctx, STAT_VAR, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->variable_list = variable_list->head;

    return &ret->stat;
}

static statement_t *new_expression_statement(parser_ctx_t *ctx, expression_t *expr)
{
    expression_statement_t *ret;

    ret = new_statement(ctx, STAT_EXPR, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->expr = expr;

    return &ret->stat;
}

static statement_t *new_if_statement(parser_ctx_t *ctx, expression_t *expr, statement_t *if_stat, statement_t *else_stat)
{
    if_statement_t *ret;

    ret = new_statement(ctx, STAT_IF, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->expr = expr;
    ret->if_stat = if_stat;
    ret->else_stat = else_stat;

    return &ret->stat;
}

static statement_t *new_while_statement(parser_ctx_t *ctx, BOOL dowhile, expression_t *expr, statement_t *stat)
{
    while_statement_t *ret;

    ret = new_statement(ctx, STAT_WHILE, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->do_while = dowhile;
    ret->expr = expr;
    ret->statement = stat;

    return &ret->stat;
}

static statement_t *new_for_statement(parser_ctx_t *ctx, variable_list_t *variable_list, expression_t *begin_expr,
        expression_t *expr, expression_t *end_expr, statement_t *statement)
{
    for_statement_t *ret;

    ret = new_statement(ctx, STAT_FOR, sizeof(*ret));
    if(!ret)
        return NULL;

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
    forin_statement_t *ret;

    ret = new_statement(ctx, STAT_FORIN, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->variable = variable;
    ret->expr = expr;
    ret->in_expr = in_expr;
    ret->statement = statement;

    return &ret->stat;
}

static statement_t *new_continue_statement(parser_ctx_t *ctx, const WCHAR *identifier)
{
    branch_statement_t *ret;

    ret = new_statement(ctx, STAT_CONTINUE, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->identifier = identifier;

    return &ret->stat;
}

static statement_t *new_break_statement(parser_ctx_t *ctx, const WCHAR *identifier)
{
    branch_statement_t *ret;

    ret = new_statement(ctx, STAT_BREAK, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->identifier = identifier;

    return &ret->stat;
}

static statement_t *new_return_statement(parser_ctx_t *ctx, expression_t *expr)
{
    expression_statement_t *ret;

    ret = new_statement(ctx, STAT_RETURN, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->expr = expr;

    return &ret->stat;
}

static statement_t *new_with_statement(parser_ctx_t *ctx, expression_t *expr, statement_t *statement)
{
    with_statement_t *ret;

    ret = new_statement(ctx, STAT_WITH, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->expr = expr;
    ret->statement = statement;

    return &ret->stat;
}

static statement_t *new_labelled_statement(parser_ctx_t *ctx, const WCHAR *identifier, statement_t *statement)
{
    labelled_statement_t *ret;

    ret = new_statement(ctx, STAT_LABEL, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->identifier = identifier;
    ret->statement = statement;

    return &ret->stat;
}

static statement_t *new_switch_statement(parser_ctx_t *ctx, expression_t *expr, case_clausule_t *case_list)
{
    switch_statement_t *ret;

    ret = new_statement(ctx, STAT_SWITCH, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->expr = expr;
    ret->case_list = case_list;

    return &ret->stat;
}

static statement_t *new_throw_statement(parser_ctx_t *ctx, expression_t *expr)
{
    expression_statement_t *ret;

    ret = new_statement(ctx, STAT_THROW, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->expr = expr;

    return &ret->stat;
}

static statement_t *new_try_statement(parser_ctx_t *ctx, statement_t *try_statement,
       catch_block_t *catch_block, statement_t *finally_statement)
{
    try_statement_t *ret;

    ret = new_statement(ctx, STAT_TRY, sizeof(*ret));
    if(!ret)
        return NULL;

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
    function_expression_t *ret = new_expression(ctx, EXPR_FUNC, sizeof(*ret));

    ret->identifier = identifier;
    ret->parameter_list = parameter_list ? parameter_list->head : NULL;
    ret->source_elements = source_elements;
    ret->src_str = src_str;
    ret->src_len = src_len;
    ret->next = NULL;

    return &ret->expr;
}

static void *new_expression(parser_ctx_t *ctx, expression_type_t type, size_t size)
{
    expression_t *ret = parser_alloc(ctx, size ? size : sizeof(*ret));

    ret->type = type;

    return ret;
}

static expression_t *new_binary_expression(parser_ctx_t *ctx, expression_type_t type,
       expression_t *expression1, expression_t *expression2)
{
    binary_expression_t *ret = new_expression(ctx, type, sizeof(*ret));

    ret->expression1 = expression1;
    ret->expression2 = expression2;

    return &ret->expr;
}

static expression_t *new_unary_expression(parser_ctx_t *ctx, expression_type_t type, expression_t *expression)
{
    unary_expression_t *ret = new_expression(ctx, type, sizeof(*ret));

    ret->expression = expression;

    return &ret->expr;
}

static expression_t *new_conditional_expression(parser_ctx_t *ctx, expression_t *expression,
       expression_t *true_expression, expression_t *false_expression)
{
    conditional_expression_t *ret = new_expression(ctx, EXPR_COND, sizeof(*ret));

    ret->expression = expression;
    ret->true_expression = true_expression;
    ret->false_expression = false_expression;

    return &ret->expr;
}

static expression_t *new_member_expression(parser_ctx_t *ctx, expression_t *expression, const WCHAR *identifier)
{
    member_expression_t *ret = new_expression(ctx, EXPR_MEMBER, sizeof(*ret));

    ret->expression = expression;
    ret->identifier = identifier;

    return &ret->expr;
}

static expression_t *new_new_expression(parser_ctx_t *ctx, expression_t *expression, argument_list_t *argument_list)
{
    call_expression_t *ret = new_expression(ctx, EXPR_NEW, sizeof(*ret));

    ret->expression = expression;
    ret->argument_list = argument_list ? argument_list->head : NULL;

    return &ret->expr;
}

static expression_t *new_call_expression(parser_ctx_t *ctx, expression_t *expression, argument_list_t *argument_list)
{
    call_expression_t *ret = new_expression(ctx, EXPR_CALL, sizeof(*ret));

    ret->expression = expression;
    ret->argument_list = argument_list ? argument_list->head : NULL;

    return &ret->expr;
}

static int parser_error(parser_ctx_t *ctx, const char *str)
{
    return 0;
}

static void set_error(parser_ctx_t *ctx, UINT error)
{
    ctx->hres = error;
}

static BOOL explicit_error(parser_ctx_t *ctx, void *obj, WCHAR next)
{
    if(obj || *(ctx->ptr-1)==next) return TRUE;

    set_error(ctx, JS_E_SYNTAX);
    return FALSE;
}


static expression_t *new_identifier_expression(parser_ctx_t *ctx, const WCHAR *identifier)
{
    identifier_expression_t *ret = new_expression(ctx, EXPR_IDENT, sizeof(*ret));

    ret->identifier = identifier;

    return &ret->expr;
}

static expression_t *new_array_literal_expression(parser_ctx_t *ctx, element_list_t *element_list, int length)
{
    array_literal_expression_t *ret = new_expression(ctx, EXPR_ARRAYLIT, sizeof(*ret));

    ret->element_list = element_list ? element_list->head : NULL;
    ret->length = length;

    return &ret->expr;
}

static expression_t *new_prop_and_value_expression(parser_ctx_t *ctx, property_list_t *property_list)
{
    property_value_expression_t *ret = new_expression(ctx, EXPR_PROPVAL, sizeof(*ret));

    ret->property_list = property_list ? property_list->head : NULL;

    return &ret->expr;
}

static expression_t *new_literal_expression(parser_ctx_t *ctx, literal_t *literal)
{
    literal_expression_t *ret = new_expression(ctx, EXPR_LITERAL, sizeof(*ret));

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

static void program_parsed(parser_ctx_t *ctx, source_elements_t *source)
{
    ctx->source = source;
    if(!ctx->lexer_error)
        ctx->hres = S_OK;
}

void parser_release(parser_ctx_t *ctx)
{
    script_release(ctx->script);
    heap_pool_free(&ctx->heap);
    heap_free(ctx);
}

HRESULT script_parse(script_ctx_t *ctx, const WCHAR *code, const WCHAR *delimiter, BOOL from_eval,
        parser_ctx_t **ret)
{
    parser_ctx_t *parser_ctx;
    heap_pool_t *mark;
    HRESULT hres;

    const WCHAR html_tagW[] = {'<','/','s','c','r','i','p','t','>',0};

    parser_ctx = heap_alloc_zero(sizeof(parser_ctx_t));
    if(!parser_ctx)
        return E_OUTOFMEMORY;

    parser_ctx->hres = JS_E_SYNTAX;
    parser_ctx->is_html = delimiter && !strcmpiW(delimiter, html_tagW);

    parser_ctx->begin = parser_ctx->ptr = code;
    parser_ctx->end = parser_ctx->begin + strlenW(parser_ctx->begin);

    script_addref(ctx);
    parser_ctx->script = ctx;

    mark = heap_pool_mark(&ctx->tmp_heap);
    heap_pool_init(&parser_ctx->heap);

    parser_parse(parser_ctx);
    heap_pool_clear(mark);
    hres = parser_ctx->hres;
    if(FAILED(hres)) {
        WARN("parser failed around %s\n",
            debugstr_w(parser_ctx->begin+20 > parser_ctx->ptr ? parser_ctx->begin : parser_ctx->ptr-20));
        parser_release(parser_ctx);
        return hres;
    }

    *ret = parser_ctx;
    return S_OK;
}

