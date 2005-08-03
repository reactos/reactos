/* A Bison parser, made from ./sql.y
   by GNU bison 1.35.  */

#define YYBISON 1  /* Identify Bison output.  */

#define yyparse SQL_parse
#define yylex SQL_lex
#define yyerror SQL_error
#define yylval SQL_lval
#define yychar SQL_char
#define yydebug SQL_debug
#define yynerrs SQL_nerrs
# define	TK_ABORT	257
# define	TK_AFTER	258
# define	TK_AGG_FUNCTION	259
# define	TK_ALL	260
# define	TK_AND	261
# define	TK_AS	262
# define	TK_ASC	263
# define	TK_BEFORE	264
# define	TK_BEGIN	265
# define	TK_BETWEEN	266
# define	TK_BITAND	267
# define	TK_BITNOT	268
# define	TK_BITOR	269
# define	TK_BY	270
# define	TK_CASCADE	271
# define	TK_CASE	272
# define	TK_CHAR	273
# define	TK_CHECK	274
# define	TK_CLUSTER	275
# define	TK_COLLATE	276
# define	TK_COLUMN	277
# define	TK_COMMA	278
# define	TK_COMMENT	279
# define	TK_COMMIT	280
# define	TK_CONCAT	281
# define	TK_CONFLICT	282
# define	TK_CONSTRAINT	283
# define	TK_COPY	284
# define	TK_CREATE	285
# define	TK_DEFAULT	286
# define	TK_DEFERRABLE	287
# define	TK_DEFERRED	288
# define	TK_DELETE	289
# define	TK_DELIMITERS	290
# define	TK_DESC	291
# define	TK_DISTINCT	292
# define	TK_DOT	293
# define	TK_DROP	294
# define	TK_EACH	295
# define	TK_ELSE	296
# define	TK_END	297
# define	TK_END_OF_FILE	298
# define	TK_EQ	299
# define	TK_EXCEPT	300
# define	TK_EXPLAIN	301
# define	TK_FAIL	302
# define	TK_FLOAT	303
# define	TK_FOR	304
# define	TK_FOREIGN	305
# define	TK_FROM	306
# define	TK_FUNCTION	307
# define	TK_GE	308
# define	TK_GLOB	309
# define	TK_GROUP	310
# define	TK_GT	311
# define	TK_HAVING	312
# define	TK_HOLD	313
# define	TK_IGNORE	314
# define	TK_ILLEGAL	315
# define	TK_IMMEDIATE	316
# define	TK_IN	317
# define	TK_INDEX	318
# define	TK_INITIALLY	319
# define	TK_ID	320
# define	TK_INSERT	321
# define	TK_INSTEAD	322
# define	TK_INT	323
# define	TK_INTEGER	324
# define	TK_INTERSECT	325
# define	TK_INTO	326
# define	TK_IS	327
# define	TK_ISNULL	328
# define	TK_JOIN	329
# define	TK_JOIN_KW	330
# define	TK_KEY	331
# define	TK_LE	332
# define	TK_LIKE	333
# define	TK_LIMIT	334
# define	TK_LONG	335
# define	TK_LONGCHAR	336
# define	TK_LP	337
# define	TK_LSHIFT	338
# define	TK_LT	339
# define	TK_LOCALIZABLE	340
# define	TK_MATCH	341
# define	TK_MINUS	342
# define	TK_NE	343
# define	TK_NOT	344
# define	TK_NOTNULL	345
# define	TK_NULL	346
# define	TK_OBJECT	347
# define	TK_OF	348
# define	TK_OFFSET	349
# define	TK_ON	350
# define	TK_OR	351
# define	TK_ORACLE_OUTER_JOIN	352
# define	TK_ORDER	353
# define	TK_PLUS	354
# define	TK_PRAGMA	355
# define	TK_PRIMARY	356
# define	TK_RAISE	357
# define	TK_REFERENCES	358
# define	TK_REM	359
# define	TK_REPLACE	360
# define	TK_RESTRICT	361
# define	TK_ROLLBACK	362
# define	TK_ROW	363
# define	TK_RP	364
# define	TK_RSHIFT	365
# define	TK_SELECT	366
# define	TK_SEMI	367
# define	TK_SET	368
# define	TK_SHORT	369
# define	TK_SLASH	370
# define	TK_SPACE	371
# define	TK_STAR	372
# define	TK_STATEMENT	373
# define	TK_STRING	374
# define	TK_TABLE	375
# define	TK_TEMP	376
# define	TK_THEN	377
# define	TK_TRANSACTION	378
# define	TK_TRIGGER	379
# define	TK_UMINUS	380
# define	TK_UNCLOSED_STRING	381
# define	TK_UNION	382
# define	TK_UNIQUE	383
# define	TK_UPDATE	384
# define	TK_UPLUS	385
# define	TK_USING	386
# define	TK_VACUUM	387
# define	TK_VALUES	388
# define	TK_VIEW	389
# define	TK_WHEN	390
# define	TK_WHERE	391
# define	TK_WILDCARD	392
# define	END_OF_FILE	393
# define	ILLEGAL	394
# define	SPACE	395
# define	UNCLOSED_STRING	396
# define	COMMENT	397
# define	FUNCTION	398
# define	COLUMN	399

#line 1 "./sql.y"


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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

extern int SQL_error(const char *str);

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
static struct expr * EXPR_column( void *info, column_info *column );
static struct expr * EXPR_ival( void *info, int val );
static struct expr * EXPR_sval( void *info, struct sql_str * );
static struct expr * EXPR_wildcard( void *info );


#line 71 "./sql.y"
#ifndef YYSTYPE
typedef union
{
    struct sql_str str;
    LPWSTR string;
    column_info *column_list;
    MSIVIEW *query;
    struct expr *expr;
    USHORT column_type;
    int integer;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
#ifndef YYDEBUG
# define YYDEBUG 0
#endif



#define	YYFINAL		127
#define	YYFLAG		-32768
#define	YYNTBASE	147

/* YYTRANSLATE(YYLEX) -- Bison token number corresponding to YYLEX. */
#define YYTRANSLATE(x) ((unsigned)(x) <= 400 ? yytranslate[x] : 177)

/* YYTRANSLATE[YYLEX] -- Bison token number corresponding to YYLEX. */
static const short yytranslate[] =
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
       2,     2,     2,     2,     2,     2,     1,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146
};

#if YYDEBUG
static const short yyprhs[] =
{
       0,     0,     2,     4,     6,     8,    10,    12,    23,    35,
      42,    50,    57,    60,    65,    69,    71,    74,    76,    79,
      81,    85,    87,    92,    94,    96,    98,   100,   102,   104,
     109,   111,   114,   118,   121,   123,   127,   129,   131,   135,
     138,   142,   146,   150,   154,   158,   162,   166,   170,   174,
     178,   182,   187,   189,   191,   193,   197,   199,   203,   207,
     209,   212,   214,   216,   218,   222,   224,   226,   228
};
static const short yyrhs[] =
{
     148,     0,   160,     0,   150,     0,   149,     0,   151,     0,
     152,     0,    67,    72,   174,    83,   163,   110,   134,    83,
     168,   110,     0,    67,    72,   174,    83,   163,   110,   134,
      83,   168,   110,   122,     0,    31,   121,   174,    83,   153,
     110,     0,    31,   121,   174,    83,   153,   110,    59,     0,
     130,   174,   114,   169,   137,   166,     0,    35,   164,     0,
     154,   102,    77,   163,     0,   154,    24,   155,     0,   155,
       0,   173,   156,     0,   157,     0,   157,    86,     0,   158,
       0,   158,    90,    92,     0,    19,     0,    19,    83,   159,
     110,     0,    82,     0,   115,     0,    69,     0,    81,     0,
      93,     0,   176,     0,   161,    99,    16,   163,     0,   161,
       0,   112,   162,     0,   112,    38,   162,     0,   163,   164,
       0,   173,     0,   173,    24,   163,     0,   118,     0,   165,
       0,   165,   137,   166,     0,    52,   174,     0,    83,   166,
     110,     0,   172,    45,   172,     0,   166,     7,   166,     0,
     166,    97,   166,     0,   172,    45,   167,     0,   172,    57,
     167,     0,   172,    85,   167,     0,   172,    78,   167,     0,
     172,    54,   167,     0,   172,    89,   167,     0,   172,    73,
      92,     0,   172,    73,    90,    92,     0,   172,     0,   171,
       0,   171,     0,   171,    24,   168,     0,   170,     0,   170,
      24,   169,     0,   173,    45,   171,     0,   176,     0,    88,
     176,     0,   120,     0,   138,     0,   173,     0,   174,    39,
     175,     0,   175,     0,   175,     0,    66,     0,    70,     0
};

#endif

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined. */
static const short yyrline[] =
{
       0,   137,   145,   147,   148,   149,   150,   153,   165,   177,
     190,   204,   217,   230,   240,   251,   257,   265,   270,   277,
     282,   288,   293,   297,   301,   305,   309,   313,   319,   328,
     341,   344,   349,   360,   376,   378,   382,   388,   390,   402,
     415,   422,   428,   434,   440,   446,   452,   458,   464,   470,
     476,   482,   490,   492,   495,   503,   513,   515,   522,   530,
     537,   543,   549,   557,   566,   573,   581,   588,   597
};
#endif


#if (YYDEBUG) || defined YYERROR_VERBOSE

/* YYTNAME[TOKEN_NUM] -- String name of the token TOKEN_NUM. */
static const char *const yytname[] =
{
  "$", "error", "$undefined.", "TK_ABORT", "TK_AFTER", "TK_AGG_FUNCTION", 
  "TK_ALL", "TK_AND", "TK_AS", "TK_ASC", "TK_BEFORE", "TK_BEGIN", 
  "TK_BETWEEN", "TK_BITAND", "TK_BITNOT", "TK_BITOR", "TK_BY", 
  "TK_CASCADE", "TK_CASE", "TK_CHAR", "TK_CHECK", "TK_CLUSTER", 
  "TK_COLLATE", "TK_COLUMN", "TK_COMMA", "TK_COMMENT", "TK_COMMIT", 
  "TK_CONCAT", "TK_CONFLICT", "TK_CONSTRAINT", "TK_COPY", "TK_CREATE", 
  "TK_DEFAULT", "TK_DEFERRABLE", "TK_DEFERRED", "TK_DELETE", 
  "TK_DELIMITERS", "TK_DESC", "TK_DISTINCT", "TK_DOT", "TK_DROP", 
  "TK_EACH", "TK_ELSE", "TK_END", "TK_END_OF_FILE", "TK_EQ", "TK_EXCEPT", 
  "TK_EXPLAIN", "TK_FAIL", "TK_FLOAT", "TK_FOR", "TK_FOREIGN", "TK_FROM", 
  "TK_FUNCTION", "TK_GE", "TK_GLOB", "TK_GROUP", "TK_GT", "TK_HAVING", 
  "TK_HOLD", "TK_IGNORE", "TK_ILLEGAL", "TK_IMMEDIATE", "TK_IN", 
  "TK_INDEX", "TK_INITIALLY", "TK_ID", "TK_INSERT", "TK_INSTEAD", 
  "TK_INT", "TK_INTEGER", "TK_INTERSECT", "TK_INTO", "TK_IS", "TK_ISNULL", 
  "TK_JOIN", "TK_JOIN_KW", "TK_KEY", "TK_LE", "TK_LIKE", "TK_LIMIT", 
  "TK_LONG", "TK_LONGCHAR", "TK_LP", "TK_LSHIFT", "TK_LT", 
  "TK_LOCALIZABLE", "TK_MATCH", "TK_MINUS", "TK_NE", "TK_NOT", 
  "TK_NOTNULL", "TK_NULL", "TK_OBJECT", "TK_OF", "TK_OFFSET", "TK_ON", 
  "TK_OR", "TK_ORACLE_OUTER_JOIN", "TK_ORDER", "TK_PLUS", "TK_PRAGMA", 
  "TK_PRIMARY", "TK_RAISE", "TK_REFERENCES", "TK_REM", "TK_REPLACE", 
  "TK_RESTRICT", "TK_ROLLBACK", "TK_ROW", "TK_RP", "TK_RSHIFT", 
  "TK_SELECT", "TK_SEMI", "TK_SET", "TK_SHORT", "TK_SLASH", "TK_SPACE", 
  "TK_STAR", "TK_STATEMENT", "TK_STRING", "TK_TABLE", "TK_TEMP", 
  "TK_THEN", "TK_TRANSACTION", "TK_TRIGGER", "TK_UMINUS", 
  "TK_UNCLOSED_STRING", "TK_UNION", "TK_UNIQUE", "TK_UPDATE", "TK_UPLUS", 
  "TK_USING", "TK_VACUUM", "TK_VALUES", "TK_VIEW", "TK_WHEN", "TK_WHERE", 
  "TK_WILDCARD", "END_OF_FILE", "ILLEGAL", "SPACE", "UNCLOSED_STRING", 
  "COMMENT", "FUNCTION", "COLUMN", "AGG_FUNCTION.", "query", "onequery", 
  "oneinsert", "onecreate", "oneupdate", "onedelete", "table_def", 
  "column_def", "column_and_type", "column_type", "data_type_l", 
  "data_type", "data_count", "oneselect", "unorderedsel", "selectfrom", 
  "selcollist", "from", "fromtable", "expr", "val", "constlist", 
  "update_assign_list", "column_assignment", "const_val", "column_val", 
  "column", "table", "id", "number", 0
};
#endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives. */
static const short yyr1[] =
{
       0,   147,   148,   148,   148,   148,   148,   149,   149,   150,
     150,   151,   152,   153,   154,   154,   155,   156,   156,   157,
     157,   158,   158,   158,   158,   158,   158,   158,   159,   160,
     160,   161,   161,   162,   163,   163,   163,   164,   164,   165,
     166,   166,   166,   166,   166,   166,   166,   166,   166,   166,
     166,   166,   167,   167,   168,   168,   169,   169,   170,   171,
     171,   171,   171,   172,   173,   173,   174,   175,   176
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN. */
static const short yyr2[] =
{
       0,     1,     1,     1,     1,     1,     1,    10,    11,     6,
       7,     6,     2,     4,     3,     1,     2,     1,     2,     1,
       3,     1,     4,     1,     1,     1,     1,     1,     1,     4,
       1,     2,     3,     2,     1,     3,     1,     1,     3,     2,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     4,     1,     1,     1,     3,     1,     3,     3,     1,
       2,     1,     1,     1,     3,     1,     1,     1,     1
};

/* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
   doesn't specify something else to do.  Zero means the default is an
   error. */
static const short yydefact[] =
{
       0,     0,     0,     0,     0,     0,     1,     4,     3,     5,
       6,     2,    30,     0,     0,    12,    37,     0,     0,    67,
      36,    31,     0,    34,     0,    65,     0,    66,     0,     0,
      39,     0,     0,    32,    33,     0,     0,     0,     0,     0,
       0,    38,     0,    63,     0,    35,    64,     0,    56,     0,
      29,     0,     0,    15,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     9,
       0,     0,    21,    25,    26,    23,    27,    24,    16,    17,
      19,    40,    42,    43,    68,     0,    61,    62,    44,    53,
      41,    59,    48,    52,    45,     0,    50,    47,    46,    49,
       0,    11,    57,    58,    10,    14,     0,     0,    18,     0,
      60,    51,     0,    13,     0,    28,    20,     0,    22,     0,
      54,     7,     0,     8,    55,     0,     0,     0
};

static const short yydefgoto[] =
{
     125,     6,     7,     8,     9,    10,    51,    52,    53,    78,
      79,    80,   114,    11,    12,    21,    22,    15,    16,    41,
      88,   119,    47,    48,    89,    42,    43,    24,    25,    91
};

static const short yypact[] =
{
     -28,  -106,   -27,   -52,   -32,   -42,-32768,-32768,-32768,-32768,
  -32768,-32768,   -70,   -42,   -42,-32768,   -97,   -42,   -48,-32768,
  -32768,-32768,   -27,     7,     2,     5,   -62,-32768,    40,   -23,
  -32768,   -55,   -22,-32768,-32768,   -48,   -42,   -42,   -48,   -42,
     -55,    -3,     0,-32768,   -48,-32768,-32768,   -65,    43,    29,
  -32768,   -34,   -19,-32768,   -18,    -7,   -55,   -55,   -58,   -58,
     -58,   -37,   -58,   -58,   -58,   -31,   -55,   -42,   -61,    23,
     -42,    10,     8,-32768,-32768,-32768,-32768,-32768,-32768,     6,
       3,-32768,    -3,    -3,-32768,    18,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,    16,-32768,-32768,-32768,-32768,
     -33,    -3,-32768,-32768,-32768,-32768,   -48,    18,-32768,    20,
  -32768,-32768,    30,-32768,     4,-32768,-32768,   -61,-32768,    11,
      91,    -6,   -61,-32768,-32768,   117,   118,-32768
};

static const short yypgoto[] =
{
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,    49,-32768,
  -32768,-32768,-32768,-32768,-32768,   102,   -25,   100,-32768,    -8,
      36,     1,    57,-32768,   -51,    47,    -2,     9,    33,   -64
};


#define	YYLAST		124


static const short yytable[] =
{
      56,    72,    23,     1,    56,    70,    18,     2,    19,    84,
      45,    19,    84,    50,    26,    13,    23,   103,    19,    65,
      17,   110,    29,    30,    19,    14,    32,    85,    40,    28,
      85,    35,    55,    23,    19,    49,    23,    54,    27,     3,
      31,    36,    23,   115,   -66,    58,    27,    27,    82,    83,
      27,    73,    37,    95,    59,    96,    38,    60,   101,    86,
      39,    44,    86,    74,    75,    49,   120,    67,    54,    46,
      20,   120,    66,    61,    68,    76,    69,    87,    62,   100,
      87,   113,   104,    71,     4,    63,    20,   106,    84,    64,
      57,   107,   108,   109,    57,    92,    94,    77,    97,    98,
      99,   112,     5,    81,    23,    90,    93,    93,   111,    93,
      93,    93,   116,   117,   118,   122,   123,   126,   127,   105,
      33,   121,    34,   124,   102
};

static const short yycheck[] =
{
       7,    19,     4,    31,     7,    24,    38,    35,    66,    70,
      35,    66,    70,    38,     5,   121,    18,    68,    66,    44,
      72,    85,    13,    14,    66,    52,    17,    88,    83,    99,
      88,    24,    40,    35,    66,    37,    38,    39,     5,    67,
     137,    39,    44,   107,    39,    45,    13,    14,    56,    57,
      17,    69,   114,    90,    54,    92,    16,    57,    66,   120,
      83,    83,   120,    81,    82,    67,   117,    24,    70,    36,
     118,   122,   137,    73,    45,    93,   110,   138,    78,   110,
     138,   106,    59,   102,   112,    85,   118,    77,    70,    89,
      97,    83,    86,    90,    97,    59,    60,   115,    62,    63,
      64,   134,   130,   110,   106,    58,    59,    60,    92,    62,
      63,    64,    92,    83,   110,    24,   122,     0,     0,    70,
      18,   110,    22,   122,    67
};
#define YYPURE 1

/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/share/bison/bison.simple"

/* Skeleton output parser for bison,

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software
   Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser when
   the %semantic_parser declaration is not specified in the grammar.
   It was written by Richard Stallman by simplifying the hairy parser
   used when %semantic_parser is specified.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

#if ! defined (yyoverflow) || defined (YYERROR_VERBOSE)

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || defined (YYERROR_VERBOSE) */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYLTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
# if YYLSP_NEEDED
  YYLTYPE yyls;
# endif
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAX (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# if YYLSP_NEEDED
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE) + sizeof (YYLTYPE))	\
      + 2 * YYSTACK_GAP_MAX)
# else
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAX)
# endif

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
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
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAX;	\
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif


#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
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
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");			\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).

   When YYLLOC_DEFAULT is run, CURRENT is set the location of the
   first token.  By default, to implement support for ranges, extend
   its range to the last symbol.  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)       	\
   Current.last_line   = Rhs[N].last_line;	\
   Current.last_column = Rhs[N].last_column;
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#if YYPURE
# if YYLSP_NEEDED
#  ifdef YYLEX_PARAM
#   define YYLEX		yylex (&yylval, &yylloc, YYLEX_PARAM)
#  else
#   define YYLEX		yylex (&yylval, &yylloc)
#  endif
# else /* !YYLSP_NEEDED */
#  ifdef YYLEX_PARAM
#   define YYLEX		yylex (&yylval, YYLEX_PARAM)
#  else
#   define YYLEX		yylex (&yylval)
#  endif
# endif /* !YYLSP_NEEDED */
#else /* !YYPURE */
# define YYLEX			yylex ()
#endif /* !YYPURE */


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
/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
#endif /* !YYDEBUG */

/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif

#ifdef YYERROR_VERBOSE

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
  register const char *yys = yystr;

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
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif
#endif

#line 315 "/usr/share/bison/bison.simple"


/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
#  define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL
# else
#  define YYPARSE_PARAM_ARG YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
# endif
#else /* !YYPARSE_PARAM */
# define YYPARSE_PARAM_ARG
# define YYPARSE_PARAM_DECL
#endif /* !YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
# ifdef YYPARSE_PARAM
int yyparse (void *);
# else
int yyparse (void);
# endif
#endif

/* YY_DECL_VARIABLES -- depending whether we use a pure parser,
   variables are global, or local to YYPARSE.  */

#define YY_DECL_NON_LSP_VARIABLES			\
/* The lookahead symbol.  */				\
int yychar;						\
							\
/* The semantic value of the lookahead symbol. */	\
YYSTYPE yylval;						\
							\
/* Number of parse errors so far.  */			\
int yynerrs;

#if YYLSP_NEEDED
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES			\
						\
/* Location data for the lookahead symbol.  */	\
YYLTYPE yylloc;
#else
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES
#endif


/* If nonreentrant, generate the variables here. */

#if !YYPURE
YY_DECL_VARIABLES
#endif  /* !YYPURE */

int
yyparse (YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  /* If reentrant, generate the variables here. */
#if YYPURE
  YY_DECL_VARIABLES
#endif  /* !YYPURE */

  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yychar1 = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack. */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;

#if YYLSP_NEEDED
  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
#endif

#if YYLSP_NEEDED
# define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
# define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  YYSIZE_T yystacksize = YYINITDEPTH;


  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
#if YYLSP_NEEDED
  YYLTYPE yyloc;
#endif

  /* When reducing, the number of symbols on the RHS of the reduced
     rule. */
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
#if YYLSP_NEEDED
  yylsp = yyls;
#endif
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

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  */
# if YYLSP_NEEDED
	YYLTYPE *yyls1 = yyls;
	/* This used to be a conditional around just the two extra args,
	   but that might be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);
	yyls = yyls1;
# else
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);
# endif
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
# if YYLSP_NEEDED
	YYSTACK_RELOCATE (yyls);
# endif
# undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
#if YYLSP_NEEDED
      yylsp = yyls + yysize - 1;
#endif

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yychar1 = YYTRANSLATE (yychar);

#if YYDEBUG
     /* We have to keep this `#if YYDEBUG', since we use variables
	which are defined only if `YYDEBUG' is set.  */
      if (yydebug)
	{
	  YYFPRINTF (stderr, "Next token is %d (%s",
		     yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise
	     meaning of a token, for further debugging info.  */
# ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
# endif
	  YYFPRINTF (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %d (%s), ",
	      yychar, yytname[yychar1]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#if YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

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

     Otherwise, the following line sets YYVAL to the semantic value of
     the lookahead token.  This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

#if YYLSP_NEEDED
  /* Similarly for the default location.  Let the user run additional
     commands if for instance locations are ranges.  */
  yyloc = yylsp[1-yylen];
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
#endif

#if YYDEBUG
  /* We have to keep this `#if YYDEBUG', since we use variables which
     are defined only if `YYDEBUG' is set.  */
  if (yydebug)
    {
      int yyi;

      YYFPRINTF (stderr, "Reducing via rule %d (line %d), ",
		 yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (yyi = yyprhs[yyn]; yyrhs[yyi] > 0; yyi++)
	YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
      YYFPRINTF (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif

  switch (yyn) {

case 1:
#line 139 "./sql.y"
{
        SQL_input* sql = (SQL_input*) info;
        *sql->view = yyvsp[0].query;
    ;
    break;}
case 7:
#line 155 "./sql.y"
{
            SQL_input *sql = (SQL_input*) info;
            MSIVIEW *insert = NULL; 
            UINT r;

            r = INSERT_CreateView( sql->db, &insert, yyvsp[-7].string, yyvsp[-5].column_list, yyvsp[-1].column_list, FALSE ); 
            if( !insert )
                YYABORT;
            yyval.query = insert;
        ;
    break;}
case 8:
#line 166 "./sql.y"
{
            SQL_input *sql = (SQL_input*) info;
            MSIVIEW *insert = NULL; 

            INSERT_CreateView( sql->db, &insert, yyvsp[-8].string, yyvsp[-6].column_list, yyvsp[-2].column_list, TRUE ); 
            if( !insert )
                YYABORT;
            yyval.query = insert;
        ;
    break;}
case 9:
#line 179 "./sql.y"
{
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *create = NULL; 

            if( !yyvsp[-1].column_list )
                YYABORT;
            CREATE_CreateView( sql->db, &create, yyvsp[-3].string, yyvsp[-1].column_list, FALSE );
            if( !create )
                YYABORT;
            yyval.query = create;
        ;
    break;}
case 10:
#line 191 "./sql.y"
{
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *create = NULL; 

            if( !yyvsp[-2].column_list )
                YYABORT;
            CREATE_CreateView( sql->db, &create, yyvsp[-4].string, yyvsp[-2].column_list, TRUE );
            if( !create )
                YYABORT;
            yyval.query = create;
        ;
    break;}
case 11:
#line 206 "./sql.y"
{
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *update = NULL; 

            UPDATE_CreateView( sql->db, &update, yyvsp[-4].string, yyvsp[-2].column_list, yyvsp[0].expr );
            if( !update )
                YYABORT;
            yyval.query = update;
        ;
    break;}
case 12:
#line 219 "./sql.y"
{
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *delete = NULL; 

            DELETE_CreateView( sql->db, &delete, yyvsp[0].query );
            if( !delete )
                YYABORT;
            yyval.query = delete;
        ;
    break;}
case 13:
#line 232 "./sql.y"
{
            if( SQL_MarkPrimaryKeys( yyvsp[-3].column_list, yyvsp[0].column_list ) )
                yyval.column_list = yyvsp[-3].column_list;
            else
                yyval.column_list = NULL;
        ;
    break;}
case 14:
#line 242 "./sql.y"
{
            column_info *ci;

            for( ci = yyvsp[-2].column_list; ci->next; ci = ci->next )
                ;

            ci->next = yyvsp[0].column_list;
            yyval.column_list = yyvsp[-2].column_list;
        ;
    break;}
case 15:
#line 252 "./sql.y"
{
            yyval.column_list = yyvsp[0].column_list;
        ;
    break;}
case 16:
#line 259 "./sql.y"
{
            yyval.column_list = yyvsp[-1].column_list;
            yyval.column_list->type = yyvsp[0].column_type;
        ;
    break;}
case 17:
#line 267 "./sql.y"
{
            yyval.column_type = yyvsp[0].column_type | MSITYPE_VALID;
        ;
    break;}
case 18:
#line 271 "./sql.y"
{
            FIXME("LOCALIZABLE ignored\n");
            yyval.column_type = yyvsp[-1].column_type | MSITYPE_VALID;
        ;
    break;}
case 19:
#line 279 "./sql.y"
{
            yyval.column_type |= MSITYPE_NULLABLE;
        ;
    break;}
case 20:
#line 283 "./sql.y"
{
            yyval.column_type = yyvsp[-2].column_type;
        ;
    break;}
case 21:
#line 290 "./sql.y"
{
            yyval.column_type = MSITYPE_STRING | 1;
        ;
    break;}
case 22:
#line 294 "./sql.y"
{
            yyval.column_type = MSITYPE_STRING | 0x400 | yyvsp[-1].column_type;
        ;
    break;}
case 23:
#line 298 "./sql.y"
{
            yyval.column_type = 2;
        ;
    break;}
case 24:
#line 302 "./sql.y"
{
            yyval.column_type = 2;
        ;
    break;}
case 25:
#line 306 "./sql.y"
{
            yyval.column_type = 2;
        ;
    break;}
case 26:
#line 310 "./sql.y"
{
            yyval.column_type = 4;
        ;
    break;}
case 27:
#line 314 "./sql.y"
{
            yyval.column_type = 0;
        ;
    break;}
case 28:
#line 321 "./sql.y"
{
            if( ( yyvsp[0].integer > 255 ) || ( yyvsp[0].integer < 0 ) )
                YYABORT;
            yyval.column_type = yyvsp[0].integer;
        ;
    break;}
case 29:
#line 330 "./sql.y"
{
            SQL_input* sql = (SQL_input*) info;

            yyval.query = NULL;
            if( yyvsp[0].column_list )
                ORDER_CreateView( sql->db, &yyval.query, yyvsp[-3].query, yyvsp[0].column_list );
            else
                yyval.query = yyvsp[-3].query;
            if( !yyval.query )
                YYABORT;
        ;
    break;}
case 31:
#line 346 "./sql.y"
{
            yyval.query = yyvsp[0].query;
        ;
    break;}
case 32:
#line 350 "./sql.y"
{
            SQL_input* sql = (SQL_input*) info;

            yyval.query = NULL;
            DISTINCT_CreateView( sql->db, &yyval.query, yyvsp[0].query );
            if( !yyval.query )
                YYABORT;
        ;
    break;}
case 33:
#line 362 "./sql.y"
{
            SQL_input* sql = (SQL_input*) info;

            yyval.query = NULL;
            if( yyvsp[-1].column_list )
                SELECT_CreateView( sql->db, &yyval.query, yyvsp[0].query, yyvsp[-1].column_list );
            else
                yyval.query = yyvsp[0].query;

            if( !yyval.query )
                YYABORT;
        ;
    break;}
case 35:
#line 379 "./sql.y"
{ 
            yyvsp[-2].column_list->next = yyvsp[0].column_list;
        ;
    break;}
case 36:
#line 383 "./sql.y"
{
            yyval.column_list = NULL;
        ;
    break;}
case 38:
#line 391 "./sql.y"
{ 
            SQL_input* sql = (SQL_input*) info;
            UINT r;

            yyval.query = NULL;
            r = WHERE_CreateView( sql->db, &yyval.query, yyvsp[-2].query, yyvsp[0].expr );
            if( r != ERROR_SUCCESS || !yyval.query )
                YYABORT;
        ;
    break;}
case 39:
#line 404 "./sql.y"
{
            SQL_input* sql = (SQL_input*) info;
            UINT r;

            yyval.query = NULL;
            r = TABLE_CreateView( sql->db, yyvsp[0].string, &yyval.query );
            if( r != ERROR_SUCCESS || !yyval.query )
                YYABORT;
        ;
    break;}
case 40:
#line 417 "./sql.y"
{
            yyval.expr = yyvsp[-1].expr;
            if( !yyval.expr )
                YYABORT;
        ;
    break;}
case 41:
#line 423 "./sql.y"
{
            yyval.expr = EXPR_complex( info, yyvsp[-2].expr, OP_EQ, yyvsp[0].expr );
            if( !yyval.expr )
                YYABORT;
        ;
    break;}
case 42:
#line 429 "./sql.y"
{
            yyval.expr = EXPR_complex( info, yyvsp[-2].expr, OP_AND, yyvsp[0].expr );
            if( !yyval.expr )
                YYABORT;
        ;
    break;}
case 43:
#line 435 "./sql.y"
{
            yyval.expr = EXPR_complex( info, yyvsp[-2].expr, OP_OR, yyvsp[0].expr );
            if( !yyval.expr )
                YYABORT;
        ;
    break;}
case 44:
#line 441 "./sql.y"
{
            yyval.expr = EXPR_complex( info, yyvsp[-2].expr, OP_EQ, yyvsp[0].expr );
            if( !yyval.expr )
                YYABORT;
        ;
    break;}
case 45:
#line 447 "./sql.y"
{
            yyval.expr = EXPR_complex( info, yyvsp[-2].expr, OP_GT, yyvsp[0].expr );
            if( !yyval.expr )
                YYABORT;
        ;
    break;}
case 46:
#line 453 "./sql.y"
{
            yyval.expr = EXPR_complex( info, yyvsp[-2].expr, OP_LT, yyvsp[0].expr );
            if( !yyval.expr )
                YYABORT;
        ;
    break;}
case 47:
#line 459 "./sql.y"
{
            yyval.expr = EXPR_complex( info, yyvsp[-2].expr, OP_LE, yyvsp[0].expr );
            if( !yyval.expr )
                YYABORT;
        ;
    break;}
case 48:
#line 465 "./sql.y"
{
            yyval.expr = EXPR_complex( info, yyvsp[-2].expr, OP_GE, yyvsp[0].expr );
            if( !yyval.expr )
                YYABORT;
        ;
    break;}
case 49:
#line 471 "./sql.y"
{
            yyval.expr = EXPR_complex( info, yyvsp[-2].expr, OP_NE, yyvsp[0].expr );
            if( !yyval.expr )
                YYABORT;
        ;
    break;}
case 50:
#line 477 "./sql.y"
{
            yyval.expr = EXPR_complex( info, yyvsp[-2].expr, OP_ISNULL, NULL );
            if( !yyval.expr )
                YYABORT;
        ;
    break;}
case 51:
#line 483 "./sql.y"
{
            yyval.expr = EXPR_complex( info, yyvsp[-3].expr, OP_NOTNULL, NULL );
            if( !yyval.expr )
                YYABORT;
        ;
    break;}
case 54:
#line 497 "./sql.y"
{
            yyval.column_list = parser_alloc_column( info, NULL, NULL );
            if( !yyval.column_list )
                YYABORT;
            yyval.column_list->val = yyvsp[0].expr;
        ;
    break;}
case 55:
#line 504 "./sql.y"
{
            yyval.column_list = parser_alloc_column( info, NULL, NULL );
            if( !yyval.column_list )
                YYABORT;
            yyval.column_list->val = yyvsp[-2].expr;
            yyval.column_list->next = yyvsp[0].column_list;
        ;
    break;}
case 57:
#line 516 "./sql.y"
{
            yyval.column_list = yyvsp[-2].column_list;
            yyval.column_list->next = yyvsp[0].column_list;
        ;
    break;}
case 58:
#line 524 "./sql.y"
{
            yyval.column_list = yyvsp[-2].column_list;
            yyval.column_list->val = yyvsp[0].expr;
        ;
    break;}
case 59:
#line 532 "./sql.y"
{
            yyval.expr = EXPR_ival( info, yyvsp[0].integer );
            if( !yyval.expr )
                YYABORT;
        ;
    break;}
case 60:
#line 538 "./sql.y"
{
            yyval.expr = EXPR_ival( info, -yyvsp[0].integer );
            if( !yyval.expr )
                YYABORT;
        ;
    break;}
case 61:
#line 544 "./sql.y"
{
            yyval.expr = EXPR_sval( info, &yyvsp[0].str );
            if( !yyval.expr )
                YYABORT;
        ;
    break;}
case 62:
#line 550 "./sql.y"
{
            yyval.expr = EXPR_wildcard( info );
            if( !yyval.expr )
                YYABORT;
        ;
    break;}
case 63:
#line 559 "./sql.y"
{
            yyval.expr = EXPR_column( info, yyvsp[0].column_list );
            if( !yyval.expr )
                YYABORT;
        ;
    break;}
case 64:
#line 568 "./sql.y"
{
            yyval.column_list = parser_alloc_column( info, yyvsp[-2].string, yyvsp[0].string );
            if( !yyval.column_list )
                YYABORT;
        ;
    break;}
case 65:
#line 574 "./sql.y"
{
            yyval.column_list = parser_alloc_column( info, NULL, yyvsp[0].string );
            if( !yyval.column_list )
                YYABORT;
        ;
    break;}
case 66:
#line 583 "./sql.y"
{
            yyval.string = yyvsp[0].string;
        ;
    break;}
case 67:
#line 590 "./sql.y"
{
            yyval.string = SQL_getstring( info, &yyvsp[0].str );
            if( !yyval.string )
                YYABORT;
        ;
    break;}
case 68:
#line 599 "./sql.y"
{
            yyval.integer = SQL_getint( info );
        ;
    break;}
}

#line 705 "/usr/share/bison/bison.simple"


  yyvsp -= yylen;
  yyssp -= yylen;
#if YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;
#if YYLSP_NEEDED
  *++yylsp = yyloc;
#endif

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("parse error, unexpected ") + 1;
	  yysize += yystrlen (yytname[YYTRANSLATE (yychar)]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "parse error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[YYTRANSLATE (yychar)]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exhausted");
	}
      else
#endif /* defined (YYERROR_VERBOSE) */
	yyerror ("parse error");
    }
  goto yyerrlab1;


/*--------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action |
`--------------------------------------------------*/
yyerrlab1:
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;
      YYDPRINTF ((stderr, "Discarding token %d (%s).\n",
		  yychar, yytname[yychar1]));
      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;


/*-------------------------------------------------------------------.
| yyerrdefault -- current state does not do anything special for the |
| error token.                                                       |
`-------------------------------------------------------------------*/
yyerrdefault:
#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */

  /* If its default is to accept any token, ok.  Otherwise pop it.  */
  yyn = yydefact[yystate];
  if (yyn)
    goto yydefault;
#endif


/*---------------------------------------------------------------.
| yyerrpop -- pop the current state because it cannot handle the |
| error token                                                    |
`---------------------------------------------------------------*/
yyerrpop:
  if (yyssp == yyss)
    YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#if YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "Error: state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

/*--------------.
| yyerrhandle.  |
`--------------*/
yyerrhandle:
  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;
#if YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

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

/*---------------------------------------------.
| yyoverflowab -- parser overflow comes here.  |
`---------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}
#line 604 "./sql.y"


static void *parser_alloc( void *info, unsigned int sz )
{
    SQL_input* sql = (SQL_input*) info;
    struct list *mem;

    mem = HeapAlloc( GetProcessHeap(), 0, sizeof (struct list) + sz );
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

int SQL_lex( void *SQL_lval, SQL_input *sql )
{
    int token;
    struct sql_str * str = SQL_lval;

    do
    {
        sql->n += sql->len;
        if( ! sql->command[sql->n] )
            return 0;  /* end of input */

        TRACE("string : %s\n", debugstr_w(&sql->command[sql->n]));
        sql->len = sqliteGetToken( &sql->command[sql->n], &token );
        if( sql->len==0 )
            break;
        str->data = &sql->command[sql->n];
        str->len = sql->len;
    }
    while( token == TK_SPACE );

    TRACE("token : %d (%s)\n", token, debugstr_wn(&sql->command[sql->n], sql->len));
    
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

int SQL_error( const char *str )
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
        if( *sql.view )
            (*sql.view)->ops->delete( *sql.view );
        *sql.view = NULL;
        return ERROR_BAD_QUERY_SYNTAX;
    }

    return ERROR_SUCCESS;
}
