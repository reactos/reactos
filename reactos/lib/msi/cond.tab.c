/* A Bison parser, made from ./cond.y
   by GNU bison 1.35.  */

#define YYBISON 1  /* Identify Bison output.  */

#define yyparse COND_parse
#define yylex COND_lex
#define yyerror COND_error
#define yylval COND_lval
#define yychar COND_char
#define yydebug COND_debug
#define yynerrs COND_nerrs
# define	COND_SPACE	257
# define	COND_EOF	258
# define	COND_OR	259
# define	COND_AND	260
# define	COND_NOT	261
# define	COND_LT	262
# define	COND_GT	263
# define	COND_EQ	264
# define	COND_LPAR	265
# define	COND_RPAR	266
# define	COND_TILDA	267
# define	COND_PERCENT	268
# define	COND_DOLLARS	269
# define	COND_QUESTION	270
# define	COND_AMPER	271
# define	COND_EXCLAM	272
# define	COND_IDENT	273
# define	COND_NUMBER	274
# define	COND_LITER	275
# define	COND_ERROR	276

#line 1 "./cond.y"


/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2003 Mike McCormack for CodeWeavers
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
#include "wine/debug.h"
#include "wine/unicode.h"

#include "msi.h"
#include "msiquery.h"
#include "msipriv.h"

#define YYLEX_PARAM info
#define YYPARSE_PARAM info

static int COND_error(char *str);

WINE_DEFAULT_DEBUG_CHANNEL(msi);

typedef struct tag_yyinput
{
    MSIPACKAGE *package;
    LPCWSTR str;
    INT    n;
    MSICONDITION result;
} COND_input;

struct cond_str {
    LPCWSTR data;
    INT len;
};

static LPWSTR COND_GetString( struct cond_str *str );
static LPWSTR COND_GetLiteral( struct cond_str *str );
static int COND_lex( void *COND_lval, COND_input *info);

typedef INT (*comp_int)(INT a, INT b);
typedef INT (*comp_str)(LPWSTR a, LPWSTR b, BOOL caseless);
typedef INT (*comp_m1)(LPWSTR a,int b);
typedef INT (*comp_m2)(int a,LPWSTR b);

static INT comp_lt_i(INT a, INT b);
static INT comp_gt_i(INT a, INT b);
static INT comp_le_i(INT a, INT b);
static INT comp_ge_i(INT a, INT b);
static INT comp_eq_i(INT a, INT b);
static INT comp_ne_i(INT a, INT b);
static INT comp_bitand(INT a, INT b);
static INT comp_highcomp(INT a, INT b);
static INT comp_lowcomp(INT a, INT b);

static INT comp_eq_s(LPWSTR a, LPWSTR b, BOOL casless);
static INT comp_ne_s(LPWSTR a, LPWSTR b, BOOL casless);
static INT comp_lt_s(LPWSTR a, LPWSTR b, BOOL casless);
static INT comp_gt_s(LPWSTR a, LPWSTR b, BOOL casless);
static INT comp_le_s(LPWSTR a, LPWSTR b, BOOL casless);
static INT comp_ge_s(LPWSTR a, LPWSTR b, BOOL casless);
static INT comp_substring(LPWSTR a, LPWSTR b, BOOL casless);
static INT comp_start(LPWSTR a, LPWSTR b, BOOL casless);
static INT comp_end(LPWSTR a, LPWSTR b, BOOL casless);

static INT comp_eq_m1(LPWSTR a, INT b);
static INT comp_ne_m1(LPWSTR a, INT b);
static INT comp_lt_m1(LPWSTR a, INT b);
static INT comp_gt_m1(LPWSTR a, INT b);
static INT comp_le_m1(LPWSTR a, INT b);
static INT comp_ge_m1(LPWSTR a, INT b);

static INT comp_eq_m2(INT a, LPWSTR b);
static INT comp_ne_m2(INT a, LPWSTR b);
static INT comp_lt_m2(INT a, LPWSTR b);
static INT comp_gt_m2(INT a, LPWSTR b);
static INT comp_le_m2(INT a, LPWSTR b);
static INT comp_ge_m2(INT a, LPWSTR b);


#line 105 "./cond.y"
#ifndef YYSTYPE
typedef union
{
    struct cond_str str;
    LPWSTR    string;
    INT       value;
    comp_int  fn_comp_int;
    comp_str  fn_comp_str;
    comp_m1   fn_comp_m1;
    comp_m2   fn_comp_m2;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
#ifndef YYDEBUG
# define YYDEBUG 0
#endif



#define	YYFINAL		74
#define	YYFLAG		-32768
#define	YYNTBASE	23

/* YYTRANSLATE(YYLEX) -- Bison token number corresponding to YYLEX. */
#define YYTRANSLATE(x) ((unsigned)(x) <= 276 ? yytranslate[x] : 39)

/* YYTRANSLATE[YYLEX] -- Bison token number corresponding to YYLEX. */
static const char yytranslate[] =
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
      16,    17,    18,    19,    20,    21,    22
};

#if YYDEBUG
static const short yyprhs[] =
{
       0,     0,     2,     4,     8,    10,    14,    16,    19,    21,
      23,    27,    31,    36,    40,    44,    48,    50,    53,    55,
      57,    60,    63,    66,    69,    72,    74,    77,    79,    81,
      84,    87,    90,    93,    96,    98,   101,   103,   105,   108,
     111,   114,   117,   120,   122,   125,   127,   129,   132,   135,
     138,   141,   144,   146,   148,   150,   152,   154,   157,   160,
     163,   166,   168,   171,   173
};
static const short yyrhs[] =
{
      24,     0,    25,     0,    25,     5,    24,     0,    26,     0,
      25,     6,    26,     0,    27,     0,     7,    27,     0,    32,
       0,    33,     0,    32,    28,    32,     0,    33,    29,    33,
       0,    33,    13,    29,    33,     0,    33,    30,    32,     0,
      32,    31,    33,     0,    11,    24,    12,     0,    10,     0,
       8,     9,     0,     8,     0,     9,     0,     8,    10,     0,
       9,    10,     0,     9,     8,     0,     8,     8,     0,     9,
       9,     0,    10,     0,     8,     9,     0,     8,     0,     9,
       0,     8,    10,     0,     9,    10,     0,     9,     8,     0,
       8,     8,     0,     9,     9,     0,    10,     0,     8,     9,
       0,     8,     0,     9,     0,     8,    10,     0,     9,    10,
       0,     9,     8,     0,     8,     8,     0,     9,     9,     0,
      10,     0,     8,     9,     0,     8,     0,     9,     0,     8,
      10,     0,     9,    10,     0,     9,     8,     0,     8,     8,
       0,     9,     9,     0,    35,     0,    38,     0,    36,     0,
      34,     0,    21,     0,    15,    37,     0,    16,    37,     0,
      17,    37,     0,    18,    37,     0,    37,     0,    14,    37,
       0,    19,     0,    20,     0
};

#endif

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined. */
static const short yyrline[] =
{
       0,   135,   143,   148,   154,   159,   165,   170,   177,   182,
     186,   190,   194,   198,   202,   206,   212,   218,   222,   226,
     230,   234,   239,   243,   247,   253,   259,   263,   267,   271,
     275,   280,   284,   288,   294,   300,   304,   308,   312,   316,
     321,   325,   329,   335,   341,   345,   349,   353,   357,   362,
     366,   370,   376,   381,   387,   392,   398,   407,   417,   426,
     435,   446,   462,   475,   484
};
#endif


#if (YYDEBUG) || defined YYERROR_VERBOSE

/* YYTNAME[TOKEN_NUM] -- String name of the token TOKEN_NUM. */
static const char *const yytname[] =
{
  "$", "error", "$undefined.", "COND_SPACE", "COND_EOF", "COND_OR", 
  "COND_AND", "COND_NOT", "COND_LT", "COND_GT", "COND_EQ", "COND_LPAR", 
  "COND_RPAR", "COND_TILDA", "COND_PERCENT", "COND_DOLLARS", 
  "COND_QUESTION", "COND_AMPER", "COND_EXCLAM", "COND_IDENT", 
  "COND_NUMBER", "COND_LITER", "COND_ERROR", "condition", "expression", 
  "boolean_term", "boolean_factor", "term", "comp_op_i", "comp_op_s", 
  "comp_op_m1", "comp_op_m2", "value_i", "value_s", "literal", "symbol_i", 
  "symbol_s", "identifier", "integer", 0
};
#endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives. */
static const short yyr1[] =
{
       0,    23,    24,    24,    25,    25,    26,    26,    27,    27,
      27,    27,    27,    27,    27,    27,    28,    28,    28,    28,
      28,    28,    28,    28,    28,    29,    29,    29,    29,    29,
      29,    29,    29,    29,    30,    30,    30,    30,    30,    30,
      30,    30,    30,    31,    31,    31,    31,    31,    31,    31,
      31,    31,    32,    32,    33,    33,    34,    35,    35,    35,
      35,    36,    36,    37,    38
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN. */
static const short yyr2[] =
{
       0,     1,     1,     3,     1,     3,     1,     2,     1,     1,
       3,     3,     4,     3,     3,     3,     1,     2,     1,     1,
       2,     2,     2,     2,     2,     1,     2,     1,     1,     2,
       2,     2,     2,     2,     1,     2,     1,     1,     2,     2,
       2,     2,     2,     1,     2,     1,     1,     2,     2,     2,
       2,     2,     1,     1,     1,     1,     1,     2,     2,     2,
       2,     1,     2,     1,     1
};

/* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
   doesn't specify something else to do.  Zero means the default is an
   error. */
static const short yydefact[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,    63,    64,
      56,     1,     2,     4,     6,     8,     9,    55,    52,    54,
      61,    53,     7,     0,    62,    57,    58,    59,    60,     0,
       0,    18,    19,    16,     0,     0,    36,    37,    34,     0,
       0,     0,    15,     3,     5,    23,    17,    20,    22,    24,
      21,    10,    14,    41,    35,    38,    40,    42,    39,    27,
      28,    25,     0,    11,    13,    32,    26,    29,    31,    33,
      30,    12,     0,     0,     0
};

static const short yydefgoto[] =
{
      72,    11,    12,    13,    14,    34,    40,    41,    35,    15,
      16,    17,    18,    19,    20,    21
};

static const short yypact[] =
{
      -5,    50,    -5,   -12,   -12,   -12,   -12,   -12,-32768,-32768,
  -32768,-32768,    -2,-32768,-32768,    48,   123,-32768,-32768,-32768,
  -32768,-32768,-32768,    -4,-32768,-32768,-32768,-32768,-32768,    -5,
      -5,    10,    24,    33,   110,    58,    27,    41,    59,   135,
      58,   110,-32768,-32768,-32768,    62,    68,    71,    72,    80,
      81,-32768,-32768,    84,    90,    93,    94,   102,   103,   138,
     141,-32768,    58,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,    17,    21,-32768
};

static const short yypgoto[] =
{
  -32768,    -1,-32768,    -8,    25,-32768,   -14,-32768,-32768,   -11,
     -35,-32768,-32768,-32768,   134,-32768
};


#define	YYLAST		151


static const short yytable[] =
{
      52,    23,     1,    29,    30,    63,     2,     8,    42,     3,
       4,     5,     6,     7,     8,     9,    10,    73,    45,    46,
      47,    74,    44,    51,   -45,    62,    22,    71,    43,   -45,
      64,   -45,    48,    49,    50,    53,    54,    55,   -46,     0,
       0,   -27,     0,   -46,     0,   -46,   -27,   -43,   -27,    56,
      57,    58,   -43,     0,   -43,   -28,    31,    32,    33,     0,
     -28,     2,   -28,     0,     3,     4,     5,     6,     7,     8,
       9,    10,     3,   -25,     0,     0,   -50,     8,   -25,    10,
     -25,   -50,   -44,   -50,     0,   -47,   -49,   -44,     0,   -44,
     -47,   -49,   -47,   -49,   -51,   -48,     0,     0,   -32,   -51,
     -48,   -51,   -48,   -32,   -26,   -32,     0,   -29,   -31,   -26,
       0,   -26,   -29,   -31,   -29,   -31,   -33,   -30,     0,     0,
       0,   -33,   -30,   -33,   -30,     4,     5,     6,     7,     0,
       9,    36,    37,    38,     0,     0,    39,    24,    25,    26,
      27,    28,     0,    59,    60,    61,    65,    66,    67,    68,
      69,    70
};

static const short yycheck[] =
{
      35,     2,     7,     5,     6,    40,    11,    19,    12,    14,
      15,    16,    17,    18,    19,    20,    21,     0,     8,     9,
      10,     0,    30,    34,    14,    39,     1,    62,    29,    19,
      41,    21,     8,     9,    10,     8,     9,    10,    14,    -1,
      -1,    14,    -1,    19,    -1,    21,    19,    14,    21,     8,
       9,    10,    19,    -1,    21,    14,     8,     9,    10,    -1,
      19,    11,    21,    -1,    14,    15,    16,    17,    18,    19,
      20,    21,    14,    14,    -1,    -1,    14,    19,    19,    21,
      21,    19,    14,    21,    -1,    14,    14,    19,    -1,    21,
      19,    19,    21,    21,    14,    14,    -1,    -1,    14,    19,
      19,    21,    21,    19,    14,    21,    -1,    14,    14,    19,
      -1,    21,    19,    19,    21,    21,    14,    14,    -1,    -1,
      -1,    19,    19,    21,    21,    15,    16,    17,    18,    -1,
      20,     8,     9,    10,    -1,    -1,    13,     3,     4,     5,
       6,     7,    -1,     8,     9,    10,     8,     9,    10,     8,
       9,    10
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
#line 137 "./cond.y"
{
            COND_input* cond = (COND_input*) info;
            cond->result = yyvsp[0].value;
        ;
    break;}
case 2:
#line 145 "./cond.y"
{
            yyval.value = yyvsp[0].value;
        ;
    break;}
case 3:
#line 149 "./cond.y"
{
            yyval.value = yyvsp[-2].value || yyvsp[0].value;
        ;
    break;}
case 4:
#line 156 "./cond.y"
{
            yyval.value = yyvsp[0].value;
        ;
    break;}
case 5:
#line 160 "./cond.y"
{
            yyval.value = yyvsp[-2].value && yyvsp[0].value;
        ;
    break;}
case 6:
#line 167 "./cond.y"
{
            yyval.value = yyvsp[0].value;
        ;
    break;}
case 7:
#line 171 "./cond.y"
{
            yyval.value = ! yyvsp[0].value;
        ;
    break;}
case 8:
#line 179 "./cond.y"
{
            yyval.value = yyvsp[0].value;
        ;
    break;}
case 9:
#line 183 "./cond.y"
{
            yyval.value = atoiW(yyvsp[0].string);
        ;
    break;}
case 10:
#line 187 "./cond.y"
{
            yyval.value = yyvsp[-1].fn_comp_int( yyvsp[-2].value, yyvsp[0].value );
        ;
    break;}
case 11:
#line 191 "./cond.y"
{
            yyval.value = yyvsp[-1].fn_comp_str( yyvsp[-2].string, yyvsp[0].string, FALSE );
        ;
    break;}
case 12:
#line 195 "./cond.y"
{
            yyval.value = yyvsp[-1].fn_comp_str( yyvsp[-3].string, yyvsp[0].string, TRUE );
        ;
    break;}
case 13:
#line 199 "./cond.y"
{
            yyval.value = yyvsp[-1].fn_comp_m1( yyvsp[-2].string, yyvsp[0].value );
        ;
    break;}
case 14:
#line 203 "./cond.y"
{
            yyval.value = yyvsp[-1].fn_comp_m2( yyvsp[-2].value, yyvsp[0].string );
        ;
    break;}
case 15:
#line 207 "./cond.y"
{
            yyval.value = yyvsp[-1].value;
        ;
    break;}
case 16:
#line 215 "./cond.y"
{
            yyval.fn_comp_int = comp_eq_i;
        ;
    break;}
case 17:
#line 219 "./cond.y"
{
            yyval.fn_comp_int = comp_ne_i;
        ;
    break;}
case 18:
#line 223 "./cond.y"
{
            yyval.fn_comp_int = comp_lt_i;
        ;
    break;}
case 19:
#line 227 "./cond.y"
{
            yyval.fn_comp_int = comp_gt_i;
        ;
    break;}
case 20:
#line 231 "./cond.y"
{
            yyval.fn_comp_int = comp_le_i;
        ;
    break;}
case 21:
#line 235 "./cond.y"
{
            yyval.fn_comp_int = comp_ge_i;
        ;
    break;}
case 22:
#line 240 "./cond.y"
{
            yyval.fn_comp_int = comp_bitand;
        ;
    break;}
case 23:
#line 244 "./cond.y"
{
            yyval.fn_comp_int = comp_highcomp;
        ;
    break;}
case 24:
#line 248 "./cond.y"
{
            yyval.fn_comp_int = comp_lowcomp;
        ;
    break;}
case 25:
#line 256 "./cond.y"
{
            yyval.fn_comp_str = comp_eq_s;
        ;
    break;}
case 26:
#line 260 "./cond.y"
{
            yyval.fn_comp_str = comp_ne_s;
        ;
    break;}
case 27:
#line 264 "./cond.y"
{
            yyval.fn_comp_str = comp_lt_s;
        ;
    break;}
case 28:
#line 268 "./cond.y"
{
            yyval.fn_comp_str = comp_gt_s;
        ;
    break;}
case 29:
#line 272 "./cond.y"
{
            yyval.fn_comp_str = comp_le_s;
        ;
    break;}
case 30:
#line 276 "./cond.y"
{
            yyval.fn_comp_str = comp_ge_s;
        ;
    break;}
case 31:
#line 281 "./cond.y"
{
            yyval.fn_comp_str = comp_substring;
        ;
    break;}
case 32:
#line 285 "./cond.y"
{
            yyval.fn_comp_str = comp_start;
        ;
    break;}
case 33:
#line 289 "./cond.y"
{
            yyval.fn_comp_str = comp_end;
        ;
    break;}
case 34:
#line 297 "./cond.y"
{
            yyval.fn_comp_m1 = comp_eq_m1;
        ;
    break;}
case 35:
#line 301 "./cond.y"
{
            yyval.fn_comp_m1 = comp_ne_m1;
        ;
    break;}
case 36:
#line 305 "./cond.y"
{
            yyval.fn_comp_m1 = comp_lt_m1;
        ;
    break;}
case 37:
#line 309 "./cond.y"
{
            yyval.fn_comp_m1 = comp_gt_m1;
        ;
    break;}
case 38:
#line 313 "./cond.y"
{
            yyval.fn_comp_m1 = comp_le_m1;
        ;
    break;}
case 39:
#line 317 "./cond.y"
{
            yyval.fn_comp_m1 = comp_ge_m1;
        ;
    break;}
case 40:
#line 322 "./cond.y"
{
            yyval.fn_comp_m1 = 0;
        ;
    break;}
case 41:
#line 326 "./cond.y"
{
            yyval.fn_comp_m1 = 0;
        ;
    break;}
case 42:
#line 330 "./cond.y"
{
            yyval.fn_comp_m1 = 0;
        ;
    break;}
case 43:
#line 338 "./cond.y"
{
            yyval.fn_comp_m2 = comp_eq_m2;
        ;
    break;}
case 44:
#line 342 "./cond.y"
{
            yyval.fn_comp_m2 = comp_ne_m2;
        ;
    break;}
case 45:
#line 346 "./cond.y"
{
            yyval.fn_comp_m2 = comp_lt_m2;
        ;
    break;}
case 46:
#line 350 "./cond.y"
{
            yyval.fn_comp_m2 = comp_gt_m2;
        ;
    break;}
case 47:
#line 354 "./cond.y"
{
            yyval.fn_comp_m2 = comp_le_m2;
        ;
    break;}
case 48:
#line 358 "./cond.y"
{
            yyval.fn_comp_m2 = comp_ge_m2;
        ;
    break;}
case 49:
#line 363 "./cond.y"
{
            yyval.fn_comp_m2 = 0;
        ;
    break;}
case 50:
#line 367 "./cond.y"
{
            yyval.fn_comp_m2 = 0;
        ;
    break;}
case 51:
#line 371 "./cond.y"
{
            yyval.fn_comp_m2 = 0;
        ;
    break;}
case 52:
#line 378 "./cond.y"
{
            yyval.value = yyvsp[0].value;
        ;
    break;}
case 53:
#line 382 "./cond.y"
{
            yyval.value = yyvsp[0].value;
        ;
    break;}
case 54:
#line 389 "./cond.y"
{
        yyval.string = yyvsp[0].string;
    ;
    break;}
case 55:
#line 393 "./cond.y"
{
        yyval.string = yyvsp[0].string;
    ;
    break;}
case 56:
#line 400 "./cond.y"
{
            yyval.string = COND_GetLiteral(&yyvsp[0].str);
            if( !yyval.string )
                YYABORT;
        ;
    break;}
case 57:
#line 409 "./cond.y"
{
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;
      
            MSI_GetComponentStateW(cond->package, yyvsp[0].string, &install, &action );
            yyval.value = action;
            HeapFree( GetProcessHeap(), 0, yyvsp[0].string );
        ;
    break;}
case 58:
#line 418 "./cond.y"
{
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;
      
            MSI_GetComponentStateW(cond->package, yyvsp[0].string, &install, &action );
            yyval.value = install;
            HeapFree( GetProcessHeap(), 0, yyvsp[0].string );
        ;
    break;}
case 59:
#line 427 "./cond.y"
{
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;
      
            MSI_GetFeatureStateW(cond->package, yyvsp[0].string, &install, &action );
            yyval.value = action;
            HeapFree( GetProcessHeap(), 0, yyvsp[0].string );
        ;
    break;}
case 60:
#line 436 "./cond.y"
{
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;
      
            MSI_GetFeatureStateW(cond->package, yyvsp[0].string, &install, &action );
            yyval.value = install;
            HeapFree( GetProcessHeap(), 0, yyvsp[0].string );
        ;
    break;}
case 61:
#line 448 "./cond.y"
{
            DWORD sz;
            COND_input* cond = (COND_input*) info;
            yyval.string = HeapAlloc( GetProcessHeap(), 0, 0x100*sizeof (WCHAR) );

            /* Lookup the identifier */

            sz=0x100;
            if (MSI_GetPropertyW(cond->package,yyvsp[0].string,yyval.string,&sz) != ERROR_SUCCESS)
            {
                yyval.string[0]=0;
            }
            HeapFree( GetProcessHeap(), 0, yyvsp[0].string );
        ;
    break;}
case 62:
#line 463 "./cond.y"
{
            UINT len = GetEnvironmentVariableW( yyvsp[0].string, NULL, 0 );
            if( len++ )
            {
                yyval.string = HeapAlloc( GetProcessHeap(), 0, len*sizeof (WCHAR) );
                if( yyval.string )
                    GetEnvironmentVariableW( yyvsp[0].string, yyval.string, len );
            }
            HeapFree( GetProcessHeap(), 0, yyvsp[0].string );
        ;
    break;}
case 63:
#line 477 "./cond.y"
{
            yyval.string = COND_GetString(&yyvsp[0].str);
            if( !yyval.string )
                YYABORT;
        ;
    break;}
case 64:
#line 486 "./cond.y"
{
            LPWSTR szNum = COND_GetString(&yyvsp[0].str);
            if( !szNum )
                YYABORT;
            yyval.value = atoiW( szNum );
            HeapFree( GetProcessHeap(), 0, szNum );
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
#line 495 "./cond.y"



static int COND_IsAlpha( WCHAR x )
{
    return( ( ( x >= 'A' ) && ( x <= 'Z' ) ) ||
            ( ( x >= 'a' ) && ( x <= 'z' ) ) ||
            ( ( x == '_' ) ) );
}

static int COND_IsNumber( WCHAR x )
{
    return( (( x >= '0' ) && ( x <= '9' ))  || (x =='-') || (x =='.') );
}


/* the mess of comparison functions */

static INT comp_lt_i(INT a, INT b)
{ return (a < b); }
static INT comp_gt_i(INT a, INT b)
{ return (a > b); }
static INT comp_le_i(INT a, INT b)
{ return (a <= b); }
static INT comp_ge_i(INT a, INT b)
{ return (a >= b); }
static INT comp_eq_i(INT a, INT b)
{ return (a == b); }
static INT comp_ne_i(INT a, INT b)
{ return (a != b); }
static INT comp_bitand(INT a, INT b)
{ return a & b;}
static INT comp_highcomp(INT a, INT b)
{ return HIWORD(a)==b; }
static INT comp_lowcomp(INT a, INT b)
{ return LOWORD(a)==b; }

static INT comp_eq_s(LPWSTR a, LPWSTR b, BOOL casless)
{ if (casless) return !strcmpiW(a,b); else return !strcmpW(a,b);}
static INT comp_ne_s(LPWSTR a, LPWSTR b, BOOL casless)
{ if (casless) return strcmpiW(a,b); else  return strcmpW(a,b);}
static INT comp_lt_s(LPWSTR a, LPWSTR b, BOOL casless)
{ if (casless) return strcmpiW(a,b)<0; else return strcmpW(a,b)<0;}
static INT comp_gt_s(LPWSTR a, LPWSTR b, BOOL casless)
{ if (casless) return strcmpiW(a,b)>0; else return strcmpW(a,b)>0;}
static INT comp_le_s(LPWSTR a, LPWSTR b, BOOL casless)
{ if (casless) return strcmpiW(a,b)<=0; else return strcmpW(a,b)<=0;}
static INT comp_ge_s(LPWSTR a, LPWSTR b, BOOL casless)
{ if (casless) return strcmpiW(a,b)>=0; else return  strcmpW(a,b)>=0;}
static INT comp_substring(LPWSTR a, LPWSTR b, BOOL casless)
/* ERROR NOT WORKING REWRITE */
{ if (casless) return strstrW(a,b)!=NULL; else return strstrW(a,b)!=NULL;}
static INT comp_start(LPWSTR a, LPWSTR b, BOOL casless)
{ if (casless) return strncmpiW(a,b,strlenW(b))==0; 
  else return strncmpW(a,b,strlenW(b))==0;}
static INT comp_end(LPWSTR a, LPWSTR b, BOOL casless)
{ 
    int i = strlenW(a); 
    int j = strlenW(b); 
    if (j>i)
        return 0;
    if (casless) return (!strcmpiW(&a[i-j-1],b));
    else  return (!strcmpW(&a[i-j-1],b));
}


static INT comp_eq_m1(LPWSTR a, INT b)
{ if (COND_IsNumber(a[0])) return atoiW(a)==b; else return 0;}
static INT comp_ne_m1(LPWSTR a, INT b)
{ if (COND_IsNumber(a[0])) return atoiW(a)!=b; else return 1;}
static INT comp_lt_m1(LPWSTR a, INT b)
{ if (COND_IsNumber(a[0])) return atoiW(a)<b; else return 0;}
static INT comp_gt_m1(LPWSTR a, INT b)
{ if (COND_IsNumber(a[0])) return atoiW(a)>b; else return 0;}
static INT comp_le_m1(LPWSTR a, INT b)
{ if (COND_IsNumber(a[0])) return atoiW(a)<=b; else return 0;}
static INT comp_ge_m1(LPWSTR a, INT b)
{ if (COND_IsNumber(a[0])) return atoiW(a)>=b; else return 0;}

static INT comp_eq_m2(INT a, LPWSTR b)
{ if (COND_IsNumber(b[0])) return a == atoiW(b); else return 0;}
static INT comp_ne_m2(INT a, LPWSTR b)
{ if (COND_IsNumber(b[0])) return a != atoiW(b); else return 1;}
static INT comp_lt_m2(INT a, LPWSTR b)
{ if (COND_IsNumber(b[0])) return a < atoiW(b); else return 0;}
static INT comp_gt_m2(INT a, LPWSTR b)
{ if (COND_IsNumber(b[0])) return a > atoiW(b); else return 0;}
static INT comp_le_m2(INT a, LPWSTR b)
{ if (COND_IsNumber(b[0])) return a <= atoiW(b); else return 0;}
static INT comp_ge_m2(INT a, LPWSTR b)
{ if (COND_IsNumber(b[0])) return a >= atoiW(b); else return 0;}



static int COND_IsIdent( WCHAR x )
{
    return( COND_IsAlpha( x ) || COND_IsNumber( x ) || ( x == '_' ) 
            || ( x == '#' ) || (x == '.') );
}

static int COND_GetOne( struct cond_str *str, COND_input *cond )
{
    static const WCHAR szNot[] = {'N','O','T',0};
    static const WCHAR szAnd[] = {'A','N','D',0};
    static const WCHAR szOr[] = {'O','R',0};
    WCHAR ch;
    int rc, len = 1;

    str->data = &cond->str[cond->n];

    ch = str->data[0];
    switch( ch )
    {
    case 0: return 0;
    case '(': rc = COND_LPAR; break;
    case ')': rc = COND_RPAR; break;
    case '&': rc = COND_AMPER; break;
    case '!': rc = COND_EXCLAM; break;
    case '$': rc = COND_DOLLARS; break;
    case '?': rc = COND_QUESTION; break;
    case '%': rc = COND_PERCENT; break;
    case ' ': rc = COND_SPACE; break;
    case '=': rc = COND_EQ; break;
    case '~': rc = COND_TILDA; break;
    case '<': rc = COND_LT; break;
    case '>': rc = COND_GT; break;
    case '"':
	{
	    const WCHAR *ch2 = str->data + 1;


	    while ( *ch2 && *ch2 != '"' )
	    	++ch2;
	    if (*ch2 == '"')
	    {
	        len = ch2 - str->data + 1;
		rc = COND_LITER;
		break;
	    }
	}
	ERR("Unterminated string\n");
	rc = COND_ERROR;
    	break;
    default: 
        if( COND_IsAlpha( ch ) )
        {
            while( COND_IsIdent( str->data[len] ) )
                len++;
            rc = COND_IDENT;
            break;
        }

        if( COND_IsNumber( ch ) )
        {
            while( COND_IsNumber( str->data[len] ) )
                len++;
            rc = COND_NUMBER;
            break;
        }

        ERR("Got unknown character %c(%x)\n",ch,ch);
        rc = COND_ERROR;
        break;
    }

    /* keyword identifiers */
    if( rc == COND_IDENT )
    {
        if( (len==3) && (strncmpiW(str->data,szNot,len)==0) )
            rc = COND_NOT;
        else if( (len==3) && (strncmpiW(str->data,szAnd,len)==0) )
            rc = COND_AND;
        else if( (len==2) && (strncmpiW(str->data,szOr,len)==0) )
            rc = COND_OR;
    }

    cond->n += len;
    str->len = len;

    return rc;
}

static int COND_lex( void *COND_lval, COND_input *cond )
{
    int rc;
    struct cond_str *str = COND_lval;

    do {
        rc = COND_GetOne( str, cond );
    } while (rc == COND_SPACE);
    
    return rc;
}

static LPWSTR COND_GetString( struct cond_str *str )
{
    LPWSTR ret;

    ret = HeapAlloc( GetProcessHeap(), 0, (str->len+1) * sizeof (WCHAR) );
    if( ret )
    {
        strncpyW( ret, str->data, str->len );
        ret[str->len]=0;
    }
    TRACE("Got identifier %s\n",debugstr_w(ret));
    return ret;
}

static LPWSTR COND_GetLiteral( struct cond_str *str )
{
    LPWSTR ret;

    ret = HeapAlloc( GetProcessHeap(), 0, (str->len-1) * sizeof (WCHAR) );
    if( ret )
    {
        memcpy( ret, str->data+1, (str->len-2) * sizeof(WCHAR) );
        ret[str->len - 2]=0;
    }
    TRACE("Got literal %s\n",debugstr_w(ret));
    return ret;
}

static int COND_error(char *str)
{
    return 0;
}

MSICONDITION MSI_EvaluateConditionW( MSIPACKAGE *package, LPCWSTR szCondition )
{
    COND_input cond;
    MSICONDITION r;

    cond.package = package;
    cond.str   = szCondition;
    cond.n     = 0;
    cond.result = -1;
    
    TRACE("Evaluating %s\n",debugstr_w(szCondition));    

    if( !COND_parse( &cond ) )
        r = cond.result;
    else
        r = MSICONDITION_ERROR;

    TRACE("Evaluates to %i\n",r);
    return r;
}

MSICONDITION WINAPI MsiEvaluateConditionW( MSIHANDLE hInstall, LPCWSTR szCondition )
{
    MSIPACKAGE *package;
    UINT ret;

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE);
    if( !package)
        return ERROR_INVALID_HANDLE;
    ret = MSI_EvaluateConditionW( package, szCondition );
    msiobj_release( &package->hdr );
    return ret;
}

MSICONDITION WINAPI MsiEvaluateConditionA( MSIHANDLE hInstall, LPCSTR szCondition )
{
    LPWSTR szwCond = NULL;
    MSICONDITION r;

    if( szCondition )
    {
        UINT len = MultiByteToWideChar( CP_ACP, 0, szCondition, -1, NULL, 0 );
        szwCond = HeapAlloc( GetProcessHeap(), 0, len * sizeof (WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, szCondition, -1, szwCond, len );
    }

    r = MsiEvaluateConditionW( hInstall, szwCond );

    if( szwCond )
        HeapFree( GetProcessHeap(), 0, szwCond );

    return r;
}
