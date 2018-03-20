/* A Bison parser, made by GNU Bison 3.0.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

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
#define YYBISON_VERSION "3.0"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         xslpattern_parse
#define yylex           xslpattern_lex
#define yyerror         xslpattern_error
#define yydebug         xslpattern_debug
#define yynerrs         xslpattern_nerrs


/* Copy the first part of user declarations.  */
#line 21 "xslpattern.y" /* yacc.c:339  */

#include "config.h"
#include "wine/port.h"

#ifdef HAVE_LIBXML2
#include "xslpattern.h"
#include <libxml/xpathInternals.h>
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);


static const xmlChar NameTest_mod_pre[] = "*[name()='";
static const xmlChar NameTest_mod_post[] = "']";

#define U(str) BAD_CAST str

static inline BOOL is_literal(xmlChar const* tok)
{
    return (tok && tok[0] && tok[1] &&
            tok[0]== tok[xmlStrlen(tok)-1] &&
            (tok[0] == '\'' || tok[0] == '"'));
}

static void xslpattern_error(parser_param* param, void const* scanner, char const* msg)
{
    FIXME("%s:\n"
          "  param {\n"
          "    yyscanner=%p\n"
          "    ctx=%p\n"
          "    in=\"%s\"\n"
          "    pos=%i\n"
          "    len=%i\n"
          "    out=\"%s\"\n"
          "    err=%i\n"
          "  }\n"
          "  scanner=%p\n",
          msg, param->yyscanner, param->ctx, param->in, param->pos,
          param->len, param->out, ++param->err, scanner);
}


#line 115 "xslpattern.tab.c" /* yacc.c:339  */

# ifndef YY_NULL
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULL nullptr
#  else
#   define YY_NULL 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "xslpattern.tab.h".  */
#ifndef YY_XSLPATTERN_E_REACTOSSYNC_GCC_DLL_WIN32_MSXML3_XSLPATTERN_TAB_H_INCLUDED
# define YY_XSLPATTERN_E_REACTOSSYNC_GCC_DLL_WIN32_MSXML3_XSLPATTERN_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int xslpattern_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    TOK_Parent = 258,
    TOK_Self = 259,
    TOK_DblFSlash = 260,
    TOK_FSlash = 261,
    TOK_Axis = 262,
    TOK_Colon = 263,
    TOK_OpAnd = 264,
    TOK_OpOr = 265,
    TOK_OpNot = 266,
    TOK_OpEq = 267,
    TOK_OpIEq = 268,
    TOK_OpNEq = 269,
    TOK_OpINEq = 270,
    TOK_OpLt = 271,
    TOK_OpILt = 272,
    TOK_OpGt = 273,
    TOK_OpIGt = 274,
    TOK_OpLEq = 275,
    TOK_OpILEq = 276,
    TOK_OpGEq = 277,
    TOK_OpIGEq = 278,
    TOK_OpAll = 279,
    TOK_OpAny = 280,
    TOK_NCName = 281,
    TOK_Literal = 282,
    TOK_Number = 283
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int xslpattern_parse (parser_param* p, void* scanner);

#endif /* !YY_XSLPATTERN_E_REACTOSSYNC_GCC_DLL_WIN32_MSXML3_XSLPATTERN_TAB_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 194 "xslpattern.tab.c" /* yacc.c:358  */

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
# elif ! defined YYSIZE_T
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
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
# if (! defined __GNUC__ || __GNUC__ < 2 \
      || (__GNUC__ == 2 && __GNUC_MINOR__ < 5))
#  define __attribute__(Spec) /* empty */
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
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
#define YYFINAL  58
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   176

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  38
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  39
/* YYNRULES -- Number of rules.  */
#define YYNRULES  104
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  157

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   283

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    35,     2,     2,     2,     2,     2,     2,
      33,    34,    30,     2,    36,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    29,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    31,     2,    32,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    37,     2,     2,     2,     2,     2,
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
      25,    26,    27,    28
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    86,    86,    96,    97,    99,   108,   119,   120,   122,
     129,   134,   136,   137,   145,   148,   157,   164,   171,   172,
     173,   175,   182,   189,   197,   198,   200,   205,   211,   230,
     238,   244,   246,   255,   261,   262,   263,   266,   274,   283,
     288,   297,   299,   300,   301,   302,   303,   304,   306,   314,
     322,   323,   326,   385,   416,   423,   425,   428,   429,   438,
     439,   447,   455,   457,   458,   467,   468,   470,   479,   480,
     482,   491,   492,   494,   502,   513,   521,   533,   534,   536,
     544,   555,   563,   574,   582,   593,   601,   615,   616,   618,
     626,   634,   642,   648,   655,   662,   669,   676,   683,   690,
     700,   710,   720,   730,   740
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "TOK_Parent", "TOK_Self",
  "TOK_DblFSlash", "TOK_FSlash", "TOK_Axis", "TOK_Colon", "TOK_OpAnd",
  "TOK_OpOr", "TOK_OpNot", "TOK_OpEq", "TOK_OpIEq", "TOK_OpNEq",
  "TOK_OpINEq", "TOK_OpLt", "TOK_OpILt", "TOK_OpGt", "TOK_OpIGt",
  "TOK_OpLEq", "TOK_OpILEq", "TOK_OpGEq", "TOK_OpIGEq", "TOK_OpAll",
  "TOK_OpAny", "TOK_NCName", "TOK_Literal", "TOK_Number", "'@'", "'*'",
  "'['", "']'", "'('", "')'", "'!'", "','", "'|'", "$accept", "XSLPattern",
  "QName", "PrefixedName", "UnprefixedName", "LocationPath",
  "AbsoluteLocationPath", "RelativeLocationPath", "Step", "AxisSpecifier",
  "Attribute", "NodeTest", "NameTest", "Predicates", "Predicate",
  "PredicateExpr", "AbbreviatedAbsoluteLocationPath",
  "AbbreviatedRelativeLocationPath", "AbbreviatedStep", "Expr", "BoolExpr",
  "PrimaryExpr", "FunctionCall", "Arguments", "Argument", "UnionExpr",
  "PathExpr", "FilterExpr", "OrExpr", "BoolOrExpr", "AndExpr",
  "BoolAndExpr", "EqualityExpr", "BoolEqualityExpr", "RelationalExpr",
  "BoolRelationalExpr", "UnaryExpr", "BoolUnaryExpr", "AllExpr", YY_NULL
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,    64,
      42,    91,    93,    40,    41,    33,    44,   124
};
# endif

#define YYPACT_NINF -34

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-34)))

#define YYTABLE_NINF -67

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      20,   -34,   -34,    25,    25,    20,   106,    20,     0,   -34,
     -34,   -21,   -34,    20,    10,   -17,   -34,   -12,   -34,   -34,
      52,   -34,    -8,   -34,    28,   -34,   -34,   -34,   -34,   -34,
     -34,   -34,   -10,    27,    35,    55,   -34,    21,   -34,    58,
     -34,   136,   -34,   -34,   -34,    52,    52,   -34,   128,   -34,
     -34,   -34,    26,    59,   -34,   -34,   -34,    40,   -34,     9,
      25,    25,   105,    28,    97,    28,   -34,   106,    88,    25,
      25,   -34,    20,    20,    20,    20,    20,    20,    20,    20,
      20,    20,    20,    20,    20,    20,   106,   106,   106,   106,
     106,   106,   106,   106,   106,   106,   106,   106,    82,   -34,
      90,   -34,   -34,   -34,    83,    84,   -34,   -34,    28,    99,
      86,    87,    96,   -34,   129,    55,   119,   130,   132,   133,
     134,   -34,    27,   -34,    52,    52,   -34,    58,   136,   136,
     136,   136,   -34,   -34,   -34,   -34,   -34,   -34,   -34,   -34,
      27,    27,    27,    27,    27,    27,    27,    27,    27,    27,
      27,    27,   -34,   -34,    20,   -34,   -34
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    39,    40,     0,    10,     0,    92,     0,     6,    50,
      51,     0,    26,     0,     0,     0,     3,    29,    59,     8,
       7,    12,     0,    19,    18,    24,    11,    14,    20,     2,
      63,    25,    87,    57,    62,    41,    66,    65,    69,    68,
      72,    71,    78,    77,    88,    37,     9,    89,     0,    91,
      90,    21,     0,     6,    23,    22,     4,     0,     1,     0,
       0,     0,     6,    17,     0,    16,    31,     0,     0,     0,
       0,    64,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    28,    27,
       0,    48,    53,    56,     0,    55,    38,    13,    15,     6,
      51,    19,     0,    34,    25,     0,    47,    69,    72,    78,
      88,    30,    58,    49,    61,    60,    67,    70,    73,    74,
      75,    76,    79,    80,    81,    82,    83,    84,    85,    86,
      93,    99,    94,   100,    95,   101,    97,   103,    96,   102,
      98,   104,     5,    52,     0,    32,    54
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -34,   -34,   126,   -34,    -5,   -34,   -34,    -1,    44,   -34,
      74,   138,   -34,   104,   -33,   -34,   -34,   -34,   -34,     4,
     -34,   -34,    -4,    15,   -34,   -34,    -6,   -34,   107,   108,
      98,   109,    95,   110,     2,   111,    14,   112,   -34
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    65,    66,   112,    26,    27,    28,   103,
     113,    30,    31,   104,   105,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    49
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      48,    71,    45,    46,    29,    53,    56,    51,    52,    54,
      58,    50,     1,     2,     3,     4,    59,    57,    62,    47,
       5,    -4,    12,     1,     2,     3,     4,    67,     1,     2,
      73,     5,   121,     6,     7,     8,     9,    10,    11,    12,
      69,    70,    13,   102,     6,     7,     8,     9,    10,    11,
      12,     8,    98,    13,    11,    12,    99,    60,    61,    64,
     114,   122,    68,    56,   123,    72,    64,   100,   124,   125,
      74,    75,    76,    77,   101,   121,   128,   129,   130,   131,
     140,   141,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   151,   132,   133,   134,   135,   136,   137,   138,   139,
       1,     2,     3,     4,   106,   107,    51,    52,     5,     1,
       2,     3,     4,    52,    53,    -5,   152,   153,   -33,   -35,
     154,     6,     7,   109,     9,   110,    11,    12,   155,   -66,
      13,   -36,     8,     9,    10,    11,    12,    55,   111,    13,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    78,    79,    80,    81,    82,    83,    84,    85,
      63,   -42,   -46,    68,   -45,   -44,   -43,   108,   127,   156,
     126,   115,   116,   117,   118,   119,   120
};

static const yytype_uint8 yycheck[] =
{
       6,    34,     3,     4,     0,    26,    11,     7,     8,    30,
       0,     7,     3,     4,     5,     6,    33,    13,    26,     5,
      11,    33,    30,     3,     4,     5,     6,    37,     3,     4,
       9,    11,    65,    24,    25,    26,    27,    28,    29,    30,
       5,     6,    33,    34,    24,    25,    26,    27,    28,    29,
      30,    26,    26,    33,    29,    30,    30,     5,     6,    31,
      64,    67,    35,    68,    68,    10,    31,     8,    69,    70,
      12,    13,    14,    15,    34,   108,    74,    75,    76,    77,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    78,    79,    80,    81,    82,    83,    84,    85,
       3,     4,     5,     6,    60,    61,     7,     8,    11,     3,
       4,     5,     6,     8,    26,    33,    26,    34,    32,    32,
      36,    24,    25,    26,    27,    28,    29,    30,    32,    10,
      33,    32,    26,    27,    28,    29,    30,    11,    64,    33,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    16,    17,    18,    19,    20,    21,    22,    23,
      22,    32,    32,    35,    32,    32,    32,    63,    73,   154,
      72,    64,    64,    64,    64,    64,    64
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,    11,    24,    25,    26,    27,
      28,    29,    30,    33,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    54,    55,    56,    57,
      59,    60,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    45,    45,    74,    64,    76,
      57,     7,     8,    26,    30,    40,    42,    57,     0,    33,
       5,     6,    26,    49,    31,    51,    52,    37,    35,     5,
       6,    52,    10,     9,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    26,    30,
       8,    34,    34,    57,    61,    62,    46,    46,    51,    26,
      28,    48,    53,    58,    60,    66,    67,    69,    71,    73,
      75,    52,    64,    60,    45,    45,    68,    70,    72,    72,
      72,    72,    74,    74,    74,    74,    74,    74,    74,    74,
      64,    64,    64,    64,    64,    64,    64,    64,    64,    64,
      64,    64,    26,    34,    36,    32,    61
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    38,    39,    40,    40,    41,    42,    43,    43,    44,
      44,    44,    45,    45,    45,    46,    46,    46,    46,    46,
      46,    47,    48,    48,    49,    49,    50,    50,    50,    50,
      51,    51,    52,    53,    53,    53,    53,    54,    55,    56,
      56,    57,    58,    58,    58,    58,    58,    58,    59,    59,
      59,    59,    60,    60,    61,    61,    62,    63,    63,    64,
      64,    64,    64,    65,    65,    66,    66,    67,    68,    68,
      69,    70,    70,    71,    71,    71,    71,    72,    72,    73,
      73,    73,    73,    73,    73,    73,    73,    74,    74,    75,
      75,    75,    75,    76,    76,    76,    76,    76,    76,    76,
      76,    76,    76,    76,    76
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     3,     1,     1,     1,     2,
       1,     1,     1,     3,     1,     3,     2,     2,     1,     1,
       1,     2,     2,     2,     1,     1,     1,     3,     3,     1,
       2,     1,     3,     1,     1,     1,     1,     2,     3,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     3,
       1,     1,     4,     3,     3,     1,     1,     1,     3,     1,
       3,     3,     1,     1,     2,     1,     1,     3,     1,     1,
       3,     1,     1,     3,     3,     3,     3,     1,     1,     3,
       3,     3,     3,     3,     3,     3,     3,     1,     1,     2,
       2,     2,     1,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
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
      yyerror (p, scanner, YY_("syntax error: cannot back up")); \
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
                  Type, Value, p, scanner); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, parser_param* p, void* scanner)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  YYUSE (p);
  YYUSE (scanner);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, parser_param* p, void* scanner)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, p, scanner);
  YYFPRINTF (yyoutput, ")");
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
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule, parser_param* p, void* scanner)
{
  unsigned long int yylno = yyrline[yyrule];
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
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              , p, scanner);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, p, scanner); \
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
  YYSIZE_T yysize0 = yytnamerr (YY_NULL, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULL;
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
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULL, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
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
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
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
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, parser_param* p, void* scanner)
{
  YYUSE (yyvaluep);
  YYUSE (p);
  YYUSE (scanner);
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
yyparse (parser_param* p, void* scanner)
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
      yychar = yylex (&yylval, scanner);
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
| yyreduce -- Do a reduction.  |
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
#line 87 "xslpattern.y" /* yacc.c:1646  */
    {
                                p->out = (yyvsp[0]);
                            }
#line 1385 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 5:
#line 100 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got PrefixedName: \"%s:%s\"\n", (yyvsp[-2]), (yyvsp[0]));
                                (yyval)=(yyvsp[-2]);
                                (yyval)=xmlStrcat((yyval),U(":"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1397 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 6:
#line 109 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got UnprefixedName: \"%s\"\n", (yyvsp[0]));
                                (yyval)=(yyvsp[0]);
                            }
#line 1406 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 9:
#line 123 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got AbsoluteLocationPath: \"/%s\"\n", (yyvsp[0]));
                                (yyval)=xmlStrdup(U("/"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1417 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 10:
#line 130 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got AbsoluteLocationPath: \"/\"\n");
                                (yyval)=xmlStrdup(U("/"));
                            }
#line 1426 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 13:
#line 138 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got RelativeLocationPath: \"%s/%s\"\n", (yyvsp[-2]), (yyvsp[0]));
                                (yyval)=(yyvsp[-2]);
                                (yyval)=xmlStrcat((yyval),U("/"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1438 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 15:
#line 149 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got Step: \"%s%s%s\"\n", (yyvsp[-2]), (yyvsp[-1]), (yyvsp[0]));
                                (yyval)=(yyvsp[-2]);
                                (yyval)=xmlStrcat((yyval),(yyvsp[-1]));
                                xmlFree((yyvsp[-1]));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1451 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 16:
#line 158 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got Step: \"%s%s\"\n", (yyvsp[-1]), (yyvsp[0]));
                                (yyval)=(yyvsp[-1]);
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1462 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 17:
#line 165 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got Step: \"%s%s\"\n", (yyvsp[-1]), (yyvsp[0]));
                                (yyval)=(yyvsp[-1]);
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1473 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 21:
#line 176 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got AxisSpecifier: \"%s::\"\n", (yyvsp[-1]));
                                (yyval)=(yyvsp[-1]);
                                (yyval)=xmlStrcat((yyval),U("::"));
                            }
#line 1483 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 22:
#line 183 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got Attribute: \"@%s\"\n", (yyvsp[0]));
                                (yyval)=xmlStrdup(U("@"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1494 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 23:
#line 190 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got All attributes pattern: \"@*\"\n");
                                (yyval)=xmlStrdup(U("@*"));
                            }
#line 1503 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 26:
#line 201 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got NameTest: \"*\"\n");
                                (yyval)=xmlStrdup(U("*"));
                            }
#line 1512 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 27:
#line 206 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got NameTest: \"%s:*\"\n", (yyvsp[-2]));
                                (yyval)=(yyvsp[-2]);
                                (yyval)=xmlStrcat((yyval),U(":*"));
                            }
#line 1522 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 28:
#line 212 "xslpattern.y" /* yacc.c:1646  */
    { /* PrefixedName */
                                xmlChar const* registeredNsURI = xmlXPathNsLookup(p->ctx, (yyvsp[-2]));
                                TRACE("Got PrefixedName: \"%s:%s\"\n", (yyvsp[-2]), (yyvsp[0]));

                                if (registeredNsURI)
                                    (yyval)=xmlStrdup(U(""));
                                else
                                    (yyval)=xmlStrdup(NameTest_mod_pre);

                                (yyval)=xmlStrcat((yyval),(yyvsp[-2]));
                                xmlFree((yyvsp[-2]));
                                (yyval)=xmlStrcat((yyval),U(":"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));

                                if (!registeredNsURI)
                                    (yyval)=xmlStrcat((yyval),NameTest_mod_post);
                            }
#line 1545 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 29:
#line 231 "xslpattern.y" /* yacc.c:1646  */
    {
                                (yyval)=xmlStrdup(NameTest_mod_pre);
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                                (yyval)=xmlStrcat((yyval),NameTest_mod_post);
                            }
#line 1556 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 30:
#line 239 "xslpattern.y" /* yacc.c:1646  */
    {
                                (yyval)=(yyvsp[-1]);
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1566 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 32:
#line 247 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got Predicate: \"[%s]\"\n", (yyvsp[-1]));
                                (yyval)=xmlStrdup(U("["));
                                (yyval)=xmlStrcat((yyval),(yyvsp[-1]));
                                xmlFree((yyvsp[-1]));
                                (yyval)=xmlStrcat((yyval),U("]"));
                            }
#line 1578 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 33:
#line 256 "xslpattern.y" /* yacc.c:1646  */
    {
                                (yyval)=xmlStrdup(U("index()="));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1588 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 37:
#line 267 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got AbbreviatedAbsoluteLocationPath: \"//%s\"\n", (yyvsp[0]));
                                (yyval)=xmlStrdup(U("//"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1599 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 38:
#line 275 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got AbbreviatedRelativeLocationPath: \"%s//%s\"\n", (yyvsp[-2]), (yyvsp[0]));
                                (yyval)=(yyvsp[-2]);
                                (yyval)=xmlStrcat((yyval),U("//"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1611 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 39:
#line 284 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got AbbreviatedStep: \"..\"\n");
                                (yyval)=xmlStrdup(U(".."));
                            }
#line 1620 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 40:
#line 289 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got AbbreviatedStep: \".\"\n");
                                (yyval)=xmlStrdup(U("."));
                            }
#line 1629 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 48:
#line 307 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got PrimaryExpr: \"(%s)\"\n", (yyvsp[-2]));
                                (yyval)=xmlStrdup(U("("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[-1]));
                                xmlFree((yyvsp[-1]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            }
#line 1641 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 49:
#line 315 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got PrimaryExpr: \"%s!%s\"\n", (yyvsp[-2]), (yyvsp[0]));
                                (yyval)=(yyvsp[-2]);
                                (yyval)=xmlStrcat((yyval),U("/"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1653 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 52:
#line 327 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got FunctionCall: \"%s(%s)\"\n", (yyvsp[-3]), (yyvsp[-1]));
                                if (xmlStrEqual((yyvsp[-3]),U("ancestor")))
                                {
                                    (yyval)=(yyvsp[-3]);
                                    (yyval)=xmlStrcat((yyval),U("::"));
                                    (yyval)=xmlStrcat((yyval),(yyvsp[-1]));
                                    xmlFree((yyvsp[-1]));
                                }
                                else if (xmlStrEqual((yyvsp[-3]),U("attribute")))
                                {
                                    if (is_literal((yyvsp[-1])))
                                    {
                                        (yyval)=xmlStrdup(U("@*[name()="));
                                        xmlFree((yyvsp[-3]));
                                        (yyval)=xmlStrcat((yyval),(yyvsp[-1]));
                                        xmlFree((yyvsp[-1]));
                                        (yyval)=xmlStrcat((yyval),U("]"));
                                    }
                                    else
                                    {
                                        /* XML_XPATH_INVALID_TYPE */
                                        (yyval)=xmlStrdup(U("error(1211, 'Error: attribute("));
                                        xmlFree((yyvsp[-3]));
                                        (yyval)=xmlStrcat((yyval),(yyvsp[-1]));
                                        xmlFree((yyvsp[-1]));
                                        (yyval)=xmlStrcat((yyval),U("): invalid argument')"));
                                    }
                                }
                                else if (xmlStrEqual((yyvsp[-3]),U("element")))
                                {
                                    if (is_literal((yyvsp[-1])))
                                    {
                                        (yyval)=xmlStrdup(U("node()[nodeType()=1][name()="));
                                        xmlFree((yyvsp[-3]));
                                        (yyval)=xmlStrcat((yyval),(yyvsp[-1]));
                                        xmlFree((yyvsp[-1]));
                                        (yyval)=xmlStrcat((yyval),U("]"));
                                    }
                                    else
                                    {
                                        /* XML_XPATH_INVALID_TYPE */
                                        (yyval)=xmlStrdup(U("error(1211, 'Error: element("));
                                        xmlFree((yyvsp[-3]));
                                        (yyval)=xmlStrcat((yyval),(yyvsp[-1]));
                                        xmlFree((yyvsp[-1]));
                                        (yyval)=xmlStrcat((yyval),U("): invalid argument')"));
                                    }
                                }
                                else
                                {
                                    (yyval)=(yyvsp[-3]);
                                    (yyval)=xmlStrcat((yyval),U("("));
                                    (yyval)=xmlStrcat((yyval),(yyvsp[-1]));
                                    xmlFree((yyvsp[-1]));
                                    (yyval)=xmlStrcat((yyval),U(")"));
                                }
                            }
#line 1716 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 53:
#line 386 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got FunctionCall: \"%s()\"\n", (yyvsp[-2]));
                                /* comment() & node() work the same in XPath */
                                if (xmlStrEqual((yyvsp[-2]),U("attribute")))
                                {
                                    (yyval)=xmlStrdup(U("@*"));
                                    xmlFree((yyvsp[-2]));
                                }
                                else if (xmlStrEqual((yyvsp[-2]),U("element")))
                                {
                                    (yyval)=xmlStrdup(U("node()[nodeType()=1]"));
                                    xmlFree((yyvsp[-2]));
                                }
                                else if (xmlStrEqual((yyvsp[-2]),U("pi")))
                                {
                                    (yyval)=xmlStrdup(U("processing-instruction()"));
                                    xmlFree((yyvsp[-2]));
                                }
                                else if (xmlStrEqual((yyvsp[-2]),U("textnode")))
                                {
                                    (yyval)=xmlStrdup(U("text()"));
                                    xmlFree((yyvsp[-2]));
                                }
                                else
                                {
                                    (yyval)=(yyvsp[-2]);
                                    (yyval)=xmlStrcat((yyval),U("()"));
                                }
                            }
#line 1750 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 54:
#line 417 "xslpattern.y" /* yacc.c:1646  */
    {
                                (yyval)=(yyvsp[-2]);
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1761 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 58:
#line 430 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got UnionExpr: \"%s|%s\"\n", (yyvsp[-2]), (yyvsp[0]));
                                (yyval)=(yyvsp[-2]);
                                (yyval)=xmlStrcat((yyval),U("|"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1773 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 60:
#line 440 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got PathExpr: \"%s/%s\"\n", (yyvsp[-2]), (yyvsp[0]));
                                (yyval)=(yyvsp[-2]);
                                (yyval)=xmlStrcat((yyval),U("/"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1785 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 61:
#line 448 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got PathExpr: \"%s//%s\"\n", (yyvsp[-2]), (yyvsp[0]));
                                (yyval)=(yyvsp[-2]);
                                (yyval)=xmlStrcat((yyval),U("//"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1797 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 64:
#line 459 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got FilterExpr: \"%s%s\"\n", (yyvsp[-1]), (yyvsp[0]));
                                (yyval)=(yyvsp[-1]);
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1808 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 67:
#line 471 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got OrExpr: \"%s $or$ %s\"\n", (yyvsp[-2]), (yyvsp[0]));
                                (yyval)=(yyvsp[-2]);
                                (yyval)=xmlStrcat((yyval),U(" or "));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1820 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 70:
#line 483 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got AndExpr: \"%s $and$ %s\"\n", (yyvsp[-2]), (yyvsp[0]));
                                (yyval)=(yyvsp[-2]);
                                (yyval)=xmlStrcat((yyval),U(" and "));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1832 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 73:
#line 495 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got EqualityExpr: \"%s $eq$ %s\"\n", (yyvsp[-2]), (yyvsp[0]));
                                (yyval)=(yyvsp[-2]);
                                (yyval)=xmlStrcat((yyval),U("="));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1844 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 74:
#line 503 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got EqualityExpr: \"%s $ieq$ %s\"\n", (yyvsp[-2]), (yyvsp[0]));
                                (yyval)=xmlStrdup(U("OP_IEq("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[-2]));
                                xmlFree((yyvsp[-2]));
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            }
#line 1859 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 75:
#line 514 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got EqualityExpr: \"%s $ne$ %s\"\n", (yyvsp[-2]), (yyvsp[0]));
                                (yyval)=(yyvsp[-2]);
                                (yyval)=xmlStrcat((yyval),U("!="));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1871 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 76:
#line 522 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got EqualityExpr: \"%s $ine$ %s\"\n", (yyvsp[-2]), (yyvsp[0]));
                                (yyval)=xmlStrdup(U("OP_INEq("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[-2]));
                                xmlFree((yyvsp[-2]));
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            }
#line 1886 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 79:
#line 537 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got RelationalExpr: \"%s $lt$ %s\"\n", (yyvsp[-2]), (yyvsp[0]));
                                (yyval)=(yyvsp[-2]);
                                (yyval)=xmlStrcat((yyval),U("<"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1898 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 80:
#line 545 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got RelationalExpr: \"%s $ilt$ %s\"\n", (yyvsp[-2]), (yyvsp[0]));
                                (yyval)=xmlStrdup(U("OP_ILt("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[-2]));
                                xmlFree((yyvsp[-2]));
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            }
#line 1913 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 81:
#line 556 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got RelationalExpr: \"%s $gt$ %s\"\n", (yyvsp[-2]), (yyvsp[0]));
                                (yyval)=(yyvsp[-2]);
                                (yyval)=xmlStrcat((yyval),U(">"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1925 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 82:
#line 564 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got RelationalExpr: \"%s $igt$ %s\"\n", (yyvsp[-2]), (yyvsp[0]));
                                (yyval)=xmlStrdup(U("OP_IGt("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[-2]));
                                xmlFree((yyvsp[-2]));
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            }
#line 1940 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 83:
#line 575 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got RelationalExpr: \"%s $le$ %s\"\n", (yyvsp[-2]), (yyvsp[0]));
                                (yyval)=(yyvsp[-2]);
                                (yyval)=xmlStrcat((yyval),U("<="));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1952 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 84:
#line 583 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got RelationalExpr: \"%s $ile$ %s\"\n", (yyvsp[-2]), (yyvsp[0]));
                                (yyval)=xmlStrdup(U("OP_ILEq("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[-2]));
                                xmlFree((yyvsp[-2]));
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            }
#line 1967 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 85:
#line 594 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got RelationalExpr: \"%s $ge$ %s\"\n", (yyvsp[-2]), (yyvsp[0]));
                                (yyval)=(yyvsp[-2]);
                                (yyval)=xmlStrcat((yyval),U(">="));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 1979 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 86:
#line 602 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got RelationalExpr: \"%s $ige$ %s\"\n", (yyvsp[-2]), (yyvsp[0]));
                                (yyval)=xmlStrdup(U("OP_IGEq("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[-2]));
                                xmlFree((yyvsp[-2]));
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            }
#line 1994 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 89:
#line 619 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got UnaryExpr: \"$not$ %s\"\n", (yyvsp[0]));
                                (yyval)=xmlStrdup(U(" not("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            }
#line 2006 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 90:
#line 627 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got UnaryExpr: \"$any$ %s\"\n", (yyvsp[0]));
                                (yyval)=xmlStrdup(U("boolean("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            }
#line 2018 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 91:
#line 635 "xslpattern.y" /* yacc.c:1646  */
    {
                                TRACE("Got UnaryExpr: \"$all$ %s\"\n", (yyvsp[0]));
                                (yyval)=xmlStrdup(U("not("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            }
#line 2030 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 92:
#line 643 "xslpattern.y" /* yacc.c:1646  */
    {
                                FIXME("Unrecognized $all$ expression - ignoring\n");
                                (yyval)=xmlStrdup(U(""));
                            }
#line 2039 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 93:
#line 649 "xslpattern.y" /* yacc.c:1646  */
    {
                                (yyval)=(yyvsp[-2]);
                                (yyval)=xmlStrcat((yyval),U("!="));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 2050 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 94:
#line 656 "xslpattern.y" /* yacc.c:1646  */
    {
                                (yyval)=(yyvsp[-2]);
                                (yyval)=xmlStrcat((yyval),U("="));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 2061 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 95:
#line 663 "xslpattern.y" /* yacc.c:1646  */
    {
                                (yyval)=(yyvsp[-2]);
                                (yyval)=xmlStrcat((yyval),U(">="));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 2072 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 96:
#line 670 "xslpattern.y" /* yacc.c:1646  */
    {
                                (yyval)=(yyvsp[-2]);
                                (yyval)=xmlStrcat((yyval),U(">"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 2083 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 97:
#line 677 "xslpattern.y" /* yacc.c:1646  */
    {
                                (yyval)=(yyvsp[-2]);
                                (yyval)=xmlStrcat((yyval),U("<="));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 2094 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 98:
#line 684 "xslpattern.y" /* yacc.c:1646  */
    {
                                (yyval)=(yyvsp[-2]);
                                (yyval)=xmlStrcat((yyval),U("<"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                            }
#line 2105 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 99:
#line 691 "xslpattern.y" /* yacc.c:1646  */
    {
                                (yyval)=xmlStrdup(U("OP_INEq("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[-2]));
                                xmlFree((yyvsp[-2]));
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            }
#line 2119 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 100:
#line 701 "xslpattern.y" /* yacc.c:1646  */
    {
                                (yyval)=xmlStrdup(U("OP_IEq("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[-2]));
                                xmlFree((yyvsp[-2]));
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            }
#line 2133 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 101:
#line 711 "xslpattern.y" /* yacc.c:1646  */
    {
                                (yyval)=xmlStrdup(U("OP_IGEq("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[-2]));
                                xmlFree((yyvsp[-2]));
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            }
#line 2147 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 102:
#line 721 "xslpattern.y" /* yacc.c:1646  */
    {
                                (yyval)=xmlStrdup(U("OP_IGt("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[-2]));
                                xmlFree((yyvsp[-2]));
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            }
#line 2161 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 103:
#line 731 "xslpattern.y" /* yacc.c:1646  */
    {
                                (yyval)=xmlStrdup(U("OP_ILEq("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[-2]));
                                xmlFree((yyvsp[-2]));
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            }
#line 2175 "xslpattern.tab.c" /* yacc.c:1646  */
    break;

  case 104:
#line 741 "xslpattern.y" /* yacc.c:1646  */
    {
                                (yyval)=xmlStrdup(U("OP_ILt("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[-2]));
                                xmlFree((yyvsp[-2]));
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[0]));
                                xmlFree((yyvsp[0]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            }
#line 2189 "xslpattern.tab.c" /* yacc.c:1646  */
    break;


#line 2193 "xslpattern.tab.c" /* yacc.c:1646  */
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

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

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
      yyerror (p, scanner, YY_("syntax error"));
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
        yyerror (p, scanner, yymsgp);
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
                      yytoken, &yylval, p, scanner);
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
                  yystos[yystate], yyvsp, p, scanner);
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
  yyerror (p, scanner, YY_("memory exhausted"));
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
                  yytoken, &yylval, p, scanner);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, p, scanner);
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
#line 752 "xslpattern.y" /* yacc.c:1906  */


#endif  /* HAVE_LIBXML2 */
