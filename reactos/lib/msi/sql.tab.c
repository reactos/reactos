/* A Bison parser, made by GNU Bison 1.875b.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

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

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* If NAME_PREFIX is specified substitute the variables and functions
   names.  */
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
     TK_ABORT = 258,
     TK_AFTER = 259,
     TK_AGG_FUNCTION = 260,
     TK_ALL = 261,
     TK_AND = 262,
     TK_AS = 263,
     TK_ASC = 264,
     TK_BEFORE = 265,
     TK_BEGIN = 266,
     TK_BETWEEN = 267,
     TK_BITAND = 268,
     TK_BITNOT = 269,
     TK_BITOR = 270,
     TK_BY = 271,
     TK_CASCADE = 272,
     TK_CASE = 273,
     TK_CHAR = 274,
     TK_CHECK = 275,
     TK_CLUSTER = 276,
     TK_COLLATE = 277,
     TK_COLUMN = 278,
     TK_COMMA = 279,
     TK_COMMENT = 280,
     TK_COMMIT = 281,
     TK_CONCAT = 282,
     TK_CONFLICT = 283,
     TK_CONSTRAINT = 284,
     TK_COPY = 285,
     TK_CREATE = 286,
     TK_DEFAULT = 287,
     TK_DEFERRABLE = 288,
     TK_DEFERRED = 289,
     TK_DELETE = 290,
     TK_DELIMITERS = 291,
     TK_DESC = 292,
     TK_DISTINCT = 293,
     TK_DOT = 294,
     TK_DROP = 295,
     TK_EACH = 296,
     TK_ELSE = 297,
     TK_END = 298,
     TK_END_OF_FILE = 299,
     TK_EQ = 300,
     TK_EXCEPT = 301,
     TK_EXPLAIN = 302,
     TK_FAIL = 303,
     TK_FLOAT = 304,
     TK_FOR = 305,
     TK_FOREIGN = 306,
     TK_FROM = 307,
     TK_FUNCTION = 308,
     TK_GE = 309,
     TK_GLOB = 310,
     TK_GROUP = 311,
     TK_GT = 312,
     TK_HAVING = 313,
     TK_HOLD = 314,
     TK_IGNORE = 315,
     TK_ILLEGAL = 316,
     TK_IMMEDIATE = 317,
     TK_IN = 318,
     TK_INDEX = 319,
     TK_INITIALLY = 320,
     TK_ID = 321,
     TK_INSERT = 322,
     TK_INSTEAD = 323,
     TK_INT = 324,
     TK_INTEGER = 325,
     TK_INTERSECT = 326,
     TK_INTO = 327,
     TK_IS = 328,
     TK_ISNULL = 329,
     TK_JOIN = 330,
     TK_JOIN_KW = 331,
     TK_KEY = 332,
     TK_LE = 333,
     TK_LIKE = 334,
     TK_LIMIT = 335,
     TK_LONG = 336,
     TK_LONGCHAR = 337,
     TK_LP = 338,
     TK_LSHIFT = 339,
     TK_LT = 340,
     TK_LOCALIZABLE = 341,
     TK_MATCH = 342,
     TK_MINUS = 343,
     TK_NE = 344,
     TK_NOT = 345,
     TK_NOTNULL = 346,
     TK_NULL = 347,
     TK_OBJECT = 348,
     TK_OF = 349,
     TK_OFFSET = 350,
     TK_ON = 351,
     TK_OR = 352,
     TK_ORACLE_OUTER_JOIN = 353,
     TK_ORDER = 354,
     TK_PLUS = 355,
     TK_PRAGMA = 356,
     TK_PRIMARY = 357,
     TK_RAISE = 358,
     TK_REFERENCES = 359,
     TK_REM = 360,
     TK_REPLACE = 361,
     TK_RESTRICT = 362,
     TK_ROLLBACK = 363,
     TK_ROW = 364,
     TK_RP = 365,
     TK_RSHIFT = 366,
     TK_SELECT = 367,
     TK_SEMI = 368,
     TK_SET = 369,
     TK_SHORT = 370,
     TK_SLASH = 371,
     TK_SPACE = 372,
     TK_STAR = 373,
     TK_STATEMENT = 374,
     TK_STRING = 375,
     TK_TABLE = 376,
     TK_TEMP = 377,
     TK_THEN = 378,
     TK_TRANSACTION = 379,
     TK_TRIGGER = 380,
     TK_UMINUS = 381,
     TK_UNCLOSED_STRING = 382,
     TK_UNION = 383,
     TK_UNIQUE = 384,
     TK_UPDATE = 385,
     TK_UPLUS = 386,
     TK_USING = 387,
     TK_VACUUM = 388,
     TK_VALUES = 389,
     TK_VIEW = 390,
     TK_WHEN = 391,
     TK_WHERE = 392,
     TK_WILDCARD = 393,
     COLUMN = 395,
     FUNCTION = 396,
     COMMENT = 397,
     UNCLOSED_STRING = 398,
     SPACE = 399,
     ILLEGAL = 400,
     END_OF_FILE = 401
   };
#endif
#define TK_ABORT 258
#define TK_AFTER 259
#define TK_AGG_FUNCTION 260
#define TK_ALL 261
#define TK_AND 262
#define TK_AS 263
#define TK_ASC 264
#define TK_BEFORE 265
#define TK_BEGIN 266
#define TK_BETWEEN 267
#define TK_BITAND 268
#define TK_BITNOT 269
#define TK_BITOR 270
#define TK_BY 271
#define TK_CASCADE 272
#define TK_CASE 273
#define TK_CHAR 274
#define TK_CHECK 275
#define TK_CLUSTER 276
#define TK_COLLATE 277
#define TK_COLUMN 278
#define TK_COMMA 279
#define TK_COMMENT 280
#define TK_COMMIT 281
#define TK_CONCAT 282
#define TK_CONFLICT 283
#define TK_CONSTRAINT 284
#define TK_COPY 285
#define TK_CREATE 286
#define TK_DEFAULT 287
#define TK_DEFERRABLE 288
#define TK_DEFERRED 289
#define TK_DELETE 290
#define TK_DELIMITERS 291
#define TK_DESC 292
#define TK_DISTINCT 293
#define TK_DOT 294
#define TK_DROP 295
#define TK_EACH 296
#define TK_ELSE 297
#define TK_END 298
#define TK_END_OF_FILE 299
#define TK_EQ 300
#define TK_EXCEPT 301
#define TK_EXPLAIN 302
#define TK_FAIL 303
#define TK_FLOAT 304
#define TK_FOR 305
#define TK_FOREIGN 306
#define TK_FROM 307
#define TK_FUNCTION 308
#define TK_GE 309
#define TK_GLOB 310
#define TK_GROUP 311
#define TK_GT 312
#define TK_HAVING 313
#define TK_HOLD 314
#define TK_IGNORE 315
#define TK_ILLEGAL 316
#define TK_IMMEDIATE 317
#define TK_IN 318
#define TK_INDEX 319
#define TK_INITIALLY 320
#define TK_ID 321
#define TK_INSERT 322
#define TK_INSTEAD 323
#define TK_INT 324
#define TK_INTEGER 325
#define TK_INTERSECT 326
#define TK_INTO 327
#define TK_IS 328
#define TK_ISNULL 329
#define TK_JOIN 330
#define TK_JOIN_KW 331
#define TK_KEY 332
#define TK_LE 333
#define TK_LIKE 334
#define TK_LIMIT 335
#define TK_LONG 336
#define TK_LONGCHAR 337
#define TK_LP 338
#define TK_LSHIFT 339
#define TK_LT 340
#define TK_LOCALIZABLE 341
#define TK_MATCH 342
#define TK_MINUS 343
#define TK_NE 344
#define TK_NOT 345
#define TK_NOTNULL 346
#define TK_NULL 347
#define TK_OBJECT 348
#define TK_OF 349
#define TK_OFFSET 350
#define TK_ON 351
#define TK_OR 352
#define TK_ORACLE_OUTER_JOIN 353
#define TK_ORDER 354
#define TK_PLUS 355
#define TK_PRAGMA 356
#define TK_PRIMARY 357
#define TK_RAISE 358
#define TK_REFERENCES 359
#define TK_REM 360
#define TK_REPLACE 361
#define TK_RESTRICT 362
#define TK_ROLLBACK 363
#define TK_ROW 364
#define TK_RP 365
#define TK_RSHIFT 366
#define TK_SELECT 367
#define TK_SEMI 368
#define TK_SET 369
#define TK_SHORT 370
#define TK_SLASH 371
#define TK_SPACE 372
#define TK_STAR 373
#define TK_STATEMENT 374
#define TK_STRING 375
#define TK_TABLE 376
#define TK_TEMP 377
#define TK_THEN 378
#define TK_TRANSACTION 379
#define TK_TRIGGER 380
#define TK_UMINUS 381
#define TK_UNCLOSED_STRING 382
#define TK_UNION 383
#define TK_UNIQUE 384
#define TK_UPDATE 385
#define TK_UPLUS 386
#define TK_USING 387
#define TK_VACUUM 388
#define TK_VALUES 389
#define TK_VIEW 390
#define TK_WHEN 391
#define TK_WHERE 392
#define TK_WILDCARD 393
#define COLUMN 395
#define FUNCTION 396
#define COMMENT 397
#define UNCLOSED_STRING 398
#define SPACE 399
#define ILLEGAL 400
#define END_OF_FILE 401




/* Copy the first part of user declarations.  */
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
#include "wine/debug.h"
#include "wine/unicode.h"

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
} SQL_input;

static LPWSTR SQL_getstring( struct sql_str *str );
static INT SQL_getint( SQL_input *sql );
static int SQL_lex( void *SQL_lval, SQL_input *info);

static MSIVIEW *do_one_select( MSIDATABASE *db, MSIVIEW *in, 
                               string_list *columns );
static MSIVIEW *do_order_by( MSIDATABASE *db, MSIVIEW *in, 
                             string_list *columns );

static BOOL SQL_MarkPrimaryKeys( create_col_info *cols,
                                 string_list *keys);

static struct expr * EXPR_complex( struct expr *l, UINT op, struct expr *r );
static struct expr * EXPR_column( LPWSTR );
static struct expr * EXPR_ival( struct sql_str *, int sign);
static struct expr * EXPR_sval( struct sql_str *);
static struct expr * EXPR_wildcard();



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

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 74 "./sql.y"
typedef union YYSTYPE {
    struct sql_str str;
    LPWSTR string;
    string_list *column_list;
    value_list *val_list;
    MSIVIEW *query;
    struct expr *expr;
    USHORT column_type;
    create_col_info *column_info;
    column_assignment update_col_info;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 457 "sql.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 469 "sql.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

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
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

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
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  23
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   126

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  147
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  25
/* YYNRULES -- Number of rules. */
#define YYNRULES  63
/* YYNRULES -- Number of states. */
#define YYNSTATES  123

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   401

#define YYTRANSLATE(YYX) 						\
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
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    22,    34,    41,
      49,    56,    61,    66,    69,    71,    74,    76,    80,    82,
      87,    89,    91,    93,    95,    97,    99,   104,   106,   110,
     115,   117,   121,   123,   126,   131,   135,   139,   143,   147,
     151,   155,   159,   163,   167,   171,   175,   180,   182,   184,
     186,   190,   192,   196,   200,   202,   205,   207,   209,   211,
     215,   217,   219,   221
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short yyrhs[] =
{
     148,     0,    -1,   158,    -1,   150,    -1,   149,    -1,   151,
      -1,    67,    72,   170,    83,   160,   110,   134,    83,   164,
     110,    -1,    67,    72,   170,    83,   160,   110,   134,    83,
     164,   110,   122,    -1,    31,   121,   170,    83,   152,   110,
      -1,    31,   121,   170,    83,   152,   110,    59,    -1,   130,
     170,   114,   165,   137,   162,    -1,   153,   102,    77,   160,
      -1,   153,    24,   169,   154,    -1,   169,   154,    -1,   155,
      -1,   155,    86,    -1,   156,    -1,   156,    90,    92,    -1,
      19,    -1,    19,    83,   157,   110,    -1,    82,    -1,   115,
      -1,    69,    -1,    81,    -1,    93,    -1,    70,    -1,   159,
      99,    16,   160,    -1,   159,    -1,   112,   160,   161,    -1,
     112,    38,   160,   161,    -1,   169,    -1,   169,    24,   160,
      -1,   118,    -1,    52,   170,    -1,    52,   170,   137,   162,
      -1,    83,   162,   110,    -1,   168,    45,   168,    -1,   162,
       7,   162,    -1,   162,    97,   162,    -1,   168,    45,   163,
      -1,   168,    57,   163,    -1,   168,    85,   163,    -1,   168,
      78,   163,    -1,   168,    54,   163,    -1,   168,    89,   163,
      -1,   168,    73,    92,    -1,   168,    73,    90,    92,    -1,
     168,    -1,   167,    -1,   167,    -1,   167,    24,   164,    -1,
     166,    -1,   166,    24,   165,    -1,   169,    45,   167,    -1,
      70,    -1,    88,    70,    -1,   120,    -1,   138,    -1,   169,
      -1,   170,    39,   171,    -1,   171,    -1,   171,    -1,    66,
      -1,   120,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   142,   142,   147,   152,   157,   165,   173,   184,   194,
     207,   218,   228,   247,   259,   263,   271,   275,   282,   286,
     290,   294,   298,   302,   306,   313,   324,   335,   339,   353,
     371,   384,   397,   404,   415,   434,   438,   442,   446,   450,
     454,   458,   462,   466,   470,   474,   478,   485,   486,   490,
     502,   517,   518,   527,   543,   547,   551,   555,   562,   569,
     573,   580,   587,   591
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "TK_ABORT", "TK_AFTER", "TK_AGG_FUNCTION", 
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
  "TK_WILDCARD", "AGG_FUNCTION.", "COLUMN", "FUNCTION", "COMMENT", 
  "UNCLOSED_STRING", "SPACE", "ILLEGAL", "END_OF_FILE", "$accept", 
  "onequery", "oneinsert", "onecreate", "oneupdate", "table_def", 
  "column_def", "column_type", "data_type_l", "data_type", "data_count", 
  "oneselect", "unorderedsel", "selcollist", "from", "expr", "val", 
  "constlist", "update_assign_list", "column_assignment", "const_val", 
  "column_val", "column", "table", "string_or_id", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
     395,   396,   397,   398,   399,   400,   401
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,   147,   148,   148,   148,   148,   149,   149,   150,   150,
     151,   152,   153,   153,   154,   154,   155,   155,   156,   156,
     156,   156,   156,   156,   156,   157,   158,   158,   159,   159,
     160,   160,   160,   161,   161,   162,   162,   162,   162,   162,
     162,   162,   162,   162,   162,   162,   162,   163,   163,   164,
     164,   165,   165,   166,   167,   167,   167,   167,   168,   169,
     169,   170,   171,   171
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     1,     1,     1,    10,    11,     6,     7,
       6,     4,     4,     2,     1,     2,     1,     3,     1,     4,
       1,     1,     1,     1,     1,     1,     4,     1,     3,     4,
       1,     3,     1,     2,     4,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     4,     1,     1,     1,
       3,     1,     3,     3,     1,     2,     1,     1,     1,     3,
       1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,     0,     0,     0,     0,     0,     4,     3,     5,     2,
      27,     0,     0,     0,    62,    32,    63,     0,    30,     0,
      60,     0,    61,     1,     0,     0,     0,     0,     0,    28,
       0,     0,     0,     0,     0,     0,    29,    33,    31,    59,
       0,    51,     0,    26,     0,     0,     0,     0,     0,     0,
       0,     0,     8,     0,     0,    18,    22,    23,    20,    24,
      21,    13,    14,    16,     0,     0,    34,     0,    58,    10,
      52,    54,     0,    56,    57,    53,     9,     0,     0,     0,
      15,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    55,    12,    11,    25,     0,    17,     0,
      35,    37,    38,    56,    39,    48,    36,    43,    47,    40,
       0,    45,    42,    41,    44,    19,     0,    49,    46,     6,
       0,     7,    50
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,     5,     6,     7,     8,    44,    45,    61,    62,    63,
      97,     9,    10,    17,    29,    66,   104,   116,    40,    41,
     105,    67,    68,    19,    20
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -104
static const yysigned_char yypact[] =
{
     -30,  -103,   -52,   -34,   -51,    21,  -104,  -104,  -104,  -104,
     -70,   -51,   -51,   -57,  -104,  -104,  -104,   -26,    11,    -6,
      -1,   -69,  -104,  -104,    32,   -25,   -15,   -26,   -51,  -104,
     -57,   -51,   -51,   -57,   -51,   -57,  -104,   -71,  -104,  -104,
     -67,    47,    27,  -104,   -36,   -19,   -17,   -29,   -53,   -53,
     -51,   -63,    26,   -51,    17,   -10,  -104,  -104,  -104,  -104,
    -104,  -104,     5,     7,   -35,   -53,    -4,    35,  -104,    -4,
    -104,  -104,    37,  -104,  -104,  -104,  -104,   -17,   -57,    39,
    -104,    18,    28,    -7,   -53,   -53,   -60,   -60,   -60,   -73,
     -60,   -60,   -60,  -104,  -104,  -104,  -104,     4,  -104,   -63,
    -104,    -4,    -4,    77,  -104,  -104,  -104,  -104,  -104,  -104,
      25,  -104,  -104,  -104,  -104,  -104,     8,    95,  -104,     0,
     -63,  -104,  -104
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
    -104,  -104,  -104,  -104,  -104,  -104,  -104,    44,  -104,  -104,
    -104,  -104,  -104,     1,    96,   -38,    14,     6,    75,  -104,
     -43,   -37,     9,    12,    84
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -64
static const yysigned_char yytable[] =
{
      84,     1,    55,    84,    13,    53,    14,    71,    75,    14,
      71,    69,    18,    14,    27,    14,    21,   110,    11,   111,
      12,    23,    18,    25,    26,    72,    28,    83,    72,    24,
      65,    38,    14,    31,    43,    30,    47,     2,   -61,    18,
      37,    42,    18,    46,    18,    32,   101,   102,    33,   106,
     108,   108,    56,   108,   108,   108,   117,    73,    34,    42,
     103,    15,    77,    16,    57,    58,    48,    16,    35,    16,
      49,    50,    51,    79,    52,    74,    59,   117,    74,    95,
      86,    64,     3,    54,    15,    76,    16,    18,    22,    87,
      85,    80,    88,    85,    78,    22,    22,    81,    60,    82,
       4,   107,   109,   100,   112,   113,   114,    93,    89,    96,
      98,    99,    22,    90,   115,    39,   -63,   118,   119,   120,
      91,    94,   121,    36,    92,    70,   122
};

static const unsigned char yycheck[] =
{
       7,    31,    19,     7,    38,    24,    66,    70,    51,    66,
      70,    49,     3,    66,    13,    66,     4,    90,   121,    92,
      72,     0,    13,    11,    12,    88,    52,    65,    88,    99,
      83,    30,    66,    39,    33,    24,    35,    67,    39,    30,
      28,    32,    33,    34,    35,   114,    84,    85,    16,    86,
      87,    88,    69,    90,    91,    92,    99,   120,    83,    50,
     120,   118,    53,   120,    81,    82,   137,   120,    83,   120,
     137,    24,    45,    83,   110,   138,    93,   120,   138,    78,
      45,   110,   112,   102,   118,    59,   120,    78,     4,    54,
      97,    86,    57,    97,    77,    11,    12,    90,   115,   134,
     130,    87,    88,   110,    90,    91,    92,    70,    73,    70,
      92,    83,    28,    78,   110,    31,    39,    92,   110,    24,
      85,    77,   122,    27,    89,    50,   120
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    31,    67,   112,   130,   148,   149,   150,   151,   158,
     159,   121,    72,    38,    66,   118,   120,   160,   169,   170,
     171,   170,   171,     0,    99,   170,   170,   160,    52,   161,
      24,    39,   114,    16,    83,    83,   161,   170,   160,   171,
     165,   166,   169,   160,   152,   153,   169,   160,   137,   137,
      24,    45,   110,    24,   102,    19,    69,    81,    82,    93,
     115,   154,   155,   156,   110,    83,   162,   168,   169,   162,
     165,    70,    88,   120,   138,   167,    59,   169,    77,    83,
      86,    90,   134,   162,     7,    97,    45,    54,    57,    73,
      78,    85,    89,    70,   154,   160,    70,   157,    92,    83,
     110,   162,   162,   120,   163,   167,   168,   163,   168,   163,
      90,    92,   163,   163,   163,   110,   164,   167,    92,   110,
      24,   122,   164
};

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
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
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
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
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

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
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
  unsigned int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
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
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
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
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

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

#endif /* !YYERROR_VERBOSE */



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
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

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
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

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

#endif
#endif
{
  /* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;

  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



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
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
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
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

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
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
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

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

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
#line 143 "./sql.y"
    {
        SQL_input* sql = (SQL_input*) info;
        *sql->view = yyvsp[0].query;
    ;}
    break;

  case 3:
#line 148 "./sql.y"
    {
        SQL_input* sql = (SQL_input*) info;
        *sql->view = yyvsp[0].query;
    ;}
    break;

  case 4:
#line 153 "./sql.y"
    {
        SQL_input* sql = (SQL_input*) info;
        *sql->view = yyvsp[0].query;
    ;}
    break;

  case 5:
#line 158 "./sql.y"
    {
        SQL_input* sql = (SQL_input*) info;
        *sql->view = yyvsp[0].query;
    ;}
    break;

  case 6:
#line 166 "./sql.y"
    {
        SQL_input *sql = (SQL_input*) info;
        MSIVIEW *insert = NULL; 

        INSERT_CreateView( sql->db, &insert, yyvsp[-7].string, yyvsp[-5].column_list, yyvsp[-1].val_list, FALSE ); 
        yyval.query = insert;
    ;}
    break;

  case 7:
#line 174 "./sql.y"
    {
        SQL_input *sql = (SQL_input*) info;
        MSIVIEW *insert = NULL; 

        INSERT_CreateView( sql->db, &insert, yyvsp[-8].string, yyvsp[-6].column_list, yyvsp[-2].val_list, TRUE ); 
        yyval.query = insert;
    ;}
    break;

  case 8:
#line 185 "./sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *create = NULL; 

            if( !yyvsp[-1].column_info )
                YYABORT;
            CREATE_CreateView( sql->db, &create, yyvsp[-3].string, yyvsp[-1].column_info, FALSE );
            yyval.query = create;
        ;}
    break;

  case 9:
#line 195 "./sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *create = NULL; 

            if( !yyvsp[-2].column_info )
                YYABORT;
            CREATE_CreateView( sql->db, &create, yyvsp[-4].string, yyvsp[-2].column_info, TRUE );
            yyval.query = create;
        ;}
    break;

  case 10:
#line 208 "./sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *update = NULL; 

            UPDATE_CreateView( sql->db, &update, yyvsp[-4].string, &yyvsp[-2].update_col_info, yyvsp[0].expr );
            yyval.query = update;
        ;}
    break;

  case 11:
#line 219 "./sql.y"
    {
            if( SQL_MarkPrimaryKeys( yyvsp[-3].column_info, yyvsp[0].column_list ) )
                yyval.column_info = yyvsp[-3].column_info;
            else
                yyval.column_info = NULL;
        ;}
    break;

  case 12:
#line 229 "./sql.y"
    {
            create_col_info *ci;

            for( ci = yyvsp[-3].column_info; ci->next; ci = ci->next )
                ;

            ci->next = HeapAlloc( GetProcessHeap(), 0, sizeof *yyval.column_info );
            if( !ci->next )
            {
                /* FIXME: free $1 */
                YYABORT;
            }
            ci->next->colname = yyvsp[-1].string;
            ci->next->type = yyvsp[0].column_type;
            ci->next->next = NULL;

            yyval.column_info = yyvsp[-3].column_info;
        ;}
    break;

  case 13:
#line 248 "./sql.y"
    {
            yyval.column_info = HeapAlloc( GetProcessHeap(), 0, sizeof *yyval.column_info );
            if( ! yyval.column_info )
                YYABORT;
            yyval.column_info->colname = yyvsp[-1].string;
            yyval.column_info->type = yyvsp[0].column_type;
            yyval.column_info->next = NULL;
        ;}
    break;

  case 14:
#line 260 "./sql.y"
    {
            yyval.column_type = yyvsp[0].column_type | MSITYPE_VALID;
        ;}
    break;

  case 15:
#line 264 "./sql.y"
    {
            FIXME("LOCALIZABLE ignored\n");
            yyval.column_type = yyvsp[-1].column_type | MSITYPE_VALID;
        ;}
    break;

  case 16:
#line 272 "./sql.y"
    {
            yyval.column_type |= MSITYPE_NULLABLE;
        ;}
    break;

  case 17:
#line 276 "./sql.y"
    {
            yyval.column_type = yyvsp[-2].column_type;
        ;}
    break;

  case 18:
#line 283 "./sql.y"
    {
            yyval.column_type = MSITYPE_STRING | 1;
        ;}
    break;

  case 19:
#line 287 "./sql.y"
    {
            yyval.column_type = MSITYPE_STRING | 0x400 | yyvsp[-1].column_type;
        ;}
    break;

  case 20:
#line 291 "./sql.y"
    {
            yyval.column_type = 2;
        ;}
    break;

  case 21:
#line 295 "./sql.y"
    {
            yyval.column_type = 2;
        ;}
    break;

  case 22:
#line 299 "./sql.y"
    {
            yyval.column_type = 2;
        ;}
    break;

  case 23:
#line 303 "./sql.y"
    {
            yyval.column_type = 4;
        ;}
    break;

  case 24:
#line 307 "./sql.y"
    {
            yyval.column_type = 0;
        ;}
    break;

  case 25:
#line 314 "./sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            int val = SQL_getint(sql);
            if( ( val > 255 ) || ( val < 0 ) )
                YYABORT;
            yyval.column_type = val;
        ;}
    break;

  case 26:
#line 325 "./sql.y"
    {
            SQL_input* sql = (SQL_input*) info;

            if( !yyvsp[-3].query )
                YYABORT;
            if( yyvsp[0].column_list )
                yyval.query = do_order_by( sql->db, yyvsp[-3].query, yyvsp[0].column_list );
            else
                yyval.query = yyvsp[-3].query;
        ;}
    break;

  case 28:
#line 340 "./sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            if( !yyvsp[0].query )
                YYABORT;
            if( yyvsp[-1].column_list )
            {
                yyval.query = do_one_select( sql->db, yyvsp[0].query, yyvsp[-1].column_list );
                if( !yyval.query )
                    YYABORT;
            }
            else
                yyval.query = yyvsp[0].query;
        ;}
    break;

  case 29:
#line 354 "./sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *view = yyvsp[0].query;

            if( !view )
                YYABORT;
            if( yyvsp[-1].column_list )
            {
                view = do_one_select( sql->db, view, yyvsp[-1].column_list );
                if( !view )
                    YYABORT;
            }
            DISTINCT_CreateView( sql->db, & yyval.query, view );
        ;}
    break;

  case 30:
#line 372 "./sql.y"
    { 
            string_list *list;

            list = HeapAlloc( GetProcessHeap(), 0, sizeof *list );
            if( !list )
                YYABORT;
            list->string = yyvsp[0].string;
            list->next = NULL;

            yyval.column_list = list;
            TRACE("Collist %s\n",debugstr_w(yyval.column_list->string));
        ;}
    break;

  case 31:
#line 385 "./sql.y"
    { 
            string_list *list;

            list = HeapAlloc( GetProcessHeap(), 0, sizeof *list );
            if( !list )
                YYABORT;
            list->string = yyvsp[-2].string;
            list->next = yyvsp[0].column_list;

            yyval.column_list = list;
            TRACE("From table: %s\n",debugstr_w(yyval.column_list->string));
        ;}
    break;

  case 32:
#line 398 "./sql.y"
    {
            yyval.column_list = NULL;
        ;}
    break;

  case 33:
#line 405 "./sql.y"
    { 
            SQL_input* sql = (SQL_input*) info;
            UINT r;

            yyval.query = NULL;
            TRACE("From table: %s\n",debugstr_w(yyvsp[0].string));
            r = TABLE_CreateView( sql->db, yyvsp[0].string, & yyval.query );
            if( r != ERROR_SUCCESS )
                YYABORT;
        ;}
    break;

  case 34:
#line 416 "./sql.y"
    { 
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *view = NULL;
            UINT r;

            yyval.query = NULL;
            TRACE("From table: %s\n",debugstr_w(yyvsp[-2].string));
            r = TABLE_CreateView( sql->db, yyvsp[-2].string, &view );
            if( r != ERROR_SUCCESS )
                YYABORT;
            r = WHERE_CreateView( sql->db, &view, view, yyvsp[0].expr );
            if( r != ERROR_SUCCESS )
                YYABORT;
            yyval.query = view;
        ;}
    break;

  case 35:
#line 435 "./sql.y"
    {
            yyval.expr = yyvsp[-1].expr;
        ;}
    break;

  case 36:
#line 439 "./sql.y"
    {
            yyval.expr = EXPR_complex( yyvsp[-2].expr, OP_EQ, yyvsp[0].expr );
        ;}
    break;

  case 37:
#line 443 "./sql.y"
    {
            yyval.expr = EXPR_complex( yyvsp[-2].expr, OP_AND, yyvsp[0].expr );
        ;}
    break;

  case 38:
#line 447 "./sql.y"
    {
            yyval.expr = EXPR_complex( yyvsp[-2].expr, OP_OR, yyvsp[0].expr );
        ;}
    break;

  case 39:
#line 451 "./sql.y"
    {
            yyval.expr = EXPR_complex( yyvsp[-2].expr, OP_EQ, yyvsp[0].expr );
        ;}
    break;

  case 40:
#line 455 "./sql.y"
    {
            yyval.expr = EXPR_complex( yyvsp[-2].expr, OP_GT, yyvsp[0].expr );
        ;}
    break;

  case 41:
#line 459 "./sql.y"
    {
            yyval.expr = EXPR_complex( yyvsp[-2].expr, OP_LT, yyvsp[0].expr );
        ;}
    break;

  case 42:
#line 463 "./sql.y"
    {
            yyval.expr = EXPR_complex( yyvsp[-2].expr, OP_LE, yyvsp[0].expr );
        ;}
    break;

  case 43:
#line 467 "./sql.y"
    {
            yyval.expr = EXPR_complex( yyvsp[-2].expr, OP_GE, yyvsp[0].expr );
        ;}
    break;

  case 44:
#line 471 "./sql.y"
    {
            yyval.expr = EXPR_complex( yyvsp[-2].expr, OP_NE, yyvsp[0].expr );
        ;}
    break;

  case 45:
#line 475 "./sql.y"
    {
            yyval.expr = EXPR_complex( yyvsp[-2].expr, OP_ISNULL, NULL );
        ;}
    break;

  case 46:
#line 479 "./sql.y"
    {
            yyval.expr = EXPR_complex( yyvsp[-3].expr, OP_NOTNULL, NULL );
        ;}
    break;

  case 49:
#line 491 "./sql.y"
    {
            value_list *vals;

            vals = HeapAlloc( GetProcessHeap(), 0, sizeof *vals );
            if( vals )
            {
                vals->val = yyvsp[0].expr;
                vals->next = NULL;
            }
            yyval.val_list = vals;
        ;}
    break;

  case 50:
#line 503 "./sql.y"
    {
            value_list *vals;

            vals = HeapAlloc( GetProcessHeap(), 0, sizeof *vals );
            if( vals )
            {
                vals->val = yyvsp[-2].expr;
                vals->next = yyvsp[0].val_list;
            }
            yyval.val_list = vals;
        ;}
    break;

  case 52:
#line 519 "./sql.y"
    {
            yyvsp[-2].update_col_info.col_list->next = yyvsp[0].update_col_info.col_list;
            yyvsp[-2].update_col_info.val_list->next = yyvsp[0].update_col_info.val_list;
            yyval.update_col_info = yyvsp[-2].update_col_info;
        ;}
    break;

  case 53:
#line 528 "./sql.y"
    {
            yyval.update_col_info.col_list = HeapAlloc( GetProcessHeap(), 0, sizeof *yyval.update_col_info.col_list );
            if( !yyval.update_col_info.col_list )
                YYABORT;
            yyval.update_col_info.col_list->string = yyvsp[-2].string;
            yyval.update_col_info.col_list->next = NULL;
            yyval.update_col_info.val_list = HeapAlloc( GetProcessHeap(), 0, sizeof *yyval.update_col_info.val_list );
            if( !yyval.update_col_info.val_list )
                YYABORT;
            yyval.update_col_info.val_list->val = yyvsp[0].expr;
            yyval.update_col_info.val_list->next = 0;
        ;}
    break;

  case 54:
#line 544 "./sql.y"
    {
            yyval.expr = EXPR_ival( &yyvsp[0].str, 1 );
        ;}
    break;

  case 55:
#line 548 "./sql.y"
    {
            yyval.expr = EXPR_ival( &yyvsp[0].str, -1 );
        ;}
    break;

  case 56:
#line 552 "./sql.y"
    {
            yyval.expr = EXPR_sval( &yyvsp[0].str );
        ;}
    break;

  case 57:
#line 556 "./sql.y"
    {
            yyval.expr = EXPR_wildcard();
        ;}
    break;

  case 58:
#line 563 "./sql.y"
    {
            yyval.expr = EXPR_column( yyvsp[0].string );
        ;}
    break;

  case 59:
#line 570 "./sql.y"
    {
            yyval.string = yyvsp[0].string;  /* FIXME */
        ;}
    break;

  case 60:
#line 574 "./sql.y"
    {
            yyval.string = yyvsp[0].string;
        ;}
    break;

  case 61:
#line 581 "./sql.y"
    {
            yyval.string = yyvsp[0].string;
        ;}
    break;

  case 62:
#line 588 "./sql.y"
    {
            yyval.string = SQL_getstring( &yyvsp[0].str );
        ;}
    break;

  case 63:
#line 592 "./sql.y"
    {
            yyval.string = SQL_getstring( &yyvsp[0].str );
        ;}
    break;


    }

/* Line 999 of yacc.c.  */
#line 2054 "sql.tab.c"

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
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  const char* yyprefix;
	  char *yymsg;
	  int yyx;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 0;

	  yyprefix = ", expecting ";
	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		yysize += yystrlen (yyprefix) + yystrlen (yytname [yyx]);
		yycount += 1;
		if (yycount == 5)
		  {
		    yysize = 0;
		    break;
		  }
	      }
	  yysize += (sizeof ("syntax error, unexpected ")
		     + yystrlen (yytname[yytype]));
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yyprefix = ", expecting ";
		  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			yyp = yystpcpy (yyp, yyprefix);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yyprefix = " or ";
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
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

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      yyvsp--;
      yystate = *--yyssp;

      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


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
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 597 "./sql.y"


int SQL_lex( void *SQL_lval, SQL_input *sql)
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

LPWSTR SQL_getstring( struct sql_str *strdata)
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
    str = HeapAlloc( GetProcessHeap(), 0, (len + 1)*sizeof(WCHAR));
    if(!str )
        return str;
    memcpy(str, p, len*sizeof(WCHAR) );
    str[len]=0;

    return str;
}

INT SQL_getint( SQL_input *sql )
{
    LPCWSTR p = &sql->command[sql->n];

    return atoiW( p );
}

int SQL_error(const char *str)
{
    return 0;
}

static MSIVIEW *do_one_select( MSIDATABASE *db, MSIVIEW *in, 
                               string_list *columns )
{
    MSIVIEW *view = NULL;

    SELECT_CreateView( db, &view, in, columns );
    delete_string_list( columns );
    if( !view )
        ERR("Error creating select query\n");
    return view;
}

static MSIVIEW *do_order_by( MSIDATABASE *db, MSIVIEW *in, 
                             string_list *columns )
{
    MSIVIEW *view = NULL;

    ORDER_CreateView( db, &view, in );
    if( view )
    {
        string_list *x = columns;

        for( x = columns; x ; x = x->next )
            ORDER_AddColumn( view, x->string );
    }
    else
        ERR("Error creating select query\n");
    delete_string_list( columns );
    return view;
}

static struct expr * EXPR_wildcard()
{
    struct expr *e = HeapAlloc( GetProcessHeap(), 0, sizeof *e );
    if( e )
    {
        e->type = EXPR_WILDCARD;
    }
    return e;
}

static struct expr * EXPR_complex( struct expr *l, UINT op, struct expr *r )
{
    struct expr *e = HeapAlloc( GetProcessHeap(), 0, sizeof *e );
    if( e )
    {
        e->type = EXPR_COMPLEX;
        e->u.expr.left = l;
        e->u.expr.op = op;
        e->u.expr.right = r;
    }
    return e;
}

static struct expr * EXPR_column( LPWSTR str )
{
    struct expr *e = HeapAlloc( GetProcessHeap(), 0, sizeof *e );
    if( e )
    {
        e->type = EXPR_COLUMN;
        e->u.sval = str;
    }
    return e;
}

static struct expr * EXPR_ival( struct sql_str *str , int sign)
{
    struct expr *e = HeapAlloc( GetProcessHeap(), 0, sizeof *e );
    if( e )
    {
        e->type = EXPR_IVAL;
        e->u.ival = atoiW( str->data ) * sign;
    }
    return e;
}

static struct expr * EXPR_sval( struct sql_str *str )
{
    struct expr *e = HeapAlloc( GetProcessHeap(), 0, sizeof *e );
    if( e )
    {
        e->type = EXPR_SVAL;
        e->u.sval = SQL_getstring( str );
    }
    return e;
}

void delete_expr( struct expr *e )
{
    if( !e )
        return;
    if( e->type == EXPR_COMPLEX )
    {
        delete_expr( e->u.expr.left );
        delete_expr( e->u.expr.right );
    }
    else if( e->type == EXPR_UTF8 )
        HeapFree( GetProcessHeap(), 0, e->u.utf8 );
    else if( e->type == EXPR_SVAL )
        HeapFree( GetProcessHeap(), 0, e->u.sval );
    HeapFree( GetProcessHeap(), 0, e );
}

void delete_string_list( string_list *sl )
{
    while( sl )
    {
        string_list *t = sl->next;
        HeapFree( GetProcessHeap(), 0, sl->string );
        HeapFree( GetProcessHeap(), 0, sl );
        sl = t;
    }
}

void delete_value_list( value_list *vl )
{
    while( vl )
    {
        value_list *t = vl->next;
        delete_expr( vl->val );
        HeapFree( GetProcessHeap(), 0, vl );
        vl = t;
    }
}

static BOOL SQL_MarkPrimaryKeys( create_col_info *cols,
                                 string_list *keys )
{
    string_list *k;
    BOOL found = TRUE;

    for( k = keys; k && found; k = k->next )
    {
        create_col_info *c;

        found = FALSE;
        for( c = cols; c && !found; c = c->next )
        {
             if( lstrcmpW( k->string, c->colname ) )
                 continue;
             c->type |= MSITYPE_KEY;
             found = TRUE;
        }
    }

    return found;
}

UINT MSI_ParseSQL( MSIDATABASE *db, LPCWSTR command, MSIVIEW **phview )
{
    SQL_input sql;
    int r;

    *phview = NULL;

    sql.db = db;
    sql.command = command;
    sql.n = 0;
    sql.len = 0;
    sql.view = phview;

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

