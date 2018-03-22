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
#define yyparse         sql_parse
#define yylex           sql_lex
#define yyerror         sql_error
#define yydebug         sql_debug
#define yynerrs         sql_nerrs


/* Copy the first part of user declarations.  */
#line 1 "sql.y" /* yacc.c:339  */


/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002-2004 Mike McCormack for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */


#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "query.h"
#include "wine/list.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static UINT SQL_getstring( void *info, const struct sql_str *strdata, LPWSTR *str );
static INT SQL_getint( void *info );
static int sql_lex( void *SQL_lval, SQL_input *info );
static int sql_error( SQL_input *info, const char *str);

static LPWSTR parser_add_table( void *info, LPCWSTR list, LPCWSTR table );
static void *parser_alloc( void *info, unsigned int sz );
static column_info *parser_alloc_column( void *info, LPCWSTR table, LPCWSTR column );

static BOOL SQL_MarkPrimaryKeys( column_info **cols, column_info *keys);

static struct expr * EXPR_complex( void *info, struct expr *l, UINT op, struct expr *r );
static struct expr * EXPR_unary( void *info, struct expr *l, UINT op );
static struct expr * EXPR_column( void *info, const column_info *column );
static struct expr * EXPR_ival( void *info, int val );
static struct expr * EXPR_sval( void *info, const struct sql_str *str );
static struct expr * EXPR_wildcard( void *info );

#define PARSER_BUBBLE_UP_VIEW( sql, result, current_view ) \
    *sql->view = current_view; \
    result = current_view


#line 134 "sql.tab.c" /* yacc.c:339  */

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
   by #include "sql.tab.h".  */
#ifndef YY_SQL_E_REACTOSSYNC_GCC_DLL_WIN32_MSI_SQL_TAB_H_INCLUDED
# define YY_SQL_E_REACTOSSYNC_GCC_DLL_WIN32_MSI_SQL_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int sql_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    TK_ALTER = 258,
    TK_AND = 259,
    TK_BY = 260,
    TK_CHAR = 261,
    TK_COMMA = 262,
    TK_CREATE = 263,
    TK_DELETE = 264,
    TK_DROP = 265,
    TK_DISTINCT = 266,
    TK_DOT = 267,
    TK_EQ = 268,
    TK_FREE = 269,
    TK_FROM = 270,
    TK_GE = 271,
    TK_GT = 272,
    TK_HOLD = 273,
    TK_ADD = 274,
    TK_ID = 275,
    TK_ILLEGAL = 276,
    TK_INSERT = 277,
    TK_INT = 278,
    TK_INTEGER = 279,
    TK_INTO = 280,
    TK_IS = 281,
    TK_KEY = 282,
    TK_LE = 283,
    TK_LONG = 284,
    TK_LONGCHAR = 285,
    TK_LP = 286,
    TK_LT = 287,
    TK_LOCALIZABLE = 288,
    TK_MINUS = 289,
    TK_NE = 290,
    TK_NOT = 291,
    TK_NULL = 292,
    TK_OBJECT = 293,
    TK_OR = 294,
    TK_ORDER = 295,
    TK_PRIMARY = 296,
    TK_RP = 297,
    TK_SELECT = 298,
    TK_SET = 299,
    TK_SHORT = 300,
    TK_SPACE = 301,
    TK_STAR = 302,
    TK_STRING = 303,
    TK_TABLE = 304,
    TK_TEMPORARY = 305,
    TK_UPDATE = 306,
    TK_VALUES = 307,
    TK_WHERE = 308,
    TK_WILDCARD = 309,
    END_OF_FILE = 310,
    ILLEGAL = 311,
    SPACE = 312,
    UNCLOSED_STRING = 313,
    COMMENT = 314,
    FUNCTION = 315,
    COLUMN = 316,
    TK_LIKE = 318,
    TK_NEGATION = 319
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 68 "sql.y" /* yacc.c:355  */

    struct sql_str str;
    LPWSTR string;
    column_info *column_list;
    MSIVIEW *query;
    struct expr *expr;
    USHORT column_type;
    int integer;

#line 248 "sql.tab.c" /* yacc.c:355  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int sql_parse (SQL_input *info);

#endif /* !YY_SQL_E_REACTOSSYNC_GCC_DLL_WIN32_MSI_SQL_TAB_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 262 "sql.tab.c" /* yacc.c:358  */

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
#define YYFINAL  36
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   156

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  65
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  37
/* YYNRULES -- Number of rules.  */
#define YYNRULES  87
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  154

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   319

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
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
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   117,   117,   125,   126,   127,   128,   129,   130,   131,
     135,   146,   160,   177,   193,   204,   218,   232,   243,   254,
     268,   272,   279,   294,   304,   314,   321,   330,   334,   338,
     345,   349,   356,   360,   364,   368,   372,   376,   380,   387,
     396,   400,   415,   435,   436,   440,   447,   448,   452,   459,
     471,   484,   488,   500,   515,   519,   528,   534,   540,   546,
     552,   558,   564,   570,   576,   582,   588,   597,   598,   602,
     609,   620,   621,   629,   637,   643,   649,   655,   664,   673,
     679,   688,   694,   700,   709,   716,   724,   732
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "TK_ALTER", "TK_AND", "TK_BY", "TK_CHAR",
  "TK_COMMA", "TK_CREATE", "TK_DELETE", "TK_DROP", "TK_DISTINCT", "TK_DOT",
  "TK_EQ", "TK_FREE", "TK_FROM", "TK_GE", "TK_GT", "TK_HOLD", "TK_ADD",
  "TK_ID", "TK_ILLEGAL", "TK_INSERT", "TK_INT", "TK_INTEGER", "TK_INTO",
  "TK_IS", "TK_KEY", "TK_LE", "TK_LONG", "TK_LONGCHAR", "TK_LP", "TK_LT",
  "TK_LOCALIZABLE", "TK_MINUS", "TK_NE", "TK_NOT", "TK_NULL", "TK_OBJECT",
  "TK_OR", "TK_ORDER", "TK_PRIMARY", "TK_RP", "TK_SELECT", "TK_SET",
  "TK_SHORT", "TK_SPACE", "TK_STAR", "TK_STRING", "TK_TABLE",
  "TK_TEMPORARY", "TK_UPDATE", "TK_VALUES", "TK_WHERE", "TK_WILDCARD",
  "END_OF_FILE", "ILLEGAL", "SPACE", "UNCLOSED_STRING", "COMMENT",
  "FUNCTION", "COLUMN", "AGG_FUNCTION.", "TK_LIKE", "TK_NEGATION",
  "$accept", "query", "onequery", "oneinsert", "onecreate", "oneupdate",
  "onedelete", "onealter", "alterop", "onedrop", "table_def", "column_def",
  "column_and_type", "column_type", "data_type_l", "data_type",
  "data_count", "oneselect", "selectfrom", "selcollist", "collist", "from",
  "unorderdfrom", "tablelist", "expr", "val", "constlist",
  "update_assign_list", "column_assignment", "const_val", "column_val",
  "column", "selcolumn", "table", "id", "string", "number", YY_NULL
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
     315,   316,   317,   318,   319
};
# endif

#define YYPACT_NINF -80

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-80)))

#define YYTABLE_NINF -85

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      36,   -44,   -39,    -1,   -26,     9,    50,    37,    56,   -80,
     -80,   -80,   -80,   -80,   -80,   -80,   -80,    37,    37,    37,
     -80,    25,    37,    37,   -18,   -80,   -80,   -80,   -80,    -1,
      78,    47,    76,   -80,    57,   -80,   -80,   105,    72,    51,
      55,   100,   -80,    81,   -80,   -80,   -18,    37,    37,   -80,
     -80,    37,   -80,    37,    62,    37,   -12,   -12,   -80,   -80,
      63,   102,   108,   101,    76,    97,    45,    83,     2,   -80,
      62,     3,    94,   -80,   -80,   126,   -80,   -80,   127,    93,
      62,    37,    52,    37,   -80,   106,   -80,   -80,   -80,   -80,
     -80,   -80,    31,   103,   118,    37,   111,    11,    62,    62,
      60,    60,    60,   -11,    60,    60,    60,   -12,    88,     3,
     -80,   -80,   117,   -80,   -80,   -80,   -80,   -80,   117,   -80,
     -80,   107,   -80,   -80,   -12,   -80,   -80,   138,   -80,   -80,
     -80,   -80,   -80,   109,   -80,   -80,   -80,   -80,   -80,   112,
     -80,   110,   -80,   -80,   -80,   -80,    52,   -80,   113,   140,
      95,    52,   -80,   -80
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     2,
       5,     4,     6,     7,     8,     9,     3,     0,     0,     0,
      16,    51,     0,     0,     0,    85,    45,    86,    40,     0,
      43,     0,    82,    83,     0,    84,     1,     0,     0,    52,
      54,     0,    22,     0,    41,    42,     0,     0,     0,    21,
      20,     0,    17,     0,     0,     0,     0,     0,    44,    81,
      15,    71,     0,     0,    80,    18,     0,     0,     0,    25,
       0,    53,     0,    78,    55,    54,    48,    50,    46,     0,
       0,     0,     0,     0,    19,    32,    36,    37,    34,    38,
      35,    26,    27,    30,    12,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    14,
      72,    87,     0,    76,    77,    73,    74,    79,     0,    28,
      29,     0,    13,    24,     0,    56,    57,    58,    59,    68,
      67,    63,    60,     0,    65,    62,    61,    64,    47,     0,
      75,     0,    39,    31,    23,    66,     0,    33,     0,    69,
      10,     0,    11,    70
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -80,   -80,   -80,   -80,   -80,   -80,   -80,   -80,   -80,   -80,
     -80,   -80,   -47,   -80,   -80,   -80,   -80,   -80,   124,   104,
     -53,   120,   -80,    96,    19,    26,     5,    73,   -80,   -79,
      -7,   -29,   -80,    14,    -6,   -80,   -16
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     8,     9,    10,    11,    12,    13,    14,    52,    15,
      67,    68,    65,    91,    92,    93,   141,    16,    28,    29,
      77,    20,    21,    39,    71,   128,   148,    60,    61,   129,
     130,    73,    30,    63,    64,    33,   116
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      32,    35,    25,   115,    79,    17,    69,    98,    25,    95,
      18,    35,    35,    35,    19,    98,    35,    35,    32,    62,
      31,    34,    66,    22,    66,   133,   134,    78,    78,    26,
      27,    37,    38,    40,    23,    76,    42,    43,    31,     1,
      32,    59,    99,    96,     2,     3,     4,    72,   123,    35,
      99,    85,    62,   125,   138,   -49,    36,    25,     5,    47,
      31,    24,    55,    72,   119,    41,    66,   149,    86,    75,
      25,   144,   149,    72,    87,    88,   111,   117,    78,     6,
      25,   120,    25,    89,   111,    46,   112,     7,   -84,    97,
      90,    72,    72,    70,   112,    78,   140,    26,    27,   109,
     113,    48,   142,    53,    54,    56,   114,   100,   113,    81,
     101,   102,    57,    83,   114,    84,    80,   126,   127,    49,
     103,    82,   104,    50,    51,    94,   105,   131,   132,   106,
     135,   136,   137,    55,   107,   108,   122,   118,   124,   121,
     139,   111,    98,   146,   143,   152,   145,   151,    44,    45,
      58,    74,   147,     0,   110,   150,   153
};

static const yytype_int16 yycheck[] =
{
       6,     7,    20,    82,    57,    49,    53,     4,    20,     7,
      49,    17,    18,    19,    15,     4,    22,    23,    24,    48,
       6,     7,    51,    49,    53,    36,    37,    56,    57,    47,
      48,    17,    18,    19,    25,    47,    22,    23,    24,     3,
      46,    47,    39,    41,     8,     9,    10,    54,    95,    55,
      39,     6,    81,    42,   107,     0,     0,    20,    22,    12,
      46,    11,     7,    70,    33,    40,    95,   146,    23,    55,
      20,   124,   151,    80,    29,    30,    24,    83,   107,    43,
      20,    50,    20,    38,    24,     7,    34,    51,    12,    70,
      45,    98,    99,    31,    34,   124,   112,    47,    48,    80,
      48,    44,   118,    31,    53,     5,    54,    13,    48,     7,
      16,    17,    31,    12,    54,    18,    53,    98,    99,    14,
      26,    13,    28,    18,    19,    42,    32,   101,   102,    35,
     104,   105,   106,     7,     7,    42,    18,    31,    27,    36,
      52,    24,     4,    31,    37,    50,    37,     7,    24,    29,
      46,    55,    42,    -1,    81,    42,   151
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     8,     9,    10,    22,    43,    51,    66,    67,
      68,    69,    70,    71,    72,    74,    82,    49,    49,    15,
      86,    87,    49,    25,    11,    20,    47,    48,    83,    84,
      97,    98,    99,   100,    98,    99,     0,    98,    98,    88,
      98,    40,    98,    98,    83,    86,     7,    12,    44,    14,
      18,    19,    73,    31,    53,     7,     5,    31,    84,    99,
      92,    93,    96,    98,    99,    77,    96,    75,    76,    77,
      31,    89,    95,    96,    88,    98,    47,    85,    96,    85,
      53,     7,    13,    12,    18,     6,    23,    29,    30,    38,
      45,    78,    79,    80,    42,     7,    41,    89,     4,    39,
      13,    16,    17,    26,    28,    32,    35,     7,    42,    89,
      92,    24,    34,    48,    54,    94,   101,    99,    31,    33,
      50,    36,    18,    77,    27,    42,    89,    89,    90,    94,
      95,    90,    90,    36,    37,    90,    90,    90,    85,    52,
     101,    81,   101,    37,    85,    37,    31,    42,    91,    94,
      42,     7,    50,    91
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    65,    66,    67,    67,    67,    67,    67,    67,    67,
      68,    68,    69,    69,    70,    70,    71,    72,    72,    72,
      73,    73,    74,    75,    76,    76,    77,    78,    78,    78,
      79,    79,    80,    80,    80,    80,    80,    80,    80,    81,
      82,    82,    83,    84,    84,    84,    85,    85,    85,    86,
      86,    86,    87,    87,    88,    88,    89,    89,    89,    89,
      89,    89,    89,    89,    89,    89,    89,    90,    90,    91,
      91,    92,    92,    93,    94,    94,    94,    94,    95,    96,
      96,    97,    97,    97,    98,    99,   100,   101
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
      10,    11,     6,     7,     6,     4,     2,     4,     5,     6,
       1,     1,     3,     4,     3,     1,     2,     1,     2,     2,
       1,     3,     1,     4,     1,     1,     1,     1,     1,     1,
       2,     3,     2,     1,     3,     1,     1,     3,     1,     2,
       4,     1,     2,     4,     1,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     4,     1,     1,     1,
       3,     1,     3,     3,     1,     2,     1,     1,     1,     3,
       1,     3,     1,     1,     1,     1,     1,     1
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
      yyerror (info, YY_("syntax error: cannot back up")); \
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
                  Type, Value, info); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, SQL_input *info)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  YYUSE (info);
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
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, SQL_input *info)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, info);
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
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule, SQL_input *info)
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
                                              , info);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, info); \
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
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, SQL_input *info)
{
  YYUSE (yyvaluep);
  YYUSE (info);
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
yyparse (SQL_input *info)
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
      yychar = yylex (&yylval, info);
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
#line 118 "sql.y" /* yacc.c:1646  */
    {
        SQL_input* sql = (SQL_input*) info;
        *sql->view = (yyvsp[0].query);
    }
#line 1450 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 10:
#line 136 "sql.y" /* yacc.c:1646  */
    {
            SQL_input *sql = (SQL_input*) info;
            MSIVIEW *insert = NULL;

            INSERT_CreateView( sql->db, &insert, (yyvsp[-7].string), (yyvsp[-5].column_list), (yyvsp[-1].column_list), FALSE );
            if( !insert )
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query),  insert );
        }
#line 1465 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 11:
#line 147 "sql.y" /* yacc.c:1646  */
    {
            SQL_input *sql = (SQL_input*) info;
            MSIVIEW *insert = NULL;

            INSERT_CreateView( sql->db, &insert, (yyvsp[-8].string), (yyvsp[-6].column_list), (yyvsp[-2].column_list), TRUE );
            if( !insert )
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query),  insert );
        }
#line 1480 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 12:
#line 161 "sql.y" /* yacc.c:1646  */
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *create = NULL;
            UINT r;

            if( !(yyvsp[-1].column_list) )
                YYABORT;
            r = CREATE_CreateView( sql->db, &create, (yyvsp[-3].string), (yyvsp[-1].column_list), FALSE );
            if( !create )
            {
                sql->r = r;
                YYABORT;
            }

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query),  create );
        }
#line 1501 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 13:
#line 178 "sql.y" /* yacc.c:1646  */
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *create = NULL;

            if( !(yyvsp[-2].column_list) )
                YYABORT;
            CREATE_CreateView( sql->db, &create, (yyvsp[-4].string), (yyvsp[-2].column_list), TRUE );
            if( !create )
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query),  create );
        }
#line 1518 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 14:
#line 194 "sql.y" /* yacc.c:1646  */
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *update = NULL;

            UPDATE_CreateView( sql->db, &update, (yyvsp[-4].string), (yyvsp[-2].column_list), (yyvsp[0].expr) );
            if( !update )
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query),  update );
        }
#line 1533 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 15:
#line 205 "sql.y" /* yacc.c:1646  */
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *update = NULL;

            UPDATE_CreateView( sql->db, &update, (yyvsp[-2].string), (yyvsp[0].column_list), NULL );
            if( !update )
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query),  update );
        }
#line 1548 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 16:
#line 219 "sql.y" /* yacc.c:1646  */
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *delete = NULL;

            DELETE_CreateView( sql->db, &delete, (yyvsp[0].query) );
            if( !delete )
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query), delete );
        }
#line 1563 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 17:
#line 233 "sql.y" /* yacc.c:1646  */
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *alter = NULL;

            ALTER_CreateView( sql->db, &alter, (yyvsp[-1].string), NULL, (yyvsp[0].integer) );
            if( !alter )
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query), alter );
        }
#line 1578 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 18:
#line 244 "sql.y" /* yacc.c:1646  */
    {
            SQL_input *sql = (SQL_input *)info;
            MSIVIEW *alter = NULL;

            ALTER_CreateView( sql->db, &alter, (yyvsp[-2].string), (yyvsp[0].column_list), 0 );
            if (!alter)
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query), alter );
        }
#line 1593 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 19:
#line 255 "sql.y" /* yacc.c:1646  */
    {
            SQL_input *sql = (SQL_input *)info;
            MSIVIEW *alter = NULL;

            ALTER_CreateView( sql->db, &alter, (yyvsp[-3].string), (yyvsp[-1].column_list), 1 );
            if (!alter)
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query), alter );
        }
#line 1608 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 20:
#line 269 "sql.y" /* yacc.c:1646  */
    {
            (yyval.integer) = 1;
        }
#line 1616 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 21:
#line 273 "sql.y" /* yacc.c:1646  */
    {
            (yyval.integer) = -1;
        }
#line 1624 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 22:
#line 280 "sql.y" /* yacc.c:1646  */
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW* drop = NULL;
            UINT r;

            r = DROP_CreateView( sql->db, &drop, (yyvsp[0].string) );
            if( r != ERROR_SUCCESS || !(yyval.query) )
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query), drop );
        }
#line 1640 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 23:
#line 295 "sql.y" /* yacc.c:1646  */
    {
            if( SQL_MarkPrimaryKeys( &(yyvsp[-3].column_list), (yyvsp[0].column_list) ) )
                (yyval.column_list) = (yyvsp[-3].column_list);
            else
                (yyval.column_list) = NULL;
        }
#line 1651 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 24:
#line 305 "sql.y" /* yacc.c:1646  */
    {
            column_info *ci;

            for( ci = (yyvsp[-2].column_list); ci->next; ci = ci->next )
                ;

            ci->next = (yyvsp[0].column_list);
            (yyval.column_list) = (yyvsp[-2].column_list);
        }
#line 1665 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 25:
#line 315 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_list) = (yyvsp[0].column_list);
        }
#line 1673 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 26:
#line 322 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_list) = (yyvsp[-1].column_list);
            (yyval.column_list)->type = ((yyvsp[0].column_type) | MSITYPE_VALID);
            (yyval.column_list)->temporary = (yyvsp[0].column_type) & MSITYPE_TEMPORARY ? TRUE : FALSE;
        }
#line 1683 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 27:
#line 331 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_type) = (yyvsp[0].column_type);
        }
#line 1691 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 28:
#line 335 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_type) = (yyvsp[-1].column_type) | MSITYPE_LOCALIZABLE;
        }
#line 1699 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 29:
#line 339 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_type) = (yyvsp[-1].column_type) | MSITYPE_TEMPORARY;
        }
#line 1707 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 30:
#line 346 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_type) |= MSITYPE_NULLABLE;
        }
#line 1715 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 31:
#line 350 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_type) = (yyvsp[-2].column_type);
        }
#line 1723 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 32:
#line 357 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_type) = MSITYPE_STRING | 0x400;
        }
#line 1731 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 33:
#line 361 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_type) = MSITYPE_STRING | 0x400 | (yyvsp[-1].column_type);
        }
#line 1739 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 34:
#line 365 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_type) = MSITYPE_STRING | 0x400;
        }
#line 1747 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 35:
#line 369 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_type) = 2 | 0x400;
        }
#line 1755 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 36:
#line 373 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_type) = 2 | 0x400;
        }
#line 1763 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 37:
#line 377 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_type) = 4;
        }
#line 1771 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 38:
#line 381 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_type) = MSITYPE_STRING | MSITYPE_VALID;
        }
#line 1779 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 39:
#line 388 "sql.y" /* yacc.c:1646  */
    {
            if( ( (yyvsp[0].integer) > 255 ) || ( (yyvsp[0].integer) < 0 ) )
                YYABORT;
            (yyval.column_type) = (yyvsp[0].integer);
        }
#line 1789 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 40:
#line 397 "sql.y" /* yacc.c:1646  */
    {
            (yyval.query) = (yyvsp[0].query);
        }
#line 1797 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 41:
#line 401 "sql.y" /* yacc.c:1646  */
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW* distinct = NULL;
            UINT r;

            r = DISTINCT_CreateView( sql->db, &distinct, (yyvsp[0].query) );
            if (r != ERROR_SUCCESS)
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query), distinct );
        }
#line 1813 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 42:
#line 416 "sql.y" /* yacc.c:1646  */
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW* select = NULL;
            UINT r;

            if( (yyvsp[-1].column_list) )
            {
                r = SELECT_CreateView( sql->db, &select, (yyvsp[0].query), (yyvsp[-1].column_list) );
                if (r != ERROR_SUCCESS)
                    YYABORT;

                PARSER_BUBBLE_UP_VIEW( sql, (yyval.query), select );
            }
            else
                (yyval.query) = (yyvsp[0].query);
        }
#line 1834 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 44:
#line 437 "sql.y" /* yacc.c:1646  */
    {
            (yyvsp[-2].column_list)->next = (yyvsp[0].column_list);
        }
#line 1842 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 45:
#line 441 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_list) = NULL;
        }
#line 1850 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 47:
#line 449 "sql.y" /* yacc.c:1646  */
    {
            (yyvsp[-2].column_list)->next = (yyvsp[0].column_list);
        }
#line 1858 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 48:
#line 453 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_list) = NULL;
        }
#line 1866 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 49:
#line 460 "sql.y" /* yacc.c:1646  */
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW* table = NULL;
            UINT r;

            r = TABLE_CreateView( sql->db, (yyvsp[0].string), &table );
            if( r != ERROR_SUCCESS || !(yyval.query) )
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query), table );
        }
#line 1882 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 50:
#line 472 "sql.y" /* yacc.c:1646  */
    {
            UINT r;

            if( (yyvsp[0].column_list) )
            {
                r = (yyvsp[-3].query)->ops->sort( (yyvsp[-3].query), (yyvsp[0].column_list) );
                if ( r != ERROR_SUCCESS)
                    YYABORT;
            }

            (yyval.query) = (yyvsp[-3].query);
        }
#line 1899 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 52:
#line 489 "sql.y" /* yacc.c:1646  */
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW* where = NULL;
            UINT r;

            r = WHERE_CreateView( sql->db, &where, (yyvsp[0].string), NULL );
            if( r != ERROR_SUCCESS )
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query), where );
        }
#line 1915 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 53:
#line 501 "sql.y" /* yacc.c:1646  */
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW* where = NULL;
            UINT r;

            r = WHERE_CreateView( sql->db, &where, (yyvsp[-2].string), (yyvsp[0].expr) );
            if( r != ERROR_SUCCESS )
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query), where );
        }
#line 1931 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 54:
#line 516 "sql.y" /* yacc.c:1646  */
    {
            (yyval.string) = (yyvsp[0].string);
        }
#line 1939 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 55:
#line 520 "sql.y" /* yacc.c:1646  */
    {
            (yyval.string) = parser_add_table( info, (yyvsp[0].string), (yyvsp[-2].string) );
            if (!(yyval.string))
                YYABORT;
        }
#line 1949 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 56:
#line 529 "sql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = (yyvsp[-1].expr);
            if( !(yyval.expr) )
                YYABORT;
        }
#line 1959 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 57:
#line 535 "sql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_AND, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        }
#line 1969 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 58:
#line 541 "sql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_OR, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        }
#line 1979 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 59:
#line 547 "sql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_EQ, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        }
#line 1989 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 60:
#line 553 "sql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_GT, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        }
#line 1999 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 61:
#line 559 "sql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_LT, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        }
#line 2009 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 62:
#line 565 "sql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_LE, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        }
#line 2019 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 63:
#line 571 "sql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_GE, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        }
#line 2029 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 64:
#line 577 "sql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_NE, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        }
#line 2039 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 65:
#line 583 "sql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = EXPR_unary( info, (yyvsp[-2].expr), OP_ISNULL );
            if( !(yyval.expr) )
                YYABORT;
        }
#line 2049 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 66:
#line 589 "sql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = EXPR_unary( info, (yyvsp[-3].expr), OP_NOTNULL );
            if( !(yyval.expr) )
                YYABORT;
        }
#line 2059 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 69:
#line 603 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_list) = parser_alloc_column( info, NULL, NULL );
            if( !(yyval.column_list) )
                YYABORT;
            (yyval.column_list)->val = (yyvsp[0].expr);
        }
#line 2070 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 70:
#line 610 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_list) = parser_alloc_column( info, NULL, NULL );
            if( !(yyval.column_list) )
                YYABORT;
            (yyval.column_list)->val = (yyvsp[-2].expr);
            (yyval.column_list)->next = (yyvsp[0].column_list);
        }
#line 2082 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 72:
#line 622 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_list) = (yyvsp[-2].column_list);
            (yyval.column_list)->next = (yyvsp[0].column_list);
        }
#line 2091 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 73:
#line 630 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_list) = (yyvsp[-2].column_list);
            (yyval.column_list)->val = (yyvsp[0].expr);
        }
#line 2100 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 74:
#line 638 "sql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = EXPR_ival( info, (yyvsp[0].integer) );
            if( !(yyval.expr) )
                YYABORT;
        }
#line 2110 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 75:
#line 644 "sql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = EXPR_ival( info, -(yyvsp[0].integer) );
            if( !(yyval.expr) )
                YYABORT;
        }
#line 2120 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 76:
#line 650 "sql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = EXPR_sval( info, &(yyvsp[0].str) );
            if( !(yyval.expr) )
                YYABORT;
        }
#line 2130 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 77:
#line 656 "sql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = EXPR_wildcard( info );
            if( !(yyval.expr) )
                YYABORT;
        }
#line 2140 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 78:
#line 665 "sql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = EXPR_column( info, (yyvsp[0].column_list) );
            if( !(yyval.expr) )
                YYABORT;
        }
#line 2150 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 79:
#line 674 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_list) = parser_alloc_column( info, (yyvsp[-2].string), (yyvsp[0].string) );
            if( !(yyval.column_list) )
                YYABORT;
        }
#line 2160 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 80:
#line 680 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_list) = parser_alloc_column( info, NULL, (yyvsp[0].string) );
            if( !(yyval.column_list) )
                YYABORT;
        }
#line 2170 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 81:
#line 689 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_list) = parser_alloc_column( info, (yyvsp[-2].string), (yyvsp[0].string) );
            if( !(yyval.column_list) )
                YYABORT;
        }
#line 2180 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 82:
#line 695 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_list) = parser_alloc_column( info, NULL, (yyvsp[0].string) );
            if( !(yyval.column_list) )
                YYABORT;
        }
#line 2190 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 83:
#line 701 "sql.y" /* yacc.c:1646  */
    {
            (yyval.column_list) = parser_alloc_column( info, NULL, (yyvsp[0].string) );
            if( !(yyval.column_list) )
                YYABORT;
        }
#line 2200 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 84:
#line 710 "sql.y" /* yacc.c:1646  */
    {
            (yyval.string) = (yyvsp[0].string);
        }
#line 2208 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 85:
#line 717 "sql.y" /* yacc.c:1646  */
    {
            if ( SQL_getstring( info, &(yyvsp[0].str), &(yyval.string) ) != ERROR_SUCCESS || !(yyval.string) )
                YYABORT;
        }
#line 2217 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 86:
#line 725 "sql.y" /* yacc.c:1646  */
    {
            if ( SQL_getstring( info, &(yyvsp[0].str), &(yyval.string) ) != ERROR_SUCCESS || !(yyval.string) )
                YYABORT;
        }
#line 2226 "sql.tab.c" /* yacc.c:1646  */
    break;

  case 87:
#line 733 "sql.y" /* yacc.c:1646  */
    {
            (yyval.integer) = SQL_getint( info );
        }
#line 2234 "sql.tab.c" /* yacc.c:1646  */
    break;


#line 2238 "sql.tab.c" /* yacc.c:1646  */
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
      yyerror (info, YY_("syntax error"));
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
        yyerror (info, yymsgp);
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
                      yytoken, &yylval, info);
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
                  yystos[yystate], yyvsp, info);
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
  yyerror (info, YY_("memory exhausted"));
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
                  yytoken, &yylval, info);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, info);
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
#line 738 "sql.y" /* yacc.c:1906  */


static LPWSTR parser_add_table( void *info, LPCWSTR list, LPCWSTR table )
{
    static const WCHAR space[] = {' ',0};
    DWORD len = strlenW( list ) + strlenW( table ) + 2;
    LPWSTR ret;

    ret = parser_alloc( info, len * sizeof(WCHAR) );
    if( ret )
    {
        strcpyW( ret, list );
        strcatW( ret, space );
        strcatW( ret, table );
    }
    return ret;
}

static void *parser_alloc( void *info, unsigned int sz )
{
    SQL_input* sql = (SQL_input*) info;
    struct list *mem;

    mem = msi_alloc( sizeof (struct list) + sz );
    list_add_tail( sql->mem, mem );
    return &mem[1];
}

static column_info *parser_alloc_column( void *info, LPCWSTR table, LPCWSTR column )
{
    column_info *col;

    col = parser_alloc( info, sizeof (*col) );
    if( col )
    {
        col->table = table;
        col->column = column;
        col->val = NULL;
        col->type = 0;
        col->next = NULL;
    }

    return col;
}

static int sql_lex( void *SQL_lval, SQL_input *sql )
{
    int token, skip;
    struct sql_str * str = SQL_lval;

    do
    {
        sql->n += sql->len;
        if( ! sql->command[sql->n] )
            return 0;  /* end of input */

        /* TRACE("string : %s\n", debugstr_w(&sql->command[sql->n])); */
        sql->len = sqliteGetToken( &sql->command[sql->n], &token, &skip );
        if( sql->len==0 )
            break;
        str->data = &sql->command[sql->n];
        str->len = sql->len;
        sql->n += skip;
    }
    while( token == TK_SPACE );

    /* TRACE("token : %d (%s)\n", token, debugstr_wn(&sql->command[sql->n], sql->len)); */

    return token;
}

UINT SQL_getstring( void *info, const struct sql_str *strdata, LPWSTR *str )
{
    LPCWSTR p = strdata->data;
    UINT len = strdata->len;

    /* match quotes */
    if( ( (p[0]=='`') && (p[len-1]!='`') ) ||
        ( (p[0]=='\'') && (p[len-1]!='\'') ) )
        return ERROR_FUNCTION_FAILED;

    /* if there are quotes, remove them */
    if( ( (p[0]=='`') && (p[len-1]=='`') ) ||
        ( (p[0]=='\'') && (p[len-1]=='\'') ) )
    {
        p++;
        len -= 2;
    }
    *str = parser_alloc( info, (len + 1)*sizeof(WCHAR) );
    if( !*str )
        return ERROR_OUTOFMEMORY;
    memcpy( *str, p, len*sizeof(WCHAR) );
    (*str)[len]=0;

    return ERROR_SUCCESS;
}

INT SQL_getint( void *info )
{
    SQL_input* sql = (SQL_input*) info;
    LPCWSTR p = &sql->command[sql->n];
    INT i, r = 0;

    for( i=0; i<sql->len; i++ )
    {
        if( '0' > p[i] || '9' < p[i] )
        {
            ERR("should only be numbers here!\n");
            break;
        }
        r = (p[i]-'0') + r*10;
    }

    return r;
}

static int sql_error( SQL_input *info, const char *str )
{
    return 0;
}

static struct expr * EXPR_wildcard( void *info )
{
    struct expr *e = parser_alloc( info, sizeof *e );
    if( e )
    {
        e->type = EXPR_WILDCARD;
    }
    return e;
}

static struct expr * EXPR_complex( void *info, struct expr *l, UINT op, struct expr *r )
{
    struct expr *e = parser_alloc( info, sizeof *e );
    if( e )
    {
        e->type = EXPR_COMPLEX;
        e->u.expr.left = l;
        e->u.expr.op = op;
        e->u.expr.right = r;
    }
    return e;
}

static struct expr * EXPR_unary( void *info, struct expr *l, UINT op )
{
    struct expr *e = parser_alloc( info, sizeof *e );
    if( e )
    {
        e->type = EXPR_UNARY;
        e->u.expr.left = l;
        e->u.expr.op = op;
        e->u.expr.right = NULL;
    }
    return e;
}

static struct expr * EXPR_column( void *info, const column_info *column )
{
    struct expr *e = parser_alloc( info, sizeof *e );
    if( e )
    {
        e->type = EXPR_COLUMN;
        e->u.column.unparsed.column = column->column;
        e->u.column.unparsed.table = column->table;
    }
    return e;
}

static struct expr * EXPR_ival( void *info, int val )
{
    struct expr *e = parser_alloc( info, sizeof *e );
    if( e )
    {
        e->type = EXPR_IVAL;
        e->u.ival = val;
    }
    return e;
}

static struct expr * EXPR_sval( void *info, const struct sql_str *str )
{
    struct expr *e = parser_alloc( info, sizeof *e );
    if( e )
    {
        e->type = EXPR_SVAL;
        if( SQL_getstring( info, str, (LPWSTR *)&e->u.sval ) != ERROR_SUCCESS )
            return NULL; /* e will be freed by query destructor */
    }
    return e;
}

static void swap_columns( column_info **cols, column_info *A, int idx )
{
    column_info *preA = NULL, *preB = NULL, *B, *ptr;
    int i = 0;

    B = NULL;
    ptr = *cols;
    while( ptr )
    {
        if( i++ == idx )
            B = ptr;
        else if( !B )
            preB = ptr;

        if( ptr->next == A )
            preA = ptr;

        ptr = ptr->next;
    }

    if( preB ) preB->next = A;
    if( preA ) preA->next = B;
    ptr = A->next;
    A->next = B->next;
    B->next = ptr;
    if( idx == 0 )
      *cols = A;
}

static BOOL SQL_MarkPrimaryKeys( column_info **cols,
                                 column_info *keys )
{
    column_info *k;
    BOOL found = TRUE;
    int count;

    for( k = keys, count = 0; k && found; k = k->next, count++ )
    {
        column_info *c;
        int idx;

        found = FALSE;
        for( c = *cols, idx = 0; c && !found; c = c->next, idx++ )
        {
            if( strcmpW( k->column, c->column ) )
                continue;
            c->type |= MSITYPE_KEY;
            found = TRUE;
            if (idx != count)
                swap_columns( cols, c, count );
        }
    }

    return found;
}

UINT MSI_ParseSQL( MSIDATABASE *db, LPCWSTR command, MSIVIEW **phview,
                   struct list *mem )
{
    SQL_input sql;
    int r;

    *phview = NULL;

    sql.db = db;
    sql.command = command;
    sql.n = 0;
    sql.len = 0;
    sql.r = ERROR_BAD_QUERY_SYNTAX;
    sql.view = phview;
    sql.mem = mem;

    r = sql_parse(&sql);

    TRACE("Parse returned %d\n", r);
    if( r )
    {
        if (*sql.view)
        {
            (*sql.view)->ops->delete(*sql.view);
            *sql.view = NULL;
        }
        return sql.r;
    }

    return ERROR_SUCCESS;
}
