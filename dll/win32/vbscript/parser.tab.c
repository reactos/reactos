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

#include "vbscript.h"

static int parser_error(parser_ctx_t *,const char*);

static void parse_complete(parser_ctx_t*,BOOL);

static void source_add_statement(parser_ctx_t*,statement_t*);
static void source_add_class(parser_ctx_t*,class_decl_t*);

static void *new_expression(parser_ctx_t*,expression_type_t,size_t);
static expression_t *new_bool_expression(parser_ctx_t*,VARIANT_BOOL);
static expression_t *new_string_expression(parser_ctx_t*,const WCHAR*);
static expression_t *new_long_expression(parser_ctx_t*,expression_type_t,LONG);
static expression_t *new_double_expression(parser_ctx_t*,double);
static expression_t *new_unary_expression(parser_ctx_t*,expression_type_t,expression_t*);
static expression_t *new_binary_expression(parser_ctx_t*,expression_type_t,expression_t*,expression_t*);
static expression_t *new_new_expression(parser_ctx_t*,const WCHAR*);

static member_expression_t *new_member_expression(parser_ctx_t*,expression_t*,const WCHAR*);

static void *new_statement(parser_ctx_t*,statement_type_t,size_t);
static statement_t *new_call_statement(parser_ctx_t*,BOOL,member_expression_t*);
static statement_t *new_assign_statement(parser_ctx_t*,member_expression_t*,expression_t*);
static statement_t *new_set_statement(parser_ctx_t*,member_expression_t*,expression_t*);
static statement_t *new_dim_statement(parser_ctx_t*,dim_decl_t*);
static statement_t *new_while_statement(parser_ctx_t*,statement_type_t,expression_t*,statement_t*);
static statement_t *new_forto_statement(parser_ctx_t*,const WCHAR*,expression_t*,expression_t*,expression_t*,statement_t*);
static statement_t *new_foreach_statement(parser_ctx_t*,const WCHAR*,expression_t*,statement_t*);
static statement_t *new_if_statement(parser_ctx_t*,expression_t*,statement_t*,elseif_decl_t*,statement_t*);
static statement_t *new_function_statement(parser_ctx_t*,function_decl_t*);
static statement_t *new_onerror_statement(parser_ctx_t*,BOOL);
static statement_t *new_const_statement(parser_ctx_t*,const_decl_t*);
static statement_t *new_select_statement(parser_ctx_t*,expression_t*,case_clausule_t*);

static dim_decl_t *new_dim_decl(parser_ctx_t*,const WCHAR*,BOOL,dim_list_t*);
static dim_list_t *new_dim(parser_ctx_t*,unsigned,dim_list_t*);
static elseif_decl_t *new_elseif_decl(parser_ctx_t*,expression_t*,statement_t*);
static function_decl_t *new_function_decl(parser_ctx_t*,const WCHAR*,function_type_t,unsigned,arg_decl_t*,statement_t*);
static arg_decl_t *new_argument_decl(parser_ctx_t*,const WCHAR*,BOOL);
static const_decl_t *new_const_decl(parser_ctx_t*,const WCHAR*,expression_t*);
static case_clausule_t *new_case_clausule(parser_ctx_t*,expression_t*,statement_t*,case_clausule_t*);

static class_decl_t *new_class_decl(parser_ctx_t*);
static class_decl_t *add_class_function(parser_ctx_t*,class_decl_t*,function_decl_t*);
static class_decl_t *add_dim_prop(parser_ctx_t*,class_decl_t*,dim_decl_t*,unsigned);

static statement_t *link_statements(statement_t*,statement_t*);

static const WCHAR propertyW[] = {'p','r','o','p','e','r','t','y',0};

#define STORAGE_IS_PRIVATE    1
#define STORAGE_IS_DEFAULT    2

#define CHECK_ERROR if(((parser_ctx_t*)ctx)->hres != S_OK) YYABORT



/* Line 268 of yacc.c  */
#line 144 "parser.tab.c"

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
     tEOF = 258,
     tNL = 259,
     tREM = 260,
     tEMPTYBRACKETS = 261,
     tTRUE = 262,
     tFALSE = 263,
     tNOT = 264,
     tAND = 265,
     tOR = 266,
     tXOR = 267,
     tEQV = 268,
     tIMP = 269,
     tNEQ = 270,
     tIS = 271,
     tLTEQ = 272,
     tGTEQ = 273,
     tMOD = 274,
     tCALL = 275,
     tDIM = 276,
     tSUB = 277,
     tFUNCTION = 278,
     tPROPERTY = 279,
     tGET = 280,
     tLET = 281,
     tCONST = 282,
     tIF = 283,
     tELSE = 284,
     tELSEIF = 285,
     tEND = 286,
     tTHEN = 287,
     tEXIT = 288,
     tWHILE = 289,
     tWEND = 290,
     tDO = 291,
     tLOOP = 292,
     tUNTIL = 293,
     tFOR = 294,
     tTO = 295,
     tSTEP = 296,
     tEACH = 297,
     tIN = 298,
     tSELECT = 299,
     tCASE = 300,
     tBYREF = 301,
     tBYVAL = 302,
     tOPTION = 303,
     tEXPLICIT = 304,
     tSTOP = 305,
     tNOTHING = 306,
     tEMPTY = 307,
     tNULL = 308,
     tCLASS = 309,
     tSET = 310,
     tNEW = 311,
     tPUBLIC = 312,
     tPRIVATE = 313,
     tDEFAULT = 314,
     tME = 315,
     tERROR = 316,
     tNEXT = 317,
     tON = 318,
     tRESUME = 319,
     tGOTO = 320,
     tIdentifier = 321,
     tString = 322,
     tLong = 323,
     tShort = 324,
     tDouble = 325
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 293 of yacc.c  */
#line 88 "parser.y"

    const WCHAR *string;
    statement_t *statement;
    expression_t *expression;
    member_expression_t *member;
    elseif_decl_t *elseif;
    dim_decl_t *dim_decl;
    dim_list_t *dim_list;
    function_decl_t *func_decl;
    arg_decl_t *arg_decl;
    class_decl_t *class_decl;
    const_decl_t *const_decl;
    case_clausule_t *case_clausule;
    unsigned uint;
    LONG lng;
    BOOL bool;
    double dbl;



/* Line 293 of yacc.c  */
#line 271 "parser.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 343 of yacc.c  */
#line 283 "parser.tab.c"

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
#define YYFINAL  5
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   862

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  87
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  59
/* YYNRULES -- Number of rules.  */
#define YYNRULES  164
/* YYNRULES -- Number of states.  */
#define YYNSTATES  335

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   325

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    81,     2,
      76,    77,    84,    82,    75,    78,    74,    85,    73,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    71,     2,
      80,    72,    79,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,    83,     2,    86,     2,     2,     2,     2,     2,
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
      65,    66,    67,    68,    69,    70
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     7,     8,    12,    13,    16,    19,    20,
      22,    24,    27,    30,    32,    35,    37,    41,    44,    47,
      51,    56,    59,    61,    67,    74,    81,    86,    88,    91,
      94,    97,   100,   103,   109,   111,   116,   121,   124,   135,
     144,   152,   154,   158,   160,   164,   166,   171,   174,   176,
     180,   182,   186,   190,   192,   195,   197,   199,   200,   203,
     213,   218,   226,   227,   230,   231,   233,   235,   238,   244,
     245,   249,   250,   255,   261,   263,   267,   269,   271,   272,
     274,   276,   280,   282,   286,   288,   292,   294,   298,   300,
     304,   306,   310,   312,   315,   317,   321,   325,   329,   333,
     337,   341,   345,   347,   351,   353,   357,   361,   363,   367,
     369,   373,   375,   379,   383,   385,   389,   391,   393,   396,
     399,   401,   404,   406,   408,   410,   412,   414,   416,   418,
     420,   422,   424,   426,   428,   430,   432,   436,   438,   446,
     447,   451,   456,   461,   465,   475,   487,   499,   508,   517,
     518,   520,   523,   525,   527,   529,   533,   535,   539,   542,
     546,   550,   552,   554,   556
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      88,     0,    -1,    89,    90,     3,    -1,    -1,    48,    49,
       4,    -1,    -1,    90,    93,    -1,    90,   135,    -1,    -1,
      92,    -1,    93,    -1,    93,    92,    -1,    94,     4,    -1,
      71,    -1,    71,    94,    -1,    95,    -1,    95,    71,    94,
      -1,    95,    71,    -1,    96,   113,    -1,    20,    96,   112,
      -1,    96,   112,    72,   116,    -1,    21,    97,    -1,   105,
      -1,    34,   116,     4,    91,    35,    -1,    36,   103,   116,
       4,    91,    37,    -1,    36,     4,    91,    37,   103,   116,
      -1,    36,     4,    91,    37,    -1,   138,    -1,    33,    36,
      -1,    33,    39,    -1,    33,    23,    -1,    33,    24,    -1,
      33,    22,    -1,    55,    96,   112,    72,   116,    -1,    50,
      -1,    63,    61,    64,    62,    -1,    63,    61,    65,    73,
      -1,    27,   100,    -1,    39,   144,    72,   116,    40,   116,
     104,     4,    91,    62,    -1,    39,    42,   144,    43,   116,
       4,    91,    62,    -1,    44,    45,   116,   145,   111,    31,
      44,    -1,   144,    -1,   130,    74,   144,    -1,    98,    -1,
      98,    75,    97,    -1,   144,    -1,   144,    76,    99,    77,
      -1,   144,     6,    -1,   133,    -1,   133,    75,    99,    -1,
     101,    -1,   101,    75,   100,    -1,   144,    72,   102,    -1,
     131,    -1,    78,   132,    -1,    34,    -1,    38,    -1,    -1,
      41,   116,    -1,    28,   116,    32,     4,    91,   107,   110,
      31,    28,    -1,    28,   116,    32,    94,    -1,    28,   116,
      32,    94,    29,    94,   106,    -1,    -1,    31,    28,    -1,
      -1,   108,    -1,   109,    -1,   109,   108,    -1,    30,   116,
      32,     4,    91,    -1,    -1,    29,     4,    91,    -1,    -1,
      45,    29,   145,    92,    -1,    45,   115,   145,    91,   111,
      -1,   114,    -1,    76,   115,    77,    -1,   114,    -1,   115,
      -1,    -1,     6,    -1,   116,    -1,   116,    75,   115,    -1,
     117,    -1,   116,    14,   117,    -1,   118,    -1,   117,    13,
     118,    -1,   119,    -1,   118,    12,   119,    -1,   120,    -1,
     119,    11,   120,    -1,   121,    -1,   120,    10,   121,    -1,
     122,    -1,     9,   121,    -1,   123,    -1,   122,    72,   123,
      -1,   122,    15,   123,    -1,   122,    79,   123,    -1,   122,
      80,   123,    -1,   122,    18,   123,    -1,   122,    17,   123,
      -1,   122,    16,   123,    -1,   124,    -1,   123,    81,   124,
      -1,   125,    -1,   124,    82,   125,    -1,   124,    78,   125,
      -1,   126,    -1,   125,    19,   126,    -1,   127,    -1,   126,
      83,   127,    -1,   128,    -1,   127,    84,   128,    -1,   127,
      85,   128,    -1,   129,    -1,   128,    86,   129,    -1,   131,
      -1,   130,    -1,    56,   144,    -1,    78,   129,    -1,   134,
      -1,    96,   112,    -1,     7,    -1,     8,    -1,    67,    -1,
     132,    -1,    52,    -1,    53,    -1,    51,    -1,    69,    -1,
      73,    -1,    68,    -1,    70,    -1,    69,    -1,    73,    -1,
      68,    -1,    76,   116,    77,    -1,    60,    -1,    54,   144,
       4,   136,    31,    54,     4,    -1,    -1,   138,     4,   136,
      -1,   140,    66,     4,   136,    -1,    21,    98,     4,   136,
      -1,   137,     4,   136,    -1,   139,    24,    25,    66,   114,
       4,    91,    31,    24,    -1,   139,    24,    26,    66,    76,
     143,    77,     4,    91,    31,    24,    -1,   139,    24,    55,
      66,    76,   143,    77,     4,    91,    31,    24,    -1,   139,
      22,   144,   141,     4,    91,    31,    22,    -1,   139,    23,
     144,   141,     4,    91,    31,    23,    -1,    -1,   140,    -1,
      57,    59,    -1,    57,    -1,    58,    -1,   114,    -1,    76,
     142,    77,    -1,   143,    -1,   143,    75,   142,    -1,   144,
     114,    -1,    46,   144,   114,    -1,    47,   144,   114,    -1,
      66,    -1,    24,    -1,     4,    -1,    71,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   147,   147,   150,   151,   153,   155,   156,   159,   160,
     163,   164,   167,   170,   171,   172,   173,   174,   177,   178,
     179,   181,   182,   183,   185,   188,   191,   192,   193,   194,
     195,   196,   197,   198,   200,   201,   202,   203,   204,   206,
     208,   212,   213,   216,   217,   220,   221,   222,   225,   226,
     229,   230,   233,   236,   237,   240,   241,   244,   245,   248,
     250,   251,   254,   256,   259,   260,   263,   264,   267,   271,
     272,   275,   276,   277,   281,   282,   285,   286,   288,   290,
     293,   294,   297,   298,   301,   302,   305,   306,   309,   310,
     313,   314,   317,   318,   321,   322,   323,   324,   325,   326,
     327,   328,   331,   332,   335,   336,   337,   340,   341,   344,
     345,   349,   350,   352,   356,   357,   360,   361,   362,   363,
     366,   367,   370,   371,   372,   373,   374,   375,   376,   379,
     380,   381,   382,   385,   386,   387,   390,   391,   394,   397,
     398,   400,   402,   403,   406,   408,   410,   414,   416,   420,
     421,   424,   425,   426,   429,   430,   433,   434,   437,   438,
     439,   443,   444,   448,   449
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "tEOF", "tNL", "tREM", "tEMPTYBRACKETS",
  "tTRUE", "tFALSE", "tNOT", "tAND", "tOR", "tXOR", "tEQV", "tIMP", "tNEQ",
  "tIS", "tLTEQ", "tGTEQ", "tMOD", "tCALL", "tDIM", "tSUB", "tFUNCTION",
  "tPROPERTY", "tGET", "tLET", "tCONST", "tIF", "tELSE", "tELSEIF", "tEND",
  "tTHEN", "tEXIT", "tWHILE", "tWEND", "tDO", "tLOOP", "tUNTIL", "tFOR",
  "tTO", "tSTEP", "tEACH", "tIN", "tSELECT", "tCASE", "tBYREF", "tBYVAL",
  "tOPTION", "tEXPLICIT", "tSTOP", "tNOTHING", "tEMPTY", "tNULL", "tCLASS",
  "tSET", "tNEW", "tPUBLIC", "tPRIVATE", "tDEFAULT", "tME", "tERROR",
  "tNEXT", "tON", "tRESUME", "tGOTO", "tIdentifier", "tString", "tLong",
  "tShort", "tDouble", "':'", "'='", "'0'", "'.'", "','", "'('", "')'",
  "'-'", "'>'", "'<'", "'&'", "'+'", "'\\\\'", "'*'", "'/'", "'^'",
  "$accept", "Program", "OptionExplicit_opt", "SourceElements",
  "StatementsNl_opt", "StatementsNl", "StatementNl", "Statement",
  "SimpleStatement", "MemberExpression", "DimDeclList", "DimDecl",
  "DimList", "ConstDeclList", "ConstDecl", "ConstExpression", "DoType",
  "Step_opt", "IfStatement", "EndIf_opt", "ElseIfs_opt", "ElseIfs",
  "ElseIf", "Else_opt", "CaseClausules", "Arguments_opt",
  "ArgumentList_opt", "EmptyBrackets_opt", "ExpressionList", "Expression",
  "EqvExpression", "XorExpression", "OrExpression", "AndExpression",
  "NotExpression", "EqualityExpression", "ConcatExpression",
  "AdditiveExpression", "ModExpression", "IntdivExpression",
  "MultiplicativeExpression", "ExpExpression", "UnaryExpression",
  "CallExpression", "LiteralExpression", "NumericLiteralExpression",
  "IntegerValue", "PrimaryExpression", "ClassDeclaration", "ClassBody",
  "PropertyDecl", "FunctionDecl", "Storage_opt", "Storage",
  "ArgumentsDecl_opt", "ArgumentDeclList", "ArgumentDecl", "Identifier",
  "StSep", 0
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
     325,    58,    61,    48,    46,    44,    40,    41,    45,    62,
      60,    38,    43,    92,    42,    47,    94
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    87,    88,    89,    89,    90,    90,    90,    91,    91,
      92,    92,    93,    94,    94,    94,    94,    94,    95,    95,
      95,    95,    95,    95,    95,    95,    95,    95,    95,    95,
      95,    95,    95,    95,    95,    95,    95,    95,    95,    95,
      95,    96,    96,    97,    97,    98,    98,    98,    99,    99,
     100,   100,   101,   102,   102,   103,   103,   104,   104,   105,
     105,   105,   106,   106,   107,   107,   108,   108,   109,   110,
     110,   111,   111,   111,   112,   112,   113,   113,   114,   114,
     115,   115,   116,   116,   117,   117,   118,   118,   119,   119,
     120,   120,   121,   121,   122,   122,   122,   122,   122,   122,
     122,   122,   123,   123,   124,   124,   124,   125,   125,   126,
     126,   127,   127,   127,   128,   128,   129,   129,   129,   129,
     130,   130,   131,   131,   131,   131,   131,   131,   131,   132,
     132,   132,   132,   133,   133,   133,   134,   134,   135,   136,
     136,   136,   136,   136,   137,   137,   137,   138,   138,   139,
     139,   140,   140,   140,   141,   141,   142,   142,   143,   143,
     143,   144,   144,   145,   145
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     3,     0,     3,     0,     2,     2,     0,     1,
       1,     2,     2,     1,     2,     1,     3,     2,     2,     3,
       4,     2,     1,     5,     6,     6,     4,     1,     2,     2,
       2,     2,     2,     5,     1,     4,     4,     2,    10,     8,
       7,     1,     3,     1,     3,     1,     4,     2,     1,     3,
       1,     3,     3,     1,     2,     1,     1,     0,     2,     9,
       4,     7,     0,     2,     0,     1,     1,     2,     5,     0,
       3,     0,     4,     5,     1,     3,     1,     1,     0,     1,
       1,     3,     1,     3,     1,     3,     1,     3,     1,     3,
       1,     3,     1,     2,     1,     3,     3,     3,     3,     3,
       3,     3,     1,     3,     1,     3,     3,     1,     3,     1,
       3,     1,     3,     3,     1,     3,     1,     1,     2,     2,
       1,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     3,     1,     7,     0,
       3,     4,     4,     3,     9,    11,    11,     8,     8,     0,
       1,     2,     1,     1,     1,     3,     1,     3,     2,     3,
       3,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       3,     0,     0,     5,     0,     1,   149,     4,     2,     0,
       0,   162,     0,     0,     0,     0,     0,     0,     0,    34,
       0,     0,   152,   153,   137,     0,   161,    13,     0,     6,
       0,    15,    78,    22,     0,   120,     7,    27,     0,   150,
      41,    78,    21,    43,    45,    37,    50,     0,   122,   123,
       0,   128,   126,   127,     0,   124,   131,   129,   132,   130,
       0,    78,     0,    82,    84,    86,    88,    90,    92,    94,
     102,   104,   107,   109,   111,   114,   117,   116,   125,    32,
      30,    31,    28,    29,     0,   149,    55,    56,     0,     0,
       0,     0,     0,    78,   151,     0,    14,     0,    12,    17,
      79,     0,   121,    18,    76,    77,    80,     0,     0,     0,
       0,    19,    74,     0,    47,     0,     0,     0,    93,   118,
     119,   121,     0,   149,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   149,     0,     9,    10,     0,     0,     0,
       0,   149,   121,     0,     0,   136,    16,     0,     0,     0,
       0,    42,    78,    78,    44,   135,   133,   134,     0,    48,
      51,     0,    52,    53,    83,     8,    60,    85,    87,    89,
      91,    96,   101,   100,    99,    95,    97,    98,   103,   106,
     105,   108,   110,   112,   113,   115,     0,    26,    11,   149,
       0,     0,   163,   164,    71,     0,     0,     0,     0,     0,
     150,     0,    35,    36,    75,    20,    81,     0,   154,     0,
       0,    46,     0,    54,    64,   149,    23,     0,     0,     0,
       0,     0,     0,     0,     0,   149,   149,     0,     0,    33,
       0,     0,     0,   156,    78,   149,   149,    49,     0,    69,
      65,    66,    62,    25,    24,   149,    57,     0,     0,     0,
     149,     0,   143,   140,     0,     0,     0,   149,    78,    78,
     155,     0,   158,     0,     0,     0,     0,     0,    67,     0,
      61,     0,     0,     0,   149,     8,    40,   142,   138,    78,
       0,     0,   141,   159,   160,   157,     0,     0,     0,   149,
       0,    63,    39,    58,   149,    72,    71,     0,     0,     0,
     147,   148,     8,    70,    59,     0,    73,   149,     0,     0,
      68,    38,     0,     0,     0,     0,   149,   149,   144,     0,
       0,     0,     0,   145,   146
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,     3,     6,   144,   145,   146,    30,    31,    61,
      42,    43,   168,    45,    46,   172,    88,   283,    33,   280,
     249,   250,   251,   277,   232,   102,   103,   112,   157,   106,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,   169,    35,    36,   206,
     207,    37,    38,    39,   219,   242,   243,    40,   204
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -146
static const yytype_int16 yypact[] =
{
     -13,     2,    57,  -146,    39,  -146,   307,  -146,  -146,     1,
      -2,  -146,    -2,   468,   184,   468,    20,   -11,    15,  -146,
      -2,     1,    40,  -146,  -146,    62,  -146,   557,   468,  -146,
      59,    84,   388,  -146,    94,  -146,  -146,  -146,   109,  -146,
    -146,     4,  -146,   101,     8,  -146,   106,   111,  -146,  -146,
     468,  -146,  -146,  -146,    -2,  -146,  -146,  -146,  -146,  -146,
     498,     4,    27,   175,   182,   188,   191,  -146,    29,   124,
      38,   193,   130,    68,   129,  -146,    94,  -146,  -146,  -146,
    -146,  -146,  -146,  -146,    24,   598,  -146,  -146,   468,    -2,
     145,   468,   214,     4,  -146,    95,  -146,     5,  -146,   557,
    -146,   468,   152,  -146,    64,  -146,    19,    -2,    -2,    -2,
     468,   161,  -146,    -2,  -146,    45,    -2,    97,  -146,  -146,
    -146,  -146,   468,   348,   468,   468,   468,   468,   498,   498,
     498,   498,   498,   498,   498,   498,   498,   498,   498,   498,
     498,   498,   498,   651,   199,  -146,   557,    48,   194,   468,
      16,   120,   166,   181,   172,  -146,  -146,   169,    35,   468,
     468,  -146,    10,    10,  -146,  -146,  -146,  -146,   170,   173,
    -146,   117,  -146,  -146,   175,   557,   221,   182,   188,   191,
    -146,   124,   124,   124,   124,   124,   124,   124,    38,   193,
     193,   130,    68,   129,   129,  -146,   217,    73,  -146,   598,
     468,   121,  -146,  -146,   208,    -2,   223,   252,   255,   150,
     195,   468,  -146,  -146,  -146,   246,  -146,    26,  -146,   258,
     259,  -146,    45,  -146,   234,   786,  -146,   468,   228,    61,
     468,   418,   236,   266,   218,   120,   120,    14,   267,   246,
      -2,    -2,   196,   200,   268,   692,   692,  -146,   468,   247,
    -146,   234,   253,   246,  -146,   733,    12,     7,     7,   233,
     120,   279,  -146,  -146,   220,   224,   225,   120,   268,   268,
    -146,    26,  -146,   256,   261,    34,   290,   265,  -146,   269,
    -146,   237,   468,   297,   786,   557,  -146,  -146,  -146,   268,
     226,   229,  -146,  -146,  -146,  -146,   282,   284,   305,   692,
     286,  -146,  -146,   246,   733,  -146,   208,   308,    26,    26,
    -146,  -146,   557,  -146,  -146,   254,  -146,   692,   238,   240,
    -146,  -146,   287,   315,   320,   306,   692,   692,  -146,   298,
     301,   309,   312,  -146,  -146
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -146,  -146,  -146,  -146,  -101,  -137,   332,   -23,  -146,    -6,
     231,   134,   123,   232,  -146,  -146,   153,  -146,  -146,  -146,
    -146,    91,  -146,  -146,    41,    -5,  -146,     0,   -31,    55,
     227,   230,   235,   245,   -42,  -146,    99,   239,    43,   215,
     216,    51,   -53,    -4,   241,   185,  -146,  -146,  -146,   -78,
    -146,  -145,  -139,  -133,   203,    88,  -112,    17,   -48
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -150
static const yytype_int16 yytable[] =
{
      32,   105,    34,    41,    96,    34,   208,   120,   118,   198,
     100,   202,   209,    11,   114,    93,   100,    34,   210,   122,
     202,    32,    11,    34,    85,    11,   122,    44,   143,    47,
     122,    89,   104,   122,    90,     1,   111,    92,   122,   264,
     265,   122,   196,     7,   128,   129,   130,   131,   122,   122,
      11,     4,   199,   282,    86,    26,   121,     5,    87,   123,
      91,    24,   122,    98,    26,   255,   298,    26,    62,   266,
      84,   119,   240,   241,   224,   122,   156,    28,   203,    32,
     110,    34,   155,    97,   115,   180,   217,   203,   152,   195,
     208,   208,    26,    32,   160,    34,   209,   209,   228,    94,
     176,   132,   210,   210,    48,    49,   148,    86,   133,   134,
     160,    87,   155,   165,   166,   208,   136,    32,   167,    34,
     137,   209,   208,    95,   161,   162,   163,   210,   209,   216,
      44,   108,   109,    47,   210,   122,   -74,    32,   -74,    34,
      32,   205,    34,   147,   273,   274,   150,   305,    51,    52,
      53,  -139,   140,   141,   281,    99,   158,   262,   263,   153,
     154,   230,   218,   218,    55,    56,    57,    58,   107,    32,
      59,    34,   108,   109,   237,   171,   113,    22,    23,   189,
     190,   116,   287,   117,   306,    56,    57,    58,   124,   292,
      59,   193,   194,    32,   125,    34,   318,   319,   313,   126,
     258,   127,   252,   315,   201,   135,    79,    80,    81,   284,
     285,   320,   138,   139,   215,   142,   322,   149,   151,    32,
      82,    34,    44,    83,   159,   329,   330,   181,   182,   183,
     184,   185,   186,   187,   244,  -121,   197,   200,   211,    32,
      32,    34,    34,   212,   272,   213,   214,   221,   222,    32,
     225,    34,   226,   231,   234,   229,   235,   268,   269,   236,
     122,   238,   245,   246,   248,   254,   239,   259,   293,   294,
     260,   267,   261,   270,   100,   271,   276,   286,    32,    32,
      34,    34,   253,   288,   279,   256,   289,   296,   244,   307,
     290,   291,   297,    32,   299,    34,   300,   301,    32,   302,
      34,   304,   308,   275,   310,   309,    32,   311,    34,   312,
       8,    32,   317,    34,   314,   323,   321,   324,   325,   326,
      32,    32,    34,    34,   327,   244,   244,     9,    10,   331,
     328,    11,   332,   333,    12,    13,   334,   303,    29,   233,
      14,    15,   278,    16,   164,   247,    17,   316,   170,   174,
     227,    18,   175,   191,   177,   192,   223,    19,   173,   295,
     178,    20,    21,     0,    22,    23,   220,    24,     9,    10,
      25,   179,    11,    26,   188,    12,    13,     0,    27,     0,
       0,    14,    15,    28,    16,     0,     0,    17,     0,     0,
       0,     0,    18,     0,   100,    48,    49,    50,    19,     0,
       0,     0,     0,    21,     0,    22,    23,     0,    24,     0,
       0,    25,    11,     0,    26,     0,     0,     0,     0,    27,
       0,     0,     0,     0,    28,    48,    49,    50,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    51,
      52,    53,    11,     0,    54,     0,     0,   257,    24,     0,
       0,     0,     0,     0,    26,    55,    56,    57,    58,     0,
       0,    59,     0,     0,   101,     0,    60,     0,     0,    51,
      52,    53,     0,     0,    54,    48,    49,    50,    24,     0,
       0,     0,     0,     0,    26,    55,    56,    57,    58,     0,
       0,    59,    11,     0,    28,     0,    60,     0,     0,     0,
       0,     0,     0,     0,     0,    48,    49,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    51,
      52,    53,    11,     0,    54,     0,     0,     0,    24,     0,
       0,     0,     0,     0,    26,    55,    56,    57,    58,     0,
       0,    59,     0,     0,    28,     0,    60,     0,     0,    51,
      52,    53,     0,     0,    54,     0,     0,     0,    24,     0,
       0,     0,     0,     0,    26,    55,    56,    57,    58,     0,
       0,    59,     0,     0,    28,     0,    60,     9,    10,  -149,
    -149,    11,     0,     0,    12,    13,     0,     0,     0,     0,
      14,    15,     0,    16,     0,     0,    17,     0,     0,     0,
       0,    18,     0,     0,     0,     0,     0,    19,     0,     0,
       0,     0,    21,     0,    22,    23,     0,    24,     9,    10,
      25,     0,    11,    26,     0,    12,    13,     0,    27,     0,
       0,    14,    15,    28,    16,    -8,     0,    17,     0,     0,
       0,     0,    18,     0,     0,     0,     0,     0,    19,     0,
       0,     0,     0,    21,     0,    22,    23,     0,    24,     0,
       0,    25,     0,     0,    26,     0,     0,     0,     0,    27,
       0,     9,    10,     0,    28,    11,     0,     0,    12,    13,
       0,     0,     0,     0,    14,    15,    -8,    16,     0,     0,
      17,     0,     0,     0,     0,    18,     0,     0,     0,     0,
       0,    19,     0,     0,     0,     0,    21,     0,    22,    23,
       0,    24,     9,    10,    25,     0,    11,    26,     0,    12,
      13,     0,    27,    -8,     0,    14,    15,    28,    16,     0,
       0,    17,     0,     0,     0,     0,    18,     0,     0,     0,
       0,     0,    19,     0,     0,     0,     0,    21,     0,    22,
      23,     0,    24,     9,    10,    25,     0,    11,    26,     0,
      12,    13,     0,    27,     0,     0,    14,    15,    28,    16,
       0,     0,    17,     0,     0,     0,     0,    18,     0,     0,
       0,     0,     0,    19,     0,     0,     0,     0,    21,     0,
      22,    23,     0,    24,     0,    -8,    25,     0,     0,    26,
       0,     0,     0,     0,    27,     0,     9,    10,     0,    28,
      11,     0,     0,    12,    13,     0,     0,     0,     0,    14,
      15,     0,    16,     0,     0,    17,     0,     0,     0,     0,
      18,     0,     0,     0,     0,     0,    19,     0,     0,     0,
       0,    21,     0,    22,    23,     0,    24,     0,     0,    25,
       0,     0,    26,     0,     0,     0,     0,    27,     0,     0,
       0,     0,    28
};

#define yypact_value_is_default(yystate) \
  ((yystate) == (-146))

#define yytable_value_is_error(yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
       6,    32,     6,     9,    27,     9,   151,    60,    50,   146,
       6,     4,   151,    24,     6,    21,     6,    21,   151,    14,
       4,    27,    24,    27,     4,    24,    14,    10,     4,    12,
      14,    42,    32,    14,    17,    48,    41,    20,    14,    25,
      26,    14,   143,     4,    15,    16,    17,    18,    14,    14,
      24,    49,     4,    41,    34,    66,    61,     0,    38,    32,
      45,    60,    14,     4,    66,     4,    32,    66,    13,    55,
      15,    54,    46,    47,   175,    14,    99,    76,    71,    85,
      76,    85,    77,    28,    76,   127,    76,    71,    93,   142,
     235,   236,    66,    99,    75,    99,   235,   236,   199,    59,
     123,    72,   235,   236,     7,     8,    89,    34,    79,    80,
      75,    38,    77,    68,    69,   260,    78,   123,    73,   123,
      82,   260,   267,    61,   107,   108,   109,   260,   267,   160,
     113,    22,    23,   116,   267,    14,    72,   143,    74,   143,
     146,    21,   146,    88,   245,   246,    91,   284,    51,    52,
      53,    31,    84,    85,   255,    71,   101,   235,   236,    64,
      65,    40,   162,   163,    67,    68,    69,    70,    74,   175,
      73,   175,    22,    23,    24,    78,    75,    57,    58,   136,
     137,    75,   260,    72,   285,    68,    69,    70,    13,   267,
      73,   140,   141,   199,    12,   199,   308,   309,   299,    11,
     231,    10,   225,   304,   149,    81,    22,    23,    24,   257,
     258,   312,    19,    83,   159,    86,   317,    72,     4,   225,
      36,   225,   205,    39,    72,   326,   327,   128,   129,   130,
     131,   132,   133,   134,   217,    74,    37,    43,    72,   245,
     246,   245,   246,    62,   244,    73,    77,    77,    75,   255,
      29,   255,    35,    45,    31,   200,     4,   240,   241,     4,
      14,    66,     4,     4,    30,    37,   211,    31,   268,   269,
       4,     4,    54,    77,     6,    75,    29,    44,   284,   285,
     284,   285,   227,     4,    31,   230,    66,    31,   271,   289,
      66,    66,    31,   299,     4,   299,    31,    28,   304,    62,
     304,     4,    76,   248,    22,    76,   312,    23,   312,     4,
       3,   317,     4,   317,    28,    77,    62,    77,    31,     4,
     326,   327,   326,   327,     4,   308,   309,    20,    21,    31,
      24,    24,    31,    24,    27,    28,    24,   282,     6,   205,
      33,    34,   251,    36,   113,   222,    39,   306,   116,   122,
     197,    44,     4,   138,   124,   139,   171,    50,   117,   271,
     125,    54,    55,    -1,    57,    58,   163,    60,    20,    21,
      63,   126,    24,    66,   135,    27,    28,    -1,    71,    -1,
      -1,    33,    34,    76,    36,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    44,    -1,     6,     7,     8,     9,    50,    -1,
      -1,    -1,    -1,    55,    -1,    57,    58,    -1,    60,    -1,
      -1,    63,    24,    -1,    66,    -1,    -1,    -1,    -1,    71,
      -1,    -1,    -1,    -1,    76,     7,     8,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    51,
      52,    53,    24,    -1,    56,    -1,    -1,    29,    60,    -1,
      -1,    -1,    -1,    -1,    66,    67,    68,    69,    70,    -1,
      -1,    73,    -1,    -1,    76,    -1,    78,    -1,    -1,    51,
      52,    53,    -1,    -1,    56,     7,     8,     9,    60,    -1,
      -1,    -1,    -1,    -1,    66,    67,    68,    69,    70,    -1,
      -1,    73,    24,    -1,    76,    -1,    78,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     7,     8,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    51,
      52,    53,    24,    -1,    56,    -1,    -1,    -1,    60,    -1,
      -1,    -1,    -1,    -1,    66,    67,    68,    69,    70,    -1,
      -1,    73,    -1,    -1,    76,    -1,    78,    -1,    -1,    51,
      52,    53,    -1,    -1,    56,    -1,    -1,    -1,    60,    -1,
      -1,    -1,    -1,    -1,    66,    67,    68,    69,    70,    -1,
      -1,    73,    -1,    -1,    76,    -1,    78,    20,    21,    22,
      23,    24,    -1,    -1,    27,    28,    -1,    -1,    -1,    -1,
      33,    34,    -1,    36,    -1,    -1,    39,    -1,    -1,    -1,
      -1,    44,    -1,    -1,    -1,    -1,    -1,    50,    -1,    -1,
      -1,    -1,    55,    -1,    57,    58,    -1,    60,    20,    21,
      63,    -1,    24,    66,    -1,    27,    28,    -1,    71,    -1,
      -1,    33,    34,    76,    36,    37,    -1,    39,    -1,    -1,
      -1,    -1,    44,    -1,    -1,    -1,    -1,    -1,    50,    -1,
      -1,    -1,    -1,    55,    -1,    57,    58,    -1,    60,    -1,
      -1,    63,    -1,    -1,    66,    -1,    -1,    -1,    -1,    71,
      -1,    20,    21,    -1,    76,    24,    -1,    -1,    27,    28,
      -1,    -1,    -1,    -1,    33,    34,    35,    36,    -1,    -1,
      39,    -1,    -1,    -1,    -1,    44,    -1,    -1,    -1,    -1,
      -1,    50,    -1,    -1,    -1,    -1,    55,    -1,    57,    58,
      -1,    60,    20,    21,    63,    -1,    24,    66,    -1,    27,
      28,    -1,    71,    31,    -1,    33,    34,    76,    36,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    44,    -1,    -1,    -1,
      -1,    -1,    50,    -1,    -1,    -1,    -1,    55,    -1,    57,
      58,    -1,    60,    20,    21,    63,    -1,    24,    66,    -1,
      27,    28,    -1,    71,    -1,    -1,    33,    34,    76,    36,
      -1,    -1,    39,    -1,    -1,    -1,    -1,    44,    -1,    -1,
      -1,    -1,    -1,    50,    -1,    -1,    -1,    -1,    55,    -1,
      57,    58,    -1,    60,    -1,    62,    63,    -1,    -1,    66,
      -1,    -1,    -1,    -1,    71,    -1,    20,    21,    -1,    76,
      24,    -1,    -1,    27,    28,    -1,    -1,    -1,    -1,    33,
      34,    -1,    36,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      44,    -1,    -1,    -1,    -1,    -1,    50,    -1,    -1,    -1,
      -1,    55,    -1,    57,    58,    -1,    60,    -1,    -1,    63,
      -1,    -1,    66,    -1,    -1,    -1,    -1,    71,    -1,    -1,
      -1,    -1,    76
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    48,    88,    89,    49,     0,    90,     4,     3,    20,
      21,    24,    27,    28,    33,    34,    36,    39,    44,    50,
      54,    55,    57,    58,    60,    63,    66,    71,    76,    93,
      94,    95,    96,   105,   130,   134,   135,   138,   139,   140,
     144,    96,    97,    98,   144,   100,   101,   144,     7,     8,
       9,    51,    52,    53,    56,    67,    68,    69,    70,    73,
      78,    96,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,    22,
      23,    24,    36,    39,   116,     4,    34,    38,   103,    42,
     144,    45,   144,    96,    59,    61,    94,   116,     4,    71,
       6,    76,   112,   113,   114,   115,   116,    74,    22,    23,
      76,   112,   114,    75,     6,    76,    75,    72,   121,   144,
     129,   112,    14,    32,    13,    12,    11,    10,    15,    16,
      17,    18,    72,    79,    80,    81,    78,    82,    19,    83,
      84,    85,    86,     4,    91,    92,    93,   116,   144,    72,
     116,     4,   112,    64,    65,    77,    94,   115,   116,    72,
      75,   144,   144,   144,    97,    68,    69,    73,    99,   133,
     100,    78,   102,   131,   117,     4,    94,   118,   119,   120,
     121,   123,   123,   123,   123,   123,   123,   123,   124,   125,
     125,   126,   127,   128,   128,   129,    91,    37,    92,     4,
      43,   116,     4,    71,   145,    21,   136,   137,   138,   139,
     140,    72,    62,    73,    77,   116,   115,    76,   114,   141,
     141,    77,    75,   132,    91,    29,    35,   103,    91,   116,
      40,    45,   111,    98,    31,     4,     4,    24,    66,   116,
      46,    47,   142,   143,   144,     4,     4,    99,    30,   107,
     108,   109,    94,   116,    37,     4,   116,    29,   115,    31,
       4,    54,   136,   136,    25,    26,    55,     4,   144,   144,
      77,    75,   114,    91,    91,   116,    29,   110,   108,    31,
     106,    91,    41,   104,   145,   145,    44,   136,     4,    66,
      66,    66,   136,   114,   114,   142,    31,    31,    32,     4,
      31,    28,    62,   116,     4,    92,    91,   114,    76,    76,
      22,    23,     4,    91,    28,    91,   111,     4,   143,   143,
      91,    62,    91,    77,    77,    31,     4,     4,    24,    91,
      91,    31,    31,    24,    24
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
#line 147 "parser.y"
    { parse_complete(ctx, (yyvsp[(1) - (3)].bool)); }
    break;

  case 3:

/* Line 1806 of yacc.c  */
#line 150 "parser.y"
    { (yyval.bool) = FALSE; }
    break;

  case 4:

/* Line 1806 of yacc.c  */
#line 151 "parser.y"
    { (yyval.bool) = TRUE; }
    break;

  case 6:

/* Line 1806 of yacc.c  */
#line 155 "parser.y"
    { source_add_statement(ctx, (yyvsp[(2) - (2)].statement)); }
    break;

  case 7:

/* Line 1806 of yacc.c  */
#line 156 "parser.y"
    { source_add_class(ctx, (yyvsp[(2) - (2)].class_decl)); }
    break;

  case 8:

/* Line 1806 of yacc.c  */
#line 159 "parser.y"
    { (yyval.statement) = NULL; }
    break;

  case 9:

/* Line 1806 of yacc.c  */
#line 160 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); }
    break;

  case 10:

/* Line 1806 of yacc.c  */
#line 163 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); }
    break;

  case 11:

/* Line 1806 of yacc.c  */
#line 164 "parser.y"
    { (yyval.statement) = link_statements((yyvsp[(1) - (2)].statement), (yyvsp[(2) - (2)].statement)); }
    break;

  case 12:

/* Line 1806 of yacc.c  */
#line 167 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (2)].statement); }
    break;

  case 13:

/* Line 1806 of yacc.c  */
#line 170 "parser.y"
    { (yyval.statement) = NULL; }
    break;

  case 14:

/* Line 1806 of yacc.c  */
#line 171 "parser.y"
    { (yyval.statement) = (yyvsp[(2) - (2)].statement); }
    break;

  case 15:

/* Line 1806 of yacc.c  */
#line 172 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); }
    break;

  case 16:

/* Line 1806 of yacc.c  */
#line 173 "parser.y"
    { (yyvsp[(1) - (3)].statement)->next = (yyvsp[(3) - (3)].statement); (yyval.statement) = (yyvsp[(1) - (3)].statement); }
    break;

  case 17:

/* Line 1806 of yacc.c  */
#line 174 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (2)].statement); }
    break;

  case 18:

/* Line 1806 of yacc.c  */
#line 177 "parser.y"
    { (yyvsp[(1) - (2)].member)->args = (yyvsp[(2) - (2)].expression); (yyval.statement) = new_call_statement(ctx, FALSE, (yyvsp[(1) - (2)].member)); CHECK_ERROR; }
    break;

  case 19:

/* Line 1806 of yacc.c  */
#line 178 "parser.y"
    { (yyvsp[(2) - (3)].member)->args = (yyvsp[(3) - (3)].expression); (yyval.statement) = new_call_statement(ctx, TRUE, (yyvsp[(2) - (3)].member)); CHECK_ERROR; }
    break;

  case 20:

/* Line 1806 of yacc.c  */
#line 180 "parser.y"
    { (yyvsp[(1) - (4)].member)->args = (yyvsp[(2) - (4)].expression); (yyval.statement) = new_assign_statement(ctx, (yyvsp[(1) - (4)].member), (yyvsp[(4) - (4)].expression)); CHECK_ERROR; }
    break;

  case 21:

/* Line 1806 of yacc.c  */
#line 181 "parser.y"
    { (yyval.statement) = new_dim_statement(ctx, (yyvsp[(2) - (2)].dim_decl)); CHECK_ERROR; }
    break;

  case 22:

/* Line 1806 of yacc.c  */
#line 182 "parser.y"
    { (yyval.statement) = (yyvsp[(1) - (1)].statement); }
    break;

  case 23:

/* Line 1806 of yacc.c  */
#line 184 "parser.y"
    { (yyval.statement) = new_while_statement(ctx, STAT_WHILE, (yyvsp[(2) - (5)].expression), (yyvsp[(4) - (5)].statement)); CHECK_ERROR; }
    break;

  case 24:

/* Line 1806 of yacc.c  */
#line 186 "parser.y"
    { (yyval.statement) = new_while_statement(ctx, (yyvsp[(2) - (6)].bool) ? STAT_WHILELOOP : STAT_UNTIL, (yyvsp[(3) - (6)].expression), (yyvsp[(5) - (6)].statement));
                                              CHECK_ERROR; }
    break;

  case 25:

/* Line 1806 of yacc.c  */
#line 189 "parser.y"
    { (yyval.statement) = new_while_statement(ctx, (yyvsp[(5) - (6)].bool) ? STAT_DOWHILE : STAT_DOUNTIL, (yyvsp[(6) - (6)].expression), (yyvsp[(3) - (6)].statement));
                                              CHECK_ERROR; }
    break;

  case 26:

/* Line 1806 of yacc.c  */
#line 191 "parser.y"
    { (yyval.statement) = new_while_statement(ctx, STAT_DOWHILE, NULL, (yyvsp[(3) - (4)].statement)); CHECK_ERROR; }
    break;

  case 27:

/* Line 1806 of yacc.c  */
#line 192 "parser.y"
    { (yyval.statement) = new_function_statement(ctx, (yyvsp[(1) - (1)].func_decl)); CHECK_ERROR; }
    break;

  case 28:

/* Line 1806 of yacc.c  */
#line 193 "parser.y"
    { (yyval.statement) = new_statement(ctx, STAT_EXITDO, 0); CHECK_ERROR; }
    break;

  case 29:

/* Line 1806 of yacc.c  */
#line 194 "parser.y"
    { (yyval.statement) = new_statement(ctx, STAT_EXITFOR, 0); CHECK_ERROR; }
    break;

  case 30:

/* Line 1806 of yacc.c  */
#line 195 "parser.y"
    { (yyval.statement) = new_statement(ctx, STAT_EXITFUNC, 0); CHECK_ERROR; }
    break;

  case 31:

/* Line 1806 of yacc.c  */
#line 196 "parser.y"
    { (yyval.statement) = new_statement(ctx, STAT_EXITPROP, 0); CHECK_ERROR; }
    break;

  case 32:

/* Line 1806 of yacc.c  */
#line 197 "parser.y"
    { (yyval.statement) = new_statement(ctx, STAT_EXITSUB, 0); CHECK_ERROR; }
    break;

  case 33:

/* Line 1806 of yacc.c  */
#line 199 "parser.y"
    { (yyvsp[(2) - (5)].member)->args = (yyvsp[(3) - (5)].expression); (yyval.statement) = new_set_statement(ctx, (yyvsp[(2) - (5)].member), (yyvsp[(5) - (5)].expression)); CHECK_ERROR; }
    break;

  case 34:

/* Line 1806 of yacc.c  */
#line 200 "parser.y"
    { (yyval.statement) = new_statement(ctx, STAT_STOP, 0); CHECK_ERROR; }
    break;

  case 35:

/* Line 1806 of yacc.c  */
#line 201 "parser.y"
    { (yyval.statement) = new_onerror_statement(ctx, TRUE); CHECK_ERROR; }
    break;

  case 36:

/* Line 1806 of yacc.c  */
#line 202 "parser.y"
    { (yyval.statement) = new_onerror_statement(ctx, FALSE); CHECK_ERROR; }
    break;

  case 37:

/* Line 1806 of yacc.c  */
#line 203 "parser.y"
    { (yyval.statement) = new_const_statement(ctx, (yyvsp[(2) - (2)].const_decl)); CHECK_ERROR; }
    break;

  case 38:

/* Line 1806 of yacc.c  */
#line 205 "parser.y"
    { (yyval.statement) = new_forto_statement(ctx, (yyvsp[(2) - (10)].string), (yyvsp[(4) - (10)].expression), (yyvsp[(6) - (10)].expression), (yyvsp[(7) - (10)].expression), (yyvsp[(9) - (10)].statement)); CHECK_ERROR; }
    break;

  case 39:

/* Line 1806 of yacc.c  */
#line 207 "parser.y"
    { (yyval.statement) = new_foreach_statement(ctx, (yyvsp[(3) - (8)].string), (yyvsp[(5) - (8)].expression), (yyvsp[(7) - (8)].statement)); }
    break;

  case 40:

/* Line 1806 of yacc.c  */
#line 209 "parser.y"
    { (yyval.statement) = new_select_statement(ctx, (yyvsp[(3) - (7)].expression), (yyvsp[(5) - (7)].case_clausule)); }
    break;

  case 41:

/* Line 1806 of yacc.c  */
#line 212 "parser.y"
    { (yyval.member) = new_member_expression(ctx, NULL, (yyvsp[(1) - (1)].string)); CHECK_ERROR; }
    break;

  case 42:

/* Line 1806 of yacc.c  */
#line 213 "parser.y"
    { (yyval.member) = new_member_expression(ctx, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].string)); CHECK_ERROR; }
    break;

  case 43:

/* Line 1806 of yacc.c  */
#line 216 "parser.y"
    { (yyval.dim_decl) = (yyvsp[(1) - (1)].dim_decl); }
    break;

  case 44:

/* Line 1806 of yacc.c  */
#line 217 "parser.y"
    { (yyvsp[(1) - (3)].dim_decl)->next = (yyvsp[(3) - (3)].dim_decl); (yyval.dim_decl) = (yyvsp[(1) - (3)].dim_decl); }
    break;

  case 45:

/* Line 1806 of yacc.c  */
#line 220 "parser.y"
    { (yyval.dim_decl) = new_dim_decl(ctx, (yyvsp[(1) - (1)].string), FALSE, NULL); CHECK_ERROR; }
    break;

  case 46:

/* Line 1806 of yacc.c  */
#line 221 "parser.y"
    { (yyval.dim_decl) = new_dim_decl(ctx, (yyvsp[(1) - (4)].string), TRUE, (yyvsp[(3) - (4)].dim_list)); CHECK_ERROR; }
    break;

  case 47:

/* Line 1806 of yacc.c  */
#line 222 "parser.y"
    { (yyval.dim_decl) = new_dim_decl(ctx, (yyvsp[(1) - (2)].string), TRUE, NULL); CHECK_ERROR; }
    break;

  case 48:

/* Line 1806 of yacc.c  */
#line 225 "parser.y"
    { (yyval.dim_list) = new_dim(ctx, (yyvsp[(1) - (1)].uint), NULL); }
    break;

  case 49:

/* Line 1806 of yacc.c  */
#line 226 "parser.y"
    { (yyval.dim_list) = new_dim(ctx, (yyvsp[(1) - (3)].uint), (yyvsp[(3) - (3)].dim_list)); }
    break;

  case 50:

/* Line 1806 of yacc.c  */
#line 229 "parser.y"
    { (yyval.const_decl) = (yyvsp[(1) - (1)].const_decl); }
    break;

  case 51:

/* Line 1806 of yacc.c  */
#line 230 "parser.y"
    { (yyvsp[(1) - (3)].const_decl)->next = (yyvsp[(3) - (3)].const_decl); (yyval.const_decl) = (yyvsp[(1) - (3)].const_decl); }
    break;

  case 52:

/* Line 1806 of yacc.c  */
#line 233 "parser.y"
    { (yyval.const_decl) = new_const_decl(ctx, (yyvsp[(1) - (3)].string), (yyvsp[(3) - (3)].expression)); CHECK_ERROR; }
    break;

  case 53:

/* Line 1806 of yacc.c  */
#line 236 "parser.y"
    { (yyval.expression) = (yyvsp[(1) - (1)].expression); }
    break;

  case 54:

/* Line 1806 of yacc.c  */
#line 237 "parser.y"
    { (yyval.expression) = new_unary_expression(ctx, EXPR_NEG, (yyvsp[(2) - (2)].expression)); CHECK_ERROR; }
    break;

  case 55:

/* Line 1806 of yacc.c  */
#line 240 "parser.y"
    { (yyval.bool) = TRUE; }
    break;

  case 56:

/* Line 1806 of yacc.c  */
#line 241 "parser.y"
    { (yyval.bool) = FALSE; }
    break;

  case 57:

/* Line 1806 of yacc.c  */
#line 244 "parser.y"
    { (yyval.expression) = NULL;}
    break;

  case 58:

/* Line 1806 of yacc.c  */
#line 245 "parser.y"
    { (yyval.expression) = (yyvsp[(2) - (2)].expression); }
    break;

  case 59:

/* Line 1806 of yacc.c  */
#line 249 "parser.y"
    { (yyval.statement) = new_if_statement(ctx, (yyvsp[(2) - (9)].expression), (yyvsp[(5) - (9)].statement), (yyvsp[(6) - (9)].elseif), (yyvsp[(7) - (9)].statement)); CHECK_ERROR; }
    break;

  case 60:

/* Line 1806 of yacc.c  */
#line 250 "parser.y"
    { (yyval.statement) = new_if_statement(ctx, (yyvsp[(2) - (4)].expression), (yyvsp[(4) - (4)].statement), NULL, NULL); CHECK_ERROR; }
    break;

  case 61:

/* Line 1806 of yacc.c  */
#line 252 "parser.y"
    { (yyval.statement) = new_if_statement(ctx, (yyvsp[(2) - (7)].expression), (yyvsp[(4) - (7)].statement), NULL, (yyvsp[(6) - (7)].statement)); CHECK_ERROR; }
    break;

  case 64:

/* Line 1806 of yacc.c  */
#line 259 "parser.y"
    { (yyval.elseif) = NULL; }
    break;

  case 65:

/* Line 1806 of yacc.c  */
#line 260 "parser.y"
    { (yyval.elseif) = (yyvsp[(1) - (1)].elseif); }
    break;

  case 66:

/* Line 1806 of yacc.c  */
#line 263 "parser.y"
    { (yyval.elseif) = (yyvsp[(1) - (1)].elseif); }
    break;

  case 67:

/* Line 1806 of yacc.c  */
#line 264 "parser.y"
    { (yyvsp[(1) - (2)].elseif)->next = (yyvsp[(2) - (2)].elseif); (yyval.elseif) = (yyvsp[(1) - (2)].elseif); }
    break;

  case 68:

/* Line 1806 of yacc.c  */
#line 268 "parser.y"
    { (yyval.elseif) = new_elseif_decl(ctx, (yyvsp[(2) - (5)].expression), (yyvsp[(5) - (5)].statement)); }
    break;

  case 69:

/* Line 1806 of yacc.c  */
#line 271 "parser.y"
    { (yyval.statement) = NULL; }
    break;

  case 70:

/* Line 1806 of yacc.c  */
#line 272 "parser.y"
    { (yyval.statement) = (yyvsp[(3) - (3)].statement); }
    break;

  case 71:

/* Line 1806 of yacc.c  */
#line 275 "parser.y"
    { (yyval.case_clausule) = NULL; }
    break;

  case 72:

/* Line 1806 of yacc.c  */
#line 276 "parser.y"
    { (yyval.case_clausule) = new_case_clausule(ctx, NULL, (yyvsp[(4) - (4)].statement), NULL); }
    break;

  case 73:

/* Line 1806 of yacc.c  */
#line 278 "parser.y"
    { (yyval.case_clausule) = new_case_clausule(ctx, (yyvsp[(2) - (5)].expression), (yyvsp[(4) - (5)].statement), (yyvsp[(5) - (5)].case_clausule)); }
    break;

  case 74:

/* Line 1806 of yacc.c  */
#line 281 "parser.y"
    { (yyval.expression) = NULL; }
    break;

  case 75:

/* Line 1806 of yacc.c  */
#line 282 "parser.y"
    { (yyval.expression) = (yyvsp[(2) - (3)].expression); }
    break;

  case 76:

/* Line 1806 of yacc.c  */
#line 285 "parser.y"
    { (yyval.expression) = NULL; }
    break;

  case 77:

/* Line 1806 of yacc.c  */
#line 286 "parser.y"
    { (yyval.expression) = (yyvsp[(1) - (1)].expression); }
    break;

  case 80:

/* Line 1806 of yacc.c  */
#line 293 "parser.y"
    { (yyval.expression) = (yyvsp[(1) - (1)].expression); }
    break;

  case 81:

/* Line 1806 of yacc.c  */
#line 294 "parser.y"
    { (yyvsp[(1) - (3)].expression)->next = (yyvsp[(3) - (3)].expression); (yyval.expression) = (yyvsp[(1) - (3)].expression); }
    break;

  case 82:

/* Line 1806 of yacc.c  */
#line 297 "parser.y"
    { (yyval.expression) = (yyvsp[(1) - (1)].expression); }
    break;

  case 83:

/* Line 1806 of yacc.c  */
#line 298 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_IMP, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression)); CHECK_ERROR; }
    break;

  case 84:

/* Line 1806 of yacc.c  */
#line 301 "parser.y"
    { (yyval.expression) = (yyvsp[(1) - (1)].expression); }
    break;

  case 85:

/* Line 1806 of yacc.c  */
#line 302 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_EQV, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression)); CHECK_ERROR; }
    break;

  case 86:

/* Line 1806 of yacc.c  */
#line 305 "parser.y"
    { (yyval.expression) = (yyvsp[(1) - (1)].expression); }
    break;

  case 87:

/* Line 1806 of yacc.c  */
#line 306 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_XOR, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression)); CHECK_ERROR; }
    break;

  case 88:

/* Line 1806 of yacc.c  */
#line 309 "parser.y"
    { (yyval.expression) = (yyvsp[(1) - (1)].expression); }
    break;

  case 89:

/* Line 1806 of yacc.c  */
#line 310 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_OR, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression)); CHECK_ERROR; }
    break;

  case 90:

/* Line 1806 of yacc.c  */
#line 313 "parser.y"
    { (yyval.expression) = (yyvsp[(1) - (1)].expression); }
    break;

  case 91:

/* Line 1806 of yacc.c  */
#line 314 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_AND, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression)); CHECK_ERROR; }
    break;

  case 92:

/* Line 1806 of yacc.c  */
#line 317 "parser.y"
    { (yyval.expression) = (yyvsp[(1) - (1)].expression); }
    break;

  case 93:

/* Line 1806 of yacc.c  */
#line 318 "parser.y"
    { (yyval.expression) = new_unary_expression(ctx, EXPR_NOT, (yyvsp[(2) - (2)].expression)); CHECK_ERROR; }
    break;

  case 94:

/* Line 1806 of yacc.c  */
#line 321 "parser.y"
    { (yyval.expression) = (yyvsp[(1) - (1)].expression); }
    break;

  case 95:

/* Line 1806 of yacc.c  */
#line 322 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_EQUAL, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression)); CHECK_ERROR; }
    break;

  case 96:

/* Line 1806 of yacc.c  */
#line 323 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_NEQUAL, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression)); CHECK_ERROR; }
    break;

  case 97:

/* Line 1806 of yacc.c  */
#line 324 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_GT, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression)); CHECK_ERROR; }
    break;

  case 98:

/* Line 1806 of yacc.c  */
#line 325 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_LT, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression)); CHECK_ERROR; }
    break;

  case 99:

/* Line 1806 of yacc.c  */
#line 326 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_GTEQ, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression)); CHECK_ERROR; }
    break;

  case 100:

/* Line 1806 of yacc.c  */
#line 327 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_LTEQ, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression)); CHECK_ERROR; }
    break;

  case 101:

/* Line 1806 of yacc.c  */
#line 328 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_IS, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression)); CHECK_ERROR; }
    break;

  case 102:

/* Line 1806 of yacc.c  */
#line 331 "parser.y"
    { (yyval.expression) = (yyvsp[(1) - (1)].expression); }
    break;

  case 103:

/* Line 1806 of yacc.c  */
#line 332 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_CONCAT, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression)); CHECK_ERROR; }
    break;

  case 104:

/* Line 1806 of yacc.c  */
#line 335 "parser.y"
    { (yyval.expression) = (yyvsp[(1) - (1)].expression); }
    break;

  case 105:

/* Line 1806 of yacc.c  */
#line 336 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_ADD, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression)); CHECK_ERROR; }
    break;

  case 106:

/* Line 1806 of yacc.c  */
#line 337 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_SUB, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression)); CHECK_ERROR; }
    break;

  case 107:

/* Line 1806 of yacc.c  */
#line 340 "parser.y"
    { (yyval.expression) = (yyvsp[(1) - (1)].expression); }
    break;

  case 108:

/* Line 1806 of yacc.c  */
#line 341 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_MOD, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression)); CHECK_ERROR; }
    break;

  case 109:

/* Line 1806 of yacc.c  */
#line 344 "parser.y"
    { (yyval.expression) = (yyvsp[(1) - (1)].expression); }
    break;

  case 110:

/* Line 1806 of yacc.c  */
#line 346 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_IDIV, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression)); CHECK_ERROR; }
    break;

  case 111:

/* Line 1806 of yacc.c  */
#line 349 "parser.y"
    { (yyval.expression) = (yyvsp[(1) - (1)].expression); }
    break;

  case 112:

/* Line 1806 of yacc.c  */
#line 351 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_MUL, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression)); CHECK_ERROR; }
    break;

  case 113:

/* Line 1806 of yacc.c  */
#line 353 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_DIV, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression)); CHECK_ERROR; }
    break;

  case 114:

/* Line 1806 of yacc.c  */
#line 356 "parser.y"
    { (yyval.expression) = (yyvsp[(1) - (1)].expression); }
    break;

  case 115:

/* Line 1806 of yacc.c  */
#line 357 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_EXP, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression)); CHECK_ERROR; }
    break;

  case 116:

/* Line 1806 of yacc.c  */
#line 360 "parser.y"
    { (yyval.expression) = (yyvsp[(1) - (1)].expression); }
    break;

  case 117:

/* Line 1806 of yacc.c  */
#line 361 "parser.y"
    { (yyval.expression) = (yyvsp[(1) - (1)].expression); }
    break;

  case 118:

/* Line 1806 of yacc.c  */
#line 362 "parser.y"
    { (yyval.expression) = new_new_expression(ctx, (yyvsp[(2) - (2)].string)); CHECK_ERROR; }
    break;

  case 119:

/* Line 1806 of yacc.c  */
#line 363 "parser.y"
    { (yyval.expression) = new_unary_expression(ctx, EXPR_NEG, (yyvsp[(2) - (2)].expression)); CHECK_ERROR; }
    break;

  case 120:

/* Line 1806 of yacc.c  */
#line 366 "parser.y"
    { (yyval.expression) = (yyvsp[(1) - (1)].expression); }
    break;

  case 121:

/* Line 1806 of yacc.c  */
#line 367 "parser.y"
    { (yyvsp[(1) - (2)].member)->args = (yyvsp[(2) - (2)].expression); (yyval.expression) = &(yyvsp[(1) - (2)].member)->expr; }
    break;

  case 122:

/* Line 1806 of yacc.c  */
#line 370 "parser.y"
    { (yyval.expression) = new_bool_expression(ctx, VARIANT_TRUE); CHECK_ERROR; }
    break;

  case 123:

/* Line 1806 of yacc.c  */
#line 371 "parser.y"
    { (yyval.expression) = new_bool_expression(ctx, VARIANT_FALSE); CHECK_ERROR; }
    break;

  case 124:

/* Line 1806 of yacc.c  */
#line 372 "parser.y"
    { (yyval.expression) = new_string_expression(ctx, (yyvsp[(1) - (1)].string)); CHECK_ERROR; }
    break;

  case 125:

/* Line 1806 of yacc.c  */
#line 373 "parser.y"
    { (yyval.expression) = (yyvsp[(1) - (1)].expression); }
    break;

  case 126:

/* Line 1806 of yacc.c  */
#line 374 "parser.y"
    { (yyval.expression) = new_expression(ctx, EXPR_EMPTY, 0); CHECK_ERROR; }
    break;

  case 127:

/* Line 1806 of yacc.c  */
#line 375 "parser.y"
    { (yyval.expression) = new_expression(ctx, EXPR_NULL, 0); CHECK_ERROR; }
    break;

  case 128:

/* Line 1806 of yacc.c  */
#line 376 "parser.y"
    { (yyval.expression) = new_expression(ctx, EXPR_NOTHING, 0); CHECK_ERROR; }
    break;

  case 129:

/* Line 1806 of yacc.c  */
#line 379 "parser.y"
    { (yyval.expression) = new_long_expression(ctx, EXPR_USHORT, (yyvsp[(1) - (1)].lng)); CHECK_ERROR; }
    break;

  case 130:

/* Line 1806 of yacc.c  */
#line 380 "parser.y"
    { (yyval.expression) = new_long_expression(ctx, EXPR_USHORT, 0); CHECK_ERROR; }
    break;

  case 131:

/* Line 1806 of yacc.c  */
#line 381 "parser.y"
    { (yyval.expression) = new_long_expression(ctx, EXPR_ULONG, (yyvsp[(1) - (1)].lng)); CHECK_ERROR; }
    break;

  case 132:

/* Line 1806 of yacc.c  */
#line 382 "parser.y"
    { (yyval.expression) = new_double_expression(ctx, (yyvsp[(1) - (1)].dbl)); CHECK_ERROR; }
    break;

  case 133:

/* Line 1806 of yacc.c  */
#line 385 "parser.y"
    { (yyval.uint) = (yyvsp[(1) - (1)].lng); }
    break;

  case 134:

/* Line 1806 of yacc.c  */
#line 386 "parser.y"
    { (yyval.uint) = 0; }
    break;

  case 135:

/* Line 1806 of yacc.c  */
#line 387 "parser.y"
    { (yyval.uint) = (yyvsp[(1) - (1)].lng); }
    break;

  case 136:

/* Line 1806 of yacc.c  */
#line 390 "parser.y"
    { (yyval.expression) = new_unary_expression(ctx, EXPR_BRACKETS, (yyvsp[(2) - (3)].expression)); }
    break;

  case 137:

/* Line 1806 of yacc.c  */
#line 391 "parser.y"
    { (yyval.expression) = new_expression(ctx, EXPR_ME, 0); CHECK_ERROR; }
    break;

  case 138:

/* Line 1806 of yacc.c  */
#line 394 "parser.y"
    { (yyvsp[(4) - (7)].class_decl)->name = (yyvsp[(2) - (7)].string); (yyval.class_decl) = (yyvsp[(4) - (7)].class_decl); }
    break;

  case 139:

/* Line 1806 of yacc.c  */
#line 397 "parser.y"
    { (yyval.class_decl) = new_class_decl(ctx); }
    break;

  case 140:

/* Line 1806 of yacc.c  */
#line 398 "parser.y"
    { (yyval.class_decl) = add_class_function(ctx, (yyvsp[(3) - (3)].class_decl), (yyvsp[(1) - (3)].func_decl)); CHECK_ERROR; }
    break;

  case 141:

/* Line 1806 of yacc.c  */
#line 400 "parser.y"
    { dim_decl_t *dim_decl = new_dim_decl(ctx, (yyvsp[(2) - (4)].string), FALSE, NULL); CHECK_ERROR;
                                                  (yyval.class_decl) = add_dim_prop(ctx, (yyvsp[(4) - (4)].class_decl), dim_decl, (yyvsp[(1) - (4)].uint)); CHECK_ERROR; }
    break;

  case 142:

/* Line 1806 of yacc.c  */
#line 402 "parser.y"
    { (yyval.class_decl) = add_dim_prop(ctx, (yyvsp[(4) - (4)].class_decl), (yyvsp[(2) - (4)].dim_decl), 0); CHECK_ERROR; }
    break;

  case 143:

/* Line 1806 of yacc.c  */
#line 403 "parser.y"
    { (yyval.class_decl) = add_class_function(ctx, (yyvsp[(3) - (3)].class_decl), (yyvsp[(1) - (3)].func_decl)); CHECK_ERROR; }
    break;

  case 144:

/* Line 1806 of yacc.c  */
#line 407 "parser.y"
    { (yyval.func_decl) = new_function_decl(ctx, (yyvsp[(4) - (9)].string), FUNC_PROPGET, (yyvsp[(1) - (9)].uint), NULL, (yyvsp[(7) - (9)].statement)); CHECK_ERROR; }
    break;

  case 145:

/* Line 1806 of yacc.c  */
#line 409 "parser.y"
    { (yyval.func_decl) = new_function_decl(ctx, (yyvsp[(4) - (11)].string), FUNC_PROPLET, (yyvsp[(1) - (11)].uint), (yyvsp[(6) - (11)].arg_decl), (yyvsp[(9) - (11)].statement)); CHECK_ERROR; }
    break;

  case 146:

/* Line 1806 of yacc.c  */
#line 411 "parser.y"
    { (yyval.func_decl) = new_function_decl(ctx, (yyvsp[(4) - (11)].string), FUNC_PROPSET, (yyvsp[(1) - (11)].uint), (yyvsp[(6) - (11)].arg_decl), (yyvsp[(9) - (11)].statement)); CHECK_ERROR; }
    break;

  case 147:

/* Line 1806 of yacc.c  */
#line 415 "parser.y"
    { (yyval.func_decl) = new_function_decl(ctx, (yyvsp[(3) - (8)].string), FUNC_SUB, (yyvsp[(1) - (8)].uint), (yyvsp[(4) - (8)].arg_decl), (yyvsp[(6) - (8)].statement)); CHECK_ERROR; }
    break;

  case 148:

/* Line 1806 of yacc.c  */
#line 417 "parser.y"
    { (yyval.func_decl) = new_function_decl(ctx, (yyvsp[(3) - (8)].string), FUNC_FUNCTION, (yyvsp[(1) - (8)].uint), (yyvsp[(4) - (8)].arg_decl), (yyvsp[(6) - (8)].statement)); CHECK_ERROR; }
    break;

  case 149:

/* Line 1806 of yacc.c  */
#line 420 "parser.y"
    { (yyval.uint) = 0; }
    break;

  case 150:

/* Line 1806 of yacc.c  */
#line 421 "parser.y"
    { (yyval.uint) = (yyvsp[(1) - (1)].uint); }
    break;

  case 151:

/* Line 1806 of yacc.c  */
#line 424 "parser.y"
    { (yyval.uint) = STORAGE_IS_DEFAULT; }
    break;

  case 152:

/* Line 1806 of yacc.c  */
#line 425 "parser.y"
    { (yyval.uint) = 0; }
    break;

  case 153:

/* Line 1806 of yacc.c  */
#line 426 "parser.y"
    { (yyval.uint) = STORAGE_IS_PRIVATE; }
    break;

  case 154:

/* Line 1806 of yacc.c  */
#line 429 "parser.y"
    { (yyval.arg_decl) = NULL; }
    break;

  case 155:

/* Line 1806 of yacc.c  */
#line 430 "parser.y"
    { (yyval.arg_decl) = (yyvsp[(2) - (3)].arg_decl); }
    break;

  case 156:

/* Line 1806 of yacc.c  */
#line 433 "parser.y"
    { (yyval.arg_decl) = (yyvsp[(1) - (1)].arg_decl); }
    break;

  case 157:

/* Line 1806 of yacc.c  */
#line 434 "parser.y"
    { (yyvsp[(1) - (3)].arg_decl)->next = (yyvsp[(3) - (3)].arg_decl); (yyval.arg_decl) = (yyvsp[(1) - (3)].arg_decl); }
    break;

  case 158:

/* Line 1806 of yacc.c  */
#line 437 "parser.y"
    { (yyval.arg_decl) = new_argument_decl(ctx, (yyvsp[(1) - (2)].string), TRUE); }
    break;

  case 159:

/* Line 1806 of yacc.c  */
#line 438 "parser.y"
    { (yyval.arg_decl) = new_argument_decl(ctx, (yyvsp[(2) - (3)].string), TRUE); }
    break;

  case 160:

/* Line 1806 of yacc.c  */
#line 439 "parser.y"
    { (yyval.arg_decl) = new_argument_decl(ctx, (yyvsp[(2) - (3)].string), FALSE); }
    break;

  case 161:

/* Line 1806 of yacc.c  */
#line 443 "parser.y"
    { (yyval.string) = (yyvsp[(1) - (1)].string); }
    break;

  case 162:

/* Line 1806 of yacc.c  */
#line 444 "parser.y"
    { (yyval.string) = propertyW; }
    break;



/* Line 1806 of yacc.c  */
#line 3032 "parser.tab.c"
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
#line 451 "parser.y"


static int parser_error(parser_ctx_t *ctx, const char *str)
{
    return 0;
}

static void source_add_statement(parser_ctx_t *ctx, statement_t *stat)
{
    if(!stat)
        return;

    if(ctx->stats) {
        ctx->stats_tail->next = stat;
        ctx->stats_tail = stat;
    }else {
        ctx->stats = ctx->stats_tail = stat;
    }
}

static void source_add_class(parser_ctx_t *ctx, class_decl_t *class_decl)
{
    class_decl->next = ctx->class_decls;
    ctx->class_decls = class_decl;
}

static void parse_complete(parser_ctx_t *ctx, BOOL option_explicit)
{
    ctx->parse_complete = TRUE;
    ctx->option_explicit = option_explicit;
}

static void *new_expression(parser_ctx_t *ctx, expression_type_t type, size_t size)
{
    expression_t *expr;

    expr = parser_alloc(ctx, size ? size : sizeof(*expr));
    if(expr) {
        expr->type = type;
        expr->next = NULL;
    }

    return expr;
}

static expression_t *new_bool_expression(parser_ctx_t *ctx, VARIANT_BOOL value)
{
    bool_expression_t *expr;

    expr = new_expression(ctx, EXPR_BOOL, sizeof(*expr));
    if(!expr)
        return NULL;

    expr->value = value;
    return &expr->expr;
}

static expression_t *new_string_expression(parser_ctx_t *ctx, const WCHAR *value)
{
    string_expression_t *expr;

    expr = new_expression(ctx, EXPR_STRING, sizeof(*expr));
    if(!expr)
        return NULL;

    expr->value = value;
    return &expr->expr;
}

static expression_t *new_long_expression(parser_ctx_t *ctx, expression_type_t type, LONG value)
{
    int_expression_t *expr;

    expr = new_expression(ctx, type, sizeof(*expr));
    if(!expr)
        return NULL;

    expr->value = value;
    return &expr->expr;
}

static expression_t *new_double_expression(parser_ctx_t *ctx, double value)
{
    double_expression_t *expr;

    expr = new_expression(ctx, EXPR_DOUBLE, sizeof(*expr));
    if(!expr)
        return NULL;

    expr->value = value;
    return &expr->expr;
}

static expression_t *new_unary_expression(parser_ctx_t *ctx, expression_type_t type, expression_t *subexpr)
{
    unary_expression_t *expr;

    expr = new_expression(ctx, type, sizeof(*expr));
    if(!expr)
        return NULL;

    expr->subexpr = subexpr;
    return &expr->expr;
}

static expression_t *new_binary_expression(parser_ctx_t *ctx, expression_type_t type, expression_t *left, expression_t *right)
{
    binary_expression_t *expr;

    expr = new_expression(ctx, type, sizeof(*expr));
    if(!expr)
        return NULL;

    expr->left = left;
    expr->right = right;
    return &expr->expr;
}

static member_expression_t *new_member_expression(parser_ctx_t *ctx, expression_t *obj_expr, const WCHAR *identifier)
{
    member_expression_t *expr;

    expr = new_expression(ctx, EXPR_MEMBER, sizeof(*expr));
    if(!expr)
        return NULL;

    expr->obj_expr = obj_expr;
    expr->identifier = identifier;
    expr->args = NULL;
    return expr;
}

static expression_t *new_new_expression(parser_ctx_t *ctx, const WCHAR *identifier)
{
    string_expression_t *expr;

    expr = new_expression(ctx, EXPR_NEW, sizeof(*expr));
    if(!expr)
        return NULL;

    expr->value = identifier;
    return &expr->expr;
}

static void *new_statement(parser_ctx_t *ctx, statement_type_t type, size_t size)
{
    statement_t *stat;

    stat = parser_alloc(ctx, size ? size : sizeof(*stat));
    if(stat) {
        stat->type = type;
        stat->next = NULL;
    }

    return stat;
}

static statement_t *new_call_statement(parser_ctx_t *ctx, BOOL is_strict, member_expression_t *expr)
{
    call_statement_t *stat;

    stat = new_statement(ctx, STAT_CALL, sizeof(*stat));
    if(!stat)
        return NULL;

    stat->expr = expr;
    stat->is_strict = is_strict;
    return &stat->stat;
}

static statement_t *new_assign_statement(parser_ctx_t *ctx, member_expression_t *left, expression_t *right)
{
    assign_statement_t *stat;

    stat = new_statement(ctx, STAT_ASSIGN, sizeof(*stat));
    if(!stat)
        return NULL;

    stat->member_expr = left;
    stat->value_expr = right;
    return &stat->stat;
}

static statement_t *new_set_statement(parser_ctx_t *ctx, member_expression_t *left, expression_t *right)
{
    assign_statement_t *stat;

    stat = new_statement(ctx, STAT_SET, sizeof(*stat));
    if(!stat)
        return NULL;

    stat->member_expr = left;
    stat->value_expr = right;
    return &stat->stat;
}

static dim_decl_t *new_dim_decl(parser_ctx_t *ctx, const WCHAR *name, BOOL is_array, dim_list_t *dims)
{
    dim_decl_t *decl;

    decl = parser_alloc(ctx, sizeof(*decl));
    if(!decl)
        return NULL;

    decl->name = name;
    decl->is_array = is_array;
    decl->dims = dims;
    decl->next = NULL;
    return decl;
}

static dim_list_t *new_dim(parser_ctx_t *ctx, unsigned val, dim_list_t *next)
{
    dim_list_t *ret;

    ret = parser_alloc(ctx, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->val = val;
    ret->next = next;
    return ret;
}

static statement_t *new_dim_statement(parser_ctx_t *ctx, dim_decl_t *decls)
{
    dim_statement_t *stat;

    stat = new_statement(ctx, STAT_DIM, sizeof(*stat));
    if(!stat)
        return NULL;

    stat->dim_decls = decls;
    return &stat->stat;
}

static elseif_decl_t *new_elseif_decl(parser_ctx_t *ctx, expression_t *expr, statement_t *stat)
{
    elseif_decl_t *decl;

    decl = parser_alloc(ctx, sizeof(*decl));
    if(!decl)
        return NULL;

    decl->expr = expr;
    decl->stat = stat;
    decl->next = NULL;
    return decl;
}

static statement_t *new_while_statement(parser_ctx_t *ctx, statement_type_t type, expression_t *expr, statement_t *body)
{
    while_statement_t *stat;

    stat = new_statement(ctx, type, sizeof(*stat));
    if(!stat)
        return NULL;

    stat->expr = expr;
    stat->body = body;
    return &stat->stat;
}

static statement_t *new_forto_statement(parser_ctx_t *ctx, const WCHAR *identifier, expression_t *from_expr,
        expression_t *to_expr, expression_t *step_expr, statement_t *body)
{
    forto_statement_t *stat;

    stat = new_statement(ctx, STAT_FORTO, sizeof(*stat));
    if(!stat)
        return NULL;

    stat->identifier = identifier;
    stat->from_expr = from_expr;
    stat->to_expr = to_expr;
    stat->step_expr = step_expr;
    stat->body = body;
    return &stat->stat;
}

static statement_t *new_foreach_statement(parser_ctx_t *ctx, const WCHAR *identifier, expression_t *group_expr,
        statement_t *body)
{
    foreach_statement_t *stat;

    stat = new_statement(ctx, STAT_FOREACH, sizeof(*stat));
    if(!stat)
        return NULL;

    stat->identifier = identifier;
    stat->group_expr = group_expr;
    stat->body = body;
    return &stat->stat;
}

static statement_t *new_if_statement(parser_ctx_t *ctx, expression_t *expr, statement_t *if_stat, elseif_decl_t *elseif_decl,
        statement_t *else_stat)
{
    if_statement_t *stat;

    stat = new_statement(ctx, STAT_IF, sizeof(*stat));
    if(!stat)
        return NULL;

    stat->expr = expr;
    stat->if_stat = if_stat;
    stat->elseifs = elseif_decl;
    stat->else_stat = else_stat;
    return &stat->stat;
}

static statement_t *new_select_statement(parser_ctx_t *ctx, expression_t *expr, case_clausule_t *case_clausules)
{
    select_statement_t *stat;

    stat = new_statement(ctx, STAT_SELECT, sizeof(*stat));
    if(!stat)
        return NULL;

    stat->expr = expr;
    stat->case_clausules = case_clausules;
    return &stat->stat;
}

static case_clausule_t *new_case_clausule(parser_ctx_t *ctx, expression_t *expr, statement_t *stat, case_clausule_t *next)
{
    case_clausule_t *ret;

    ret = parser_alloc(ctx, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->expr = expr;
    ret->stat = stat;
    ret->next = next;
    return ret;
}

static statement_t *new_onerror_statement(parser_ctx_t *ctx, BOOL resume_next)
{
    onerror_statement_t *stat;

    stat = new_statement(ctx, STAT_ONERROR, sizeof(*stat));
    if(!stat)
        return NULL;

    stat->resume_next = resume_next;
    return &stat->stat;
}

static arg_decl_t *new_argument_decl(parser_ctx_t *ctx, const WCHAR *name, BOOL by_ref)
{
    arg_decl_t *arg_decl;

    arg_decl = parser_alloc(ctx, sizeof(*arg_decl));
    if(!arg_decl)
        return NULL;

    arg_decl->name = name;
    arg_decl->by_ref = by_ref;
    arg_decl->next = NULL;
    return arg_decl;
}

static function_decl_t *new_function_decl(parser_ctx_t *ctx, const WCHAR *name, function_type_t type,
        unsigned storage_flags, arg_decl_t *arg_decl, statement_t *body)
{
    function_decl_t *decl;

    if(storage_flags & STORAGE_IS_DEFAULT) {
        if(type == FUNC_PROPGET) {
            type = FUNC_DEFGET;
        }else {
            FIXME("Invalid default property\n");
            ctx->hres = E_FAIL;
            return NULL;
        }
    }

    decl = parser_alloc(ctx, sizeof(*decl));
    if(!decl)
        return NULL;

    decl->name = name;
    decl->type = type;
    decl->is_public = !(storage_flags & STORAGE_IS_PRIVATE);
    decl->args = arg_decl;
    decl->body = body;
    decl->next = NULL;
    decl->next_prop_func = NULL;
    return decl;
}

static statement_t *new_function_statement(parser_ctx_t *ctx, function_decl_t *decl)
{
    function_statement_t *stat;

    stat = new_statement(ctx, STAT_FUNC, sizeof(*stat));
    if(!stat)
        return NULL;

    stat->func_decl = decl;
    return &stat->stat;
}

static class_decl_t *new_class_decl(parser_ctx_t *ctx)
{
    class_decl_t *class_decl;

    class_decl = parser_alloc(ctx, sizeof(*class_decl));
    if(!class_decl)
        return NULL;

    class_decl->funcs = NULL;
    class_decl->props = NULL;
    class_decl->next = NULL;
    return class_decl;
}

static class_decl_t *add_class_function(parser_ctx_t *ctx, class_decl_t *class_decl, function_decl_t *decl)
{
    function_decl_t *iter;

    for(iter = class_decl->funcs; iter; iter = iter->next) {
        if(!strcmpiW(iter->name, decl->name)) {
            if(decl->type == FUNC_SUB || decl->type == FUNC_FUNCTION) {
                FIXME("Redefinition of %s::%s\n", debugstr_w(class_decl->name), debugstr_w(decl->name));
                ctx->hres = E_FAIL;
                return NULL;
            }

            while(1) {
                if(iter->type == decl->type) {
                    FIXME("Redefinition of %s::%s\n", debugstr_w(class_decl->name), debugstr_w(decl->name));
                    ctx->hres = E_FAIL;
                    return NULL;
                }
                if(!iter->next_prop_func)
                    break;
                iter = iter->next_prop_func;
            }

            iter->next_prop_func = decl;
            return class_decl;
        }
    }

    decl->next = class_decl->funcs;
    class_decl->funcs = decl;
    return class_decl;
}

static class_decl_t *add_dim_prop(parser_ctx_t *ctx, class_decl_t *class_decl, dim_decl_t *dim_decl, unsigned storage_flags)
{
    if(storage_flags & STORAGE_IS_DEFAULT) {
        FIXME("variant prop van't be default value\n");
        ctx->hres = E_FAIL;
        return NULL;
    }

    dim_decl->is_public = !(storage_flags & STORAGE_IS_PRIVATE);
    dim_decl->next = class_decl->props;
    class_decl->props = dim_decl;
    return class_decl;
}

static const_decl_t *new_const_decl(parser_ctx_t *ctx, const WCHAR *name, expression_t *expr)
{
    const_decl_t *decl;

    decl = parser_alloc(ctx, sizeof(*decl));
    if(!decl)
        return NULL;

    decl->name = name;
    decl->value_expr = expr;
    decl->next = NULL;
    return decl;
}

static statement_t *new_const_statement(parser_ctx_t *ctx, const_decl_t *decls)
{
    const_statement_t *stat;

    stat = new_statement(ctx, STAT_CONST, sizeof(*stat));
    if(!stat)
        return NULL;

    stat->decls = decls;
    return &stat->stat;
}

static statement_t *link_statements(statement_t *head, statement_t *tail)
{
    statement_t *iter;

    for(iter = head; iter->next; iter = iter->next);
    iter->next = tail;

    return head;
}

void *parser_alloc(parser_ctx_t *ctx, size_t size)
{
    void *ret;

    ret = heap_pool_alloc(&ctx->heap, size);
    if(!ret)
        ctx->hres = E_OUTOFMEMORY;
    return ret;
}

HRESULT parse_script(parser_ctx_t *ctx, const WCHAR *code, const WCHAR *delimiter)
{
    const WCHAR html_delimiterW[] = {'<','/','s','c','r','i','p','t','>',0};

    ctx->code = ctx->ptr = code;
    ctx->end = ctx->code + strlenW(ctx->code);

    heap_pool_init(&ctx->heap);

    ctx->parse_complete = FALSE;
    ctx->hres = S_OK;

    ctx->last_token = tNL;
    ctx->last_nl = 0;
    ctx->stats = ctx->stats_tail = NULL;
    ctx->class_decls = NULL;
    ctx->option_explicit = FALSE;
    ctx->is_html = delimiter && !strcmpiW(delimiter, html_delimiterW);

    parser_parse(ctx);

    if(FAILED(ctx->hres))
        return ctx->hres;
    if(!ctx->parse_complete) {
        FIXME("parser failed around %s\n", debugstr_w(ctx->code+20 > ctx->ptr ? ctx->code : ctx->ptr-20));
        return E_FAIL;
    }

    return S_OK;
}

void parser_release(parser_ctx_t *ctx)
{
    heap_pool_free(&ctx->heap);
}

