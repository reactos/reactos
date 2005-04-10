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
#define YYFINAL  27
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   128

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  147
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  26
/* YYNRULES -- Number of rules. */
#define YYNRULES  65
/* YYNRULES -- Number of states. */
#define YYNSTATES  126

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
       0,     0,     3,     5,     7,     9,    11,    13,    24,    36,
      43,    51,    58,    61,    66,    71,    74,    76,    79,    81,
      85,    87,    92,    94,    96,    98,   100,   102,   104,   109,
     111,   115,   120,   122,   126,   128,   131,   136,   140,   144,
     148,   152,   156,   160,   164,   168,   172,   176,   180,   185,
     187,   189,   191,   195,   197,   201,   205,   207,   210,   212,
     214,   216,   220,   222,   224,   226
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short yyrhs[] =
{
     148,     0,    -1,   159,    -1,   150,    -1,   149,    -1,   151,
      -1,   152,    -1,    67,    72,   171,    83,   161,   110,   134,
      83,   165,   110,    -1,    67,    72,   171,    83,   161,   110,
     134,    83,   165,   110,   122,    -1,    31,   121,   171,    83,
     153,   110,    -1,    31,   121,   171,    83,   153,   110,    59,
      -1,   130,   171,   114,   166,   137,   163,    -1,    35,   162,
      -1,   154,   102,    77,   161,    -1,   154,    24,   170,   155,
      -1,   170,   155,    -1,   156,    -1,   156,    86,    -1,   157,
      -1,   157,    90,    92,    -1,    19,    -1,    19,    83,   158,
     110,    -1,    82,    -1,   115,    -1,    69,    -1,    81,    -1,
      93,    -1,    70,    -1,   160,    99,    16,   161,    -1,   160,
      -1,   112,   161,   162,    -1,   112,    38,   161,   162,    -1,
     170,    -1,   170,    24,   161,    -1,   118,    -1,    52,   171,
      -1,    52,   171,   137,   163,    -1,    83,   163,   110,    -1,
     169,    45,   169,    -1,   163,     7,   163,    -1,   163,    97,
     163,    -1,   169,    45,   164,    -1,   169,    57,   164,    -1,
     169,    85,   164,    -1,   169,    78,   164,    -1,   169,    54,
     164,    -1,   169,    89,   164,    -1,   169,    73,    92,    -1,
     169,    73,    90,    92,    -1,   169,    -1,   168,    -1,   168,
      -1,   168,    24,   165,    -1,   167,    -1,   167,    24,   166,
      -1,   170,    45,   168,    -1,    70,    -1,    88,    70,    -1,
     120,    -1,   138,    -1,   170,    -1,   171,    39,   172,    -1,
     172,    -1,   172,    -1,    66,    -1,   120,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   143,   143,   148,   153,   158,   163,   171,   179,   190,
     200,   213,   224,   235,   245,   264,   276,   280,   288,   292,
     299,   303,   307,   311,   315,   319,   323,   330,   341,   352,
     356,   370,   388,   401,   414,   421,   432,   451,   455,   459,
     463,   467,   471,   475,   479,   483,   487,   491,   495,   502,
     503,   507,   519,   534,   535,   544,   560,   564,   568,   572,
     579,   586,   590,   597,   604,   608
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
  "onequery", "oneinsert", "onecreate", "oneupdate", "onedelete", 
  "table_def", "column_def", "column_type", "data_type_l", "data_type", 
  "data_count", "oneselect", "unorderedsel", "selcollist", "from", "expr", 
  "val", "constlist", "update_assign_list", "column_assignment", 
  "const_val", "column_val", "column", "table", "string_or_id", 0
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
       0,   147,   148,   148,   148,   148,   148,   149,   149,   150,
     150,   151,   152,   153,   154,   154,   155,   155,   156,   156,
     157,   157,   157,   157,   157,   157,   157,   158,   159,   159,
     160,   160,   161,   161,   161,   162,   162,   163,   163,   163,
     163,   163,   163,   163,   163,   163,   163,   163,   163,   164,
     164,   165,   165,   166,   166,   167,   168,   168,   168,   168,
     169,   170,   170,   171,   172,   172
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,    10,    11,     6,
       7,     6,     2,     4,     4,     2,     1,     2,     1,     3,
       1,     4,     1,     1,     1,     1,     1,     1,     4,     1,
       3,     4,     1,     3,     1,     2,     4,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     4,     1,
       1,     1,     3,     1,     3,     3,     1,     2,     1,     1,
       1,     3,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,     0,     0,     0,     0,     0,     0,     4,     3,     5,
       6,     2,    29,     0,     0,    12,     0,     0,    64,    34,
      65,     0,    32,     0,    62,     0,    63,     1,     0,     0,
      35,     0,     0,    30,     0,     0,     0,     0,     0,     0,
       0,    31,    33,    61,     0,    53,     0,    28,     0,     0,
       0,     0,    36,     0,    60,     0,     0,     0,     0,     9,
       0,     0,    20,    24,    25,    22,    26,    23,    15,    16,
      18,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    11,    54,    56,     0,    58,    59,    55,    10,
       0,     0,     0,    17,     0,    37,    39,    40,    58,    41,
      50,    38,    45,    49,    42,     0,    47,    44,    43,    46,
       0,    57,    14,    13,    27,     0,    19,    48,     0,    21,
       0,    51,     7,     0,     8,    52
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,     6,     7,     8,     9,    10,    48,    49,    68,    69,
      70,   115,    11,    12,    21,    15,    52,    99,   120,    44,
      45,   100,    53,    54,    23,    24
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -106
static const yysigned_char yypact[] =
{
     -30,  -105,   -33,   -54,   -34,   -51,    22,  -106,  -106,  -106,
    -106,  -106,   -79,   -51,   -51,  -106,   -51,   -60,  -106,  -106,
    -106,   -33,    17,    10,    11,   -63,  -106,  -106,    41,   -24,
     -75,   -13,   -33,  -106,   -60,   -51,   -51,   -60,   -51,   -45,
     -60,  -106,  -106,  -106,   -65,    50,    32,  -106,   -32,   -15,
     -17,   -45,    -4,    26,  -106,   -22,   -45,   -51,   -59,    30,
     -51,    24,    19,  -106,  -106,  -106,  -106,  -106,  -106,    21,
      15,    -7,   -45,   -45,   -53,   -53,   -53,   -78,   -53,   -53,
     -53,   -28,    -4,  -106,  -106,    46,  -106,  -106,  -106,  -106,
     -17,   -60,    47,  -106,    27,  -106,    -4,    -4,    79,  -106,
    -106,  -106,  -106,  -106,  -106,    28,  -106,  -106,  -106,  -106,
      38,  -106,  -106,  -106,  -106,    12,  -106,  -106,   -59,  -106,
      13,   100,     3,   -59,  -106,  -106
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
    -106,  -106,  -106,  -106,  -106,  -106,  -106,  -106,    36,  -106,
    -106,  -106,  -106,  -106,   -10,     7,   -25,    16,     4,    71,
    -106,   -50,    34,     6,    40,    20
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -66
static const yysigned_char yytable[] =
{
      72,     1,    62,    72,    17,     2,    18,    32,    88,    60,
      22,    84,   105,    18,   106,    18,    13,    84,    16,    14,
      28,    18,    27,    22,    42,    26,    71,    47,    33,    85,
      55,    82,    18,    26,    26,    85,    26,     3,    51,    41,
      22,    34,    46,    22,    50,    25,    22,    96,    97,    35,
     -63,    36,    63,    29,    30,    43,    31,    37,    19,    38,
      20,    86,    39,    46,    64,    65,    90,    98,   121,    20,
      40,    74,    56,   121,    57,    20,    66,    58,    59,    87,
      75,   113,     4,    76,    19,    87,    20,    61,    81,    89,
      73,   102,   104,    73,   107,   108,   109,    22,    67,    77,
       5,    91,    92,    95,    78,    94,   110,    93,   101,   103,
     103,    79,   103,   103,   103,    80,   111,   114,   -65,   116,
     117,   118,   119,   122,   123,   124,   112,   125,    83
};

static const unsigned char yycheck[] =
{
       7,    31,    19,     7,    38,    35,    66,    17,    58,    24,
       4,    70,    90,    66,    92,    66,   121,    70,    72,    52,
      99,    66,     0,    17,    34,     5,    51,    37,    21,    88,
      40,    56,    66,    13,    14,    88,    16,    67,    83,    32,
      34,    24,    36,    37,    38,     5,    40,    72,    73,    39,
      39,   114,    69,    13,    14,    35,    16,    16,   118,    83,
     120,   120,   137,    57,    81,    82,    60,   120,   118,   120,
      83,    45,   137,   123,    24,   120,    93,    45,   110,   138,
      54,    91,   112,    57,   118,   138,   120,   102,   110,    59,
      97,    75,    76,    97,    78,    79,    80,    91,   115,    73,
     130,    77,    83,   110,    78,    90,   134,    86,    74,    75,
      76,    85,    78,    79,    80,    89,    70,    70,    39,    92,
      92,    83,   110,   110,    24,   122,    90,   123,    57
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    31,    35,    67,   112,   130,   148,   149,   150,   151,
     152,   159,   160,   121,    52,   162,    72,    38,    66,   118,
     120,   161,   170,   171,   172,   171,   172,     0,    99,   171,
     171,   171,   161,   162,    24,    39,   114,    16,    83,   137,
      83,   162,   161,   172,   166,   167,   170,   161,   153,   154,
     170,    83,   163,   169,   170,   161,   137,    24,    45,   110,
      24,   102,    19,    69,    81,    82,    93,   115,   155,   156,
     157,   163,     7,    97,    45,    54,    57,    73,    78,    85,
      89,   110,   163,   166,    70,    88,   120,   138,   168,    59,
     170,    77,    83,    86,    90,   110,   163,   163,   120,   164,
     168,   169,   164,   169,   164,    90,    92,   164,   164,   164,
     134,    70,   155,   161,    70,   158,    92,    92,    83,   110,
     165,   168,   110,    24,   122,   165
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
#line 144 "./sql.y"
    {
        SQL_input* sql = (SQL_input*) info;
        *sql->view = yyvsp[0].query;
    ;}
    break;

  case 3:
#line 149 "./sql.y"
    {
        SQL_input* sql = (SQL_input*) info;
        *sql->view = yyvsp[0].query;
    ;}
    break;

  case 4:
#line 154 "./sql.y"
    {
        SQL_input* sql = (SQL_input*) info;
        *sql->view = yyvsp[0].query;
    ;}
    break;

  case 5:
#line 159 "./sql.y"
    {
        SQL_input* sql = (SQL_input*) info;
        *sql->view = yyvsp[0].query;
    ;}
    break;

  case 6:
#line 164 "./sql.y"
    {
        SQL_input* sql = (SQL_input*) info;
        *sql->view = yyvsp[0].query;
    ;}
    break;

  case 7:
#line 172 "./sql.y"
    {
        SQL_input *sql = (SQL_input*) info;
        MSIVIEW *insert = NULL; 

        INSERT_CreateView( sql->db, &insert, yyvsp[-7].string, yyvsp[-5].column_list, yyvsp[-1].val_list, FALSE ); 
        yyval.query = insert;
    ;}
    break;

  case 8:
#line 180 "./sql.y"
    {
        SQL_input *sql = (SQL_input*) info;
        MSIVIEW *insert = NULL; 

        INSERT_CreateView( sql->db, &insert, yyvsp[-8].string, yyvsp[-6].column_list, yyvsp[-2].val_list, TRUE ); 
        yyval.query = insert;
    ;}
    break;

  case 9:
#line 191 "./sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *create = NULL; 

            if( !yyvsp[-1].column_info )
                YYABORT;
            CREATE_CreateView( sql->db, &create, yyvsp[-3].string, yyvsp[-1].column_info, FALSE );
            yyval.query = create;
        ;}
    break;

  case 10:
#line 201 "./sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *create = NULL; 

            if( !yyvsp[-2].column_info )
                YYABORT;
            CREATE_CreateView( sql->db, &create, yyvsp[-4].string, yyvsp[-2].column_info, TRUE );
            yyval.query = create;
        ;}
    break;

  case 11:
#line 214 "./sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *update = NULL; 

            UPDATE_CreateView( sql->db, &update, yyvsp[-4].string, &yyvsp[-2].update_col_info, yyvsp[0].expr );
            yyval.query = update;
        ;}
    break;

  case 12:
#line 225 "./sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *delete = NULL; 

            DELETE_CreateView( sql->db, &delete, yyvsp[0].query );
            yyval.query = delete;
        ;}
    break;

  case 13:
#line 236 "./sql.y"
    {
            if( SQL_MarkPrimaryKeys( yyvsp[-3].column_info, yyvsp[0].column_list ) )
                yyval.column_info = yyvsp[-3].column_info;
            else
                yyval.column_info = NULL;
        ;}
    break;

  case 14:
#line 246 "./sql.y"
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

  case 15:
#line 265 "./sql.y"
    {
            yyval.column_info = HeapAlloc( GetProcessHeap(), 0, sizeof *yyval.column_info );
            if( ! yyval.column_info )
                YYABORT;
            yyval.column_info->colname = yyvsp[-1].string;
            yyval.column_info->type = yyvsp[0].column_type;
            yyval.column_info->next = NULL;
        ;}
    break;

  case 16:
#line 277 "./sql.y"
    {
            yyval.column_type = yyvsp[0].column_type | MSITYPE_VALID;
        ;}
    break;

  case 17:
#line 281 "./sql.y"
    {
            FIXME("LOCALIZABLE ignored\n");
            yyval.column_type = yyvsp[-1].column_type | MSITYPE_VALID;
        ;}
    break;

  case 18:
#line 289 "./sql.y"
    {
            yyval.column_type |= MSITYPE_NULLABLE;
        ;}
    break;

  case 19:
#line 293 "./sql.y"
    {
            yyval.column_type = yyvsp[-2].column_type;
        ;}
    break;

  case 20:
#line 300 "./sql.y"
    {
            yyval.column_type = MSITYPE_STRING | 1;
        ;}
    break;

  case 21:
#line 304 "./sql.y"
    {
            yyval.column_type = MSITYPE_STRING | 0x400 | yyvsp[-1].column_type;
        ;}
    break;

  case 22:
#line 308 "./sql.y"
    {
            yyval.column_type = 2;
        ;}
    break;

  case 23:
#line 312 "./sql.y"
    {
            yyval.column_type = 2;
        ;}
    break;

  case 24:
#line 316 "./sql.y"
    {
            yyval.column_type = 2;
        ;}
    break;

  case 25:
#line 320 "./sql.y"
    {
            yyval.column_type = 4;
        ;}
    break;

  case 26:
#line 324 "./sql.y"
    {
            yyval.column_type = 0;
        ;}
    break;

  case 27:
#line 331 "./sql.y"
    {
            SQL_input* sql = (SQL_input*) info;
            int val = SQL_getint(sql);
            if( ( val > 255 ) || ( val < 0 ) )
                YYABORT;
            yyval.column_type = val;
        ;}
    break;

  case 28:
#line 342 "./sql.y"
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

  case 30:
#line 357 "./sql.y"
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

  case 31:
#line 371 "./sql.y"
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

  case 32:
#line 389 "./sql.y"
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

  case 33:
#line 402 "./sql.y"
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

  case 34:
#line 415 "./sql.y"
    {
            yyval.column_list = NULL;
        ;}
    break;

  case 35:
#line 422 "./sql.y"
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

  case 36:
#line 433 "./sql.y"
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

  case 37:
#line 452 "./sql.y"
    {
            yyval.expr = yyvsp[-1].expr;
        ;}
    break;

  case 38:
#line 456 "./sql.y"
    {
            yyval.expr = EXPR_complex( yyvsp[-2].expr, OP_EQ, yyvsp[0].expr );
        ;}
    break;

  case 39:
#line 460 "./sql.y"
    {
            yyval.expr = EXPR_complex( yyvsp[-2].expr, OP_AND, yyvsp[0].expr );
        ;}
    break;

  case 40:
#line 464 "./sql.y"
    {
            yyval.expr = EXPR_complex( yyvsp[-2].expr, OP_OR, yyvsp[0].expr );
        ;}
    break;

  case 41:
#line 468 "./sql.y"
    {
            yyval.expr = EXPR_complex( yyvsp[-2].expr, OP_EQ, yyvsp[0].expr );
        ;}
    break;

  case 42:
#line 472 "./sql.y"
    {
            yyval.expr = EXPR_complex( yyvsp[-2].expr, OP_GT, yyvsp[0].expr );
        ;}
    break;

  case 43:
#line 476 "./sql.y"
    {
            yyval.expr = EXPR_complex( yyvsp[-2].expr, OP_LT, yyvsp[0].expr );
        ;}
    break;

  case 44:
#line 480 "./sql.y"
    {
            yyval.expr = EXPR_complex( yyvsp[-2].expr, OP_LE, yyvsp[0].expr );
        ;}
    break;

  case 45:
#line 484 "./sql.y"
    {
            yyval.expr = EXPR_complex( yyvsp[-2].expr, OP_GE, yyvsp[0].expr );
        ;}
    break;

  case 46:
#line 488 "./sql.y"
    {
            yyval.expr = EXPR_complex( yyvsp[-2].expr, OP_NE, yyvsp[0].expr );
        ;}
    break;

  case 47:
#line 492 "./sql.y"
    {
            yyval.expr = EXPR_complex( yyvsp[-2].expr, OP_ISNULL, NULL );
        ;}
    break;

  case 48:
#line 496 "./sql.y"
    {
            yyval.expr = EXPR_complex( yyvsp[-3].expr, OP_NOTNULL, NULL );
        ;}
    break;

  case 51:
#line 508 "./sql.y"
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

  case 52:
#line 520 "./sql.y"
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

  case 54:
#line 536 "./sql.y"
    {
            yyvsp[-2].update_col_info.col_list->next = yyvsp[0].update_col_info.col_list;
            yyvsp[-2].update_col_info.val_list->next = yyvsp[0].update_col_info.val_list;
            yyval.update_col_info = yyvsp[-2].update_col_info;
        ;}
    break;

  case 55:
#line 545 "./sql.y"
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

  case 56:
#line 561 "./sql.y"
    {
            yyval.expr = EXPR_ival( &yyvsp[0].str, 1 );
        ;}
    break;

  case 57:
#line 565 "./sql.y"
    {
            yyval.expr = EXPR_ival( &yyvsp[0].str, -1 );
        ;}
    break;

  case 58:
#line 569 "./sql.y"
    {
            yyval.expr = EXPR_sval( &yyvsp[0].str );
        ;}
    break;

  case 59:
#line 573 "./sql.y"
    {
            yyval.expr = EXPR_wildcard();
        ;}
    break;

  case 60:
#line 580 "./sql.y"
    {
            yyval.expr = EXPR_column( yyvsp[0].string );
        ;}
    break;

  case 61:
#line 587 "./sql.y"
    {
            yyval.string = yyvsp[0].string;  /* FIXME */
        ;}
    break;

  case 62:
#line 591 "./sql.y"
    {
            yyval.string = yyvsp[0].string;
        ;}
    break;

  case 63:
#line 598 "./sql.y"
    {
            yyval.string = yyvsp[0].string;
        ;}
    break;

  case 64:
#line 605 "./sql.y"
    {
            yyval.string = SQL_getstring( &yyvsp[0].str );
        ;}
    break;

  case 65:
#line 609 "./sql.y"
    {
            yyval.string = SQL_getstring( &yyvsp[0].str );
        ;}
    break;


    }

/* Line 999 of yacc.c.  */
#line 2073 "sql.tab.c"

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


#line 614 "./sql.y"


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

