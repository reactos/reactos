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
#define yyparse sql_parse
#define yylex   sql_lex
#define yyerror sql_error
#define yylval  sql_lval
#define yychar  sql_char
#define yydebug sql_debug
#define yynerrs sql_nerrs


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
/* Tokens.  */
#define TK_ALTER 258
#define TK_AND 259
#define TK_BY 260
#define TK_CHAR 261
#define TK_COMMA 262
#define TK_CREATE 263
#define TK_DELETE 264
#define TK_DROP 265
#define TK_DISTINCT 266
#define TK_DOT 267
#define TK_EQ 268
#define TK_FREE 269
#define TK_FROM 270
#define TK_GE 271
#define TK_GT 272
#define TK_HOLD 273
#define TK_ADD 274
#define TK_ID 275
#define TK_ILLEGAL 276
#define TK_INSERT 277
#define TK_INT 278
#define TK_INTEGER 279
#define TK_INTO 280
#define TK_IS 281
#define TK_KEY 282
#define TK_LE 283
#define TK_LONG 284
#define TK_LONGCHAR 285
#define TK_LP 286
#define TK_LT 287
#define TK_LOCALIZABLE 288
#define TK_MINUS 289
#define TK_NE 290
#define TK_NOT 291
#define TK_NULL 292
#define TK_OBJECT 293
#define TK_OR 294
#define TK_ORDER 295
#define TK_PRIMARY 296
#define TK_RP 297
#define TK_SELECT 298
#define TK_SET 299
#define TK_SHORT 300
#define TK_SPACE 301
#define TK_STAR 302
#define TK_STRING 303
#define TK_TABLE 304
#define TK_TEMPORARY 305
#define TK_UPDATE 306
#define TK_VALUES 307
#define TK_WHERE 308
#define TK_WILDCARD 309
#define COLUMN 311
#define FUNCTION 312
#define COMMENT 313
#define UNCLOSED_STRING 314
#define SPACE 315
#define ILLEGAL 316
#define END_OF_FILE 317
#define TK_LIKE 318
#define TK_NEGATION 319




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

static int sql_error(const char *str);

WINE_DEFAULT_DEBUG_CHANNEL(msi);

typedef struct tag_SQL_input
{
    MSIDATABASE *db;
    LPCWSTR command;
    DWORD n, len;
    MSIVIEW **view;  /* view structure for the resulting query */
    struct list *mem;
} SQL_input;

static LPWSTR SQL_getstring( void *info, const struct sql_str *str );
static INT SQL_getint( void *info );
static int sql_lex( void *SQL_lval, SQL_input *info );

static LPWSTR parser_add_table( LPWSTR list, LPWSTR table );
static void *parser_alloc( void *info, unsigned int sz );
static column_info *parser_alloc_column( void *info, LPCWSTR table, LPCWSTR column );

static BOOL SQL_MarkPrimaryKeys( column_info *cols, column_info *keys);

static struct expr * EXPR_complex( void *info, struct expr *l, UINT op, struct expr *r );
static struct expr * EXPR_unary( void *info, struct expr *l, UINT op );
static struct expr * EXPR_column( void *info, const column_info *column );
static struct expr * EXPR_ival( void *info, int val );
static struct expr * EXPR_sval( void *info, const struct sql_str *str );
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
#line 74 "sql.y"
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
#line 299 "sql.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */
#line 311 "sql.tab.c"

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
#define YYFINAL  35
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   144

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  65
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  35
/* YYNRULES -- Number of rules. */
#define YYNRULES  81
/* YYNRULES -- Number of states. */
#define YYNSTATES  145

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   319

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
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    13,    15,    17,
      19,    30,    42,    49,    57,    64,    69,    72,    77,    83,
      90,    92,    94,    98,   103,   107,   109,   112,   114,   117,
     120,   122,   126,   128,   133,   135,   137,   139,   141,   143,
     145,   150,   152,   155,   159,   162,   164,   168,   170,   172,
     176,   179,   182,   184,   188,   192,   196,   200,   204,   208,
     212,   216,   220,   224,   228,   233,   235,   237,   239,   243,
     245,   249,   253,   255,   258,   260,   262,   264,   268,   270,
     272,   274
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      66,     0,    -1,    67,    -1,    82,    -1,    69,    -1,    68,
      -1,    70,    -1,    71,    -1,    72,    -1,    74,    -1,    22,
      25,    97,    31,    85,    42,    52,    31,    91,    42,    -1,
      22,    25,    97,    31,    85,    42,    52,    31,    91,    42,
      50,    -1,     8,    49,    97,    31,    75,    42,    -1,     8,
      49,    97,    31,    75,    42,    18,    -1,    51,    97,    44,
      92,    53,    89,    -1,    51,    97,    44,    92,    -1,     9,
      86,    -1,     3,    49,    97,    73,    -1,     3,    49,    97,
      19,    77,    -1,     3,    49,    97,    19,    77,    18,    -1,
      18,    -1,    14,    -1,    10,    49,    97,    -1,    76,    41,
      27,    85,    -1,    76,     7,    77,    -1,    77,    -1,    96,
      78,    -1,    79,    -1,    79,    33,    -1,    79,    50,    -1,
      80,    -1,    80,    36,    37,    -1,     6,    -1,     6,    31,
      81,    42,    -1,    30,    -1,    45,    -1,    23,    -1,    29,
      -1,    38,    -1,    99,    -1,    83,    40,     5,    85,    -1,
      83,    -1,    43,    84,    -1,    43,    11,    84,    -1,    85,
      86,    -1,    96,    -1,    96,     7,    85,    -1,    47,    -1,
      87,    -1,    87,    53,    89,    -1,    15,    97,    -1,    15,
      88,    -1,    97,    -1,    97,     7,    88,    -1,    31,    89,
      42,    -1,    89,     4,    89,    -1,    89,    39,    89,    -1,
      95,    13,    90,    -1,    95,    17,    90,    -1,    95,    32,
      90,    -1,    95,    28,    90,    -1,    95,    16,    90,    -1,
      95,    35,    90,    -1,    95,    26,    37,    -1,    95,    26,
      36,    37,    -1,    95,    -1,    94,    -1,    94,    -1,    94,
       7,    91,    -1,    93,    -1,    93,     7,    92,    -1,    96,
      13,    94,    -1,    99,    -1,    34,    99,    -1,    48,    -1,
      54,    -1,    96,    -1,    97,    12,    98,    -1,    98,    -1,
      98,    -1,    20,    -1,    24,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   124,   124,   132,   133,   134,   135,   136,   137,   138,
     142,   152,   165,   177,   192,   202,   215,   228,   238,   248,
     261,   265,   272,   285,   295,   305,   312,   321,   325,   329,
     336,   340,   347,   351,   355,   359,   363,   367,   371,   378,
     387,   400,   404,   408,   424,   445,   446,   450,   457,   458,
     474,   484,   497,   502,   511,   517,   523,   529,   535,   541,
     547,   553,   559,   565,   571,   580,   581,   585,   592,   603,
     604,   612,   620,   626,   632,   638,   647,   656,   662,   671,
     678,   687
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
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
  "data_count", "oneselect", "unorderedsel", "selectfrom", "selcollist",
  "from", "fromtable", "tablelist", "expr", "val", "constlist",
  "update_assign_list", "column_assignment", "const_val", "column_val",
  "column", "table", "id", "number", 0
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
     315,   316,   317,   318,   319
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    65,    66,    67,    67,    67,    67,    67,    67,    67,
      68,    68,    69,    69,    70,    70,    71,    72,    72,    72,
      73,    73,    74,    75,    76,    76,    77,    78,    78,    78,
      79,    79,    80,    80,    80,    80,    80,    80,    80,    81,
      82,    82,    83,    83,    84,    85,    85,    85,    86,    86,
      87,    87,    88,    88,    89,    89,    89,    89,    89,    89,
      89,    89,    89,    89,    89,    90,    90,    91,    91,    92,
      92,    93,    94,    94,    94,    94,    95,    96,    96,    97,
      98,    99
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
      10,    11,     6,     7,     6,     4,     2,     4,     5,     6,
       1,     1,     3,     4,     3,     1,     2,     1,     2,     2,
       1,     3,     1,     4,     1,     1,     1,     1,     1,     1,
       4,     1,     2,     3,     2,     1,     3,     1,     1,     3,
       2,     2,     1,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     4,     1,     1,     1,     3,     1,
       3,     3,     1,     2,     1,     1,     1,     3,     1,     1,
       1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     2,
       5,     4,     6,     7,     8,     9,     3,    41,     0,     0,
       0,    16,    48,     0,     0,     0,    80,    47,    42,     0,
      45,     0,    78,     0,    79,     1,     0,     0,     0,    51,
      50,     0,    22,     0,    43,    44,     0,     0,     0,     0,
      21,    20,     0,    17,     0,     0,     0,    49,     0,    76,
       0,    46,    77,    15,    69,     0,    40,    18,     0,     0,
       0,    25,    53,    52,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    19,    32,
      36,    37,    34,    38,    35,    26,    27,    30,    12,     0,
       0,    54,    55,    56,    81,     0,    74,    75,    57,    66,
      65,    72,    61,    58,     0,    63,    60,    59,    62,     0,
      14,    70,    71,     0,    28,    29,     0,    13,    24,     0,
      73,    64,     0,     0,    39,    31,    23,     0,    33,     0,
      67,    10,     0,    11,    68
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,     8,     9,    10,    11,    12,    13,    14,    53,    15,
      69,    70,    67,    95,    96,    97,   133,    16,    17,    28,
      29,    21,    22,    39,    57,   108,   139,    63,    64,   109,
     110,    59,    31,    32,   111
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -97
static const short int yypact[] =
{
       4,   -41,   -19,    56,   -14,    42,    14,    59,    72,   -97,
     -97,   -97,   -97,   -97,   -97,   -97,   -97,    36,    59,    59,
      59,   -97,    30,    59,    59,     3,   -97,   -97,   -97,    56,
      82,    79,    80,    50,   -97,   -97,    93,    51,    73,   -97,
      96,    29,   -97,    74,   -97,   -97,     3,    59,    59,     3,
     -97,   -97,    59,   -97,    59,    59,    29,    20,    83,   -97,
       3,   -97,   -97,    57,    99,   100,   -97,    94,    52,    84,
      21,   -97,   -97,    96,    -1,    29,    29,    54,    54,    54,
     -26,    54,    54,    54,    85,    29,    59,    53,   -97,    91,
     -97,   -97,   -97,   -97,   -97,   -97,     6,    78,   106,    59,
      98,   -97,   -97,   124,   -97,   105,   -97,   -97,   -97,   -97,
     -97,   -97,   -97,   -97,    95,   -97,   -97,   -97,   -97,    81,
      20,   -97,   -97,   105,   -97,   -97,    97,   -97,   -97,     3,
     -97,   -97,   104,    88,   -97,   -97,   -97,    53,   -97,    89,
     129,    87,    53,   -97,   -97
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -97,   -97,   -97,   -97,   -97,   -97,   -97,   -97,   -97,   -97,
     -97,   -97,   -48,   -97,   -97,   -97,   -97,   -97,   -97,   113,
     -45,   110,   -97,    86,    10,    38,     0,    58,   -97,   -85,
     -12,    -6,    13,    -2,   -96
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -80
static const short int yytable[] =
{
      30,    61,   122,    75,    66,    34,    71,     1,    18,   130,
     114,   115,     2,     3,     4,    84,    34,    34,    34,    30,
      33,    34,    34,    26,    75,    25,     5,   134,    99,    58,
      19,    37,    38,    40,    26,    23,    42,    43,    76,   124,
      30,   101,    65,    30,    58,    62,    68,     6,    68,    26,
      27,   128,   140,    34,    30,     7,   125,   140,    89,    76,
      56,    27,   100,    58,    58,    50,    74,    24,    73,    51,
      52,    20,    35,    58,    26,    90,    36,   104,   104,    26,
      65,    91,    92,    41,   136,   102,   103,   105,   105,    46,
      93,    47,   -79,    68,    48,   120,    77,    94,    49,    78,
      79,   106,   106,    55,    54,    60,    86,   107,   107,    80,
      85,    81,    88,    87,   126,    82,   112,   113,    83,   116,
     117,   118,   123,    30,   127,   129,    98,   119,    75,   104,
     138,   141,   131,   132,   135,   137,   142,   143,    44,    45,
       0,    72,   144,     0,   121
};

static const short int yycheck[] =
{
       6,    46,    87,     4,    49,     7,    54,     3,    49,   105,
      36,    37,     8,     9,    10,    60,    18,    19,    20,    25,
       7,    23,    24,    20,     4,    11,    22,   123,     7,    41,
      49,    18,    19,    20,    20,    49,    23,    24,    39,    33,
      46,    42,    48,    49,    56,    47,    52,    43,    54,    20,
      47,    99,   137,    55,    60,    51,    50,   142,     6,    39,
      31,    47,    41,    75,    76,    14,    56,    25,    55,    18,
      19,    15,     0,    85,    20,    23,    40,    24,    24,    20,
      86,    29,    30,    53,   129,    75,    76,    34,    34,     7,
      38,    12,    12,    99,    44,    85,    13,    45,     5,    16,
      17,    48,    48,     7,    31,    31,     7,    54,    54,    26,
      53,    28,    18,    13,    36,    32,    78,    79,    35,    81,
      82,    83,    31,   129,    18,    27,    42,    42,     4,    24,
      42,    42,    37,    52,    37,    31,     7,    50,    25,    29,
      -1,    55,   142,    -1,    86
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     3,     8,     9,    10,    22,    43,    51,    66,    67,
      68,    69,    70,    71,    72,    74,    82,    83,    49,    49,
      15,    86,    87,    49,    25,    11,    20,    47,    84,    85,
      96,    97,    98,    97,    98,     0,    40,    97,    97,    88,
      97,    53,    97,    97,    84,    86,     7,    12,    44,     5,
      14,    18,    19,    73,    31,     7,    31,    89,    95,    96,
      31,    85,    98,    92,    93,    96,    85,    77,    96,    75,
      76,    77,    88,    97,    89,     4,    39,    13,    16,    17,
      26,    28,    32,    35,    85,    53,     7,    13,    18,     6,
      23,    29,    30,    38,    45,    78,    79,    80,    42,     7,
      41,    42,    89,    89,    24,    34,    48,    54,    90,    94,
      95,    99,    90,    90,    36,    37,    90,    90,    90,    42,
      89,    92,    94,    31,    33,    50,    36,    18,    77,    27,
      99,    37,    52,    81,    99,    37,    85,    31,    42,    91,
      94,    42,     7,    50,    91
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
#line 125 "sql.y"
    {
        SQL_input* sql = (SQL_input*) info;
        *sql->view = (yyvsp[0].query);
    ;}
    break;

  case 10:
#line 143 "sql.y"
    {
            SQL_input *sql = (SQL_input*) info;
            MSIVIEW *insert = NULL;

            INSERT_CreateView( sql->db, &insert, (yyvsp[-7].string), (yyvsp[-5].column_list), (yyvsp[-1].column_list), FALSE );
            if( !insert )
                YYABORT;
            (yyval.query) = insert;
        ;}
    break;

  case 11:
#line 153 "sql.y"
    {
            SQL_input *sql = (SQL_input*) info;
            MSIVIEW *insert = NULL;

            INSERT_CreateView( sql->db, &insert, (yyvsp[-8].string), (yyvsp[-6].column_list), (yyvsp[-2].column_list), TRUE );
            if( !insert )
                YYABORT;
            (yyval.query) = insert;
        ;}
    break;

  case 12:
#line 166 "sql.y"
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

  case 13:
#line 178 "sql.y"
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

  case 14:
#line 193 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *update = NULL;

            UPDATE_CreateView( sql->db, &update, (yyvsp[-4].string), (yyvsp[-2].column_list), (yyvsp[0].expr) );
            if( !update )
                YYABORT;
            (yyval.query) = update;
        ;}
    break;

  case 15:
#line 203 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *update = NULL;

            UPDATE_CreateView( sql->db, &update, (yyvsp[-2].string), (yyvsp[0].column_list), NULL );
            if( !update )
                YYABORT;
            (yyval.query) = update;
        ;}
    break;

  case 16:
#line 216 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *delete = NULL;

            DELETE_CreateView( sql->db, &delete, (yyvsp[0].query) );
            if( !delete )
                YYABORT;
            (yyval.query) = delete;
        ;}
    break;

  case 17:
#line 229 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *alter = NULL;

            ALTER_CreateView( sql->db, &alter, (yyvsp[-1].string), NULL, (yyvsp[0].integer) );
            if( !alter )
                YYABORT;
            (yyval.query) = alter;
        ;}
    break;

  case 18:
#line 239 "sql.y"
    {
            SQL_input *sql = (SQL_input *)info;
            MSIVIEW *alter = NULL;

            ALTER_CreateView( sql->db, &alter, (yyvsp[-2].string), (yyvsp[0].column_list), 0 );
            if (!alter)
                YYABORT;
            (yyval.query) = alter;
        ;}
    break;

  case 19:
#line 249 "sql.y"
    {
            SQL_input *sql = (SQL_input *)info;
            MSIVIEW *alter = NULL;

            ALTER_CreateView( sql->db, &alter, (yyvsp[-3].string), (yyvsp[-1].column_list), 1 );
            if (!alter)
                YYABORT;
            (yyval.query) = alter;
        ;}
    break;

  case 20:
#line 262 "sql.y"
    {
            (yyval.integer) = 1;
        ;}
    break;

  case 21:
#line 266 "sql.y"
    {
            (yyval.integer) = -1;
        ;}
    break;

  case 22:
#line 273 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            UINT r;

            (yyval.query) = NULL;
            r = DROP_CreateView( sql->db, &(yyval.query), (yyvsp[0].string) );
            if( r != ERROR_SUCCESS || !(yyval.query) )
                YYABORT;
        ;}
    break;

  case 23:
#line 286 "sql.y"
    {
            if( SQL_MarkPrimaryKeys( (yyvsp[-3].column_list), (yyvsp[0].column_list) ) )
                (yyval.column_list) = (yyvsp[-3].column_list);
            else
                (yyval.column_list) = NULL;
        ;}
    break;

  case 24:
#line 296 "sql.y"
    {
            column_info *ci;

            for( ci = (yyvsp[-2].column_list); ci->next; ci = ci->next )
                ;

            ci->next = (yyvsp[0].column_list);
            (yyval.column_list) = (yyvsp[-2].column_list);
        ;}
    break;

  case 25:
#line 306 "sql.y"
    {
            (yyval.column_list) = (yyvsp[0].column_list);
        ;}
    break;

  case 26:
#line 313 "sql.y"
    {
            (yyval.column_list) = (yyvsp[-1].column_list);
            (yyval.column_list)->type = ((yyvsp[0].column_type) | MSITYPE_VALID);
            (yyval.column_list)->temporary = (yyvsp[0].column_type) & MSITYPE_TEMPORARY ? TRUE : FALSE;
        ;}
    break;

  case 27:
#line 322 "sql.y"
    {
            (yyval.column_type) = (yyvsp[0].column_type);
        ;}
    break;

  case 28:
#line 326 "sql.y"
    {
            (yyval.column_type) = (yyvsp[-1].column_type) | MSITYPE_LOCALIZABLE;
        ;}
    break;

  case 29:
#line 330 "sql.y"
    {
            (yyval.column_type) = (yyvsp[-1].column_type) | MSITYPE_TEMPORARY;
        ;}
    break;

  case 30:
#line 337 "sql.y"
    {
            (yyval.column_type) |= MSITYPE_NULLABLE;
        ;}
    break;

  case 31:
#line 341 "sql.y"
    {
            (yyval.column_type) = (yyvsp[-2].column_type);
        ;}
    break;

  case 32:
#line 348 "sql.y"
    {
            (yyval.column_type) = MSITYPE_STRING | 1;
        ;}
    break;

  case 33:
#line 352 "sql.y"
    {
            (yyval.column_type) = MSITYPE_STRING | 0x400 | (yyvsp[-1].column_type);
        ;}
    break;

  case 34:
#line 356 "sql.y"
    {
            (yyval.column_type) = 2;
        ;}
    break;

  case 35:
#line 360 "sql.y"
    {
            (yyval.column_type) = 2;
        ;}
    break;

  case 36:
#line 364 "sql.y"
    {
            (yyval.column_type) = 2;
        ;}
    break;

  case 37:
#line 368 "sql.y"
    {
            (yyval.column_type) = 4;
        ;}
    break;

  case 38:
#line 372 "sql.y"
    {
            (yyval.column_type) = MSITYPE_STRING | MSITYPE_VALID;
        ;}
    break;

  case 39:
#line 379 "sql.y"
    {
            if( ( (yyvsp[0].integer) > 255 ) || ( (yyvsp[0].integer) < 0 ) )
                YYABORT;
            (yyval.column_type) = (yyvsp[0].integer);
        ;}
    break;

  case 40:
#line 388 "sql.y"
    {
            UINT r;

            if( (yyvsp[0].column_list) )
            {
                r = (yyvsp[-3].query)->ops->sort( (yyvsp[-3].query), (yyvsp[0].column_list) );
                if ( r != ERROR_SUCCESS)
                    YYABORT;
            }

            (yyval.query) = (yyvsp[-3].query);
        ;}
    break;

  case 42:
#line 405 "sql.y"
    {
            (yyval.query) = (yyvsp[0].query);
        ;}
    break;

  case 43:
#line 409 "sql.y"
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

  case 44:
#line 425 "sql.y"
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

  case 46:
#line 447 "sql.y"
    {
            (yyvsp[-2].column_list)->next = (yyvsp[0].column_list);
        ;}
    break;

  case 47:
#line 451 "sql.y"
    {
            (yyval.column_list) = NULL;
        ;}
    break;

  case 49:
#line 459 "sql.y"
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

  case 50:
#line 475 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            UINT r;

            (yyval.query) = NULL;
            r = TABLE_CreateView( sql->db, (yyvsp[0].string), &(yyval.query) );
            if( r != ERROR_SUCCESS || !(yyval.query) )
                YYABORT;
        ;}
    break;

  case 51:
#line 485 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            UINT r;

            r = JOIN_CreateView( sql->db, &(yyval.query), (yyvsp[0].string) );
            msi_free( (yyvsp[0].string) );
            if( r != ERROR_SUCCESS )
                YYABORT;
        ;}
    break;

  case 52:
#line 498 "sql.y"
    {
            (yyval.string) = strdupW((yyvsp[0].string));
        ;}
    break;

  case 53:
#line 503 "sql.y"
    {
            (yyval.string) = parser_add_table((yyvsp[0].string), (yyvsp[-2].string));
            if (!(yyval.string))
                YYABORT;
        ;}
    break;

  case 54:
#line 512 "sql.y"
    {
            (yyval.expr) = (yyvsp[-1].expr);
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 55:
#line 518 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_AND, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 56:
#line 524 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_OR, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 57:
#line 530 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_EQ, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 58:
#line 536 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_GT, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 59:
#line 542 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_LT, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 60:
#line 548 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_LE, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 61:
#line 554 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_GE, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 62:
#line 560 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_NE, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 63:
#line 566 "sql.y"
    {
            (yyval.expr) = EXPR_unary( info, (yyvsp[-2].expr), OP_ISNULL );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 64:
#line 572 "sql.y"
    {
            (yyval.expr) = EXPR_unary( info, (yyvsp[-3].expr), OP_NOTNULL );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 67:
#line 586 "sql.y"
    {
            (yyval.column_list) = parser_alloc_column( info, NULL, NULL );
            if( !(yyval.column_list) )
                YYABORT;
            (yyval.column_list)->val = (yyvsp[0].expr);
        ;}
    break;

  case 68:
#line 593 "sql.y"
    {
            (yyval.column_list) = parser_alloc_column( info, NULL, NULL );
            if( !(yyval.column_list) )
                YYABORT;
            (yyval.column_list)->val = (yyvsp[-2].expr);
            (yyval.column_list)->next = (yyvsp[0].column_list);
        ;}
    break;

  case 70:
#line 605 "sql.y"
    {
            (yyval.column_list) = (yyvsp[-2].column_list);
            (yyval.column_list)->next = (yyvsp[0].column_list);
        ;}
    break;

  case 71:
#line 613 "sql.y"
    {
            (yyval.column_list) = (yyvsp[-2].column_list);
            (yyval.column_list)->val = (yyvsp[0].expr);
        ;}
    break;

  case 72:
#line 621 "sql.y"
    {
            (yyval.expr) = EXPR_ival( info, (yyvsp[0].integer) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 73:
#line 627 "sql.y"
    {
            (yyval.expr) = EXPR_ival( info, -(yyvsp[0].integer) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 74:
#line 633 "sql.y"
    {
            (yyval.expr) = EXPR_sval( info, &(yyvsp[0].str) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 75:
#line 639 "sql.y"
    {
            (yyval.expr) = EXPR_wildcard( info );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 76:
#line 648 "sql.y"
    {
            (yyval.expr) = EXPR_column( info, (yyvsp[0].column_list) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 77:
#line 657 "sql.y"
    {
            (yyval.column_list) = parser_alloc_column( info, (yyvsp[-2].string), (yyvsp[0].string) );
            if( !(yyval.column_list) )
                YYABORT;
        ;}
    break;

  case 78:
#line 663 "sql.y"
    {
            (yyval.column_list) = parser_alloc_column( info, NULL, (yyvsp[0].string) );
            if( !(yyval.column_list) )
                YYABORT;
        ;}
    break;

  case 79:
#line 672 "sql.y"
    {
            (yyval.string) = (yyvsp[0].string);
        ;}
    break;

  case 80:
#line 679 "sql.y"
    {
            (yyval.string) = SQL_getstring( info, &(yyvsp[0].str) );
            if( !(yyval.string) )
                YYABORT;
        ;}
    break;

  case 81:
#line 688 "sql.y"
    {
            (yyval.integer) = SQL_getint( info );
        ;}
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
#line 2102 "sql.tab.c"

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


#line 693 "sql.y"


static LPWSTR parser_add_table(LPWSTR list, LPWSTR table)
{
    DWORD size = lstrlenW(list) + lstrlenW(table) + 2;
    static const WCHAR space[] = {' ',0};

    list = msi_realloc(list, size * sizeof(WCHAR));
    if (!list) return NULL;

    lstrcatW(list, space);
    lstrcatW(list, table);
    return list;
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

LPWSTR SQL_getstring( void *info, const struct sql_str *strdata )
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

static struct expr * EXPR_sval( void *info, const struct sql_str *str )
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

    r = sql_parse(&sql);

    TRACE("Parse returned %d\n", r);
    if( r )
    {
        *sql.view = NULL;
        return ERROR_BAD_QUERY_SYNTAX;
    }

    return ERROR_SUCCESS;
}

