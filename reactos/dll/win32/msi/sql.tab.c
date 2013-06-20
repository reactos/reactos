
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
#define yyparse         sql_parse
#define yylex           sql_lex
#define yyerror         sql_error
#define yylval          sql_lval
#define yychar          sql_char
#define yydebug         sql_debug
#define yynerrs         sql_nerrs

/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 1 "sql.y"


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

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <config.h>

#include <stdarg.h>
//#include <stdio.h>
//#include <stdlib.h>

#include <windef.h>
//#include "winbase.h"
#include "query.h"
//#include "wine/list.h"
#include <wine/debug.h>
#include <wine/unicode.h>

#define YYLEX_PARAM info
#define YYPARSE_PARAM info

static int sql_error(const char *str);

WINE_DEFAULT_DEBUG_CHANNEL(msi);

typedef struct tag_SQL_input
{
    MSIDATABASE *db;
    LPCWSTR command;
    DWORD n, len;
    UINT r;
    MSIVIEW **view;  /* View structure for the resulting query.  This value
                      * tracks the view currently being created so we can free
                      * this view on syntax error.
                      */
    struct list *mem;
} SQL_input;

static UINT SQL_getstring( void *info, const struct sql_str *strdata, LPWSTR *str );
static INT SQL_getint( void *info );
static int sql_lex( void *SQL_lval, SQL_input *info );

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



/* Line 189 of yacc.c  */
#line 161 "sql.tab.c"

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
     COLUMN = 311,
     FUNCTION = 312,
     COMMENT = 313,
     UNCLOSED_STRING = 314,
     SPACE = 315,
     ILLEGAL = 316,
     END_OF_FILE = 317,
     TK_LIKE = 318,
     TK_NEGATION = 319
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 83 "sql.y"

    struct sql_str str;
    LPWSTR string;
    column_info *column_list;
    MSIVIEW *query;
    struct expr *expr;
    USHORT column_type;
    int integer;



/* Line 214 of yacc.c  */
#line 272 "sql.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 276 "sql.tab.c"

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
#define YYFINAL  36
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   156

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  65
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  37
/* YYNRULES -- Number of rules.  */
#define YYNRULES  87
/* YYNRULES -- Number of states.  */
#define YYNSTATES  154

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   319

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
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
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    13,    15,    17,
      19,    30,    42,    49,    57,    64,    69,    72,    77,    83,
      90,    92,    94,    98,   103,   107,   109,   112,   114,   117,
     120,   122,   126,   128,   133,   135,   137,   139,   141,   143,
     145,   148,   152,   155,   157,   161,   163,   165,   169,   171,
     174,   179,   181,   184,   189,   191,   195,   199,   203,   207,
     211,   215,   219,   223,   227,   231,   235,   240,   242,   244,
     246,   250,   252,   256,   260,   262,   265,   267,   269,   271,
     275,   277,   281,   283,   285,   287,   289,   291
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      66,     0,    -1,    67,    -1,    82,    -1,    69,    -1,    68,
      -1,    70,    -1,    71,    -1,    72,    -1,    74,    -1,    22,
      25,    98,    31,    85,    42,    52,    31,    91,    42,    -1,
      22,    25,    98,    31,    85,    42,    52,    31,    91,    42,
      50,    -1,     8,    49,    98,    31,    75,    42,    -1,     8,
      49,    98,    31,    75,    42,    18,    -1,    51,    98,    44,
      92,    53,    89,    -1,    51,    98,    44,    92,    -1,     9,
      86,    -1,     3,    49,    98,    73,    -1,     3,    49,    98,
      19,    77,    -1,     3,    49,    98,    19,    77,    18,    -1,
      18,    -1,    14,    -1,    10,    49,    98,    -1,    76,    41,
      27,    85,    -1,    76,     7,    77,    -1,    77,    -1,    96,
      78,    -1,    79,    -1,    79,    33,    -1,    79,    50,    -1,
      80,    -1,    80,    36,    37,    -1,     6,    -1,     6,    31,
      81,    42,    -1,    30,    -1,    45,    -1,    23,    -1,    29,
      -1,    38,    -1,   101,    -1,    43,    83,    -1,    43,    11,
      83,    -1,    84,    86,    -1,    97,    -1,    97,     7,    84,
      -1,    47,    -1,    96,    -1,    96,     7,    85,    -1,    47,
      -1,    15,    98,    -1,    87,    40,     5,    85,    -1,    87,
      -1,    15,    88,    -1,    15,    88,    53,    89,    -1,    98,
      -1,    98,     7,    88,    -1,    31,    89,    42,    -1,    89,
       4,    89,    -1,    89,    39,    89,    -1,    95,    13,    90,
      -1,    95,    17,    90,    -1,    95,    32,    90,    -1,    95,
      28,    90,    -1,    95,    16,    90,    -1,    95,    35,    90,
      -1,    95,    26,    37,    -1,    95,    26,    36,    37,    -1,
      95,    -1,    94,    -1,    94,    -1,    94,     7,    91,    -1,
      93,    -1,    93,     7,    92,    -1,    96,    13,    94,    -1,
     101,    -1,    34,   101,    -1,    48,    -1,    54,    -1,    96,
      -1,    98,    12,    99,    -1,    99,    -1,    98,    12,    99,
      -1,    99,    -1,   100,    -1,    99,    -1,    20,    -1,    48,
      -1,    24,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   132,   132,   140,   141,   142,   143,   144,   145,   146,
     150,   161,   175,   192,   208,   219,   233,   247,   258,   269,
     283,   287,   294,   309,   319,   329,   336,   345,   349,   353,
     360,   364,   371,   375,   379,   383,   387,   391,   395,   402,
     411,   415,   430,   450,   451,   455,   462,   463,   467,   474,
     486,   499,   503,   515,   530,   534,   543,   549,   555,   561,
     567,   573,   579,   585,   591,   597,   603,   612,   613,   617,
     624,   635,   636,   644,   652,   658,   664,   670,   679,   688,
     694,   703,   709,   715,   724,   731,   739,   747
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
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
  "AGG_FUNCTION.", "COLUMN", "FUNCTION", "COMMENT", "UNCLOSED_STRING",
  "SPACE", "ILLEGAL", "END_OF_FILE", "TK_LIKE", "TK_NEGATION", "$accept",
  "query", "onequery", "oneinsert", "onecreate", "oneupdate", "onedelete",
  "onealter", "alterop", "onedrop", "table_def", "column_def",
  "column_and_type", "column_type", "data_type_l", "data_type",
  "data_count", "oneselect", "selectfrom", "selcollist", "collist", "from",
  "unorderdfrom", "tablelist", "expr", "val", "constlist",
  "update_assign_list", "column_assignment", "const_val", "column_val",
  "column", "selcolumn", "table", "id", "string", "number", 0
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
     315,   316,   317,   318,   319
};
# endif

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

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
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

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
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

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     8,     9,    10,    11,    12,    13,    14,    52,    15,
      67,    68,    65,    91,    92,    93,   141,    16,    28,    29,
      77,    20,    21,    39,    71,   128,   148,    60,    61,   129,
     130,    73,    30,    63,    64,    33,   116
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -80
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

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -80,   -80,   -80,   -80,   -80,   -80,   -80,   -80,   -80,   -80,
     -80,   -80,   -47,   -80,   -80,   -80,   -80,   -80,   124,   104,
     -53,   120,   -80,    96,    19,    26,     5,    73,   -80,   -79,
      -7,   -29,   -80,    14,    -6,   -80,   -16
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -85
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
#line 133 "sql.y"
    {
        SQL_input* sql = (SQL_input*) info;
        *sql->view = (yyvsp[(1) - (1)].query);
    ;}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 151 "sql.y"
    {
            SQL_input *sql = (SQL_input*) info;
            MSIVIEW *insert = NULL;

            INSERT_CreateView( sql->db, &insert, (yyvsp[(3) - (10)].string), (yyvsp[(5) - (10)].column_list), (yyvsp[(9) - (10)].column_list), FALSE );
            if( !insert )
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query),  insert );
        ;}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 162 "sql.y"
    {
            SQL_input *sql = (SQL_input*) info;
            MSIVIEW *insert = NULL;

            INSERT_CreateView( sql->db, &insert, (yyvsp[(3) - (11)].string), (yyvsp[(5) - (11)].column_list), (yyvsp[(9) - (11)].column_list), TRUE );
            if( !insert )
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query),  insert );
        ;}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 176 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *create = NULL;
            UINT r;

            if( !(yyvsp[(5) - (6)].column_list) )
                YYABORT;
            r = CREATE_CreateView( sql->db, &create, (yyvsp[(3) - (6)].string), (yyvsp[(5) - (6)].column_list), FALSE );
            if( !create )
            {
                sql->r = r;
                YYABORT;
            }

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query),  create );
        ;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 193 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *create = NULL;

            if( !(yyvsp[(5) - (7)].column_list) )
                YYABORT;
            CREATE_CreateView( sql->db, &create, (yyvsp[(3) - (7)].string), (yyvsp[(5) - (7)].column_list), TRUE );
            if( !create )
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query),  create );
        ;}
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 209 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *update = NULL;

            UPDATE_CreateView( sql->db, &update, (yyvsp[(2) - (6)].string), (yyvsp[(4) - (6)].column_list), (yyvsp[(6) - (6)].expr) );
            if( !update )
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query),  update );
        ;}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 220 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *update = NULL;

            UPDATE_CreateView( sql->db, &update, (yyvsp[(2) - (4)].string), (yyvsp[(4) - (4)].column_list), NULL );
            if( !update )
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query),  update );
        ;}
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 234 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *delete = NULL;

            DELETE_CreateView( sql->db, &delete, (yyvsp[(2) - (2)].query) );
            if( !delete )
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query), delete );
        ;}
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 248 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *alter = NULL;

            ALTER_CreateView( sql->db, &alter, (yyvsp[(3) - (4)].string), NULL, (yyvsp[(4) - (4)].integer) );
            if( !alter )
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query), alter );
        ;}
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 259 "sql.y"
    {
            SQL_input *sql = (SQL_input *)info;
            MSIVIEW *alter = NULL;

            ALTER_CreateView( sql->db, &alter, (yyvsp[(3) - (5)].string), (yyvsp[(5) - (5)].column_list), 0 );
            if (!alter)
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query), alter );
        ;}
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 270 "sql.y"
    {
            SQL_input *sql = (SQL_input *)info;
            MSIVIEW *alter = NULL;

            ALTER_CreateView( sql->db, &alter, (yyvsp[(3) - (6)].string), (yyvsp[(5) - (6)].column_list), 1 );
            if (!alter)
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query), alter );
        ;}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 284 "sql.y"
    {
            (yyval.integer) = 1;
        ;}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 288 "sql.y"
    {
            (yyval.integer) = -1;
        ;}
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 295 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW* drop = NULL;
            UINT r;

            r = DROP_CreateView( sql->db, &drop, (yyvsp[(3) - (3)].string) );
            if( r != ERROR_SUCCESS || !(yyval.query) )
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query), drop );
        ;}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 310 "sql.y"
    {
            if( SQL_MarkPrimaryKeys( &(yyvsp[(1) - (4)].column_list), (yyvsp[(4) - (4)].column_list) ) )
                (yyval.column_list) = (yyvsp[(1) - (4)].column_list);
            else
                (yyval.column_list) = NULL;
        ;}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 320 "sql.y"
    {
            column_info *ci;

            for( ci = (yyvsp[(1) - (3)].column_list); ci->next; ci = ci->next )
                ;

            ci->next = (yyvsp[(3) - (3)].column_list);
            (yyval.column_list) = (yyvsp[(1) - (3)].column_list);
        ;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 330 "sql.y"
    {
            (yyval.column_list) = (yyvsp[(1) - (1)].column_list);
        ;}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 337 "sql.y"
    {
            (yyval.column_list) = (yyvsp[(1) - (2)].column_list);
            (yyval.column_list)->type = ((yyvsp[(2) - (2)].column_type) | MSITYPE_VALID);
            (yyval.column_list)->temporary = (yyvsp[(2) - (2)].column_type) & MSITYPE_TEMPORARY ? TRUE : FALSE;
        ;}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 346 "sql.y"
    {
            (yyval.column_type) = (yyvsp[(1) - (1)].column_type);
        ;}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 350 "sql.y"
    {
            (yyval.column_type) = (yyvsp[(1) - (2)].column_type) | MSITYPE_LOCALIZABLE;
        ;}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 354 "sql.y"
    {
            (yyval.column_type) = (yyvsp[(1) - (2)].column_type) | MSITYPE_TEMPORARY;
        ;}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 361 "sql.y"
    {
            (yyval.column_type) |= MSITYPE_NULLABLE;
        ;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 365 "sql.y"
    {
            (yyval.column_type) = (yyvsp[(1) - (3)].column_type);
        ;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 372 "sql.y"
    {
            (yyval.column_type) = MSITYPE_STRING | 1;
        ;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 376 "sql.y"
    {
            (yyval.column_type) = MSITYPE_STRING | 0x400 | (yyvsp[(3) - (4)].column_type);
        ;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 380 "sql.y"
    {
            (yyval.column_type) = MSITYPE_STRING | 0x400;
        ;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 384 "sql.y"
    {
            (yyval.column_type) = 2 | 0x400;
        ;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 388 "sql.y"
    {
            (yyval.column_type) = 2 | 0x400;
        ;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 392 "sql.y"
    {
            (yyval.column_type) = 4;
        ;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 396 "sql.y"
    {
            (yyval.column_type) = MSITYPE_STRING | MSITYPE_VALID;
        ;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 403 "sql.y"
    {
            if( ( (yyvsp[(1) - (1)].integer) > 255 ) || ( (yyvsp[(1) - (1)].integer) < 0 ) )
                YYABORT;
            (yyval.column_type) = (yyvsp[(1) - (1)].integer);
        ;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 412 "sql.y"
    {
            (yyval.query) = (yyvsp[(2) - (2)].query);
        ;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 416 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW* distinct = NULL;
            UINT r;

            r = DISTINCT_CreateView( sql->db, &distinct, (yyvsp[(3) - (3)].query) );
            if (r != ERROR_SUCCESS)
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query), distinct );
        ;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 431 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW* select = NULL;
            UINT r;

            if( (yyvsp[(1) - (2)].column_list) )
            {
                r = SELECT_CreateView( sql->db, &select, (yyvsp[(2) - (2)].query), (yyvsp[(1) - (2)].column_list) );
                if (r != ERROR_SUCCESS)
                    YYABORT;

                PARSER_BUBBLE_UP_VIEW( sql, (yyval.query), select );
            }
            else
                (yyval.query) = (yyvsp[(2) - (2)].query);
        ;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 452 "sql.y"
    {
            (yyvsp[(1) - (3)].column_list)->next = (yyvsp[(3) - (3)].column_list);
        ;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 456 "sql.y"
    {
            (yyval.column_list) = NULL;
        ;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 464 "sql.y"
    {
            (yyvsp[(1) - (3)].column_list)->next = (yyvsp[(3) - (3)].column_list);
        ;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 468 "sql.y"
    {
            (yyval.column_list) = NULL;
        ;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 475 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW* table = NULL;
            UINT r;

            r = TABLE_CreateView( sql->db, (yyvsp[(2) - (2)].string), &table );
            if( r != ERROR_SUCCESS || !(yyval.query) )
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query), table );
        ;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 487 "sql.y"
    {
            UINT r;

            if( (yyvsp[(4) - (4)].column_list) )
            {
                r = (yyvsp[(1) - (4)].query)->ops->sort( (yyvsp[(1) - (4)].query), (yyvsp[(4) - (4)].column_list) );
                if ( r != ERROR_SUCCESS)
                    YYABORT;
            }

            (yyval.query) = (yyvsp[(1) - (4)].query);
        ;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 504 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW* where = NULL;
            UINT r;

            r = WHERE_CreateView( sql->db, &where, (yyvsp[(2) - (2)].string), NULL );
            if( r != ERROR_SUCCESS )
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query), where );
        ;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 516 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW* where = NULL;
            UINT r;

            r = WHERE_CreateView( sql->db, &where, (yyvsp[(2) - (4)].string), (yyvsp[(4) - (4)].expr) );
            if( r != ERROR_SUCCESS )
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( sql, (yyval.query), where );
        ;}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 531 "sql.y"
    {
            (yyval.string) = (yyvsp[(1) - (1)].string);
        ;}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 535 "sql.y"
    {
            (yyval.string) = parser_add_table( info, (yyvsp[(3) - (3)].string), (yyvsp[(1) - (3)].string) );
            if (!(yyval.string))
                YYABORT;
        ;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 544 "sql.y"
    {
            (yyval.expr) = (yyvsp[(2) - (3)].expr);
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 550 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[(1) - (3)].expr), OP_AND, (yyvsp[(3) - (3)].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 556 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[(1) - (3)].expr), OP_OR, (yyvsp[(3) - (3)].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 562 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[(1) - (3)].expr), OP_EQ, (yyvsp[(3) - (3)].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 568 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[(1) - (3)].expr), OP_GT, (yyvsp[(3) - (3)].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 574 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[(1) - (3)].expr), OP_LT, (yyvsp[(3) - (3)].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 580 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[(1) - (3)].expr), OP_LE, (yyvsp[(3) - (3)].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 586 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[(1) - (3)].expr), OP_GE, (yyvsp[(3) - (3)].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 592 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[(1) - (3)].expr), OP_NE, (yyvsp[(3) - (3)].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 598 "sql.y"
    {
            (yyval.expr) = EXPR_unary( info, (yyvsp[(1) - (3)].expr), OP_ISNULL );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 604 "sql.y"
    {
            (yyval.expr) = EXPR_unary( info, (yyvsp[(1) - (4)].expr), OP_NOTNULL );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 618 "sql.y"
    {
            (yyval.column_list) = parser_alloc_column( info, NULL, NULL );
            if( !(yyval.column_list) )
                YYABORT;
            (yyval.column_list)->val = (yyvsp[(1) - (1)].expr);
        ;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 625 "sql.y"
    {
            (yyval.column_list) = parser_alloc_column( info, NULL, NULL );
            if( !(yyval.column_list) )
                YYABORT;
            (yyval.column_list)->val = (yyvsp[(1) - (3)].expr);
            (yyval.column_list)->next = (yyvsp[(3) - (3)].column_list);
        ;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 637 "sql.y"
    {
            (yyval.column_list) = (yyvsp[(1) - (3)].column_list);
            (yyval.column_list)->next = (yyvsp[(3) - (3)].column_list);
        ;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 645 "sql.y"
    {
            (yyval.column_list) = (yyvsp[(1) - (3)].column_list);
            (yyval.column_list)->val = (yyvsp[(3) - (3)].expr);
        ;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 653 "sql.y"
    {
            (yyval.expr) = EXPR_ival( info, (yyvsp[(1) - (1)].integer) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 659 "sql.y"
    {
            (yyval.expr) = EXPR_ival( info, -(yyvsp[(2) - (2)].integer) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 665 "sql.y"
    {
            (yyval.expr) = EXPR_sval( info, &(yyvsp[(1) - (1)].str) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 671 "sql.y"
    {
            (yyval.expr) = EXPR_wildcard( info );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 680 "sql.y"
    {
            (yyval.expr) = EXPR_column( info, (yyvsp[(1) - (1)].column_list) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 689 "sql.y"
    {
            (yyval.column_list) = parser_alloc_column( info, (yyvsp[(1) - (3)].string), (yyvsp[(3) - (3)].string) );
            if( !(yyval.column_list) )
                YYABORT;
        ;}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 695 "sql.y"
    {
            (yyval.column_list) = parser_alloc_column( info, NULL, (yyvsp[(1) - (1)].string) );
            if( !(yyval.column_list) )
                YYABORT;
        ;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 704 "sql.y"
    {
            (yyval.column_list) = parser_alloc_column( info, (yyvsp[(1) - (3)].string), (yyvsp[(3) - (3)].string) );
            if( !(yyval.column_list) )
                YYABORT;
        ;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 710 "sql.y"
    {
            (yyval.column_list) = parser_alloc_column( info, NULL, (yyvsp[(1) - (1)].string) );
            if( !(yyval.column_list) )
                YYABORT;
        ;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 716 "sql.y"
    {
            (yyval.column_list) = parser_alloc_column( info, NULL, (yyvsp[(1) - (1)].string) );
            if( !(yyval.column_list) )
                YYABORT;
        ;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 725 "sql.y"
    {
            (yyval.string) = (yyvsp[(1) - (1)].string);
        ;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 732 "sql.y"
    {
            if ( SQL_getstring( info, &(yyvsp[(1) - (1)].str), &(yyval.string) ) != ERROR_SUCCESS || !(yyval.string) )
                YYABORT;
        ;}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 740 "sql.y"
    {
            if ( SQL_getstring( info, &(yyvsp[(1) - (1)].str), &(yyval.string) ) != ERROR_SUCCESS || !(yyval.string) )
                YYABORT;
        ;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 748 "sql.y"
    {
            (yyval.integer) = SQL_getint( info );
        ;}
    break;



/* Line 1455 of yacc.c  */
#line 2488 "sql.tab.c"
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
#line 753 "sql.y"


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

    /* if there's quotes, remove them */
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

static int sql_error( const char *str )
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

