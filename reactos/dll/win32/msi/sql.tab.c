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
#define yyparse SQL_parse
#define yylex   SQL_lex
#define yyerror SQL_error
#define yylval  SQL_lval
#define yychar  SQL_char
#define yydebug SQL_debug
#define yynerrs SQL_nerrs


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
     TK_DISTINCT = 265,
     TK_DOT = 266,
     TK_EQ = 267,
     TK_FREE = 268,
     TK_FROM = 269,
     TK_GE = 270,
     TK_GT = 271,
     TK_HOLD = 272,
     TK_ID = 273,
     TK_ILLEGAL = 274,
     TK_INSERT = 275,
     TK_INT = 276,
     TK_INTEGER = 277,
     TK_INTO = 278,
     TK_IS = 279,
     TK_KEY = 280,
     TK_LE = 281,
     TK_LONG = 282,
     TK_LONGCHAR = 283,
     TK_LP = 284,
     TK_LT = 285,
     TK_LOCALIZABLE = 286,
     TK_MINUS = 287,
     TK_NE = 288,
     TK_NOT = 289,
     TK_NULL = 290,
     TK_OBJECT = 291,
     TK_OR = 292,
     TK_ORDER = 293,
     TK_PRIMARY = 294,
     TK_RP = 295,
     TK_SELECT = 296,
     TK_SET = 297,
     TK_SHORT = 298,
     TK_SPACE = 299,
     TK_STAR = 300,
     TK_STRING = 301,
     TK_TABLE = 302,
     TK_TEMPORARY = 303,
     TK_UPDATE = 304,
     TK_VALUES = 305,
     TK_WHERE = 306,
     TK_WILDCARD = 307,
     COLUMN = 309,
     FUNCTION = 310,
     COMMENT = 311,
     UNCLOSED_STRING = 312,
     SPACE = 313,
     ILLEGAL = 314,
     END_OF_FILE = 315,
     TK_LIKE = 316,
     TK_NEGATION = 317
   };
#endif
/* Tokens.  */
#define TK_ALTER 258
#define TK_AND 259
#define TK_BY 260
#define TK_CHAR 261
#define TK_COMMA 262
#define TK_CREATE 263
#define TK_DELETE 264
#define TK_DISTINCT 265
#define TK_DOT 266
#define TK_EQ 267
#define TK_FREE 268
#define TK_FROM 269
#define TK_GE 270
#define TK_GT 271
#define TK_HOLD 272
#define TK_ID 273
#define TK_ILLEGAL 274
#define TK_INSERT 275
#define TK_INT 276
#define TK_INTEGER 277
#define TK_INTO 278
#define TK_IS 279
#define TK_KEY 280
#define TK_LE 281
#define TK_LONG 282
#define TK_LONGCHAR 283
#define TK_LP 284
#define TK_LT 285
#define TK_LOCALIZABLE 286
#define TK_MINUS 287
#define TK_NE 288
#define TK_NOT 289
#define TK_NULL 290
#define TK_OBJECT 291
#define TK_OR 292
#define TK_ORDER 293
#define TK_PRIMARY 294
#define TK_RP 295
#define TK_SELECT 296
#define TK_SET 297
#define TK_SHORT 298
#define TK_SPACE 299
#define TK_STAR 300
#define TK_STRING 301
#define TK_TABLE 302
#define TK_TEMPORARY 303
#define TK_UPDATE 304
#define TK_VALUES 305
#define TK_WHERE 306
#define TK_WILDCARD 307
#define COLUMN 309
#define FUNCTION 310
#define COMMENT 311
#define UNCLOSED_STRING 312
#define SPACE 313
#define ILLEGAL 314
#define END_OF_FILE 315
#define TK_LIKE 316
#define TK_NEGATION 317




/* Copy the first part of user declarations.  */
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


#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "query.h"
#include "wine/list.h"
#include "wine/debug.h"

#define YYLEX_PARAM info
#define YYPARSE_PARAM info

static int SQL_error(const char *str);

WINE_DEFAULT_DEBUG_CHANNEL(msi);

typedef struct tag_SQL_input
{
    MSIDATABASE *db;
    LPCWSTR command;
    DWORD n, len;
    MSIVIEW **view;  /* view structure for the resulting query */
    struct list *mem;
} SQL_input;

static LPWSTR SQL_getstring( void *info, struct sql_str *str );
static INT SQL_getint( void *info );
static int SQL_lex( void *SQL_lval, SQL_input *info );

static void *parser_alloc( void *info, unsigned int sz );
static column_info *parser_alloc_column( void *info, LPCWSTR table, LPCWSTR column );

static BOOL SQL_MarkPrimaryKeys( column_info *cols, column_info *keys);

static struct expr * EXPR_complex( void *info, struct expr *l, UINT op, struct expr *r );
static struct expr * EXPR_unary( void *info, struct expr *l, UINT op );
static struct expr * EXPR_column( void *info, column_info *column );
static struct expr * EXPR_ival( void *info, int val );
static struct expr * EXPR_sval( void *info, struct sql_str * );
static struct expr * EXPR_wildcard( void *info );



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
#line 73 "sql.y"
typedef union YYSTYPE {
    struct sql_str str;
    LPWSTR string;
    column_info *column_list;
    MSIVIEW *query;
    struct expr *expr;
    USHORT column_type;
    int integer;
} YYSTYPE;
/* Line 196 of yacc.c.  */
#line 294 "sql.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */
#line 306 "sql.tab.c"

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
#define YYFINAL  32
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   133

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  63
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  33
/* YYNRULES -- Number of rules. */
#define YYNRULES  75
/* YYNRULES -- Number of states. */
#define YYNSTATES  136

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   317

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
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
      55,    56,    57,    58,    59,    60,    61,    62
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    13,    15,    17,
      28,    40,    47,    55,    62,    67,    70,    75,    77,    79,
      84,    88,    90,    93,    95,    98,   101,   103,   107,   109,
     114,   116,   118,   120,   122,   124,   126,   131,   133,   136,
     140,   143,   145,   149,   151,   153,   157,   160,   165,   169,
     173,   177,   181,   185,   189,   193,   197,   201,   205,   210,
     212,   214,   216,   220,   222,   226,   230,   232,   235,   237,
     239,   241,   245,   247,   249,   251
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      64,     0,    -1,    65,    -1,    79,    -1,    67,    -1,    66,
      -1,    68,    -1,    69,    -1,    70,    -1,    20,    23,    93,
      29,    82,    40,    50,    29,    87,    40,    -1,    20,    23,
      93,    29,    82,    40,    50,    29,    87,    40,    48,    -1,
       8,    47,    93,    29,    72,    40,    -1,     8,    47,    93,
      29,    72,    40,    17,    -1,    49,    93,    42,    88,    51,
      85,    -1,    49,    93,    42,    88,    -1,     9,    83,    -1,
       3,    47,    93,    71,    -1,    17,    -1,    13,    -1,    73,
      39,    25,    82,    -1,    73,     7,    74,    -1,    74,    -1,
      92,    75,    -1,    76,    -1,    76,    31,    -1,    76,    48,
      -1,    77,    -1,    77,    34,    35,    -1,     6,    -1,     6,
      29,    78,    40,    -1,    28,    -1,    43,    -1,    21,    -1,
      27,    -1,    36,    -1,    95,    -1,    80,    38,     5,    82,
      -1,    80,    -1,    41,    81,    -1,    41,    10,    81,    -1,
      82,    83,    -1,    92,    -1,    92,     7,    82,    -1,    45,
      -1,    84,    -1,    84,    51,    85,    -1,    14,    93,    -1,
      14,    93,     7,    93,    -1,    29,    85,    40,    -1,    85,
       4,    85,    -1,    85,    37,    85,    -1,    91,    12,    86,
      -1,    91,    16,    86,    -1,    91,    30,    86,    -1,    91,
      26,    86,    -1,    91,    15,    86,    -1,    91,    33,    86,
      -1,    91,    24,    35,    -1,    91,    24,    34,    35,    -1,
      91,    -1,    90,    -1,    90,    -1,    90,     7,    87,    -1,
      89,    -1,    89,     7,    88,    -1,    92,    12,    90,    -1,
      95,    -1,    32,    95,    -1,    46,    -1,    52,    -1,    92,
      -1,    93,    11,    94,    -1,    94,    -1,    94,    -1,    18,
      -1,    22,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   123,   123,   131,   132,   133,   134,   135,   136,   140,
     151,   164,   176,   191,   201,   214,   227,   240,   244,   251,
     261,   271,   278,   286,   290,   294,   301,   305,   312,   316,
     320,   324,   328,   332,   336,   343,   352,   364,   368,   372,
     388,   409,   410,   414,   421,   422,   438,   448,   461,   467,
     473,   479,   485,   491,   497,   503,   509,   515,   521,   530,
     531,   535,   542,   553,   554,   562,   570,   576,   582,   588,
     597,   606,   612,   621,   628,   637
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "TK_ALTER", "TK_AND", "TK_BY", "TK_CHAR",
  "TK_COMMA", "TK_CREATE", "TK_DELETE", "TK_DISTINCT", "TK_DOT", "TK_EQ",
  "TK_FREE", "TK_FROM", "TK_GE", "TK_GT", "TK_HOLD", "TK_ID", "TK_ILLEGAL",
  "TK_INSERT", "TK_INT", "TK_INTEGER", "TK_INTO", "TK_IS", "TK_KEY",
  "TK_LE", "TK_LONG", "TK_LONGCHAR", "TK_LP", "TK_LT", "TK_LOCALIZABLE",
  "TK_MINUS", "TK_NE", "TK_NOT", "TK_NULL", "TK_OBJECT", "TK_OR",
  "TK_ORDER", "TK_PRIMARY", "TK_RP", "TK_SELECT", "TK_SET", "TK_SHORT",
  "TK_SPACE", "TK_STAR", "TK_STRING", "TK_TABLE", "TK_TEMPORARY",
  "TK_UPDATE", "TK_VALUES", "TK_WHERE", "TK_WILDCARD", "AGG_FUNCTION.",
  "COLUMN", "FUNCTION", "COMMENT", "UNCLOSED_STRING", "SPACE", "ILLEGAL",
  "END_OF_FILE", "TK_LIKE", "TK_NEGATION", "$accept", "query", "onequery",
  "oneinsert", "onecreate", "oneupdate", "onedelete", "onealter",
  "alterop", "table_def", "column_def", "column_and_type", "column_type",
  "data_type_l", "data_type", "data_count", "oneselect", "unorderedsel",
  "selectfrom", "selcollist", "from", "fromtable", "expr", "val",
  "constlist", "update_assign_list", "column_assignment", "const_val",
  "column_val", "column", "table", "id", "number", 0
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
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    63,    64,    65,    65,    65,    65,    65,    65,    66,
      66,    67,    67,    68,    68,    69,    70,    71,    71,    72,
      73,    73,    74,    75,    75,    75,    76,    76,    77,    77,
      77,    77,    77,    77,    77,    78,    79,    79,    80,    80,
      81,    82,    82,    82,    83,    83,    84,    84,    85,    85,
      85,    85,    85,    85,    85,    85,    85,    85,    85,    86,
      86,    87,    87,    88,    88,    89,    90,    90,    90,    90,
      91,    92,    92,    93,    94,    95
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,    10,
      11,     6,     7,     6,     4,     2,     4,     1,     1,     4,
       3,     1,     2,     1,     2,     2,     1,     3,     1,     4,
       1,     1,     1,     1,     1,     1,     4,     1,     2,     3,
       2,     1,     3,     1,     1,     3,     2,     4,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     4,     1,
       1,     1,     3,     1,     3,     3,     1,     2,     1,     1,
       1,     3,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     2,     5,
       4,     6,     7,     8,     3,    37,     0,     0,     0,    15,
      44,     0,     0,    74,    43,    38,     0,    41,     0,    72,
       0,    73,     1,     0,     0,     0,    46,     0,     0,    39,
      40,     0,     0,     0,     0,    18,    17,    16,     0,     0,
       0,    45,     0,    70,     0,    42,    71,    14,    63,     0,
      36,     0,     0,    21,     0,    47,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      11,     0,     0,    28,    32,    33,    30,    34,    31,    22,
      23,    26,    48,    49,    50,    75,     0,    68,    69,    51,
      60,    59,    66,    55,    52,     0,    57,    54,    53,    56,
       0,    13,    64,    65,    12,    20,     0,     0,    24,    25,
       0,    67,    58,     0,    19,     0,    35,    27,     0,    29,
       0,    61,     9,     0,    10,    62
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,     7,     8,     9,    10,    11,    12,    13,    47,    61,
      62,    63,    89,    90,    91,   125,    14,    15,    25,    26,
      19,    20,    51,    99,   130,    57,    58,   100,   101,    53,
      28,    29,   102
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -89
static const yysigned_char yypact[] =
{
       3,   -34,   -28,    31,    25,    48,    36,    60,   -89,   -89,
     -89,   -89,   -89,   -89,   -89,    32,    36,    36,    36,   -89,
      28,    36,    12,   -89,   -89,   -89,    31,    81,    78,    79,
      49,   -89,   -89,    87,     1,    69,    92,    51,    74,   -89,
     -89,    12,    36,    36,    12,   -89,   -89,   -89,    36,    36,
      51,     5,    71,   -89,    12,   -89,   -89,    54,    93,    94,
     -89,    67,    -4,   -89,    35,   -89,     0,    51,    51,    50,
      50,    50,    -2,    50,    50,    50,    68,    51,    36,    29,
      98,    36,    91,    88,   -89,   -89,   -89,   -89,   -89,   -89,
     -24,    84,   -89,   -89,   115,   -89,    99,   -89,   -89,   -89,
     -89,   -89,   -89,   -89,   -89,    85,   -89,   -89,   -89,   -89,
      72,     5,   -89,   -89,   -89,   -89,    12,    99,   -89,   -89,
      89,   -89,   -89,    96,   -89,    83,   -89,   -89,    29,   -89,
      86,   120,    80,    29,   -89,   -89
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -89,   -89,   -89,   -89,   -89,   -89,   -89,   -89,   -89,   -89,
     -89,    52,   -89,   -89,   -89,   -89,   -89,   -89,   107,   -39,
     104,   -89,    17,    39,    -1,    53,   -89,   -78,    -3,    -5,
      10,     4,   -88
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -74
static const short int yytable[] =
{
      27,   113,    55,    81,    67,    60,     1,   118,   121,    67,
      31,     2,     3,    16,    45,    76,    30,    27,    46,    17,
      31,    31,    31,     4,   119,    31,    34,    35,    36,   126,
      23,    38,   105,   106,    52,    82,    27,    68,    59,    27,
      92,    83,    68,    64,     5,    18,    56,    52,    21,    27,
     131,    95,     6,    31,    23,   131,    84,    24,    22,    65,
      32,    96,    85,    86,    52,    52,    23,    66,    23,    23,
      33,    87,    95,    59,    52,    97,    64,   124,    88,    37,
      50,    98,    96,    69,    93,    94,    70,    71,    41,    42,
     -73,    43,    44,    24,   111,    72,    97,    73,    48,    49,
      78,    74,    98,    54,    75,    77,    79,    80,   110,   103,
     104,    27,   107,   108,   109,   114,   116,   117,   120,    67,
     122,    95,   123,   129,   127,   128,   132,   133,   134,    39,
      40,   112,   135,   115
};

static const unsigned char yycheck[] =
{
       5,    79,    41,     7,     4,    44,     3,    31,    96,     4,
       6,     8,     9,    47,    13,    54,     6,    22,    17,    47,
      16,    17,    18,    20,    48,    21,    16,    17,    18,   117,
      18,    21,    34,    35,    37,    39,    41,    37,    43,    44,
      40,     6,    37,    48,    41,    14,    42,    50,    23,    54,
     128,    22,    49,    49,    18,   133,    21,    45,    10,    49,
       0,    32,    27,    28,    67,    68,    18,    50,    18,    18,
      38,    36,    22,    78,    77,    46,    81,   116,    43,    51,
      29,    52,    32,    12,    67,    68,    15,    16,     7,    11,
      11,    42,     5,    45,    77,    24,    46,    26,    29,     7,
       7,    30,    52,    29,    33,    51,    12,    40,    40,    70,
      71,   116,    73,    74,    75,    17,    25,    29,    34,     4,
      35,    22,    50,    40,    35,    29,    40,     7,    48,    22,
      26,    78,   133,    81
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     3,     8,     9,    20,    41,    49,    64,    65,    66,
      67,    68,    69,    70,    79,    80,    47,    47,    14,    83,
      84,    23,    10,    18,    45,    81,    82,    92,    93,    94,
      93,    94,     0,    38,    93,    93,    93,    51,    93,    81,
      83,     7,    11,    42,     5,    13,    17,    71,    29,     7,
      29,    85,    91,    92,    29,    82,    94,    88,    89,    92,
      82,    72,    73,    74,    92,    93,    85,     4,    37,    12,
      15,    16,    24,    26,    30,    33,    82,    51,     7,    12,
      40,     7,    39,     6,    21,    27,    28,    36,    43,    75,
      76,    77,    40,    85,    85,    22,    32,    46,    52,    86,
      90,    91,    95,    86,    86,    34,    35,    86,    86,    86,
      40,    85,    88,    90,    17,    74,    25,    29,    31,    48,
      34,    95,    35,    50,    82,    78,    95,    35,    29,    40,
      87,    90,    40,     7,    48,    87
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
#line 124 "sql.y"
    {
        SQL_input* sql = (SQL_input*) info;
        *sql->view = (yyvsp[0].query);
    ;}
    break;

  case 9:
#line 141 "sql.y"
    {
            SQL_input *sql = (SQL_input*) info;
            MSIVIEW *insert = NULL;
            UINT r;

            r = INSERT_CreateView( sql->db, &insert, (yyvsp[-7].string), (yyvsp[-5].column_list), (yyvsp[-1].column_list), FALSE );
            if( !insert )
                YYABORT;
            (yyval.query) = insert;
        ;}
    break;

  case 10:
#line 152 "sql.y"
    {
            SQL_input *sql = (SQL_input*) info;
            MSIVIEW *insert = NULL;

            INSERT_CreateView( sql->db, &insert, (yyvsp[-8].string), (yyvsp[-6].column_list), (yyvsp[-2].column_list), TRUE );
            if( !insert )
                YYABORT;
            (yyval.query) = insert;
        ;}
    break;

  case 11:
#line 165 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *create = NULL;

            if( !(yyvsp[-1].column_list) )
                YYABORT;
            CREATE_CreateView( sql->db, &create, (yyvsp[-3].string), (yyvsp[-1].column_list), FALSE );
            if( !create )
                YYABORT;
            (yyval.query) = create;
        ;}
    break;

  case 12:
#line 177 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *create = NULL;

            if( !(yyvsp[-2].column_list) )
                YYABORT;
            CREATE_CreateView( sql->db, &create, (yyvsp[-4].string), (yyvsp[-2].column_list), TRUE );
            if( !create )
                YYABORT;
            (yyval.query) = create;
        ;}
    break;

  case 13:
#line 192 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *update = NULL;

            UPDATE_CreateView( sql->db, &update, (yyvsp[-4].string), (yyvsp[-2].column_list), (yyvsp[0].expr) );
            if( !update )
                YYABORT;
            (yyval.query) = update;
        ;}
    break;

  case 14:
#line 202 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *update = NULL;

            UPDATE_CreateView( sql->db, &update, (yyvsp[-2].string), (yyvsp[0].column_list), NULL );
            if( !update )
                YYABORT;
            (yyval.query) = update;
        ;}
    break;

  case 15:
#line 215 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *delete = NULL;

            DELETE_CreateView( sql->db, &delete, (yyvsp[0].query) );
            if( !delete )
                YYABORT;
            (yyval.query) = delete;
        ;}
    break;

  case 16:
#line 228 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *alter = NULL;

            ALTER_CreateView( sql->db, &alter, (yyvsp[-1].string), (yyvsp[0].integer) );
            if( !alter )
                YYABORT;
            (yyval.query) = alter;
        ;}
    break;

  case 17:
#line 241 "sql.y"
    {
            (yyval.integer) = 1;
        ;}
    break;

  case 18:
#line 245 "sql.y"
    {
            (yyval.integer) = -1;
        ;}
    break;

  case 19:
#line 252 "sql.y"
    {
            if( SQL_MarkPrimaryKeys( (yyvsp[-3].column_list), (yyvsp[0].column_list) ) )
                (yyval.column_list) = (yyvsp[-3].column_list);
            else
                (yyval.column_list) = NULL;
        ;}
    break;

  case 20:
#line 262 "sql.y"
    {
            column_info *ci;

            for( ci = (yyvsp[-2].column_list); ci->next; ci = ci->next )
                ;

            ci->next = (yyvsp[0].column_list);
            (yyval.column_list) = (yyvsp[-2].column_list);
        ;}
    break;

  case 21:
#line 272 "sql.y"
    {
            (yyval.column_list) = (yyvsp[0].column_list);
        ;}
    break;

  case 22:
#line 279 "sql.y"
    {
            (yyval.column_list) = (yyvsp[-1].column_list);
            (yyval.column_list)->type = (yyvsp[0].column_type) | MSITYPE_VALID;
        ;}
    break;

  case 23:
#line 287 "sql.y"
    {
            (yyval.column_type) = (yyvsp[0].column_type);
        ;}
    break;

  case 24:
#line 291 "sql.y"
    {
            (yyval.column_type) = (yyvsp[-1].column_type) | MSITYPE_LOCALIZABLE;
        ;}
    break;

  case 25:
#line 295 "sql.y"
    {
            FIXME("temporary column\n");
        ;}
    break;

  case 26:
#line 302 "sql.y"
    {
            (yyval.column_type) |= MSITYPE_NULLABLE;
        ;}
    break;

  case 27:
#line 306 "sql.y"
    {
            (yyval.column_type) = (yyvsp[-2].column_type);
        ;}
    break;

  case 28:
#line 313 "sql.y"
    {
            (yyval.column_type) = MSITYPE_STRING | 1;
        ;}
    break;

  case 29:
#line 317 "sql.y"
    {
            (yyval.column_type) = MSITYPE_STRING | 0x400 | (yyvsp[-1].column_type);
        ;}
    break;

  case 30:
#line 321 "sql.y"
    {
            (yyval.column_type) = 2;
        ;}
    break;

  case 31:
#line 325 "sql.y"
    {
            (yyval.column_type) = 2;
        ;}
    break;

  case 32:
#line 329 "sql.y"
    {
            (yyval.column_type) = 2;
        ;}
    break;

  case 33:
#line 333 "sql.y"
    {
            (yyval.column_type) = 4;
        ;}
    break;

  case 34:
#line 337 "sql.y"
    {
            (yyval.column_type) = MSITYPE_STRING | MSITYPE_VALID;
        ;}
    break;

  case 35:
#line 344 "sql.y"
    {
            if( ( (yyvsp[0].integer) > 255 ) || ( (yyvsp[0].integer) < 0 ) )
                YYABORT;
            (yyval.column_type) = (yyvsp[0].integer);
        ;}
    break;

  case 36:
#line 353 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;

            (yyval.query) = NULL;
            if( (yyvsp[0].column_list) )
                ORDER_CreateView( sql->db, &(yyval.query), (yyvsp[-3].query), (yyvsp[0].column_list) );
            else
                (yyval.query) = (yyvsp[-3].query);
            if( !(yyval.query) )
                YYABORT;
        ;}
    break;

  case 38:
#line 369 "sql.y"
    {
            (yyval.query) = (yyvsp[0].query);
        ;}
    break;

  case 39:
#line 373 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            UINT r;

            (yyval.query) = NULL;
            r = DISTINCT_CreateView( sql->db, &(yyval.query), (yyvsp[0].query) );
            if (r != ERROR_SUCCESS)
            {
                (yyvsp[0].query)->ops->delete((yyvsp[0].query));
                YYABORT;
            }
        ;}
    break;

  case 40:
#line 389 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            UINT r;

            (yyval.query) = NULL;
            if( (yyvsp[-1].column_list) )
            {
                r = SELECT_CreateView( sql->db, &(yyval.query), (yyvsp[0].query), (yyvsp[-1].column_list) );
                if (r != ERROR_SUCCESS)
                {
                    (yyvsp[0].query)->ops->delete((yyvsp[0].query));
                    YYABORT;
                }
            }
            else
                (yyval.query) = (yyvsp[0].query);
        ;}
    break;

  case 42:
#line 411 "sql.y"
    {
            (yyvsp[-2].column_list)->next = (yyvsp[0].column_list);
        ;}
    break;

  case 43:
#line 415 "sql.y"
    {
            (yyval.column_list) = NULL;
        ;}
    break;

  case 45:
#line 423 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            UINT r;

            (yyval.query) = NULL;
            r = WHERE_CreateView( sql->db, &(yyval.query), (yyvsp[-2].query), (yyvsp[0].expr) );
            if( r != ERROR_SUCCESS )
            {
                (yyvsp[-2].query)->ops->delete( (yyvsp[-2].query) );
                YYABORT;
            }
        ;}
    break;

  case 46:
#line 439 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            UINT r;

            (yyval.query) = NULL;
            r = TABLE_CreateView( sql->db, (yyvsp[0].string), &(yyval.query) );
            if( r != ERROR_SUCCESS || !(yyval.query) )
                YYABORT;
        ;}
    break;

  case 47:
#line 449 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            UINT r;

            /* only support inner joins on two tables */
            r = JOIN_CreateView( sql->db, &(yyval.query), (yyvsp[-2].string), (yyvsp[0].string) );
            if( r != ERROR_SUCCESS )
                YYABORT;
        ;}
    break;

  case 48:
#line 462 "sql.y"
    {
            (yyval.expr) = (yyvsp[-1].expr);
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 49:
#line 468 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_AND, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 50:
#line 474 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_OR, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 51:
#line 480 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_EQ, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 52:
#line 486 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_GT, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 53:
#line 492 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_LT, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 54:
#line 498 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_LE, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 55:
#line 504 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_GE, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 56:
#line 510 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_NE, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 57:
#line 516 "sql.y"
    {
            (yyval.expr) = EXPR_unary( info, (yyvsp[-2].expr), OP_ISNULL );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 58:
#line 522 "sql.y"
    {
            (yyval.expr) = EXPR_unary( info, (yyvsp[-3].expr), OP_NOTNULL );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 61:
#line 536 "sql.y"
    {
            (yyval.column_list) = parser_alloc_column( info, NULL, NULL );
            if( !(yyval.column_list) )
                YYABORT;
            (yyval.column_list)->val = (yyvsp[0].expr);
        ;}
    break;

  case 62:
#line 543 "sql.y"
    {
            (yyval.column_list) = parser_alloc_column( info, NULL, NULL );
            if( !(yyval.column_list) )
                YYABORT;
            (yyval.column_list)->val = (yyvsp[-2].expr);
            (yyval.column_list)->next = (yyvsp[0].column_list);
        ;}
    break;

  case 64:
#line 555 "sql.y"
    {
            (yyval.column_list) = (yyvsp[-2].column_list);
            (yyval.column_list)->next = (yyvsp[0].column_list);
        ;}
    break;

  case 65:
#line 563 "sql.y"
    {
            (yyval.column_list) = (yyvsp[-2].column_list);
            (yyval.column_list)->val = (yyvsp[0].expr);
        ;}
    break;

  case 66:
#line 571 "sql.y"
    {
            (yyval.expr) = EXPR_ival( info, (yyvsp[0].integer) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 67:
#line 577 "sql.y"
    {
            (yyval.expr) = EXPR_ival( info, -(yyvsp[0].integer) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 68:
#line 583 "sql.y"
    {
            (yyval.expr) = EXPR_sval( info, &(yyvsp[0].str) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 69:
#line 589 "sql.y"
    {
            (yyval.expr) = EXPR_wildcard( info );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 70:
#line 598 "sql.y"
    {
            (yyval.expr) = EXPR_column( info, (yyvsp[0].column_list) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 71:
#line 607 "sql.y"
    {
            (yyval.column_list) = parser_alloc_column( info, (yyvsp[-2].string), (yyvsp[0].string) );
            if( !(yyval.column_list) )
                YYABORT;
        ;}
    break;

  case 72:
#line 613 "sql.y"
    {
            (yyval.column_list) = parser_alloc_column( info, NULL, (yyvsp[0].string) );
            if( !(yyval.column_list) )
                YYABORT;
        ;}
    break;

  case 73:
#line 622 "sql.y"
    {
            (yyval.string) = (yyvsp[0].string);
        ;}
    break;

  case 74:
#line 629 "sql.y"
    {
            (yyval.string) = SQL_getstring( info, &(yyvsp[0].str) );
            if( !(yyval.string) )
                YYABORT;
        ;}
    break;

  case 75:
#line 638 "sql.y"
    {
            (yyval.integer) = SQL_getint( info );
        ;}
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
#line 2029 "sql.tab.c"

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


#line 643 "sql.y"


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

static int SQL_lex( void *SQL_lval, SQL_input *sql )
{
    int token;
    struct sql_str * str = SQL_lval;

    do
    {
        sql->n += sql->len;
        if( ! sql->command[sql->n] )
            return 0;  /* end of input */

        /* TRACE("string : %s\n", debugstr_w(&sql->command[sql->n])); */
        sql->len = sqliteGetToken( &sql->command[sql->n], &token );
        if( sql->len==0 )
            break;
        str->data = &sql->command[sql->n];
        str->len = sql->len;
    }
    while( token == TK_SPACE );

    /* TRACE("token : %d (%s)\n", token, debugstr_wn(&sql->command[sql->n], sql->len)); */

    return token;
}

LPWSTR SQL_getstring( void *info, struct sql_str *strdata )
{
    LPCWSTR p = strdata->data;
    UINT len = strdata->len;
    LPWSTR str;

    /* if there's quotes, remove them */
    if( ( (p[0]=='`') && (p[len-1]=='`') ) ||
        ( (p[0]=='\'') && (p[len-1]=='\'') ) )
    {
        p++;
        len -= 2;
    }
    str = parser_alloc( info, (len + 1)*sizeof(WCHAR) );
    if( !str )
        return str;
    memcpy( str, p, len*sizeof(WCHAR) );
    str[len]=0;

    return str;
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

static int SQL_error( const char *str )
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

static struct expr * EXPR_column( void *info, column_info *column )
{
    struct expr *e = parser_alloc( info, sizeof *e );
    if( e )
    {
        e->type = EXPR_COLUMN;
        e->u.sval = column->column;
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

static struct expr * EXPR_sval( void *info, struct sql_str *str )
{
    struct expr *e = parser_alloc( info, sizeof *e );
    if( e )
    {
        e->type = EXPR_SVAL;
        e->u.sval = SQL_getstring( info, str );
    }
    return e;
}

static BOOL SQL_MarkPrimaryKeys( column_info *cols,
                                 column_info *keys )
{
    column_info *k;
    BOOL found = TRUE;

    for( k = keys; k && found; k = k->next )
    {
        column_info *c;

        found = FALSE;
        for( c = cols; c && !found; c = c->next )
        {
             if( lstrcmpW( k->column, c->column ) )
                 continue;
             c->type |= MSITYPE_KEY;
             found = TRUE;
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
    sql.view = phview;
    sql.mem = mem;

    r = SQL_parse(&sql);

    TRACE("Parse returned %d\n", r);
    if( r )
    {
        *sql.view = NULL;
        return ERROR_BAD_QUERY_SYNTAX;
    }

    return ERROR_SUCCESS;
}

