/* A Bison parser, made by GNU Bison 3.4.1.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2019 Free Software Foundation,
   Inc.

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

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         parser_parse
#define yylex           parser_lex
#define yyerror         parser_error
#define yydebug         parser_debug
#define yynerrs         parser_nerrs


/* First part of user prologue.  */
#line 19 "parser.y"


#include "vbscript.h"
#include "parse.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vbscript);

static int parser_error(parser_ctx_t *,const char*);

static void parse_complete(parser_ctx_t*,BOOL);
static void handle_isexpression_script(parser_ctx_t *ctx, expression_t *expr);

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

#define STORAGE_IS_PRIVATE    1
#define STORAGE_IS_DEFAULT    2

#define CHECK_ERROR if(((parser_ctx_t*)ctx)->hres != S_OK) YYABORT


#line 139 "parser.tab.c"

# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
#ifndef YY_PARSER_E_REACTOSSYNC_GCC_DLL_WIN32_VBSCRIPT_PARSER_TAB_H_INCLUDED
# define YY_PARSER_E_REACTOSSYNC_GCC_DLL_WIN32_VBSCRIPT_PARSER_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int parser_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    tEXPRESSION = 258,
    tEOF = 259,
    tNL = 260,
    tEMPTYBRACKETS = 261,
    tLTEQ = 262,
    tGTEQ = 263,
    tNEQ = 264,
    tSTOP = 265,
    tME = 266,
    tREM = 267,
    tTRUE = 268,
    tFALSE = 269,
    tNOT = 270,
    tAND = 271,
    tOR = 272,
    tXOR = 273,
    tEQV = 274,
    tIMP = 275,
    tIS = 276,
    tMOD = 277,
    tCALL = 278,
    tDIM = 279,
    tSUB = 280,
    tFUNCTION = 281,
    tGET = 282,
    tLET = 283,
    tCONST = 284,
    tIF = 285,
    tELSE = 286,
    tELSEIF = 287,
    tEND = 288,
    tTHEN = 289,
    tEXIT = 290,
    tWHILE = 291,
    tWEND = 292,
    tDO = 293,
    tLOOP = 294,
    tUNTIL = 295,
    tFOR = 296,
    tTO = 297,
    tEACH = 298,
    tIN = 299,
    tSELECT = 300,
    tCASE = 301,
    tBYREF = 302,
    tBYVAL = 303,
    tOPTION = 304,
    tNOTHING = 305,
    tEMPTY = 306,
    tNULL = 307,
    tCLASS = 308,
    tSET = 309,
    tNEW = 310,
    tPUBLIC = 311,
    tPRIVATE = 312,
    tNEXT = 313,
    tON = 314,
    tRESUME = 315,
    tGOTO = 316,
    tIdentifier = 317,
    tString = 318,
    tDEFAULT = 319,
    tERROR = 320,
    tEXPLICIT = 321,
    tPROPERTY = 322,
    tSTEP = 323,
    tInt = 324,
    tDouble = 325
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 87 "parser.y"

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
    LONG integer;
    BOOL boolean;
    double dbl;

#line 272 "parser.tab.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int parser_parse (parser_ctx_t *ctx);

#endif /* !YY_PARSER_E_REACTOSSYNC_GCC_DLL_WIN32_VBSCRIPT_PARSER_TAB_H_INCLUDED  */



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
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
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
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
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
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  48
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1153

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  87
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  63
/* YYNRULES -- Number of rules.  */
#define YYNRULES  232
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  405

#define YYUNDEFTOK  2
#define YYMAXUTOK   325

/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                \
  ((unsigned) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
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
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   148,   148,   149,   152,   153,   156,   157,   158,   161,
     162,   165,   166,   167,   170,   171,   174,   175,   178,   181,
     182,   183,   184,   185,   188,   189,   190,   192,   193,   194,
     196,   199,   202,   203,   204,   205,   206,   207,   208,   209,
     211,   212,   213,   214,   215,   217,   219,   223,   224,   227,
     228,   231,   232,   233,   236,   237,   240,   241,   244,   247,
     248,   251,   252,   255,   256,   259,   261,   262,   266,   267,
     270,   271,   274,   275,   278,   282,   283,   286,   287,   288,
     292,   293,   296,   297,   300,   301,   302,   305,   306,   309,
     310,   313,   314,   317,   318,   321,   322,   325,   326,   329,
     330,   333,   334,   337,   338,   339,   340,   341,   342,   343,
     344,   347,   348,   351,   352,   353,   356,   357,   360,   361,
     365,   366,   368,   372,   373,   376,   377,   378,   379,   380,
     383,   384,   387,   388,   389,   390,   391,   392,   393,   396,
     397,   398,   401,   402,   405,   406,   409,   412,   413,   414,
     416,   418,   420,   421,   422,   423,   426,   428,   430,   434,
     436,   440,   441,   444,   445,   446,   449,   450,   453,   454,
     457,   458,   459,   463,   464,   465,   466,   467,   468,   472,
     473,   474,   475,   476,   477,   478,   479,   480,   481,   482,
     483,   484,   485,   486,   487,   488,   489,   490,   491,   492,
     493,   494,   495,   496,   497,   498,   499,   500,   501,   502,
     503,   504,   505,   506,   507,   508,   509,   510,   511,   512,
     513,   514,   515,   516,   517,   518,   519,   520,   521,   525,
     526,   527,   528
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "tEXPRESSION", "tEOF", "tNL",
  "tEMPTYBRACKETS", "tLTEQ", "tGTEQ", "tNEQ", "tSTOP", "tME", "tREM",
  "tTRUE", "tFALSE", "tNOT", "tAND", "tOR", "tXOR", "tEQV", "tIMP", "tIS",
  "tMOD", "tCALL", "tDIM", "tSUB", "tFUNCTION", "tGET", "tLET", "tCONST",
  "tIF", "tELSE", "tELSEIF", "tEND", "tTHEN", "tEXIT", "tWHILE", "tWEND",
  "tDO", "tLOOP", "tUNTIL", "tFOR", "tTO", "tEACH", "tIN", "tSELECT",
  "tCASE", "tBYREF", "tBYVAL", "tOPTION", "tNOTHING", "tEMPTY", "tNULL",
  "tCLASS", "tSET", "tNEW", "tPUBLIC", "tPRIVATE", "tNEXT", "tON",
  "tRESUME", "tGOTO", "tIdentifier", "tString", "tDEFAULT", "tERROR",
  "tEXPLICIT", "tPROPERTY", "tSTEP", "tInt", "tDouble", "':'", "'='",
  "'0'", "'.'", "','", "'('", "')'", "'-'", "'>'", "'<'", "'&'", "'+'",
  "'\\\\'", "'*'", "'/'", "'^'", "$accept", "Program",
  "OptionExplicit_opt", "SourceElements", "ExpressionNl_opt",
  "BodyStatements", "StatementsNl_opt", "StatementsNl", "StatementNl",
  "Statement", "SimpleStatement", "MemberExpression", "DimDeclList",
  "DimDecl", "DimList", "ConstDeclList", "ConstDecl", "ConstExpression",
  "DoType", "Step_opt", "IfStatement", "EndIf_opt", "ElseIfs_opt",
  "ElseIfs", "ElseIf", "Else_opt", "CaseClausules", "Arguments_opt",
  "ArgumentList_opt", "ArgumentList", "EmptyBrackets_opt",
  "ExpressionList", "Expression", "EqvExpression", "XorExpression",
  "OrExpression", "AndExpression", "NotExpression", "EqualityExpression",
  "ConcatExpression", "AdditiveExpression", "ModExpression",
  "IntdivExpression", "MultiplicativeExpression", "ExpExpression",
  "UnaryExpression", "CallExpression", "LiteralExpression",
  "NumericLiteralExpression", "IntegerValue", "PrimaryExpression",
  "ClassDeclaration", "ClassBody", "PropertyDecl", "FunctionDecl",
  "Storage_opt", "Storage", "ArgumentsDecl_opt", "ArgumentDeclList",
  "ArgumentDecl", "Identifier", "DotIdentifier", "StSep", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
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

#define YYPACT_NINF -294

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-294)))

#define YYTABLE_NINF -162

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      34,   617,   -19,    50,  -294,  -294,  -294,  -294,   617,  -294,
    -294,  -294,   166,  -294,  -294,  -294,  -294,  -294,  -294,  -294,
    -294,  -294,  -294,   617,   660,   660,    30,     2,    16,    43,
      96,    51,   111,  -294,    37,    53,   -17,   110,   100,    41,
      65,  -294,   119,  -294,  -294,  -294,  -294,    15,  -294,   466,
    -294,  -294,    19,  -294,  -294,  -294,  -294,   545,  -294,  -294,
    -294,   617,   617,   617,   617,   617,   660,   660,   660,   660,
     660,   660,   660,   660,   660,   660,   660,   660,   660,   660,
     660,  1085,    15,    15,  -294,  -294,  -294,   368,   166,   166,
     617,   117,   617,    20,   107,   163,   166,   368,   126,  -294,
     149,   721,  -294,   212,   152,   391,  -294,   119,  -294,  -294,
     131,  -294,  -294,   545,   147,    -8,    43,    96,    51,   111,
    -294,    53,    53,    53,    53,    53,    53,    53,   -17,   110,
     110,   100,    41,    65,    65,  -294,  -294,  -294,  -294,  -294,
    -294,  -294,  -294,  -294,  -294,  -294,  -294,  -294,  -294,  -294,
    -294,  -294,  -294,  -294,  -294,  -294,  -294,  -294,  -294,  -294,
    -294,  -294,  -294,  -294,  -294,  -294,  -294,  -294,  -294,  -294,
    -294,  -294,  -294,  -294,  -294,  -294,  -294,  -294,  -294,  -294,
    -294,  -294,  -294,  -294,  -294,  -294,  -294,  -294,  -294,     2,
    -294,   154,    11,  -294,   160,   164,    18,  -294,  -294,  -294,
    -294,  -294,     9,  -294,  -294,   617,   771,   166,   169,   617,
      15,     2,  -294,   144,  -294,  -294,   721,   545,   172,  -294,
    -294,   -32,   166,   166,  -294,  -294,   545,   171,   166,  -294,
      68,   166,   116,   516,   821,     9,   199,  -294,   721,   202,
     617,     9,    82,   175,   190,   176,  -294,     4,   617,    13,
      13,  -294,  -294,  -294,  -294,   173,   177,  -294,   -16,  -294,
    -294,   721,    66,   216,   771,   142,  -294,   617,   104,   211,
     166,   227,    15,    15,    85,   201,   617,  -294,  -294,   241,
      97,  -294,    15,    15,  -294,    68,  -294,   234,  1021,   237,
    -294,  -294,   230,   617,     9,   617,   588,   238,    15,   221,
      82,    82,    -5,    15,   241,   166,   166,   200,   203,   270,
     871,   871,  -294,   617,   249,  -294,   234,   248,  -294,  -294,
     241,   921,     7,    15,    15,    10,   239,    82,    15,  -294,
    -294,   224,   225,   226,    82,   270,   270,  -294,    97,  -294,
     250,   871,   212,   256,   113,   285,   258,  -294,  -294,   243,
     617,    15,  1021,   721,   617,  -294,  -294,  -294,    13,   228,
     229,  -294,  -294,  -294,  -294,   267,  -294,   271,   305,   971,
     283,  -294,   241,   921,  -294,   211,  -294,    15,    97,    97,
    -294,  -294,   721,  -294,  -294,   257,  -294,   871,   240,   242,
    -294,  -294,   281,    15,    15,   251,   871,   871,  -294,   287,
     288,   255,   261,  -294,  -294
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       4,     9,     0,     0,     6,   145,   132,   133,     0,   138,
     136,   137,     0,   173,   134,   174,   175,   176,   177,   178,
     140,   141,   139,     0,     0,     0,     0,    87,     0,    91,
      93,    95,    97,    99,   101,   103,   111,   113,   116,   118,
     120,   123,   126,   125,   135,   130,    47,     0,     1,   161,
     102,   127,     0,   128,   129,     3,    88,     0,   131,    80,
      10,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   229,   230,     5,     2,    40,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   164,   165,
       0,    19,     7,     0,    21,    87,    28,     0,     8,    33,
       0,   162,   144,     0,     0,    84,    92,    94,    96,    98,
     100,   109,   108,   105,   110,   104,   106,   107,   112,   115,
     114,   117,   119,   121,   122,   124,   180,   181,   182,   183,
     184,   185,   186,   187,   188,   189,   190,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,   201,   202,   203,
     204,   205,   206,   207,   208,   209,   210,   211,   212,   213,
     214,   215,   216,   217,   218,   219,   220,   221,   222,   223,
     224,   225,   226,   227,   228,   179,    48,   231,   232,    87,
      27,    49,    51,    43,    56,     0,     0,    38,    36,    34,
      35,    37,     0,    61,    62,     0,   161,     0,     0,     0,
       0,    87,   163,     0,    20,    18,    23,     0,   131,    24,
      83,    82,     0,     0,    86,    81,     0,    25,     0,    53,
       0,     0,     0,   161,   161,     0,     0,    15,    16,     0,
       0,     0,   161,   131,     0,     0,    22,     0,     0,    87,
      87,    85,    50,   143,   142,     0,    54,    57,     0,    58,
      59,    14,    68,     0,   161,    32,    17,     0,     0,    77,
       0,     0,   154,   148,     0,   162,     0,    41,    42,    26,
       0,   166,     0,     0,    52,     0,    60,    70,   161,     0,
      66,    29,     0,     0,     0,     0,     0,     0,   152,     0,
     161,   161,     0,   150,    39,     0,     0,     0,   168,    87,
     161,   161,    55,     0,    75,    71,    72,    68,    69,    30,
      31,   161,    63,     0,     0,    89,     0,   161,     0,   155,
     149,     0,     0,     0,   161,    87,    87,   167,     0,   170,
       0,   161,    12,     0,     0,     0,     0,    73,    67,     0,
       0,     0,   161,    14,     0,    46,   153,   146,    87,     0,
       0,   151,   171,   172,   169,     0,    13,     0,     0,   161,
       0,    45,    64,   161,    78,    77,    90,     0,     0,     0,
     159,   160,    14,    76,    65,     0,    79,   161,     0,     0,
      74,    44,     0,     0,     0,     0,   161,   161,   156,     0,
       0,     0,     0,   157,   158
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -294,  -294,  -294,  -294,  -294,  -293,  -233,  -231,   -38,   -85,
    -294,   -46,   101,    54,    40,    99,  -294,  -294,    69,  -294,
    -294,    23,  -294,    22,  -294,  -294,   -40,   -99,  -294,  -103,
     -96,   -11,     3,   276,   282,   284,   290,    27,  -294,   130,
     272,   102,   279,   269,   128,    39,    21,   120,    90,  -294,
    -294,  -294,  -268,  -294,  -229,  -227,  -199,  -245,    25,  -157,
     -12,  -294,    98
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     3,     4,    49,    26,   340,   236,   237,   238,   103,
     104,    27,   190,   191,   255,   193,   194,   259,   205,   351,
     106,   290,   314,   315,   316,   346,   297,    58,   219,   114,
      59,   324,   115,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,   256,
      45,   108,   271,   272,   109,   110,   111,   282,   307,   308,
      46,   186,    84
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      51,   263,   220,   105,    28,   283,   218,   266,    56,   221,
     224,   102,    61,   273,    82,   274,   214,   229,   343,    56,
      82,    60,   331,   332,    61,    82,    52,    61,   287,    61,
      61,   292,   329,   330,    55,    50,    61,     1,    61,    61,
     -80,   189,   -80,   275,    66,    67,    68,    47,   366,   333,
      48,   211,   233,    20,    21,   105,   203,    22,    69,   356,
     204,    74,    62,    53,    54,    75,   361,   226,    64,   185,
     107,   273,   273,   274,   274,   350,   192,   195,    57,   226,
      83,   112,   208,     2,   210,   354,    83,   230,   349,   280,
     227,    83,   120,   196,   392,   202,   112,   288,   273,   289,
     274,   275,   275,   399,   400,   273,   270,   274,   107,    70,
     222,   223,   243,   377,    63,  -147,    71,    72,   107,   135,
     375,   374,   107,   251,    61,    78,    79,    65,   275,     6,
       7,   246,    76,    61,    73,   275,   383,   253,    98,    99,
     385,   254,   197,   198,   305,   306,   295,   368,   262,   390,
     207,    80,   302,   281,   281,   199,   222,   223,   200,    13,
     105,    15,    16,    17,    18,    19,     9,    10,    11,    13,
     105,    15,    16,    17,    18,    19,   129,   130,   203,    14,
     187,   188,   204,    77,   201,    20,    21,   105,   105,    22,
     212,   206,   105,    81,   258,   239,   121,   122,   123,   124,
     125,   126,   127,   317,   244,   245,   133,   134,   235,   209,
     249,   250,   241,   339,   213,   105,   192,   215,   105,   195,
     247,   388,   389,   216,   225,   342,   342,   107,    13,   228,
      15,    16,    17,    18,    19,   231,   232,   107,   265,   362,
     363,   240,   105,   268,   248,  -131,   267,   276,   277,   278,
     284,   279,   285,   291,   107,   107,   342,   296,   192,   107,
     299,    61,   281,   303,   105,   105,   313,   318,   309,   319,
     294,   326,   341,   341,   328,   105,    56,   337,   338,   304,
     345,   289,   107,   365,   355,   107,   358,   359,   360,   367,
     369,   370,   380,   335,   336,   105,   320,   381,   322,   325,
     234,   371,   342,   341,   378,   379,   105,   105,   242,   107,
     382,   342,   342,   384,   395,   391,   344,   393,   398,   394,
     401,   402,   403,   105,   298,   312,   309,   105,   404,   252,
     257,   107,   107,   264,   293,   386,   105,   116,   347,   269,
     348,   105,   107,   376,   117,   128,   132,   118,   286,   341,
     105,   105,   260,   372,   119,   131,     0,   325,   341,   341,
       0,     0,   107,   364,     0,     0,   309,   309,     0,     0,
     300,   301,     0,   107,   107,     0,     0,     0,     0,     5,
     310,   311,     0,     0,     0,     0,     0,     0,     0,     0,
     107,     0,   321,     0,   107,     0,   327,    56,     0,     0,
       0,   334,     5,   107,     6,     7,     8,     0,   107,     0,
       0,     0,     0,     0,     0,     0,     0,   107,   107,     0,
       0,   352,   353,     0,     0,     0,   357,     0,     0,     0,
      13,     0,    15,    16,    17,    18,    19,     0,     0,     0,
       0,     9,    10,    11,    23,     0,    12,     0,     0,   373,
       0,     0,     0,    13,    14,    15,    16,    17,    18,    19,
      20,    21,     0,     0,    22,     0,   113,   217,     0,    24,
      85,     0,     0,    25,     0,   387,    86,     5,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    87,
      88,   396,   397,     0,     0,    89,    90,     0,     0,     0,
       0,    91,    92,     0,    93,     0,     0,    94,     0,     0,
       0,    95,     0,     0,     0,     0,     0,     0,     0,    96,
      97,   261,    98,    99,     0,   100,    86,     5,    13,     0,
      15,    16,    17,    18,    19,     0,     0,   101,     0,    87,
      88,     0,    23,     0,     0,    89,    90,     0,     0,     0,
       0,    91,    92,     0,    93,     0,     5,    94,     6,     7,
       8,    95,     0,     0,     0,     0,     0,     0,     0,     0,
      97,     0,    98,    99,     0,   100,     0,     0,    13,     0,
      15,    16,    17,    18,    19,     0,     0,   101,     0,     0,
       0,     0,    23,     0,     0,     9,    10,    11,     0,     5,
      12,     6,     7,     8,     0,     0,     0,    13,    14,    15,
      16,    17,    18,    19,    20,    21,     0,     0,    22,   323,
     113,    23,     0,    24,     0,     0,     0,    25,     5,     0,
       6,     7,     8,     0,     0,     0,     0,     0,     9,    10,
      11,     0,     0,    12,     0,     0,     0,     0,     0,     0,
      13,    14,    15,    16,    17,    18,    19,    20,    21,     0,
       0,    22,     0,     0,    23,     0,    24,     9,    10,    11,
      25,     5,    12,     6,     7,     0,     0,     0,     0,    13,
      14,    15,    16,    17,    18,    19,    20,    21,     0,     0,
      22,     0,     0,    23,     0,    24,     0,     0,     0,    25,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       9,    10,    11,     0,     0,    12,     0,     0,     0,     0,
       0,     0,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    86,     5,    22,     0,     0,    23,     0,    24,     0,
       0,     0,    25,     0,    87,    88,  -161,  -161,     0,     0,
      89,    90,     0,     0,     0,     0,    91,    92,     0,    93,
       0,     0,    94,     0,     0,     0,    95,     0,     0,     0,
       0,     0,     0,     0,     0,    97,     0,    98,    99,     0,
     100,    86,     5,    13,     0,    15,    16,    17,    18,    19,
       0,     0,   101,     0,    87,    88,     0,    23,     0,     0,
      89,    90,     0,     0,     0,     0,    91,    92,     0,    93,
     -14,     0,    94,     0,     0,     0,    95,     0,     0,     0,
       0,     0,     0,     0,     0,    97,     0,    98,    99,     0,
     100,    86,     5,    13,     0,    15,    16,    17,    18,    19,
       0,     0,   101,     0,    87,    88,     0,    23,     0,     0,
      89,    90,     0,     0,     0,     0,    91,    92,   -14,    93,
       0,     0,    94,     0,     0,     0,    95,     0,     0,     0,
       0,     0,     0,     0,     0,    97,     0,    98,    99,     0,
     100,    86,     5,    13,     0,    15,    16,    17,    18,    19,
       0,     0,   101,     0,    87,    88,     0,    23,     0,     0,
      89,    90,     0,     0,   -11,     0,    91,    92,     0,    93,
       0,     0,    94,     0,     0,     0,    95,     0,     0,     0,
       0,     0,     0,     0,     0,    97,     0,    98,    99,     0,
     100,    86,     5,    13,     0,    15,    16,    17,    18,    19,
       0,     0,   101,     0,    87,    88,     0,    23,     0,     0,
      89,    90,     0,     0,     0,     0,    91,    92,     0,    93,
       0,     0,    94,     0,     0,     0,    95,     0,     0,     0,
       0,     0,     0,     0,     0,    97,     0,    98,    99,   -14,
     100,    86,     5,    13,     0,    15,    16,    17,    18,    19,
       0,     0,   101,     0,    87,    88,     0,    23,     0,     0,
      89,    90,     0,     0,   -14,     0,    91,    92,     0,    93,
       0,     0,    94,     0,     0,     0,    95,     0,     0,     0,
       0,     0,     0,     0,     0,    97,     0,    98,    99,     0,
     100,    86,     5,    13,     0,    15,    16,    17,    18,    19,
       0,     0,   101,     0,    87,    88,     0,    23,     0,     0,
      89,    90,     0,     0,     0,     0,    91,    92,     0,    93,
       0,     0,    94,     0,     0,     0,    95,     0,     0,     0,
       0,     0,     0,     0,     0,    97,     0,    98,    99,     0,
     100,     0,     0,    13,     0,    15,    16,    17,    18,    19,
       0,     0,   101,     0,     0,     0,     0,    23,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   152,   153,   154,   155,   156,   157,
     158,   159,   160,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   176,   177,
     178,   179,   180,   181,   182,   183,   184,    13,     0,    15,
      16,    17,    18,    19
};

static const yytype_int16 yycheck[] =
{
      12,   234,   105,    49,     1,   250,   105,   238,     6,   105,
     113,    49,    20,   242,     5,   242,   101,     6,   311,     6,
       5,     5,    27,    28,    20,     5,    23,    20,   261,    20,
      20,   264,   300,   301,     4,     8,    20,     3,    20,    20,
      72,    87,    74,   242,     7,     8,     9,    66,   341,    54,
       0,    97,    34,    69,    70,   101,    36,    73,    21,   327,
      40,    78,    19,    24,    25,    82,   334,    75,    17,    81,
      49,   300,   301,   300,   301,    68,    88,    89,    76,    75,
      71,    77,    94,    49,    96,    75,    71,    76,   321,    76,
     189,    71,    65,    90,   387,    92,    77,    31,   327,    33,
     327,   300,   301,   396,   397,   334,    24,   334,    87,    72,
      25,    26,   211,   358,    18,    33,    79,    80,    97,    80,
     353,   352,   101,   226,    20,    84,    85,    16,   327,    13,
      14,   216,    22,    20,    81,   334,   369,    69,    56,    57,
     373,    73,    25,    26,    47,    48,    42,    34,   233,   382,
      43,    86,    67,   249,   250,    38,    25,    26,    41,    62,
     206,    64,    65,    66,    67,    68,    50,    51,    52,    62,
     216,    64,    65,    66,    67,    68,    74,    75,    36,    63,
      82,    83,    40,    83,    67,    69,    70,   233,   234,    73,
      64,    93,   238,    74,    78,   207,    66,    67,    68,    69,
      70,    71,    72,   288,    60,    61,    78,    79,   205,    46,
     222,   223,   209,   309,    65,   261,   228,     5,   264,   231,
     217,   378,   379,    71,    77,   310,   311,   206,    62,    75,
      64,    65,    66,    67,    68,    75,    72,   216,    39,   335,
     336,    72,   288,   240,    72,    74,    44,    72,    58,    73,
      77,   248,    75,    37,   233,   234,   341,    46,   270,   238,
      33,    20,   358,    62,   310,   311,    32,    30,   280,    39,
     267,    33,   310,   311,    53,   321,     6,    77,    75,   276,
      31,    33,   261,    33,    45,   264,    62,    62,    62,    33,
       5,    33,    25,   305,   306,   341,   293,    26,   295,   296,
     202,    58,   387,   341,    76,    76,   352,   353,   210,   288,
       5,   396,   397,    30,    33,    58,   313,    77,    67,    77,
      33,    33,    67,   369,   270,   285,   338,   373,    67,   228,
     231,   310,   311,   235,   265,   375,   382,    61,   316,   241,
     317,   387,   321,   354,    62,    73,    77,    63,   258,   387,
     396,   397,   232,   350,    64,    76,    -1,   354,   396,   397,
      -1,    -1,   341,   338,    -1,    -1,   378,   379,    -1,    -1,
     272,   273,    -1,   352,   353,    -1,    -1,    -1,    -1,    11,
     282,   283,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     369,    -1,   294,    -1,   373,    -1,   298,     6,    -1,    -1,
      -1,   303,    11,   382,    13,    14,    15,    -1,   387,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   396,   397,    -1,
      -1,   323,   324,    -1,    -1,    -1,   328,    -1,    -1,    -1,
      62,    -1,    64,    65,    66,    67,    68,    -1,    -1,    -1,
      -1,    50,    51,    52,    76,    -1,    55,    -1,    -1,   351,
      -1,    -1,    -1,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    -1,    -1,    73,    -1,    75,    76,    -1,    78,
       4,    -1,    -1,    82,    -1,   377,    10,    11,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    23,
      24,   393,   394,    -1,    -1,    29,    30,    -1,    -1,    -1,
      -1,    35,    36,    -1,    38,    -1,    -1,    41,    -1,    -1,
      -1,    45,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    53,
      54,     5,    56,    57,    -1,    59,    10,    11,    62,    -1,
      64,    65,    66,    67,    68,    -1,    -1,    71,    -1,    23,
      24,    -1,    76,    -1,    -1,    29,    30,    -1,    -1,    -1,
      -1,    35,    36,    -1,    38,    -1,    11,    41,    13,    14,
      15,    45,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      54,    -1,    56,    57,    -1,    59,    -1,    -1,    62,    -1,
      64,    65,    66,    67,    68,    -1,    -1,    71,    -1,    -1,
      -1,    -1,    76,    -1,    -1,    50,    51,    52,    -1,    11,
      55,    13,    14,    15,    -1,    -1,    -1,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    -1,    -1,    73,    31,
      75,    76,    -1,    78,    -1,    -1,    -1,    82,    11,    -1,
      13,    14,    15,    -1,    -1,    -1,    -1,    -1,    50,    51,
      52,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    -1,
      -1,    73,    -1,    -1,    76,    -1,    78,    50,    51,    52,
      82,    11,    55,    13,    14,    -1,    -1,    -1,    -1,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    -1,    -1,
      73,    -1,    -1,    76,    -1,    78,    -1,    -1,    -1,    82,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      50,    51,    52,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    10,    11,    73,    -1,    -1,    76,    -1,    78,    -1,
      -1,    -1,    82,    -1,    23,    24,    25,    26,    -1,    -1,
      29,    30,    -1,    -1,    -1,    -1,    35,    36,    -1,    38,
      -1,    -1,    41,    -1,    -1,    -1,    45,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    54,    -1,    56,    57,    -1,
      59,    10,    11,    62,    -1,    64,    65,    66,    67,    68,
      -1,    -1,    71,    -1,    23,    24,    -1,    76,    -1,    -1,
      29,    30,    -1,    -1,    -1,    -1,    35,    36,    -1,    38,
      39,    -1,    41,    -1,    -1,    -1,    45,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    54,    -1,    56,    57,    -1,
      59,    10,    11,    62,    -1,    64,    65,    66,    67,    68,
      -1,    -1,    71,    -1,    23,    24,    -1,    76,    -1,    -1,
      29,    30,    -1,    -1,    -1,    -1,    35,    36,    37,    38,
      -1,    -1,    41,    -1,    -1,    -1,    45,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    54,    -1,    56,    57,    -1,
      59,    10,    11,    62,    -1,    64,    65,    66,    67,    68,
      -1,    -1,    71,    -1,    23,    24,    -1,    76,    -1,    -1,
      29,    30,    -1,    -1,    33,    -1,    35,    36,    -1,    38,
      -1,    -1,    41,    -1,    -1,    -1,    45,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    54,    -1,    56,    57,    -1,
      59,    10,    11,    62,    -1,    64,    65,    66,    67,    68,
      -1,    -1,    71,    -1,    23,    24,    -1,    76,    -1,    -1,
      29,    30,    -1,    -1,    -1,    -1,    35,    36,    -1,    38,
      -1,    -1,    41,    -1,    -1,    -1,    45,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    54,    -1,    56,    57,    58,
      59,    10,    11,    62,    -1,    64,    65,    66,    67,    68,
      -1,    -1,    71,    -1,    23,    24,    -1,    76,    -1,    -1,
      29,    30,    -1,    -1,    33,    -1,    35,    36,    -1,    38,
      -1,    -1,    41,    -1,    -1,    -1,    45,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    54,    -1,    56,    57,    -1,
      59,    10,    11,    62,    -1,    64,    65,    66,    67,    68,
      -1,    -1,    71,    -1,    23,    24,    -1,    76,    -1,    -1,
      29,    30,    -1,    -1,    -1,    -1,    35,    36,    -1,    38,
      -1,    -1,    41,    -1,    -1,    -1,    45,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    54,    -1,    56,    57,    -1,
      59,    -1,    -1,    62,    -1,    64,    65,    66,    67,    68,
      -1,    -1,    71,    -1,    -1,    -1,    -1,    76,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    -1,    64,
      65,    66,    67,    68
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,    49,    88,    89,    11,    13,    14,    15,    50,
      51,    52,    55,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    73,    76,    78,    82,    91,    98,   119,   120,
     121,   122,   123,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   137,   147,    66,     0,    90,
     124,   147,   119,   132,   132,     4,     6,    76,   114,   117,
       5,    20,    19,    18,    17,    16,     7,     8,     9,    21,
      72,    79,    80,    81,    78,    82,    22,    83,    84,    85,
      86,    74,     5,    71,   149,     4,    10,    23,    24,    29,
      30,    35,    36,    38,    41,    45,    53,    54,    56,    57,
      59,    71,    95,    96,    97,    98,   107,   133,   138,   141,
     142,   143,    77,    75,   116,   119,   120,   121,   122,   123,
     124,   126,   126,   126,   126,   126,   126,   126,   127,   128,
     128,   129,   130,   131,   131,   132,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,   147,   148,   149,   149,    98,
      99,   100,   147,   102,   103,   147,   119,    25,    26,    38,
      41,    67,   119,    36,    40,   105,   149,    43,   147,    46,
     147,    98,    64,    65,    96,     5,    71,    76,   114,   115,
     116,   117,    25,    26,   116,    77,    75,   114,    75,     6,
      76,    75,    72,    34,   149,   119,    93,    94,    95,   147,
      72,   119,   149,   114,    60,    61,    96,   119,    72,   147,
     147,   116,    99,    69,    73,   101,   136,   102,    78,   104,
     134,     5,    96,    93,   149,    39,    94,    44,   119,   149,
      24,   139,   140,   141,   142,   143,    72,    58,    73,   119,
      76,   117,   144,   144,    77,    75,   135,    93,    31,    33,
     108,    37,    93,   105,   119,    42,    46,   113,   100,    33,
     149,   149,    67,    62,   119,    47,    48,   145,   146,   147,
     149,   149,   101,    32,   109,   110,   111,    96,    30,    39,
     119,   149,   119,    31,   118,   119,    33,   149,    53,   139,
     139,    27,    28,    54,   149,   147,   147,    77,    75,   117,
      92,    95,    96,    92,   119,    31,   112,   110,   108,    93,
      68,   106,   149,   149,    75,    45,   139,   149,    62,    62,
      62,   139,   117,   117,   145,    33,    92,    33,    34,     5,
      33,    58,   119,   149,    94,    93,   118,   144,    76,    76,
      25,    26,     5,    93,    30,    93,   113,   149,   146,   146,
      93,    58,    92,    77,    77,    33,   149,   149,    67,    92,
      92,    33,    33,    67,    67
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    87,    88,    88,    89,    89,    90,    90,    90,    91,
      91,    92,    92,    92,    93,    93,    94,    94,    95,    96,
      96,    96,    96,    96,    97,    97,    97,    97,    97,    97,
      97,    97,    97,    97,    97,    97,    97,    97,    97,    97,
      97,    97,    97,    97,    97,    97,    97,    98,    98,    99,
      99,   100,   100,   100,   101,   101,   102,   102,   103,   104,
     104,   105,   105,   106,   106,   107,   107,   107,   108,   108,
     109,   109,   110,   110,   111,   112,   112,   113,   113,   113,
     114,   114,   115,   115,   116,   116,   116,   117,   117,   118,
     118,   119,   119,   120,   120,   121,   121,   122,   122,   123,
     123,   124,   124,   125,   125,   125,   125,   125,   125,   125,
     125,   126,   126,   127,   127,   127,   128,   128,   129,   129,
     130,   130,   130,   131,   131,   132,   132,   132,   132,   132,
     133,   133,   134,   134,   134,   134,   134,   134,   134,   135,
     135,   135,   136,   136,   137,   137,   138,   139,   139,   139,
     139,   139,   139,   139,   139,   139,   140,   140,   140,   141,
     141,   142,   142,   143,   143,   143,   144,   144,   145,   145,
     146,   146,   146,   147,   147,   147,   147,   147,   147,   148,
     148,   148,   148,   148,   148,   148,   148,   148,   148,   148,
     148,   148,   148,   148,   148,   148,   148,   148,   148,   148,
     148,   148,   148,   148,   148,   148,   148,   148,   148,   148,
     148,   148,   148,   148,   148,   148,   148,   148,   148,   148,
     148,   148,   148,   148,   148,   148,   148,   148,   148,   149,
     149,   149,   149
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     3,     3,     0,     3,     0,     2,     2,     0,
       2,     0,     1,     2,     0,     1,     1,     2,     2,     1,
       2,     1,     3,     2,     2,     3,     4,     2,     1,     5,
       6,     6,     4,     1,     2,     2,     2,     2,     2,     5,
       1,     4,     4,     2,    10,     8,     7,     1,     3,     1,
       3,     1,     4,     2,     1,     3,     1,     3,     3,     1,
       2,     1,     1,     0,     2,     9,     5,     7,     0,     2,
       0,     1,     1,     2,     5,     0,     3,     0,     4,     5,
       1,     3,     1,     1,     1,     3,     2,     0,     1,     1,
       3,     1,     3,     1,     3,     1,     3,     1,     3,     1,
       3,     1,     2,     1,     3,     3,     3,     3,     3,     3,
       3,     1,     3,     1,     3,     3,     1,     3,     1,     3,
       1,     3,     3,     1,     3,     1,     1,     2,     2,     2,
       1,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     1,     7,     0,     1,     3,
       2,     4,     2,     4,     1,     3,     9,    11,    11,     8,
       8,     0,     1,     2,     1,     1,     1,     3,     1,     3,
       2,     3,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     2,     2
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (ctx, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value, ctx); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep, parser_ctx_t *ctx)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  YYUSE (ctx);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep, parser_ctx_t *ctx)
{
  YYFPRINTF (yyo, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyo, yytype, yyvaluep, ctx);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule, parser_ctx_t *ctx)
{
  unsigned long yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &yyvsp[(yyi + 1) - (yynrhs)]
                                              , ctx);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, ctx); \
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
#ifndef YYINITDEPTH
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
static YYSIZE_T
yystrlen (const char *yystr)
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
static char *
yystpcpy (char *yydest, const char *yysrc)
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
            else
              goto append;

          append:
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

  return (YYSIZE_T) (yystpcpy (yyres, yystr) - yyres);
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
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
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
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
                    yysize = yysize1;
                  else
                    return 2;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
      yysize = yysize1;
    else
      return 2;
  }

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

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, parser_ctx_t *ctx)
{
  YYUSE (yyvaluep);
  YYUSE (ctx);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/*----------.
| yyparse.  |
`----------*/

int
yyparse (parser_ctx_t *ctx)
{
/* The lookahead symbol.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
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
  int yytoken = 0;
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

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yynewstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  *yyssp = (yytype_int16) yystate;

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = (YYSIZE_T) (yyssp - yyss + 1);

# if defined yyoverflow
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
# else /* defined YYSTACK_RELOCATE */
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
# undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

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
      yychar = yylex (&yylval, ctx);
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
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
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
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

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
#line 148 "parser.y"
    { parse_complete(ctx, (yyvsp[-2].boolean)); }
#line 1830 "parser.tab.c"
    break;

  case 3:
#line 149 "parser.y"
    { handle_isexpression_script(ctx, (yyvsp[-1].expression)); }
#line 1836 "parser.tab.c"
    break;

  case 4:
#line 152 "parser.y"
    { (yyval.boolean) = FALSE; }
#line 1842 "parser.tab.c"
    break;

  case 5:
#line 153 "parser.y"
    { (yyval.boolean) = TRUE; }
#line 1848 "parser.tab.c"
    break;

  case 7:
#line 157 "parser.y"
    { source_add_statement(ctx, (yyvsp[0].statement)); }
#line 1854 "parser.tab.c"
    break;

  case 8:
#line 158 "parser.y"
    { source_add_class(ctx, (yyvsp[0].class_decl)); }
#line 1860 "parser.tab.c"
    break;

  case 9:
#line 161 "parser.y"
    { (yyval.expression) = NULL; }
#line 1866 "parser.tab.c"
    break;

  case 10:
#line 162 "parser.y"
    { (yyval.expression) = (yyvsp[-1].expression); }
#line 1872 "parser.tab.c"
    break;

  case 11:
#line 165 "parser.y"
    { (yyval.statement) = NULL; }
#line 1878 "parser.tab.c"
    break;

  case 12:
#line 166 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1884 "parser.tab.c"
    break;

  case 13:
#line 167 "parser.y"
    { (yyval.statement) = link_statements((yyvsp[-1].statement), (yyvsp[0].statement)); }
#line 1890 "parser.tab.c"
    break;

  case 14:
#line 170 "parser.y"
    { (yyval.statement) = NULL; }
#line 1896 "parser.tab.c"
    break;

  case 15:
#line 171 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1902 "parser.tab.c"
    break;

  case 16:
#line 174 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1908 "parser.tab.c"
    break;

  case 17:
#line 175 "parser.y"
    { (yyval.statement) = link_statements((yyvsp[-1].statement), (yyvsp[0].statement)); }
#line 1914 "parser.tab.c"
    break;

  case 18:
#line 178 "parser.y"
    { (yyval.statement) = (yyvsp[-1].statement); }
#line 1920 "parser.tab.c"
    break;

  case 19:
#line 181 "parser.y"
    { (yyval.statement) = NULL; }
#line 1926 "parser.tab.c"
    break;

  case 20:
#line 182 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1932 "parser.tab.c"
    break;

  case 21:
#line 183 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1938 "parser.tab.c"
    break;

  case 22:
#line 184 "parser.y"
    { (yyvsp[-2].statement)->next = (yyvsp[0].statement); (yyval.statement) = (yyvsp[-2].statement); }
#line 1944 "parser.tab.c"
    break;

  case 23:
#line 185 "parser.y"
    { (yyval.statement) = (yyvsp[-1].statement); }
#line 1950 "parser.tab.c"
    break;

  case 24:
#line 188 "parser.y"
    { (yyvsp[-1].member)->args = (yyvsp[0].expression); (yyval.statement) = new_call_statement(ctx, FALSE, (yyvsp[-1].member)); CHECK_ERROR; }
#line 1956 "parser.tab.c"
    break;

  case 25:
#line 189 "parser.y"
    { (yyvsp[-1].member)->args = (yyvsp[0].expression); (yyval.statement) = new_call_statement(ctx, TRUE, (yyvsp[-1].member)); CHECK_ERROR; }
#line 1962 "parser.tab.c"
    break;

  case 26:
#line 191 "parser.y"
    { (yyvsp[-3].member)->args = (yyvsp[-2].expression); (yyval.statement) = new_assign_statement(ctx, (yyvsp[-3].member), (yyvsp[0].expression)); CHECK_ERROR; }
#line 1968 "parser.tab.c"
    break;

  case 27:
#line 192 "parser.y"
    { (yyval.statement) = new_dim_statement(ctx, (yyvsp[0].dim_decl)); CHECK_ERROR; }
#line 1974 "parser.tab.c"
    break;

  case 28:
#line 193 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1980 "parser.tab.c"
    break;

  case 29:
#line 195 "parser.y"
    { (yyval.statement) = new_while_statement(ctx, STAT_WHILE, (yyvsp[-3].expression), (yyvsp[-1].statement)); CHECK_ERROR; }
#line 1986 "parser.tab.c"
    break;

  case 30:
#line 197 "parser.y"
    { (yyval.statement) = new_while_statement(ctx, (yyvsp[-4].boolean) ? STAT_WHILELOOP : STAT_UNTIL, (yyvsp[-3].expression), (yyvsp[-1].statement));
                                              CHECK_ERROR; }
#line 1993 "parser.tab.c"
    break;

  case 31:
#line 200 "parser.y"
    { (yyval.statement) = new_while_statement(ctx, (yyvsp[-1].boolean) ? STAT_DOWHILE : STAT_DOUNTIL, (yyvsp[0].expression), (yyvsp[-3].statement));
                                              CHECK_ERROR; }
#line 2000 "parser.tab.c"
    break;

  case 32:
#line 202 "parser.y"
    { (yyval.statement) = new_while_statement(ctx, STAT_DOWHILE, NULL, (yyvsp[-1].statement)); CHECK_ERROR; }
#line 2006 "parser.tab.c"
    break;

  case 33:
#line 203 "parser.y"
    { (yyval.statement) = new_function_statement(ctx, (yyvsp[0].func_decl)); CHECK_ERROR; }
#line 2012 "parser.tab.c"
    break;

  case 34:
#line 204 "parser.y"
    { (yyval.statement) = new_statement(ctx, STAT_EXITDO, 0); CHECK_ERROR; }
#line 2018 "parser.tab.c"
    break;

  case 35:
#line 205 "parser.y"
    { (yyval.statement) = new_statement(ctx, STAT_EXITFOR, 0); CHECK_ERROR; }
#line 2024 "parser.tab.c"
    break;

  case 36:
#line 206 "parser.y"
    { (yyval.statement) = new_statement(ctx, STAT_EXITFUNC, 0); CHECK_ERROR; }
#line 2030 "parser.tab.c"
    break;

  case 37:
#line 207 "parser.y"
    { (yyval.statement) = new_statement(ctx, STAT_EXITPROP, 0); CHECK_ERROR; }
#line 2036 "parser.tab.c"
    break;

  case 38:
#line 208 "parser.y"
    { (yyval.statement) = new_statement(ctx, STAT_EXITSUB, 0); CHECK_ERROR; }
#line 2042 "parser.tab.c"
    break;

  case 39:
#line 210 "parser.y"
    { (yyvsp[-3].member)->args = (yyvsp[-2].expression); (yyval.statement) = new_set_statement(ctx, (yyvsp[-3].member), (yyvsp[0].expression)); CHECK_ERROR; }
#line 2048 "parser.tab.c"
    break;

  case 40:
#line 211 "parser.y"
    { (yyval.statement) = new_statement(ctx, STAT_STOP, 0); CHECK_ERROR; }
#line 2054 "parser.tab.c"
    break;

  case 41:
#line 212 "parser.y"
    { (yyval.statement) = new_onerror_statement(ctx, TRUE); CHECK_ERROR; }
#line 2060 "parser.tab.c"
    break;

  case 42:
#line 213 "parser.y"
    { (yyval.statement) = new_onerror_statement(ctx, FALSE); CHECK_ERROR; }
#line 2066 "parser.tab.c"
    break;

  case 43:
#line 214 "parser.y"
    { (yyval.statement) = new_const_statement(ctx, (yyvsp[0].const_decl)); CHECK_ERROR; }
#line 2072 "parser.tab.c"
    break;

  case 44:
#line 216 "parser.y"
    { (yyval.statement) = new_forto_statement(ctx, (yyvsp[-8].string), (yyvsp[-6].expression), (yyvsp[-4].expression), (yyvsp[-3].expression), (yyvsp[-1].statement)); CHECK_ERROR; }
#line 2078 "parser.tab.c"
    break;

  case 45:
#line 218 "parser.y"
    { (yyval.statement) = new_foreach_statement(ctx, (yyvsp[-5].string), (yyvsp[-3].expression), (yyvsp[-1].statement)); }
#line 2084 "parser.tab.c"
    break;

  case 46:
#line 220 "parser.y"
    { (yyval.statement) = new_select_statement(ctx, (yyvsp[-4].expression), (yyvsp[-2].case_clausule)); }
#line 2090 "parser.tab.c"
    break;

  case 47:
#line 223 "parser.y"
    { (yyval.member) = new_member_expression(ctx, NULL, (yyvsp[0].string)); CHECK_ERROR; }
#line 2096 "parser.tab.c"
    break;

  case 48:
#line 224 "parser.y"
    { (yyval.member) = new_member_expression(ctx, (yyvsp[-2].expression), (yyvsp[0].string)); CHECK_ERROR; }
#line 2102 "parser.tab.c"
    break;

  case 49:
#line 227 "parser.y"
    { (yyval.dim_decl) = (yyvsp[0].dim_decl); }
#line 2108 "parser.tab.c"
    break;

  case 50:
#line 228 "parser.y"
    { (yyvsp[-2].dim_decl)->next = (yyvsp[0].dim_decl); (yyval.dim_decl) = (yyvsp[-2].dim_decl); }
#line 2114 "parser.tab.c"
    break;

  case 51:
#line 231 "parser.y"
    { (yyval.dim_decl) = new_dim_decl(ctx, (yyvsp[0].string), FALSE, NULL); CHECK_ERROR; }
#line 2120 "parser.tab.c"
    break;

  case 52:
#line 232 "parser.y"
    { (yyval.dim_decl) = new_dim_decl(ctx, (yyvsp[-3].string), TRUE, (yyvsp[-1].dim_list)); CHECK_ERROR; }
#line 2126 "parser.tab.c"
    break;

  case 53:
#line 233 "parser.y"
    { (yyval.dim_decl) = new_dim_decl(ctx, (yyvsp[-1].string), TRUE, NULL); CHECK_ERROR; }
#line 2132 "parser.tab.c"
    break;

  case 54:
#line 236 "parser.y"
    { (yyval.dim_list) = new_dim(ctx, (yyvsp[0].uint), NULL); }
#line 2138 "parser.tab.c"
    break;

  case 55:
#line 237 "parser.y"
    { (yyval.dim_list) = new_dim(ctx, (yyvsp[-2].uint), (yyvsp[0].dim_list)); }
#line 2144 "parser.tab.c"
    break;

  case 56:
#line 240 "parser.y"
    { (yyval.const_decl) = (yyvsp[0].const_decl); }
#line 2150 "parser.tab.c"
    break;

  case 57:
#line 241 "parser.y"
    { (yyvsp[-2].const_decl)->next = (yyvsp[0].const_decl); (yyval.const_decl) = (yyvsp[-2].const_decl); }
#line 2156 "parser.tab.c"
    break;

  case 58:
#line 244 "parser.y"
    { (yyval.const_decl) = new_const_decl(ctx, (yyvsp[-2].string), (yyvsp[0].expression)); CHECK_ERROR; }
#line 2162 "parser.tab.c"
    break;

  case 59:
#line 247 "parser.y"
    { (yyval.expression) = (yyvsp[0].expression); }
#line 2168 "parser.tab.c"
    break;

  case 60:
#line 248 "parser.y"
    { (yyval.expression) = new_unary_expression(ctx, EXPR_NEG, (yyvsp[0].expression)); CHECK_ERROR; }
#line 2174 "parser.tab.c"
    break;

  case 61:
#line 251 "parser.y"
    { (yyval.boolean) = TRUE; }
#line 2180 "parser.tab.c"
    break;

  case 62:
#line 252 "parser.y"
    { (yyval.boolean) = FALSE; }
#line 2186 "parser.tab.c"
    break;

  case 63:
#line 255 "parser.y"
    { (yyval.expression) = NULL;}
#line 2192 "parser.tab.c"
    break;

  case 64:
#line 256 "parser.y"
    { (yyval.expression) = (yyvsp[0].expression); }
#line 2198 "parser.tab.c"
    break;

  case 65:
#line 260 "parser.y"
    { (yyval.statement) = new_if_statement(ctx, (yyvsp[-7].expression), (yyvsp[-4].statement), (yyvsp[-3].elseif), (yyvsp[-2].statement)); CHECK_ERROR; }
#line 2204 "parser.tab.c"
    break;

  case 66:
#line 261 "parser.y"
    { (yyval.statement) = new_if_statement(ctx, (yyvsp[-3].expression), (yyvsp[-1].statement), NULL, NULL); CHECK_ERROR; }
#line 2210 "parser.tab.c"
    break;

  case 67:
#line 263 "parser.y"
    { (yyval.statement) = new_if_statement(ctx, (yyvsp[-5].expression), (yyvsp[-3].statement), NULL, (yyvsp[-1].statement)); CHECK_ERROR; }
#line 2216 "parser.tab.c"
    break;

  case 70:
#line 270 "parser.y"
    { (yyval.elseif) = NULL; }
#line 2222 "parser.tab.c"
    break;

  case 71:
#line 271 "parser.y"
    { (yyval.elseif) = (yyvsp[0].elseif); }
#line 2228 "parser.tab.c"
    break;

  case 72:
#line 274 "parser.y"
    { (yyval.elseif) = (yyvsp[0].elseif); }
#line 2234 "parser.tab.c"
    break;

  case 73:
#line 275 "parser.y"
    { (yyvsp[-1].elseif)->next = (yyvsp[0].elseif); (yyval.elseif) = (yyvsp[-1].elseif); }
#line 2240 "parser.tab.c"
    break;

  case 74:
#line 279 "parser.y"
    { (yyval.elseif) = new_elseif_decl(ctx, (yyvsp[-3].expression), (yyvsp[0].statement)); }
#line 2246 "parser.tab.c"
    break;

  case 75:
#line 282 "parser.y"
    { (yyval.statement) = NULL; }
#line 2252 "parser.tab.c"
    break;

  case 76:
#line 283 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); }
#line 2258 "parser.tab.c"
    break;

  case 77:
#line 286 "parser.y"
    { (yyval.case_clausule) = NULL; }
#line 2264 "parser.tab.c"
    break;

  case 78:
#line 287 "parser.y"
    { (yyval.case_clausule) = new_case_clausule(ctx, NULL, (yyvsp[0].statement), NULL); }
#line 2270 "parser.tab.c"
    break;

  case 79:
#line 289 "parser.y"
    { (yyval.case_clausule) = new_case_clausule(ctx, (yyvsp[-3].expression), (yyvsp[-1].statement), (yyvsp[0].case_clausule)); }
#line 2276 "parser.tab.c"
    break;

  case 80:
#line 292 "parser.y"
    { (yyval.expression) = NULL; }
#line 2282 "parser.tab.c"
    break;

  case 81:
#line 293 "parser.y"
    { (yyval.expression) = (yyvsp[-1].expression); }
#line 2288 "parser.tab.c"
    break;

  case 82:
#line 296 "parser.y"
    { (yyval.expression) = NULL; }
#line 2294 "parser.tab.c"
    break;

  case 83:
#line 297 "parser.y"
    { (yyval.expression) = (yyvsp[0].expression); }
#line 2300 "parser.tab.c"
    break;

  case 84:
#line 300 "parser.y"
    { (yyval.expression) = (yyvsp[0].expression); }
#line 2306 "parser.tab.c"
    break;

  case 85:
#line 301 "parser.y"
    { (yyvsp[-2].expression)->next = (yyvsp[0].expression); (yyval.expression) = (yyvsp[-2].expression); }
#line 2312 "parser.tab.c"
    break;

  case 86:
#line 302 "parser.y"
    { (yyval.expression) = new_expression(ctx, EXPR_NOARG, 0); CHECK_ERROR; (yyval.expression)->next = (yyvsp[0].expression); }
#line 2318 "parser.tab.c"
    break;

  case 89:
#line 309 "parser.y"
    { (yyval.expression) = (yyvsp[0].expression); }
#line 2324 "parser.tab.c"
    break;

  case 90:
#line 310 "parser.y"
    { (yyvsp[-2].expression)->next = (yyvsp[0].expression); (yyval.expression) = (yyvsp[-2].expression); }
#line 2330 "parser.tab.c"
    break;

  case 91:
#line 313 "parser.y"
    { (yyval.expression) = (yyvsp[0].expression); }
#line 2336 "parser.tab.c"
    break;

  case 92:
#line 314 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_IMP, (yyvsp[-2].expression), (yyvsp[0].expression)); CHECK_ERROR; }
#line 2342 "parser.tab.c"
    break;

  case 93:
#line 317 "parser.y"
    { (yyval.expression) = (yyvsp[0].expression); }
#line 2348 "parser.tab.c"
    break;

  case 94:
#line 318 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_EQV, (yyvsp[-2].expression), (yyvsp[0].expression)); CHECK_ERROR; }
#line 2354 "parser.tab.c"
    break;

  case 95:
#line 321 "parser.y"
    { (yyval.expression) = (yyvsp[0].expression); }
#line 2360 "parser.tab.c"
    break;

  case 96:
#line 322 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_XOR, (yyvsp[-2].expression), (yyvsp[0].expression)); CHECK_ERROR; }
#line 2366 "parser.tab.c"
    break;

  case 97:
#line 325 "parser.y"
    { (yyval.expression) = (yyvsp[0].expression); }
#line 2372 "parser.tab.c"
    break;

  case 98:
#line 326 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_OR, (yyvsp[-2].expression), (yyvsp[0].expression)); CHECK_ERROR; }
#line 2378 "parser.tab.c"
    break;

  case 99:
#line 329 "parser.y"
    { (yyval.expression) = (yyvsp[0].expression); }
#line 2384 "parser.tab.c"
    break;

  case 100:
#line 330 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_AND, (yyvsp[-2].expression), (yyvsp[0].expression)); CHECK_ERROR; }
#line 2390 "parser.tab.c"
    break;

  case 101:
#line 333 "parser.y"
    { (yyval.expression) = (yyvsp[0].expression); }
#line 2396 "parser.tab.c"
    break;

  case 102:
#line 334 "parser.y"
    { (yyval.expression) = new_unary_expression(ctx, EXPR_NOT, (yyvsp[0].expression)); CHECK_ERROR; }
#line 2402 "parser.tab.c"
    break;

  case 103:
#line 337 "parser.y"
    { (yyval.expression) = (yyvsp[0].expression); }
#line 2408 "parser.tab.c"
    break;

  case 104:
#line 338 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_EQUAL, (yyvsp[-2].expression), (yyvsp[0].expression)); CHECK_ERROR; }
#line 2414 "parser.tab.c"
    break;

  case 105:
#line 339 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_NEQUAL, (yyvsp[-2].expression), (yyvsp[0].expression)); CHECK_ERROR; }
#line 2420 "parser.tab.c"
    break;

  case 106:
#line 340 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_GT, (yyvsp[-2].expression), (yyvsp[0].expression)); CHECK_ERROR; }
#line 2426 "parser.tab.c"
    break;

  case 107:
#line 341 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_LT, (yyvsp[-2].expression), (yyvsp[0].expression)); CHECK_ERROR; }
#line 2432 "parser.tab.c"
    break;

  case 108:
#line 342 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_GTEQ, (yyvsp[-2].expression), (yyvsp[0].expression)); CHECK_ERROR; }
#line 2438 "parser.tab.c"
    break;

  case 109:
#line 343 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_LTEQ, (yyvsp[-2].expression), (yyvsp[0].expression)); CHECK_ERROR; }
#line 2444 "parser.tab.c"
    break;

  case 110:
#line 344 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_IS, (yyvsp[-2].expression), (yyvsp[0].expression)); CHECK_ERROR; }
#line 2450 "parser.tab.c"
    break;

  case 111:
#line 347 "parser.y"
    { (yyval.expression) = (yyvsp[0].expression); }
#line 2456 "parser.tab.c"
    break;

  case 112:
#line 348 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_CONCAT, (yyvsp[-2].expression), (yyvsp[0].expression)); CHECK_ERROR; }
#line 2462 "parser.tab.c"
    break;

  case 113:
#line 351 "parser.y"
    { (yyval.expression) = (yyvsp[0].expression); }
#line 2468 "parser.tab.c"
    break;

  case 114:
#line 352 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_ADD, (yyvsp[-2].expression), (yyvsp[0].expression)); CHECK_ERROR; }
#line 2474 "parser.tab.c"
    break;

  case 115:
#line 353 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_SUB, (yyvsp[-2].expression), (yyvsp[0].expression)); CHECK_ERROR; }
#line 2480 "parser.tab.c"
    break;

  case 116:
#line 356 "parser.y"
    { (yyval.expression) = (yyvsp[0].expression); }
#line 2486 "parser.tab.c"
    break;

  case 117:
#line 357 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_MOD, (yyvsp[-2].expression), (yyvsp[0].expression)); CHECK_ERROR; }
#line 2492 "parser.tab.c"
    break;

  case 118:
#line 360 "parser.y"
    { (yyval.expression) = (yyvsp[0].expression); }
#line 2498 "parser.tab.c"
    break;

  case 119:
#line 362 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_IDIV, (yyvsp[-2].expression), (yyvsp[0].expression)); CHECK_ERROR; }
#line 2504 "parser.tab.c"
    break;

  case 120:
#line 365 "parser.y"
    { (yyval.expression) = (yyvsp[0].expression); }
#line 2510 "parser.tab.c"
    break;

  case 121:
#line 367 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_MUL, (yyvsp[-2].expression), (yyvsp[0].expression)); CHECK_ERROR; }
#line 2516 "parser.tab.c"
    break;

  case 122:
#line 369 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_DIV, (yyvsp[-2].expression), (yyvsp[0].expression)); CHECK_ERROR; }
#line 2522 "parser.tab.c"
    break;

  case 123:
#line 372 "parser.y"
    { (yyval.expression) = (yyvsp[0].expression); }
#line 2528 "parser.tab.c"
    break;

  case 124:
#line 373 "parser.y"
    { (yyval.expression) = new_binary_expression(ctx, EXPR_EXP, (yyvsp[-2].expression), (yyvsp[0].expression)); CHECK_ERROR; }
#line 2534 "parser.tab.c"
    break;

  case 125:
#line 376 "parser.y"
    { (yyval.expression) = (yyvsp[0].expression); }
#line 2540 "parser.tab.c"
    break;

  case 126:
#line 377 "parser.y"
    { (yyval.expression) = (yyvsp[0].expression); }
#line 2546 "parser.tab.c"
    break;

  case 127:
#line 378 "parser.y"
    { (yyval.expression) = new_new_expression(ctx, (yyvsp[0].string)); CHECK_ERROR; }
#line 2552 "parser.tab.c"
    break;

  case 128:
#line 379 "parser.y"
    { (yyval.expression) = new_unary_expression(ctx, EXPR_NEG, (yyvsp[0].expression)); CHECK_ERROR; }
#line 2558 "parser.tab.c"
    break;

  case 129:
#line 380 "parser.y"
    { (yyval.expression) = (yyvsp[0].expression); }
#line 2564 "parser.tab.c"
    break;

  case 130:
#line 383 "parser.y"
    { (yyval.expression) = (yyvsp[0].expression); }
#line 2570 "parser.tab.c"
    break;

  case 131:
#line 384 "parser.y"
    { (yyvsp[-1].member)->args = (yyvsp[0].expression); (yyval.expression) = &(yyvsp[-1].member)->expr; }
#line 2576 "parser.tab.c"
    break;

  case 132:
#line 387 "parser.y"
    { (yyval.expression) = new_bool_expression(ctx, VARIANT_TRUE); CHECK_ERROR; }
#line 2582 "parser.tab.c"
    break;

  case 133:
#line 388 "parser.y"
    { (yyval.expression) = new_bool_expression(ctx, VARIANT_FALSE); CHECK_ERROR; }
#line 2588 "parser.tab.c"
    break;

  case 134:
#line 389 "parser.y"
    { (yyval.expression) = new_string_expression(ctx, (yyvsp[0].string)); CHECK_ERROR; }
#line 2594 "parser.tab.c"
    break;

  case 135:
#line 390 "parser.y"
    { (yyval.expression) = (yyvsp[0].expression); }
#line 2600 "parser.tab.c"
    break;

  case 136:
#line 391 "parser.y"
    { (yyval.expression) = new_expression(ctx, EXPR_EMPTY, 0); CHECK_ERROR; }
#line 2606 "parser.tab.c"
    break;

  case 137:
#line 392 "parser.y"
    { (yyval.expression) = new_expression(ctx, EXPR_NULL, 0); CHECK_ERROR; }
#line 2612 "parser.tab.c"
    break;

  case 138:
#line 393 "parser.y"
    { (yyval.expression) = new_expression(ctx, EXPR_NOTHING, 0); CHECK_ERROR; }
#line 2618 "parser.tab.c"
    break;

  case 139:
#line 396 "parser.y"
    { (yyval.expression) = new_long_expression(ctx, EXPR_INT, 0); CHECK_ERROR; }
#line 2624 "parser.tab.c"
    break;

  case 140:
#line 397 "parser.y"
    { (yyval.expression) = new_long_expression(ctx, EXPR_INT, (yyvsp[0].integer)); CHECK_ERROR; }
#line 2630 "parser.tab.c"
    break;

  case 141:
#line 398 "parser.y"
    { (yyval.expression) = new_double_expression(ctx, (yyvsp[0].dbl)); CHECK_ERROR; }
#line 2636 "parser.tab.c"
    break;

  case 142:
#line 401 "parser.y"
    { (yyval.uint) = 0; }
#line 2642 "parser.tab.c"
    break;

  case 143:
#line 402 "parser.y"
    { (yyval.uint) = (yyvsp[0].integer); }
#line 2648 "parser.tab.c"
    break;

  case 144:
#line 405 "parser.y"
    { (yyval.expression) = new_unary_expression(ctx, EXPR_BRACKETS, (yyvsp[-1].expression)); }
#line 2654 "parser.tab.c"
    break;

  case 145:
#line 406 "parser.y"
    { (yyval.expression) = new_expression(ctx, EXPR_ME, 0); CHECK_ERROR; }
#line 2660 "parser.tab.c"
    break;

  case 146:
#line 409 "parser.y"
    { (yyvsp[-3].class_decl)->name = (yyvsp[-5].string); (yyval.class_decl) = (yyvsp[-3].class_decl); }
#line 2666 "parser.tab.c"
    break;

  case 147:
#line 412 "parser.y"
    { (yyval.class_decl) = new_class_decl(ctx); }
#line 2672 "parser.tab.c"
    break;

  case 148:
#line 413 "parser.y"
    { (yyval.class_decl) = add_class_function(ctx, new_class_decl(ctx), (yyvsp[0].func_decl)); CHECK_ERROR; }
#line 2678 "parser.tab.c"
    break;

  case 149:
#line 414 "parser.y"
    { (yyval.class_decl) = add_class_function(ctx, (yyvsp[0].class_decl), (yyvsp[-2].func_decl)); CHECK_ERROR; }
#line 2684 "parser.tab.c"
    break;

  case 150:
#line 416 "parser.y"
    { dim_decl_t *dim_decl = new_dim_decl(ctx, (yyvsp[0].string), FALSE, NULL); CHECK_ERROR;
                                                  (yyval.class_decl) = add_dim_prop(ctx, new_class_decl(ctx), dim_decl, (yyvsp[-1].uint)); CHECK_ERROR; }
#line 2691 "parser.tab.c"
    break;

  case 151:
#line 418 "parser.y"
    { dim_decl_t *dim_decl = new_dim_decl(ctx, (yyvsp[-2].string), FALSE, NULL); CHECK_ERROR;
                                                  (yyval.class_decl) = add_dim_prop(ctx, (yyvsp[0].class_decl), dim_decl, (yyvsp[-3].uint)); CHECK_ERROR; }
#line 2698 "parser.tab.c"
    break;

  case 152:
#line 420 "parser.y"
    { (yyval.class_decl) = add_dim_prop(ctx, new_class_decl(ctx), (yyvsp[0].dim_decl), 0); CHECK_ERROR; }
#line 2704 "parser.tab.c"
    break;

  case 153:
#line 421 "parser.y"
    { (yyval.class_decl) = add_dim_prop(ctx, (yyvsp[0].class_decl), (yyvsp[-2].dim_decl), 0); CHECK_ERROR; }
#line 2710 "parser.tab.c"
    break;

  case 154:
#line 422 "parser.y"
    { (yyval.class_decl) = add_class_function(ctx, new_class_decl(ctx), (yyvsp[0].func_decl)); CHECK_ERROR; }
#line 2716 "parser.tab.c"
    break;

  case 155:
#line 423 "parser.y"
    { (yyval.class_decl) = add_class_function(ctx, (yyvsp[0].class_decl), (yyvsp[-2].func_decl)); CHECK_ERROR; }
#line 2722 "parser.tab.c"
    break;

  case 156:
#line 427 "parser.y"
    { (yyval.func_decl) = new_function_decl(ctx, (yyvsp[-5].string), FUNC_PROPGET, (yyvsp[-8].uint), (yyvsp[-4].arg_decl), (yyvsp[-2].statement)); CHECK_ERROR; }
#line 2728 "parser.tab.c"
    break;

  case 157:
#line 429 "parser.y"
    { (yyval.func_decl) = new_function_decl(ctx, (yyvsp[-7].string), FUNC_PROPLET, (yyvsp[-10].uint), (yyvsp[-5].arg_decl), (yyvsp[-2].statement)); CHECK_ERROR; }
#line 2734 "parser.tab.c"
    break;

  case 158:
#line 431 "parser.y"
    { (yyval.func_decl) = new_function_decl(ctx, (yyvsp[-7].string), FUNC_PROPSET, (yyvsp[-10].uint), (yyvsp[-5].arg_decl), (yyvsp[-2].statement)); CHECK_ERROR; }
#line 2740 "parser.tab.c"
    break;

  case 159:
#line 435 "parser.y"
    { (yyval.func_decl) = new_function_decl(ctx, (yyvsp[-5].string), FUNC_SUB, (yyvsp[-7].uint), (yyvsp[-4].arg_decl), (yyvsp[-2].statement)); CHECK_ERROR; }
#line 2746 "parser.tab.c"
    break;

  case 160:
#line 437 "parser.y"
    { (yyval.func_decl) = new_function_decl(ctx, (yyvsp[-5].string), FUNC_FUNCTION, (yyvsp[-7].uint), (yyvsp[-4].arg_decl), (yyvsp[-2].statement)); CHECK_ERROR; }
#line 2752 "parser.tab.c"
    break;

  case 161:
#line 440 "parser.y"
    { (yyval.uint) = 0; }
#line 2758 "parser.tab.c"
    break;

  case 162:
#line 441 "parser.y"
    { (yyval.uint) = (yyvsp[0].uint); }
#line 2764 "parser.tab.c"
    break;

  case 163:
#line 444 "parser.y"
    { (yyval.uint) = STORAGE_IS_DEFAULT; }
#line 2770 "parser.tab.c"
    break;

  case 164:
#line 445 "parser.y"
    { (yyval.uint) = 0; }
#line 2776 "parser.tab.c"
    break;

  case 165:
#line 446 "parser.y"
    { (yyval.uint) = STORAGE_IS_PRIVATE; }
#line 2782 "parser.tab.c"
    break;

  case 166:
#line 449 "parser.y"
    { (yyval.arg_decl) = NULL; }
#line 2788 "parser.tab.c"
    break;

  case 167:
#line 450 "parser.y"
    { (yyval.arg_decl) = (yyvsp[-1].arg_decl); }
#line 2794 "parser.tab.c"
    break;

  case 168:
#line 453 "parser.y"
    { (yyval.arg_decl) = (yyvsp[0].arg_decl); }
#line 2800 "parser.tab.c"
    break;

  case 169:
#line 454 "parser.y"
    { (yyvsp[-2].arg_decl)->next = (yyvsp[0].arg_decl); (yyval.arg_decl) = (yyvsp[-2].arg_decl); }
#line 2806 "parser.tab.c"
    break;

  case 170:
#line 457 "parser.y"
    { (yyval.arg_decl) = new_argument_decl(ctx, (yyvsp[-1].string), TRUE); }
#line 2812 "parser.tab.c"
    break;

  case 171:
#line 458 "parser.y"
    { (yyval.arg_decl) = new_argument_decl(ctx, (yyvsp[-1].string), TRUE); }
#line 2818 "parser.tab.c"
    break;

  case 172:
#line 459 "parser.y"
    { (yyval.arg_decl) = new_argument_decl(ctx, (yyvsp[-1].string), FALSE); }
#line 2824 "parser.tab.c"
    break;

  case 173:
#line 463 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2830 "parser.tab.c"
    break;

  case 174:
#line 464 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2836 "parser.tab.c"
    break;

  case 175:
#line 465 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2842 "parser.tab.c"
    break;

  case 176:
#line 466 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2848 "parser.tab.c"
    break;

  case 177:
#line 467 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2854 "parser.tab.c"
    break;

  case 178:
#line 468 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2860 "parser.tab.c"
    break;

  case 179:
#line 472 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2866 "parser.tab.c"
    break;

  case 180:
#line 473 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2872 "parser.tab.c"
    break;

  case 181:
#line 474 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2878 "parser.tab.c"
    break;

  case 182:
#line 475 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2884 "parser.tab.c"
    break;

  case 183:
#line 476 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2890 "parser.tab.c"
    break;

  case 184:
#line 477 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2896 "parser.tab.c"
    break;

  case 185:
#line 478 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2902 "parser.tab.c"
    break;

  case 186:
#line 479 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2908 "parser.tab.c"
    break;

  case 187:
#line 480 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2914 "parser.tab.c"
    break;

  case 188:
#line 481 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2920 "parser.tab.c"
    break;

  case 189:
#line 482 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2926 "parser.tab.c"
    break;

  case 190:
#line 483 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2932 "parser.tab.c"
    break;

  case 191:
#line 484 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2938 "parser.tab.c"
    break;

  case 192:
#line 485 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2944 "parser.tab.c"
    break;

  case 193:
#line 486 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2950 "parser.tab.c"
    break;

  case 194:
#line 487 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2956 "parser.tab.c"
    break;

  case 195:
#line 488 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2962 "parser.tab.c"
    break;

  case 196:
#line 489 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2968 "parser.tab.c"
    break;

  case 197:
#line 490 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2974 "parser.tab.c"
    break;

  case 198:
#line 491 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2980 "parser.tab.c"
    break;

  case 199:
#line 492 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2986 "parser.tab.c"
    break;

  case 200:
#line 493 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2992 "parser.tab.c"
    break;

  case 201:
#line 494 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 2998 "parser.tab.c"
    break;

  case 202:
#line 495 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3004 "parser.tab.c"
    break;

  case 203:
#line 496 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3010 "parser.tab.c"
    break;

  case 204:
#line 497 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3016 "parser.tab.c"
    break;

  case 205:
#line 498 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3022 "parser.tab.c"
    break;

  case 206:
#line 499 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3028 "parser.tab.c"
    break;

  case 207:
#line 500 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3034 "parser.tab.c"
    break;

  case 208:
#line 501 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3040 "parser.tab.c"
    break;

  case 209:
#line 502 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3046 "parser.tab.c"
    break;

  case 210:
#line 503 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3052 "parser.tab.c"
    break;

  case 211:
#line 504 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3058 "parser.tab.c"
    break;

  case 212:
#line 505 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3064 "parser.tab.c"
    break;

  case 213:
#line 506 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3070 "parser.tab.c"
    break;

  case 214:
#line 507 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3076 "parser.tab.c"
    break;

  case 215:
#line 508 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3082 "parser.tab.c"
    break;

  case 216:
#line 509 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3088 "parser.tab.c"
    break;

  case 217:
#line 510 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3094 "parser.tab.c"
    break;

  case 218:
#line 511 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3100 "parser.tab.c"
    break;

  case 219:
#line 512 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3106 "parser.tab.c"
    break;

  case 220:
#line 513 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3112 "parser.tab.c"
    break;

  case 221:
#line 514 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3118 "parser.tab.c"
    break;

  case 222:
#line 515 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3124 "parser.tab.c"
    break;

  case 223:
#line 516 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3130 "parser.tab.c"
    break;

  case 224:
#line 517 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3136 "parser.tab.c"
    break;

  case 225:
#line 518 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3142 "parser.tab.c"
    break;

  case 226:
#line 519 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3148 "parser.tab.c"
    break;

  case 227:
#line 520 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3154 "parser.tab.c"
    break;

  case 228:
#line 521 "parser.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 3160 "parser.tab.c"
    break;


#line 3164 "parser.tab.c"

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

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
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
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

  /* Do not reclaim the symbols of the rule whose action triggered
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
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

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

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


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


#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (ctx, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif


/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, ctx);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
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
  return yyresult;
}
#line 530 "parser.y"


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

static void handle_isexpression_script(parser_ctx_t *ctx, expression_t *expr)
{
    retval_statement_t *stat;

    ctx->parse_complete = TRUE;
    if(!expr)
        return;

    stat = new_statement(ctx, STAT_RETVAL, sizeof(*stat));
    if(!stat)
        return;

    stat->expr = expr;
    ctx->stats = &stat->stat;
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
        if(!wcsicmp(iter->name, decl->name)) {
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

HRESULT parse_script(parser_ctx_t *ctx, const WCHAR *code, const WCHAR *delimiter, DWORD flags)
{
    static const WCHAR html_delimiterW[] = {'<','/','s','c','r','i','p','t','>',0};

    ctx->code = ctx->ptr = code;
    ctx->end = ctx->code + lstrlenW(ctx->code);

    heap_pool_init(&ctx->heap);

    ctx->parse_complete = FALSE;
    ctx->hres = S_OK;

    ctx->last_token = tNL;
    ctx->last_nl = 0;
    ctx->stats = ctx->stats_tail = NULL;
    ctx->class_decls = NULL;
    ctx->option_explicit = FALSE;
    ctx->is_html = delimiter && !wcsicmp(delimiter, html_delimiterW);

    if(flags & SCRIPTTEXT_ISEXPRESSION)
        ctx->last_token = tEXPRESSION;

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
