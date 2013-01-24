
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
#define yyparse         xslpattern_parse
#define yylex           xslpattern_lex
#define yyerror         xslpattern_error
#define yylval          xslpattern_lval
#define yychar          xslpattern_char
#define yydebug         xslpattern_debug
#define yynerrs         xslpattern_nerrs


/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 21 "xslpattern.y"

#include <config.h>
#include <wine/port.h>

#ifdef HAVE_LIBXML2
#include "xslpattern.h"
#include <libxml/xpathInternals.h>
#include <wine/debug.h>

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



/* Line 189 of yacc.c  */
#line 125 "xslpattern.tab.c"

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



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 195 "xslpattern.tab.c"

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
#define YYFINAL  57
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   176

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  38
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  39
/* YYNRULES -- Number of rules.  */
#define YYNRULES  103
/* YYNRULES -- Number of states.  */
#define YYNSTATES  156

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   283

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
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
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    13,    15,    17,    19,
      22,    24,    26,    28,    32,    34,    38,    41,    44,    46,
      48,    50,    53,    56,    58,    60,    62,    66,    70,    72,
      75,    77,    81,    83,    85,    87,    89,    92,    96,    98,
     100,   102,   104,   106,   108,   110,   112,   114,   118,   122,
     124,   126,   131,   135,   139,   141,   143,   145,   149,   151,
     155,   159,   161,   163,   166,   168,   170,   174,   176,   178,
     182,   184,   186,   190,   194,   198,   202,   204,   206,   210,
     214,   218,   222,   226,   230,   234,   238,   240,   242,   245,
     248,   251,   253,   257,   261,   265,   269,   273,   277,   281,
     285,   289,   293,   297
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      39,     0,    -1,    57,    -1,    41,    -1,    42,    -1,    26,
       8,    26,    -1,    26,    -1,    45,    -1,    44,    -1,     6,
      45,    -1,     6,    -1,    54,    -1,    46,    -1,    45,     6,
      46,    -1,    55,    -1,    47,    49,    51,    -1,    49,    51,
      -1,    47,    49,    -1,    49,    -1,    48,    -1,    56,    -1,
      26,     7,    -1,    29,    40,    -1,    50,    -1,    60,    -1,
      30,    -1,    26,     8,    30,    -1,    26,     8,    26,    -1,
      42,    -1,    51,    52,    -1,    52,    -1,    31,    53,    32,
      -1,    28,    -1,    58,    -1,    48,    -1,    26,    -1,     5,
      45,    -1,    45,     5,    46,    -1,     3,    -1,     4,    -1,
      66,    -1,    60,    -1,    75,    -1,    73,    -1,    71,    -1,
      69,    -1,    67,    -1,    33,    57,    34,    -1,    64,    35,
      60,    -1,    27,    -1,    28,    -1,    40,    33,    61,    34,
      -1,    40,    33,    34,    -1,    62,    36,    61,    -1,    62,
      -1,    57,    -1,    64,    -1,    63,    37,    64,    -1,    43,
      -1,    65,     6,    45,    -1,    65,     5,    45,    -1,    65,
      -1,    59,    -1,    65,    52,    -1,    68,    -1,    67,    -1,
      66,    10,    68,    -1,    70,    -1,    69,    -1,    68,     9,
      70,    -1,    72,    -1,    71,    -1,    70,    12,    72,    -1,
      70,    13,    72,    -1,    70,    14,    72,    -1,    70,    15,
      72,    -1,    74,    -1,    73,    -1,    72,    16,    74,    -1,
      72,    17,    74,    -1,    72,    18,    74,    -1,    72,    19,
      74,    -1,    72,    20,    74,    -1,    72,    21,    74,    -1,
      72,    22,    74,    -1,    72,    23,    74,    -1,    63,    -1,
      75,    -1,    11,    74,    -1,    25,    57,    -1,    24,    76,
      -1,    24,    -1,    64,    12,    64,    -1,    64,    14,    64,
      -1,    64,    16,    64,    -1,    64,    20,    64,    -1,    64,
      18,    64,    -1,    64,    22,    64,    -1,    64,    13,    64,
      -1,    64,    15,    64,    -1,    64,    17,    64,    -1,    64,
      21,    64,    -1,    64,    19,    64,    -1,    64,    23,    64,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    86,    86,    96,    97,    99,   108,   119,   120,   122,
     129,   134,   136,   137,   145,   148,   157,   164,   171,   172,
     173,   175,   182,   192,   193,   195,   200,   206,   225,   233,
     239,   241,   250,   256,   257,   258,   261,   269,   278,   283,
     292,   294,   295,   296,   297,   298,   299,   301,   309,   317,
     318,   321,   380,   411,   418,   420,   423,   424,   433,   434,
     442,   450,   452,   453,   462,   463,   465,   474,   475,   477,
     486,   487,   489,   497,   508,   516,   528,   529,   531,   539,
     550,   558,   569,   577,   588,   596,   610,   611,   613,   621,
     629,   637,   643,   650,   657,   664,   671,   678,   685,   695,
     705,   715,   725,   735
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
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
  "BoolRelationalExpr", "UnaryExpr", "BoolUnaryExpr", "AllExpr", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,    64,
      42,    91,    93,    40,    41,    33,    44,   124
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    38,    39,    40,    40,    41,    42,    43,    43,    44,
      44,    44,    45,    45,    45,    46,    46,    46,    46,    46,
      46,    47,    48,    49,    49,    50,    50,    50,    50,    51,
      51,    52,    53,    53,    53,    53,    54,    55,    56,    56,
      57,    58,    58,    58,    58,    58,    58,    59,    59,    59,
      59,    60,    60,    61,    61,    62,    63,    63,    64,    64,
      64,    64,    65,    65,    66,    66,    67,    68,    68,    69,
      70,    70,    71,    71,    71,    71,    72,    72,    73,    73,
      73,    73,    73,    73,    73,    73,    74,    74,    75,    75,
      75,    75,    76,    76,    76,    76,    76,    76,    76,    76,
      76,    76,    76,    76
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     3,     1,     1,     1,     2,
       1,     1,     1,     3,     1,     3,     2,     2,     1,     1,
       1,     2,     2,     1,     1,     1,     3,     3,     1,     2,
       1,     3,     1,     1,     1,     1,     2,     3,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     3,     3,     1,
       1,     4,     3,     3,     1,     1,     1,     3,     1,     3,
       3,     1,     1,     2,     1,     1,     3,     1,     1,     3,
       1,     1,     3,     3,     3,     3,     1,     1,     3,     3,
       3,     3,     3,     3,     3,     3,     1,     1,     2,     2,
       2,     1,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    38,    39,     0,    10,     0,    91,     0,     6,    49,
      50,     0,    25,     0,     0,     0,     3,    28,    58,     8,
       7,    12,     0,    19,    18,    23,    11,    14,    20,     2,
      62,    24,    86,    56,    61,    40,    65,    64,    68,    67,
      71,    70,    77,    76,    87,    36,     9,    88,     0,    90,
      89,    21,     0,     6,    22,     4,     0,     1,     0,     0,
       0,     6,    17,     0,    16,    30,     0,     0,     0,     0,
      63,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    27,    26,     0,
      47,    52,    55,     0,    54,    37,    13,    15,     6,    50,
      19,     0,    33,    24,     0,    46,    68,    71,    77,    87,
      29,    57,    48,    60,    59,    66,    69,    72,    73,    74,
      75,    78,    79,    80,    81,    82,    83,    84,    85,    92,
      98,    93,    99,    94,   100,    96,   102,    95,   101,    97,
     103,     5,    51,     0,    31,    53
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    64,    65,   111,    26,    27,    28,   102,
     112,    30,    31,   103,   104,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    49
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -34
static const yytype_int16 yypact[] =
{
      21,   -34,   -34,    26,    26,    21,   105,    21,     0,   -34,
     -34,   -16,   -34,    21,    16,   -12,   -34,    11,   -34,   -34,
      53,   -34,   -21,   -34,    -3,   -34,   -34,   -34,   -34,   -34,
     -34,   -34,   -14,    28,    35,    51,   -34,    64,   -34,    57,
     -34,   135,   -34,   -34,   -34,    53,    53,   -34,   127,   -34,
     -34,   -34,    -8,    95,   -34,   -34,    70,   -34,     9,    26,
      26,   104,    -3,    96,    -3,   -34,   105,   -16,    26,    26,
     -34,    21,    21,    21,    21,    21,    21,    21,    21,    21,
      21,    21,    21,    21,    21,   105,   105,   105,   105,   105,
     105,   105,   105,   105,   105,   105,   105,    80,   -34,    88,
     -34,   -34,   -34,    81,    82,   -34,   -34,    -3,    98,    84,
      85,    87,   -34,   128,    51,   117,   129,   131,   132,   133,
     -34,    28,   -34,    53,    53,   -34,    57,   135,   135,   135,
     135,   -34,   -34,   -34,   -34,   -34,   -34,   -34,   -34,    28,
      28,    28,    28,    28,    28,    28,    28,    28,    28,    28,
      28,   -34,   -34,    21,   -34,   -34
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -34,   -34,   125,   -34,    -5,   -34,   -34,    -1,     5,   -34,
      65,   115,   -34,    97,   -33,   -34,   -34,   -34,   -34,     4,
     -34,   -34,   -10,    13,   -34,   -34,    -6,   -34,   106,   107,
     100,   109,   101,   111,     2,   112,    14,   113,   -34
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -66
static const yytype_int16 yytable[] =
{
      48,    70,    45,    46,    29,    61,    55,    51,    52,    12,
      53,    50,     1,     2,     3,     4,    57,    56,    97,    47,
       5,    58,    98,    66,     1,     2,     3,     4,    63,     1,
       2,   120,     5,     6,     7,     8,     9,    10,    11,    12,
      68,    69,    13,   101,    -4,     6,     7,     8,     9,    10,
      11,    12,     8,   113,    13,    11,    12,   122,    59,    60,
     121,    71,    55,    67,   105,   106,    63,   123,   124,    73,
      74,    75,    76,    72,   120,   127,   128,   129,   130,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   131,   132,   133,   134,   135,   136,   137,   138,     1,
       2,     3,     4,    99,   100,    51,    52,     5,     1,     2,
       3,     4,    52,    -5,   151,   152,   -32,   -34,   153,   154,
       6,     7,   108,     9,   109,    11,    12,   -65,   110,    13,
     -35,     8,     9,    10,    11,    12,    54,    62,    13,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    77,    78,    79,    80,    81,    82,    83,    84,   107,
     -41,   -45,    67,   -44,   -43,   -42,   155,     0,     0,   114,
     115,   125,   116,   126,   117,   118,   119
};

static const yytype_int16 yycheck[] =
{
       6,    34,     3,     4,     0,    26,    11,     7,     8,    30,
      26,     7,     3,     4,     5,     6,     0,    13,    26,     5,
      11,    33,    30,    37,     3,     4,     5,     6,    31,     3,
       4,    64,    11,    24,    25,    26,    27,    28,    29,    30,
       5,     6,    33,    34,    33,    24,    25,    26,    27,    28,
      29,    30,    26,    63,    33,    29,    30,    67,     5,     6,
      66,    10,    67,    35,    59,    60,    31,    68,    69,    12,
      13,    14,    15,     9,   107,    73,    74,    75,    76,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    77,    78,    79,    80,    81,    82,    83,    84,     3,
       4,     5,     6,     8,    34,     7,     8,    11,     3,     4,
       5,     6,     8,    33,    26,    34,    32,    32,    36,    32,
      24,    25,    26,    27,    28,    29,    30,    10,    63,    33,
      32,    26,    27,    28,    29,    30,    11,    22,    33,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    16,    17,    18,    19,    20,    21,    22,    23,    62,
      32,    32,    35,    32,    32,    32,   153,    -1,    -1,    63,
      63,    71,    63,    72,    63,    63,    63
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
      57,     7,     8,    26,    40,    42,    57,     0,    33,     5,
       6,    26,    49,    31,    51,    52,    37,    35,     5,     6,
      52,    10,     9,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    26,    30,     8,
      34,    34,    57,    61,    62,    46,    46,    51,    26,    28,
      48,    53,    58,    60,    66,    67,    69,    71,    73,    75,
      52,    64,    60,    45,    45,    68,    70,    72,    72,    72,
      72,    74,    74,    74,    74,    74,    74,    74,    74,    64,
      64,    64,    64,    64,    64,    64,    64,    64,    64,    64,
      64,    26,    34,    36,    32,    61
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
      yyerror (p, scanner, YY_("syntax error: cannot back up")); \
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
# define YYLEX yylex (&yylval, scanner)
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
		  Type, Value, p, scanner); \
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
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, parser_param* p, void* scanner)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, p, scanner)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    parser_param* p;
    void* scanner;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (p);
  YYUSE (scanner);
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
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, parser_param* p, void* scanner)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, p, scanner)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    parser_param* p;
    void* scanner;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, p, scanner);
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
yy_reduce_print (YYSTYPE *yyvsp, int yyrule, parser_param* p, void* scanner)
#else
static void
yy_reduce_print (yyvsp, yyrule, p, scanner)
    YYSTYPE *yyvsp;
    int yyrule;
    parser_param* p;
    void* scanner;
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
		       		       , p, scanner);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule, p, scanner); \
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
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, parser_param* p, void* scanner)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, p, scanner)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    parser_param* p;
    void* scanner;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (p);
  YYUSE (scanner);

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
int yyparse (parser_param* p, void* scanner);
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
yyparse (parser_param* p, void* scanner)
#else
int
yyparse (p, scanner)
    parser_param* p;
    void* scanner;
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
#line 87 "xslpattern.y"
    {
                                p->out = (yyvsp[(1) - (1)]);
                            ;}
    break;

  case 5:

/* Line 1455 of yacc.c  */
#line 100 "xslpattern.y"
    {
                                TRACE("Got PrefixedName: \"%s:%s\"\n", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
                                (yyval)=(yyvsp[(1) - (3)]);
                                (yyval)=xmlStrcat((yyval),U(":"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                            ;}
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 109 "xslpattern.y"
    {
                                TRACE("Got UnprefixedName: \"%s\"\n", (yyvsp[(1) - (1)]));
                                (yyval)=(yyvsp[(1) - (1)]);
                            ;}
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 123 "xslpattern.y"
    {
                                TRACE("Got AbsoluteLocationPath: \"/%s\"\n", (yyvsp[(2) - (2)]));
                                (yyval)=xmlStrdup(U("/"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(2) - (2)]));
                                xmlFree((yyvsp[(2) - (2)]));
                            ;}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 130 "xslpattern.y"
    {
                                TRACE("Got AbsoluteLocationPath: \"/\"\n");
                                (yyval)=xmlStrdup(U("/"));
                            ;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 138 "xslpattern.y"
    {
                                TRACE("Got RelativeLocationPath: \"%s/%s\"\n", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
                                (yyval)=(yyvsp[(1) - (3)]);
                                (yyval)=xmlStrcat((yyval),U("/"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                            ;}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 149 "xslpattern.y"
    {
                                TRACE("Got Step: \"%s%s%s\"\n", (yyvsp[(1) - (3)]), (yyvsp[(2) - (3)]), (yyvsp[(3) - (3)]));
                                (yyval)=(yyvsp[(1) - (3)]);
                                (yyval)=xmlStrcat((yyval),(yyvsp[(2) - (3)]));
                                xmlFree((yyvsp[(2) - (3)]));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                            ;}
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 158 "xslpattern.y"
    {
                                TRACE("Got Step: \"%s%s\"\n", (yyvsp[(1) - (2)]), (yyvsp[(2) - (2)]));
                                (yyval)=(yyvsp[(1) - (2)]);
                                (yyval)=xmlStrcat((yyval),(yyvsp[(2) - (2)]));
                                xmlFree((yyvsp[(2) - (2)]));
                            ;}
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 165 "xslpattern.y"
    {
                                TRACE("Got Step: \"%s%s\"\n", (yyvsp[(1) - (2)]), (yyvsp[(2) - (2)]));
                                (yyval)=(yyvsp[(1) - (2)]);
                                (yyval)=xmlStrcat((yyval),(yyvsp[(2) - (2)]));
                                xmlFree((yyvsp[(2) - (2)]));
                            ;}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 176 "xslpattern.y"
    {
                                TRACE("Got AxisSpecifier: \"%s::\"\n", (yyvsp[(1) - (2)]));
                                (yyval)=(yyvsp[(1) - (2)]);
                                (yyval)=xmlStrcat((yyval),U("::"));
                            ;}
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 183 "xslpattern.y"
    {
                                TRACE("Got Attribute: \"@%s\"\n", (yyvsp[(2) - (2)]));
                                (yyval)=xmlStrdup(U("@"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(2) - (2)]));
                                xmlFree((yyvsp[(2) - (2)]));
                            ;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 196 "xslpattern.y"
    {
                                TRACE("Got NameTest: \"*\"\n");
                                (yyval)=xmlStrdup(U("*"));
                            ;}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 201 "xslpattern.y"
    {
                                TRACE("Got NameTest: \"%s:*\"\n", (yyvsp[(1) - (3)]));
                                (yyval)=(yyvsp[(1) - (3)]);
                                (yyval)=xmlStrcat((yyval),U(":*"));
                            ;}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 207 "xslpattern.y"
    { /* PrefixedName */
                                xmlChar const* registeredNsURI = xmlXPathNsLookup(p->ctx, (yyvsp[(1) - (3)]));
                                TRACE("Got PrefixedName: \"%s:%s\"\n", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));

                                if (registeredNsURI)
                                    (yyval)=xmlStrdup(U(""));
                                else
                                    (yyval)=xmlStrdup(NameTest_mod_pre);

                                (yyval)=xmlStrcat((yyval),(yyvsp[(1) - (3)]));
                                xmlFree((yyvsp[(1) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(":"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));

                                if (!registeredNsURI)
                                    (yyval)=xmlStrcat((yyval),NameTest_mod_post);
                            ;}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 226 "xslpattern.y"
    {
                                (yyval)=xmlStrdup(NameTest_mod_pre);
                                (yyval)=xmlStrcat((yyval),(yyvsp[(1) - (1)]));
                                xmlFree((yyvsp[(1) - (1)]));
                                (yyval)=xmlStrcat((yyval),NameTest_mod_post);
                            ;}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 234 "xslpattern.y"
    {
                                (yyval)=(yyvsp[(1) - (2)]);
                                (yyval)=xmlStrcat((yyval),(yyvsp[(2) - (2)]));
                                xmlFree((yyvsp[(2) - (2)]));
                            ;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 242 "xslpattern.y"
    {
                                TRACE("Got Predicate: \"[%s]\"\n", (yyvsp[(2) - (3)]));
                                (yyval)=xmlStrdup(U("["));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(2) - (3)]));
                                xmlFree((yyvsp[(2) - (3)]));
                                (yyval)=xmlStrcat((yyval),U("]"));
                            ;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 251 "xslpattern.y"
    {
                                (yyval)=xmlStrdup(U("index()="));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(1) - (1)]));
                                xmlFree((yyvsp[(1) - (1)]));
                            ;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 262 "xslpattern.y"
    {
                                TRACE("Got AbbreviatedAbsoluteLocationPath: \"//%s\"\n", (yyvsp[(2) - (2)]));
                                (yyval)=xmlStrdup(U("//"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(2) - (2)]));
                                xmlFree((yyvsp[(2) - (2)]));
                            ;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 270 "xslpattern.y"
    {
                                TRACE("Got AbbreviatedRelativeLocationPath: \"%s//%s\"\n", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
                                (yyval)=(yyvsp[(1) - (3)]);
                                (yyval)=xmlStrcat((yyval),U("//"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                            ;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 279 "xslpattern.y"
    {
                                TRACE("Got AbbreviatedStep: \"..\"\n");
                                (yyval)=xmlStrdup(U(".."));
                            ;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 284 "xslpattern.y"
    {
                                TRACE("Got AbbreviatedStep: \".\"\n");
                                (yyval)=xmlStrdup(U("."));
                            ;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 302 "xslpattern.y"
    {
                                TRACE("Got PrimaryExpr: \"(%s)\"\n", (yyvsp[(1) - (3)]));
                                (yyval)=xmlStrdup(U("("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(2) - (3)]));
                                xmlFree((yyvsp[(2) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            ;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 310 "xslpattern.y"
    {
                                TRACE("Got PrimaryExpr: \"%s!%s\"\n", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
                                (yyval)=(yyvsp[(1) - (3)]);
                                (yyval)=xmlStrcat((yyval),U("/"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                            ;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 322 "xslpattern.y"
    {
                                TRACE("Got FunctionCall: \"%s(%s)\"\n", (yyvsp[(1) - (4)]), (yyvsp[(3) - (4)]));
                                if (xmlStrEqual((yyvsp[(1) - (4)]),U("ancestor")))
                                {
                                    (yyval)=(yyvsp[(1) - (4)]);
                                    (yyval)=xmlStrcat((yyval),U("::"));
                                    (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (4)]));
                                    xmlFree((yyvsp[(3) - (4)]));
                                }
                                else if (xmlStrEqual((yyvsp[(1) - (4)]),U("attribute")))
                                {
                                    if (is_literal((yyvsp[(3) - (4)])))
                                    {
                                        (yyval)=xmlStrdup(U("@*[name()="));
                                        xmlFree((yyvsp[(1) - (4)]));
                                        (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (4)]));
                                        xmlFree((yyvsp[(3) - (4)]));
                                        (yyval)=xmlStrcat((yyval),U("]"));
                                    }
                                    else
                                    {
                                        /* XML_XPATH_INVALID_TYPE */
                                        (yyval)=xmlStrdup(U("error(1211, 'Error: attribute("));
                                        xmlFree((yyvsp[(1) - (4)]));
                                        (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (4)]));
                                        xmlFree((yyvsp[(3) - (4)]));
                                        (yyval)=xmlStrcat((yyval),U("): invalid argument')"));
                                    }
                                }
                                else if (xmlStrEqual((yyvsp[(1) - (4)]),U("element")))
                                {
                                    if (is_literal((yyvsp[(3) - (4)])))
                                    {
                                        (yyval)=xmlStrdup(U("node()[nodeType()=1][name()="));
                                        xmlFree((yyvsp[(1) - (4)]));
                                        (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (4)]));
                                        xmlFree((yyvsp[(3) - (4)]));
                                        (yyval)=xmlStrcat((yyval),U("]"));
                                    }
                                    else
                                    {
                                        /* XML_XPATH_INVALID_TYPE */
                                        (yyval)=xmlStrdup(U("error(1211, 'Error: element("));
                                        xmlFree((yyvsp[(1) - (4)]));
                                        (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (4)]));
                                        xmlFree((yyvsp[(3) - (4)]));
                                        (yyval)=xmlStrcat((yyval),U("): invalid argument')"));
                                    }
                                }
                                else
                                {
                                    (yyval)=(yyvsp[(1) - (4)]);
                                    (yyval)=xmlStrcat((yyval),U("("));
                                    (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (4)]));
                                    xmlFree((yyvsp[(3) - (4)]));
                                    (yyval)=xmlStrcat((yyval),U(")"));
                                }
                            ;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 381 "xslpattern.y"
    {
                                TRACE("Got FunctionCall: \"%s()\"\n", (yyvsp[(1) - (3)]));
                                /* comment() & node() work the same in XPath */
                                if (xmlStrEqual((yyvsp[(1) - (3)]),U("attribute")))
                                {
                                    (yyval)=xmlStrdup(U("@*"));
                                    xmlFree((yyvsp[(1) - (3)]));
                                }
                                else if (xmlStrEqual((yyvsp[(1) - (3)]),U("element")))
                                {
                                    (yyval)=xmlStrdup(U("node()[nodeType()=1]"));
                                    xmlFree((yyvsp[(1) - (3)]));
                                }
                                else if (xmlStrEqual((yyvsp[(1) - (3)]),U("pi")))
                                {
                                    (yyval)=xmlStrdup(U("processing-instruction()"));
                                    xmlFree((yyvsp[(1) - (3)]));
                                }
                                else if (xmlStrEqual((yyvsp[(1) - (3)]),U("textnode")))
                                {
                                    (yyval)=xmlStrdup(U("text()"));
                                    xmlFree((yyvsp[(1) - (3)]));
                                }
                                else
                                {
                                    (yyval)=(yyvsp[(1) - (3)]);
                                    (yyval)=xmlStrcat((yyval),U("()"));
                                }
                            ;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 412 "xslpattern.y"
    {
                                (yyval)=(yyvsp[(1) - (3)]);
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                            ;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 425 "xslpattern.y"
    {
                                TRACE("Got UnionExpr: \"%s|%s\"\n", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
                                (yyval)=(yyvsp[(1) - (3)]);
                                (yyval)=xmlStrcat((yyval),U("|"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                            ;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 435 "xslpattern.y"
    {
                                TRACE("Got PathExpr: \"%s/%s\"\n", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
                                (yyval)=(yyvsp[(1) - (3)]);
                                (yyval)=xmlStrcat((yyval),U("/"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                            ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 443 "xslpattern.y"
    {
                                TRACE("Got PathExpr: \"%s//%s\"\n", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
                                (yyval)=(yyvsp[(1) - (3)]);
                                (yyval)=xmlStrcat((yyval),U("//"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                            ;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 454 "xslpattern.y"
    {
                                TRACE("Got FilterExpr: \"%s%s\"\n", (yyvsp[(1) - (2)]), (yyvsp[(2) - (2)]));
                                (yyval)=(yyvsp[(1) - (2)]);
                                (yyval)=xmlStrcat((yyval),(yyvsp[(2) - (2)]));
                                xmlFree((yyvsp[(2) - (2)]));
                            ;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 466 "xslpattern.y"
    {
                                TRACE("Got OrExpr: \"%s $or$ %s\"\n", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
                                (yyval)=(yyvsp[(1) - (3)]);
                                (yyval)=xmlStrcat((yyval),U(" or "));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                            ;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 478 "xslpattern.y"
    {
                                TRACE("Got AndExpr: \"%s $and$ %s\"\n", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
                                (yyval)=(yyvsp[(1) - (3)]);
                                (yyval)=xmlStrcat((yyval),U(" and "));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                            ;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 490 "xslpattern.y"
    {
                                TRACE("Got EqualityExpr: \"%s $eq$ %s\"\n", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
                                (yyval)=(yyvsp[(1) - (3)]);
                                (yyval)=xmlStrcat((yyval),U("="));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                            ;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 498 "xslpattern.y"
    {
                                TRACE("Got EqualityExpr: \"%s $ieq$ %s\"\n", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
                                (yyval)=xmlStrdup(U("OP_IEq("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(1) - (3)]));
                                xmlFree((yyvsp[(1) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            ;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 509 "xslpattern.y"
    {
                                TRACE("Got EqualityExpr: \"%s $ne$ %s\"\n", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
                                (yyval)=(yyvsp[(1) - (3)]);
                                (yyval)=xmlStrcat((yyval),U("!="));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                            ;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 517 "xslpattern.y"
    {
                                TRACE("Got EqualityExpr: \"%s $ine$ %s\"\n", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
                                (yyval)=xmlStrdup(U("OP_INEq("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(1) - (3)]));
                                xmlFree((yyvsp[(1) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            ;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 532 "xslpattern.y"
    {
                                TRACE("Got RelationalExpr: \"%s $lt$ %s\"\n", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
                                (yyval)=(yyvsp[(1) - (3)]);
                                (yyval)=xmlStrcat((yyval),U("<"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                            ;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 540 "xslpattern.y"
    {
                                TRACE("Got RelationalExpr: \"%s $ilt$ %s\"\n", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
                                (yyval)=xmlStrdup(U("OP_ILt("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(1) - (3)]));
                                xmlFree((yyvsp[(1) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            ;}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 551 "xslpattern.y"
    {
                                TRACE("Got RelationalExpr: \"%s $gt$ %s\"\n", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
                                (yyval)=(yyvsp[(1) - (3)]);
                                (yyval)=xmlStrcat((yyval),U(">"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                            ;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 559 "xslpattern.y"
    {
                                TRACE("Got RelationalExpr: \"%s $igt$ %s\"\n", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
                                (yyval)=xmlStrdup(U("OP_IGt("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(1) - (3)]));
                                xmlFree((yyvsp[(1) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            ;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 570 "xslpattern.y"
    {
                                TRACE("Got RelationalExpr: \"%s $le$ %s\"\n", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
                                (yyval)=(yyvsp[(1) - (3)]);
                                (yyval)=xmlStrcat((yyval),U("<="));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                            ;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 578 "xslpattern.y"
    {
                                TRACE("Got RelationalExpr: \"%s $ile$ %s\"\n", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
                                (yyval)=xmlStrdup(U("OP_ILEq("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(1) - (3)]));
                                xmlFree((yyvsp[(1) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            ;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 589 "xslpattern.y"
    {
                                TRACE("Got RelationalExpr: \"%s $ge$ %s\"\n", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
                                (yyval)=(yyvsp[(1) - (3)]);
                                (yyval)=xmlStrcat((yyval),U(">="));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                            ;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 597 "xslpattern.y"
    {
                                TRACE("Got RelationalExpr: \"%s $ige$ %s\"\n", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
                                (yyval)=xmlStrdup(U("OP_IGEq("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(1) - (3)]));
                                xmlFree((yyvsp[(1) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            ;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 614 "xslpattern.y"
    {
                                TRACE("Got UnaryExpr: \"$not$ %s\"\n", (yyvsp[(2) - (2)]));
                                (yyval)=xmlStrdup(U(" not("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(2) - (2)]));
                                xmlFree((yyvsp[(2) - (2)]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            ;}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 622 "xslpattern.y"
    {
                                TRACE("Got UnaryExpr: \"$any$ %s\"\n", (yyvsp[(2) - (2)]));
                                (yyval)=xmlStrdup(U("boolean("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(2) - (2)]));
                                xmlFree((yyvsp[(2) - (2)]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            ;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 630 "xslpattern.y"
    {
                                TRACE("Got UnaryExpr: \"$all$ %s\"\n", (yyvsp[(2) - (2)]));
                                (yyval)=xmlStrdup(U("not("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(2) - (2)]));
                                xmlFree((yyvsp[(2) - (2)]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            ;}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 638 "xslpattern.y"
    {
                                FIXME("Unrecognized $all$ expression - ignoring\n");
                                (yyval)=xmlStrdup(U(""));
                            ;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 644 "xslpattern.y"
    {
                                (yyval)=(yyvsp[(1) - (3)]);
                                (yyval)=xmlStrcat((yyval),U("!="));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                            ;}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 651 "xslpattern.y"
    {
                                (yyval)=(yyvsp[(1) - (3)]);
                                (yyval)=xmlStrcat((yyval),U("="));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                            ;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 658 "xslpattern.y"
    {
                                (yyval)=(yyvsp[(1) - (3)]);
                                (yyval)=xmlStrcat((yyval),U(">="));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                            ;}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 665 "xslpattern.y"
    {
                                (yyval)=(yyvsp[(1) - (3)]);
                                (yyval)=xmlStrcat((yyval),U(">"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                            ;}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 672 "xslpattern.y"
    {
                                (yyval)=(yyvsp[(1) - (3)]);
                                (yyval)=xmlStrcat((yyval),U("<="));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                            ;}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 679 "xslpattern.y"
    {
                                (yyval)=(yyvsp[(1) - (3)]);
                                (yyval)=xmlStrcat((yyval),U("<"));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                            ;}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 686 "xslpattern.y"
    {
                                (yyval)=xmlStrdup(U("OP_INEq("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(1) - (3)]));
                                xmlFree((yyvsp[(1) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            ;}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 696 "xslpattern.y"
    {
                                (yyval)=xmlStrdup(U("OP_IEq("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(1) - (3)]));
                                xmlFree((yyvsp[(1) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            ;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 706 "xslpattern.y"
    {
                                (yyval)=xmlStrdup(U("OP_IGEq("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(1) - (3)]));
                                xmlFree((yyvsp[(1) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            ;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 716 "xslpattern.y"
    {
                                (yyval)=xmlStrdup(U("OP_IGt("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(1) - (3)]));
                                xmlFree((yyvsp[(1) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            ;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 726 "xslpattern.y"
    {
                                (yyval)=xmlStrdup(U("OP_ILEq("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(1) - (3)]));
                                xmlFree((yyvsp[(1) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            ;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 736 "xslpattern.y"
    {
                                (yyval)=xmlStrdup(U("OP_ILt("));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(1) - (3)]));
                                xmlFree((yyvsp[(1) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(","));
                                (yyval)=xmlStrcat((yyval),(yyvsp[(3) - (3)]));
                                xmlFree((yyvsp[(3) - (3)]));
                                (yyval)=xmlStrcat((yyval),U(")"));
                            ;}
    break;



/* Line 1455 of yacc.c  */
#line 2423 "xslpattern.tab.c"
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
      yyerror (p, scanner, YY_("syntax error"));
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
	    yyerror (p, scanner, yymsg);
	  }
	else
	  {
	    yyerror (p, scanner, YY_("syntax error"));
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
		  yystos[yystate], yyvsp, p, scanner);
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
  yyerror (p, scanner, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, p, scanner);
  /* Do not reclaim the symbols of the rule which action triggered
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
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 747 "xslpattern.y"


#endif  /* HAVE_LIBXML2 */

