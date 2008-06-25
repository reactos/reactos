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
     TK_DISTINCT = 265,
     TK_DOT = 266,
     TK_EQ = 267,
     TK_FREE = 268,
     TK_FROM = 269,
     TK_GE = 270,
     TK_GT = 271,
     TK_HOLD = 272,
     TK_ADD = 273,
     TK_ID = 274,
     TK_ILLEGAL = 275,
     TK_INSERT = 276,
     TK_INT = 277,
     TK_INTEGER = 278,
     TK_INTO = 279,
     TK_IS = 280,
     TK_KEY = 281,
     TK_LE = 282,
     TK_LONG = 283,
     TK_LONGCHAR = 284,
     TK_LP = 285,
     TK_LT = 286,
     TK_LOCALIZABLE = 287,
     TK_MINUS = 288,
     TK_NE = 289,
     TK_NOT = 290,
     TK_NULL = 291,
     TK_OBJECT = 292,
     TK_OR = 293,
     TK_ORDER = 294,
     TK_PRIMARY = 295,
     TK_RP = 296,
     TK_SELECT = 297,
     TK_SET = 298,
     TK_SHORT = 299,
     TK_SPACE = 300,
     TK_STAR = 301,
     TK_STRING = 302,
     TK_TABLE = 303,
     TK_TEMPORARY = 304,
     TK_UPDATE = 305,
     TK_VALUES = 306,
     TK_WHERE = 307,
     TK_WILDCARD = 308,
     COLUMN = 310,
     FUNCTION = 311,
     COMMENT = 312,
     UNCLOSED_STRING = 313,
     SPACE = 314,
     ILLEGAL = 315,
     END_OF_FILE = 316,
     TK_LIKE = 317,
     TK_NEGATION = 318
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
#define TK_ADD 273
#define TK_ID 274
#define TK_ILLEGAL 275
#define TK_INSERT 276
#define TK_INT 277
#define TK_INTEGER 278
#define TK_INTO 279
#define TK_IS 280
#define TK_KEY 281
#define TK_LE 282
#define TK_LONG 283
#define TK_LONGCHAR 284
#define TK_LP 285
#define TK_LT 286
#define TK_LOCALIZABLE 287
#define TK_MINUS 288
#define TK_NE 289
#define TK_NOT 290
#define TK_NULL 291
#define TK_OBJECT 292
#define TK_OR 293
#define TK_ORDER 294
#define TK_PRIMARY 295
#define TK_RP 296
#define TK_SELECT 297
#define TK_SET 298
#define TK_SHORT 299
#define TK_SPACE 300
#define TK_STAR 301
#define TK_STRING 302
#define TK_TABLE 303
#define TK_TEMPORARY 304
#define TK_UPDATE 305
#define TK_VALUES 306
#define TK_WHERE 307
#define TK_WILDCARD 308
#define COLUMN 310
#define FUNCTION 311
#define COMMENT 312
#define UNCLOSED_STRING 313
#define SPACE 314
#define ILLEGAL 315
#define END_OF_FILE 316
#define TK_LIKE 317
#define TK_NEGATION 318




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
#line 297 "sql.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */
#line 309 "sql.tab.c"

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
#define YYLAST   138

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  64
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  34
/* YYNRULES -- Number of rules. */
#define YYNRULES  79
/* YYNRULES -- Number of states. */
#define YYNSTATES  141

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   318

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
      55,    56,    57,    58,    59,    60,    61,    62,    63
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    13,    15,    17,
      28,    40,    47,    55,    62,    67,    70,    75,    81,    88,
      90,    92,    97,   101,   103,   106,   108,   111,   114,   116,
     120,   122,   127,   129,   131,   133,   135,   137,   139,   144,
     146,   149,   153,   156,   158,   162,   164,   166,   170,   173,
     176,   178,   182,   186,   190,   194,   198,   202,   206,   210,
     214,   218,   222,   227,   229,   231,   233,   237,   239,   243,
     247,   249,   252,   254,   256,   258,   262,   264,   266,   268
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      65,     0,    -1,    66,    -1,    80,    -1,    68,    -1,    67,
      -1,    69,    -1,    70,    -1,    71,    -1,    21,    24,    95,
      30,    83,    41,    51,    30,    89,    41,    -1,    21,    24,
      95,    30,    83,    41,    51,    30,    89,    41,    49,    -1,
       8,    48,    95,    30,    73,    41,    -1,     8,    48,    95,
      30,    73,    41,    17,    -1,    50,    95,    43,    90,    52,
      87,    -1,    50,    95,    43,    90,    -1,     9,    84,    -1,
       3,    48,    95,    72,    -1,     3,    48,    95,    18,    75,
      -1,     3,    48,    95,    18,    75,    17,    -1,    17,    -1,
      13,    -1,    74,    40,    26,    83,    -1,    74,     7,    75,
      -1,    75,    -1,    94,    76,    -1,    77,    -1,    77,    32,
      -1,    77,    49,    -1,    78,    -1,    78,    35,    36,    -1,
       6,    -1,     6,    30,    79,    41,    -1,    29,    -1,    44,
      -1,    22,    -1,    28,    -1,    37,    -1,    97,    -1,    81,
      39,     5,    83,    -1,    81,    -1,    42,    82,    -1,    42,
      10,    82,    -1,    83,    84,    -1,    94,    -1,    94,     7,
      83,    -1,    46,    -1,    85,    -1,    85,    52,    87,    -1,
      14,    95,    -1,    14,    86,    -1,    95,    -1,    95,     7,
      86,    -1,    30,    87,    41,    -1,    87,     4,    87,    -1,
      87,    38,    87,    -1,    93,    12,    88,    -1,    93,    16,
      88,    -1,    93,    31,    88,    -1,    93,    27,    88,    -1,
      93,    15,    88,    -1,    93,    34,    88,    -1,    93,    25,
      36,    -1,    93,    25,    35,    36,    -1,    93,    -1,    92,
      -1,    92,    -1,    92,     7,    89,    -1,    91,    -1,    91,
       7,    90,    -1,    94,    12,    92,    -1,    97,    -1,    33,
      97,    -1,    47,    -1,    53,    -1,    94,    -1,    95,    11,
      96,    -1,    96,    -1,    96,    -1,    19,    -1,    23,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   124,   124,   132,   133,   134,   135,   136,   137,   141,
     151,   164,   176,   191,   201,   214,   227,   237,   247,   260,
     264,   271,   281,   291,   298,   307,   311,   315,   322,   326,
     333,   337,   341,   345,   349,   353,   357,   364,   373,   386,
     390,   394,   410,   431,   432,   436,   443,   444,   460,   470,
     483,   488,   497,   503,   509,   515,   521,   527,   533,   539,
     545,   551,   557,   566,   567,   571,   578,   589,   590,   598,
     606,   612,   618,   624,   633,   642,   648,   657,   664,   673
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "TK_ALTER", "TK_AND", "TK_BY", "TK_CHAR",
  "TK_COMMA", "TK_CREATE", "TK_DELETE", "TK_DISTINCT", "TK_DOT", "TK_EQ",
  "TK_FREE", "TK_FROM", "TK_GE", "TK_GT", "TK_HOLD", "TK_ADD", "TK_ID",
  "TK_ILLEGAL", "TK_INSERT", "TK_INT", "TK_INTEGER", "TK_INTO", "TK_IS",
  "TK_KEY", "TK_LE", "TK_LONG", "TK_LONGCHAR", "TK_LP", "TK_LT",
  "TK_LOCALIZABLE", "TK_MINUS", "TK_NE", "TK_NOT", "TK_NULL", "TK_OBJECT",
  "TK_OR", "TK_ORDER", "TK_PRIMARY", "TK_RP", "TK_SELECT", "TK_SET",
  "TK_SHORT", "TK_SPACE", "TK_STAR", "TK_STRING", "TK_TABLE",
  "TK_TEMPORARY", "TK_UPDATE", "TK_VALUES", "TK_WHERE", "TK_WILDCARD",
  "AGG_FUNCTION.", "COLUMN", "FUNCTION", "COMMENT", "UNCLOSED_STRING",
  "SPACE", "ILLEGAL", "END_OF_FILE", "TK_LIKE", "TK_NEGATION", "$accept",
  "query", "onequery", "oneinsert", "onecreate", "oneupdate", "onedelete",
  "onealter", "alterop", "table_def", "column_def", "column_and_type",
  "column_type", "data_type_l", "data_type", "data_count", "oneselect",
  "unorderedsel", "selectfrom", "selcollist", "from", "fromtable",
  "tablelist", "expr", "val", "constlist", "update_assign_list",
  "column_assignment", "const_val", "column_val", "column", "table", "id",
  "number", 0
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
     315,   316,   317,   318
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    64,    65,    66,    66,    66,    66,    66,    66,    67,
      67,    68,    68,    69,    69,    70,    71,    71,    71,    72,
      72,    73,    74,    74,    75,    76,    76,    76,    77,    77,
      78,    78,    78,    78,    78,    78,    78,    79,    80,    80,
      81,    81,    82,    83,    83,    83,    84,    84,    85,    85,
      86,    86,    87,    87,    87,    87,    87,    87,    87,    87,
      87,    87,    87,    88,    88,    89,    89,    90,    90,    91,
      92,    92,    92,    92,    93,    94,    94,    95,    96,    97
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,    10,
      11,     6,     7,     6,     4,     2,     4,     5,     6,     1,
       1,     4,     3,     1,     2,     1,     2,     2,     1,     3,
       1,     4,     1,     1,     1,     1,     1,     1,     4,     1,
       2,     3,     2,     1,     3,     1,     1,     3,     2,     2,
       1,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     4,     1,     1,     1,     3,     1,     3,     3,
       1,     2,     1,     1,     1,     3,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     2,     5,
       4,     6,     7,     8,     3,    39,     0,     0,     0,    15,
      46,     0,     0,    78,    45,    40,     0,    43,     0,    76,
       0,    77,     1,     0,     0,     0,    49,    48,     0,     0,
      41,    42,     0,     0,     0,     0,    20,    19,     0,    16,
       0,     0,     0,    47,     0,    74,     0,    44,    75,    14,
      67,     0,    38,    17,     0,     0,     0,    23,    51,    50,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    18,    30,    34,    35,    32,    36,
      33,    24,    25,    28,    11,     0,     0,    52,    53,    54,
      79,     0,    72,    73,    55,    64,    63,    70,    59,    56,
       0,    61,    58,    57,    60,     0,    13,    68,    69,     0,
      26,    27,     0,    12,    22,     0,    71,    62,     0,     0,
      37,    29,    21,     0,    31,     0,    65,     9,     0,    10,
      66
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,     7,     8,     9,    10,    11,    12,    13,    49,    65,
      66,    63,    91,    92,    93,   129,    14,    15,    25,    26,
      19,    20,    36,    53,   104,   135,    59,    60,   105,   106,
      55,    28,    29,   107
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -96
static const yysigned_char yypact[] =
{
       0,   -43,   -34,     4,    24,     1,     6,    19,   -96,   -96,
     -96,   -96,   -96,   -96,   -96,    31,     6,     6,     6,   -96,
      30,     6,    35,   -96,   -96,   -96,     4,    67,    47,    65,
      42,   -96,   -96,    81,   101,    59,   -96,    89,    48,    68,
     -96,   -96,    35,     6,     6,    35,   -96,   -96,     6,   -96,
       6,     6,    48,     3,    76,   -96,    35,   -96,   -96,    45,
      95,    93,   -96,    98,    43,    75,    22,   -96,   -96,    89,
      18,    48,    48,    13,    13,    13,   -23,    13,    13,    13,
      80,    48,     6,    46,   -96,    87,   -96,   -96,   -96,   -96,
     -96,   -96,    51,    71,   105,     6,    97,   -96,   -96,   120,
     -96,   102,   -96,   -96,   -96,   -96,   -96,   -96,   -96,   -96,
      90,   -96,   -96,   -96,   -96,    77,     3,   -96,   -96,   102,
     -96,   -96,    91,   -96,   -96,    35,   -96,   -96,    99,    92,
     -96,   -96,   -96,    46,   -96,    94,   123,    82,    46,   -96,
     -96
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -96,   -96,   -96,   -96,   -96,   -96,   -96,   -96,   -96,   -96,
     -96,   -40,   -96,   -96,   -96,   -96,   -96,   -96,   110,   -41,
     108,   -96,    85,    23,    34,    -1,    56,   -96,   -81,    -8,
      -5,    17,    10,   -95
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -78
static const short int yytable[] =
{
      27,    57,   118,     1,    62,    16,   126,    71,     2,     3,
      67,    22,   110,   111,    17,    80,    31,    27,    18,    32,
      23,     4,    71,    30,   130,    23,    31,    31,    31,    95,
      54,    31,    23,    34,    35,    37,   100,    27,    39,    61,
      27,    72,     5,    64,    54,    64,   101,    24,    21,    85,
       6,    27,   136,    58,    23,   124,    72,   136,    43,    97,
     102,    31,    96,    54,    54,    86,   103,    23,    69,   100,
      33,    87,    88,    54,    42,    70,   -77,    61,    52,   101,
      89,    24,    38,   120,   132,    44,    45,    90,    73,    50,
      64,    74,    75,   102,    98,    99,    51,    81,    56,   103,
     121,    76,    82,    77,   116,    83,   122,    78,   108,   109,
      79,   112,   113,   114,    46,    84,    94,   119,    47,    48,
      27,   115,   123,   125,    71,   100,   127,   131,   128,   133,
     138,   139,    40,   134,    41,   137,    68,   140,   117
};

static const unsigned char yycheck[] =
{
       5,    42,    83,     3,    45,    48,   101,     4,     8,     9,
      50,    10,    35,    36,    48,    56,     6,    22,    14,     0,
      19,    21,     4,     6,   119,    19,    16,    17,    18,     7,
      38,    21,    19,    16,    17,    18,    23,    42,    21,    44,
      45,    38,    42,    48,    52,    50,    33,    46,    24,     6,
      50,    56,   133,    43,    19,    95,    38,   138,    11,    41,
      47,    51,    40,    71,    72,    22,    53,    19,    51,    23,
      39,    28,    29,    81,     7,    52,    11,    82,    30,    33,
      37,    46,    52,    32,   125,    43,     5,    44,    12,    30,
      95,    15,    16,    47,    71,    72,     7,    52,    30,    53,
      49,    25,     7,    27,    81,    12,    35,    31,    74,    75,
      34,    77,    78,    79,    13,    17,    41,    30,    17,    18,
     125,    41,    17,    26,     4,    23,    36,    36,    51,    30,
       7,    49,    22,    41,    26,    41,    51,   138,    82
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     3,     8,     9,    21,    42,    50,    65,    66,    67,
      68,    69,    70,    71,    80,    81,    48,    48,    14,    84,
      85,    24,    10,    19,    46,    82,    83,    94,    95,    96,
      95,    96,     0,    39,    95,    95,    86,    95,    52,    95,
      82,    84,     7,    11,    43,     5,    13,    17,    18,    72,
      30,     7,    30,    87,    93,    94,    30,    83,    96,    90,
      91,    94,    83,    75,    94,    73,    74,    75,    86,    95,
      87,     4,    38,    12,    15,    16,    25,    27,    31,    34,
      83,    52,     7,    12,    17,     6,    22,    28,    29,    37,
      44,    76,    77,    78,    41,     7,    40,    41,    87,    87,
      23,    33,    47,    53,    88,    92,    93,    97,    88,    88,
      35,    36,    88,    88,    88,    41,    87,    90,    92,    30,
      32,    49,    35,    17,    75,    26,    97,    36,    51,    79,
      97,    36,    83,    30,    41,    89,    92,    41,     7,    49,
      89
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

  case 9:
#line 142 "sql.y"
    {
            SQL_input *sql = (SQL_input*) info;
            MSIVIEW *insert = NULL;

            INSERT_CreateView( sql->db, &insert, (yyvsp[-7].string), (yyvsp[-5].column_list), (yyvsp[-1].column_list), FALSE );
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

            ALTER_CreateView( sql->db, &alter, (yyvsp[-1].string), NULL, (yyvsp[0].integer) );
            if( !alter )
                YYABORT;
            (yyval.query) = alter;
        ;}
    break;

  case 17:
#line 238 "sql.y"
    {
            SQL_input *sql = (SQL_input *)info;
            MSIVIEW *alter = NULL;

            ALTER_CreateView( sql->db, &alter, (yyvsp[-2].string), (yyvsp[0].column_list), 0 );
            if (!alter)
                YYABORT;
            (yyval.query) = alter;
        ;}
    break;

  case 18:
#line 248 "sql.y"
    {
            SQL_input *sql = (SQL_input *)info;
            MSIVIEW *alter = NULL;

            ALTER_CreateView( sql->db, &alter, (yyvsp[-3].string), (yyvsp[-1].column_list), 1 );
            if (!alter)
                YYABORT;
            (yyval.query) = alter;
        ;}
    break;

  case 19:
#line 261 "sql.y"
    {
            (yyval.integer) = 1;
        ;}
    break;

  case 20:
#line 265 "sql.y"
    {
            (yyval.integer) = -1;
        ;}
    break;

  case 21:
#line 272 "sql.y"
    {
            if( SQL_MarkPrimaryKeys( (yyvsp[-3].column_list), (yyvsp[0].column_list) ) )
                (yyval.column_list) = (yyvsp[-3].column_list);
            else
                (yyval.column_list) = NULL;
        ;}
    break;

  case 22:
#line 282 "sql.y"
    {
            column_info *ci;

            for( ci = (yyvsp[-2].column_list); ci->next; ci = ci->next )
                ;

            ci->next = (yyvsp[0].column_list);
            (yyval.column_list) = (yyvsp[-2].column_list);
        ;}
    break;

  case 23:
#line 292 "sql.y"
    {
            (yyval.column_list) = (yyvsp[0].column_list);
        ;}
    break;

  case 24:
#line 299 "sql.y"
    {
            (yyval.column_list) = (yyvsp[-1].column_list);
            (yyval.column_list)->type = ((yyvsp[0].column_type) | MSITYPE_VALID);
            (yyval.column_list)->temporary = (yyvsp[0].column_type) & MSITYPE_TEMPORARY ? TRUE : FALSE;
        ;}
    break;

  case 25:
#line 308 "sql.y"
    {
            (yyval.column_type) = (yyvsp[0].column_type);
        ;}
    break;

  case 26:
#line 312 "sql.y"
    {
            (yyval.column_type) = (yyvsp[-1].column_type) | MSITYPE_LOCALIZABLE;
        ;}
    break;

  case 27:
#line 316 "sql.y"
    {
            (yyval.column_type) = (yyvsp[-1].column_type) | MSITYPE_TEMPORARY;
        ;}
    break;

  case 28:
#line 323 "sql.y"
    {
            (yyval.column_type) |= MSITYPE_NULLABLE;
        ;}
    break;

  case 29:
#line 327 "sql.y"
    {
            (yyval.column_type) = (yyvsp[-2].column_type);
        ;}
    break;

  case 30:
#line 334 "sql.y"
    {
            (yyval.column_type) = MSITYPE_STRING | 1;
        ;}
    break;

  case 31:
#line 338 "sql.y"
    {
            (yyval.column_type) = MSITYPE_STRING | 0x400 | (yyvsp[-1].column_type);
        ;}
    break;

  case 32:
#line 342 "sql.y"
    {
            (yyval.column_type) = 2;
        ;}
    break;

  case 33:
#line 346 "sql.y"
    {
            (yyval.column_type) = 2;
        ;}
    break;

  case 34:
#line 350 "sql.y"
    {
            (yyval.column_type) = 2;
        ;}
    break;

  case 35:
#line 354 "sql.y"
    {
            (yyval.column_type) = 4;
        ;}
    break;

  case 36:
#line 358 "sql.y"
    {
            (yyval.column_type) = MSITYPE_STRING | MSITYPE_VALID;
        ;}
    break;

  case 37:
#line 365 "sql.y"
    {
            if( ( (yyvsp[0].integer) > 255 ) || ( (yyvsp[0].integer) < 0 ) )
                YYABORT;
            (yyval.column_type) = (yyvsp[0].integer);
        ;}
    break;

  case 38:
#line 374 "sql.y"
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

  case 40:
#line 391 "sql.y"
    {
            (yyval.query) = (yyvsp[0].query);
        ;}
    break;

  case 41:
#line 395 "sql.y"
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

  case 42:
#line 411 "sql.y"
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

  case 44:
#line 433 "sql.y"
    {
            (yyvsp[-2].column_list)->next = (yyvsp[0].column_list);
        ;}
    break;

  case 45:
#line 437 "sql.y"
    {
            (yyval.column_list) = NULL;
        ;}
    break;

  case 47:
#line 445 "sql.y"
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

  case 48:
#line 461 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            UINT r;

            (yyval.query) = NULL;
            r = TABLE_CreateView( sql->db, (yyvsp[0].string), &(yyval.query) );
            if( r != ERROR_SUCCESS || !(yyval.query) )
                YYABORT;
        ;}
    break;

  case 49:
#line 471 "sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            UINT r;

            r = JOIN_CreateView( sql->db, &(yyval.query), (yyvsp[0].string) );
            msi_free( (yyvsp[0].string) );
            if( r != ERROR_SUCCESS )
                YYABORT;
        ;}
    break;

  case 50:
#line 484 "sql.y"
    {
            (yyval.string) = strdupW((yyvsp[0].string));
        ;}
    break;

  case 51:
#line 489 "sql.y"
    {
            (yyval.string) = parser_add_table((yyvsp[0].string), (yyvsp[-2].string));
            if (!(yyval.string))
                YYABORT;
        ;}
    break;

  case 52:
#line 498 "sql.y"
    {
            (yyval.expr) = (yyvsp[-1].expr);
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 53:
#line 504 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_AND, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 54:
#line 510 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_OR, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 55:
#line 516 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_EQ, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 56:
#line 522 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_GT, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 57:
#line 528 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_LT, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 58:
#line 534 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_LE, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 59:
#line 540 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_GE, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 60:
#line 546 "sql.y"
    {
            (yyval.expr) = EXPR_complex( info, (yyvsp[-2].expr), OP_NE, (yyvsp[0].expr) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 61:
#line 552 "sql.y"
    {
            (yyval.expr) = EXPR_unary( info, (yyvsp[-2].expr), OP_ISNULL );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 62:
#line 558 "sql.y"
    {
            (yyval.expr) = EXPR_unary( info, (yyvsp[-3].expr), OP_NOTNULL );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 65:
#line 572 "sql.y"
    {
            (yyval.column_list) = parser_alloc_column( info, NULL, NULL );
            if( !(yyval.column_list) )
                YYABORT;
            (yyval.column_list)->val = (yyvsp[0].expr);
        ;}
    break;

  case 66:
#line 579 "sql.y"
    {
            (yyval.column_list) = parser_alloc_column( info, NULL, NULL );
            if( !(yyval.column_list) )
                YYABORT;
            (yyval.column_list)->val = (yyvsp[-2].expr);
            (yyval.column_list)->next = (yyvsp[0].column_list);
        ;}
    break;

  case 68:
#line 591 "sql.y"
    {
            (yyval.column_list) = (yyvsp[-2].column_list);
            (yyval.column_list)->next = (yyvsp[0].column_list);
        ;}
    break;

  case 69:
#line 599 "sql.y"
    {
            (yyval.column_list) = (yyvsp[-2].column_list);
            (yyval.column_list)->val = (yyvsp[0].expr);
        ;}
    break;

  case 70:
#line 607 "sql.y"
    {
            (yyval.expr) = EXPR_ival( info, (yyvsp[0].integer) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 71:
#line 613 "sql.y"
    {
            (yyval.expr) = EXPR_ival( info, -(yyvsp[0].integer) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 72:
#line 619 "sql.y"
    {
            (yyval.expr) = EXPR_sval( info, &(yyvsp[0].str) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 73:
#line 625 "sql.y"
    {
            (yyval.expr) = EXPR_wildcard( info );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 74:
#line 634 "sql.y"
    {
            (yyval.expr) = EXPR_column( info, (yyvsp[0].column_list) );
            if( !(yyval.expr) )
                YYABORT;
        ;}
    break;

  case 75:
#line 643 "sql.y"
    {
            (yyval.column_list) = parser_alloc_column( info, (yyvsp[-2].string), (yyvsp[0].string) );
            if( !(yyval.column_list) )
                YYABORT;
        ;}
    break;

  case 76:
#line 649 "sql.y"
    {
            (yyval.column_list) = parser_alloc_column( info, NULL, (yyvsp[0].string) );
            if( !(yyval.column_list) )
                YYABORT;
        ;}
    break;

  case 77:
#line 658 "sql.y"
    {
            (yyval.string) = (yyvsp[0].string);
        ;}
    break;

  case 78:
#line 665 "sql.y"
    {
            (yyval.string) = SQL_getstring( info, &(yyvsp[0].str) );
            if( !(yyval.string) )
                YYABORT;
        ;}
    break;

  case 79:
#line 674 "sql.y"
    {
            (yyval.integer) = SQL_getint( info );
        ;}
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
#line 2080 "sql.tab.c"

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


#line 679 "sql.y"


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

