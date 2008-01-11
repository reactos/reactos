/* A Bison parser, made by GNU Bison 1.875c.  */

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
#define yyparse COND_parse
#define yylex   COND_lex
#define yyerror COND_error
#define yylval  COND_lval
#define yychar  COND_char
#define yydebug COND_debug
#define yynerrs COND_nerrs


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     COND_SPACE = 258,
     COND_EOF = 259,
     COND_OR = 260,
     COND_AND = 261,
     COND_NOT = 262,
     COND_LT = 263,
     COND_GT = 264,
     COND_EQ = 265,
     COND_LPAR = 266,
     COND_RPAR = 267,
     COND_TILDA = 268,
     COND_PERCENT = 269,
     COND_DOLLARS = 270,
     COND_QUESTION = 271,
     COND_AMPER = 272,
     COND_EXCLAM = 273,
     COND_IDENT = 274,
     COND_NUMBER = 275,
     COND_LITER = 276,
     COND_ERROR = 277
   };
#endif
#define COND_SPACE 258
#define COND_EOF 259
#define COND_OR 260
#define COND_AND 261
#define COND_NOT 262
#define COND_LT 263
#define COND_GT 264
#define COND_EQ 265
#define COND_LPAR 266
#define COND_RPAR 267
#define COND_TILDA 268
#define COND_PERCENT 269
#define COND_DOLLARS 270
#define COND_QUESTION 271
#define COND_AMPER 272
#define COND_EXCLAM 273
#define COND_IDENT 274
#define COND_NUMBER 275
#define COND_LITER 276
#define COND_ERROR 277




/* Copy the first part of user declarations.  */
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

static int COND_error(const char *str);

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
#line 106 "./cond.y"
typedef union YYSTYPE {
    struct cond_str str;
    LPWSTR    string;
    INT       value;
    comp_int  fn_comp_int;
    comp_str  fn_comp_str;
    comp_m1   fn_comp_m1;
    comp_m2   fn_comp_m2;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 241 "cond.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 253 "cond.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

# ifndef YYFREE
#  define YYFREE free
# endif
# ifndef YYMALLOC
#  define YYMALLOC malloc
# endif

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   define YYSTACK_ALLOC alloca
#  endif
# else
#  if defined (alloca) || defined (_ALLOCA_H)
#   define YYSTACK_ALLOC alloca
#  else
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
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
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

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
#  if defined (__GNUC__) && 1 < __GNUC__
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
#define YYFINAL  30
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   146

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  23
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  17
/* YYNRULES -- Number of rules. */
#define YYNRULES  65
/* YYNRULES -- Number of states. */
#define YYNSTATES  74

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   277

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
      15,    16,    17,    18,    19,    20,    21,    22
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     5,     7,    11,    13,    17,    19,    22,
      24,    26,    30,    34,    39,    43,    47,    51,    53,    56,
      58,    60,    63,    66,    69,    72,    75,    77,    80,    82,
      84,    87,    90,    93,    96,    99,   101,   104,   106,   108,
     111,   114,   117,   120,   123,   125,   128,   130,   132,   135,
     138,   141,   144,   147,   149,   151,   153,   155,   157,   160,
     163,   166,   169,   171,   174,   176
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      24,     0,    -1,    25,    -1,    26,    -1,    26,     5,    25,
      -1,    27,    -1,    26,     6,    27,    -1,    28,    -1,     7,
      28,    -1,    33,    -1,    34,    -1,    33,    29,    33,    -1,
      34,    30,    34,    -1,    34,    13,    30,    34,    -1,    34,
      31,    33,    -1,    33,    32,    34,    -1,    11,    25,    12,
      -1,    10,    -1,     8,     9,    -1,     8,    -1,     9,    -1,
       8,    10,    -1,     9,    10,    -1,     9,     8,    -1,     8,
       8,    -1,     9,     9,    -1,    10,    -1,     8,     9,    -1,
       8,    -1,     9,    -1,     8,    10,    -1,     9,    10,    -1,
       9,     8,    -1,     8,     8,    -1,     9,     9,    -1,    10,
      -1,     8,     9,    -1,     8,    -1,     9,    -1,     8,    10,
      -1,     9,    10,    -1,     9,     8,    -1,     8,     8,    -1,
       9,     9,    -1,    10,    -1,     8,     9,    -1,     8,    -1,
       9,    -1,     8,    10,    -1,     9,    10,    -1,     9,     8,
      -1,     8,     8,    -1,     9,     9,    -1,    36,    -1,    39,
      -1,    37,    -1,    35,    -1,    21,    -1,    15,    38,    -1,
      16,    38,    -1,    17,    38,    -1,    18,    38,    -1,    38,
      -1,    14,    38,    -1,    19,    -1,    20,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   136,   136,   144,   148,   155,   159,   166,   170,   178,
     182,   187,   191,   197,   203,   208,   213,   221,   225,   229,
     233,   237,   241,   246,   250,   254,   262,   266,   270,   274,
     278,   282,   287,   291,   295,   303,   307,   311,   315,   319,
     323,   328,   332,   336,   344,   348,   352,   356,   360,   364,
     369,   373,   377,   384,   388,   395,   399,   406,   415,   424,
     433,   442,   454,   477,   491,   500
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "COND_SPACE", "COND_EOF", "COND_OR",
  "COND_AND", "COND_NOT", "COND_LT", "COND_GT", "COND_EQ", "COND_LPAR",
  "COND_RPAR", "COND_TILDA", "COND_PERCENT", "COND_DOLLARS",
  "COND_QUESTION", "COND_AMPER", "COND_EXCLAM", "COND_IDENT",
  "COND_NUMBER", "COND_LITER", "COND_ERROR", "$accept", "condition",
  "expression", "boolean_term", "boolean_factor", "term", "comp_op_i",
  "comp_op_s", "comp_op_m1", "comp_op_m2", "value_i", "value_s", "literal",
  "symbol_i", "symbol_s", "identifier", "integer", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    23,    24,    25,    25,    26,    26,    27,    27,    28,
      28,    28,    28,    28,    28,    28,    28,    29,    29,    29,
      29,    29,    29,    29,    29,    29,    30,    30,    30,    30,
      30,    30,    30,    30,    30,    31,    31,    31,    31,    31,
      31,    31,    31,    31,    32,    32,    32,    32,    32,    32,
      32,    32,    32,    33,    33,    34,    34,    35,    36,    36,
      36,    36,    37,    37,    38,    39
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     1,     3,     1,     3,     1,     2,     1,
       1,     3,     3,     4,     3,     3,     3,     1,     2,     1,
       1,     2,     2,     2,     2,     2,     1,     2,     1,     1,
       2,     2,     2,     2,     2,     1,     2,     1,     1,     2,
       2,     2,     2,     2,     1,     2,     1,     1,     2,     2,
       2,     2,     2,     1,     1,     1,     1,     1,     2,     2,
       2,     2,     1,     2,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,    64,    65,
      57,     0,     2,     3,     5,     7,     9,    10,    56,    53,
      55,    62,    54,     8,     0,    63,    58,    59,    60,    61,
       1,     0,     0,    19,    20,    17,     0,     0,    37,    38,
      35,     0,     0,     0,    16,     4,     6,    24,    18,    21,
      23,    25,    22,    11,    15,    42,    36,    39,    41,    43,
      40,    28,    29,    26,     0,    12,    14,    33,    27,    30,
      32,    34,    31,    13
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,    11,    12,    13,    14,    15,    36,    42,    43,    37,
      16,    17,    18,    19,    20,    21,    22
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -37
static const short yypact[] =
{
      -4,    51,    -4,   -11,   -11,   -11,   -11,   -11,   -37,   -37,
     -37,    18,   -37,    -1,   -37,   -37,    49,    14,   -37,   -37,
     -37,   -37,   -37,   -37,    19,   -37,   -37,   -37,   -37,   -37,
     -37,    -4,    -4,    11,    25,    34,   111,    59,    28,    42,
      60,   130,    59,   111,   -37,   -37,   -37,    63,    69,    72,
      73,    81,    82,   -37,   -37,    85,    91,    94,    95,   103,
     104,   133,   136,   -37,    59,   -37,   -37,   -37,   -37,   -37,
     -37,   -37,   -37,   -37
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
     -37,   -37,    -2,   -37,    -6,    39,   -37,     0,   -37,   -37,
     -34,   -36,   -37,   -37,   -37,   129,   -37
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -53
static const yysigned_char yytable[] =
{
      24,    54,    53,     1,    31,    32,    65,     2,     8,    66,
       3,     4,     5,     6,     7,     8,     9,    10,    30,    47,
      48,    49,    38,    39,    40,   -46,    46,    41,    73,    45,
     -46,    44,   -46,    50,    51,    52,    55,    56,    57,   -47,
      23,    64,   -28,     0,   -47,     0,   -47,   -28,   -44,   -28,
      58,    59,    60,   -44,     0,   -44,   -29,    33,    34,    35,
       0,   -29,     2,   -29,     0,     3,     4,     5,     6,     7,
       8,     9,    10,     3,   -26,     0,     0,   -51,     8,   -26,
      10,   -26,   -51,   -45,   -51,     0,   -48,   -50,   -45,     0,
     -45,   -48,   -50,   -48,   -50,   -52,   -49,     0,     0,   -33,
     -52,   -49,   -52,   -49,   -33,   -27,   -33,     0,   -30,   -32,
     -27,     0,   -27,   -30,   -32,   -30,   -32,   -34,   -31,     0,
       0,     0,   -34,   -31,   -34,   -31,     4,     5,     6,     7,
       0,     9,    25,    26,    27,    28,    29,     0,    61,    62,
      63,    67,    68,    69,    70,    71,    72
};

static const yysigned_char yycheck[] =
{
       2,    37,    36,     7,     5,     6,    42,    11,    19,    43,
      14,    15,    16,    17,    18,    19,    20,    21,     0,     8,
       9,    10,     8,     9,    10,    14,    32,    13,    64,    31,
      19,    12,    21,     8,     9,    10,     8,     9,    10,    14,
       1,    41,    14,    -1,    19,    -1,    21,    19,    14,    21,
       8,     9,    10,    19,    -1,    21,    14,     8,     9,    10,
      -1,    19,    11,    21,    -1,    14,    15,    16,    17,    18,
      19,    20,    21,    14,    14,    -1,    -1,    14,    19,    19,
      21,    21,    19,    14,    21,    -1,    14,    14,    19,    -1,
      21,    19,    19,    21,    21,    14,    14,    -1,    -1,    14,
      19,    19,    21,    21,    19,    14,    21,    -1,    14,    14,
      19,    -1,    21,    19,    19,    21,    21,    14,    14,    -1,
      -1,    -1,    19,    19,    21,    21,    15,    16,    17,    18,
      -1,    20,     3,     4,     5,     6,     7,    -1,     8,     9,
      10,     8,     9,    10,     8,     9,    10
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     7,    11,    14,    15,    16,    17,    18,    19,    20,
      21,    24,    25,    26,    27,    28,    33,    34,    35,    36,
      37,    38,    39,    28,    25,    38,    38,    38,    38,    38,
       0,     5,     6,     8,     9,    10,    29,    32,     8,     9,
      10,    13,    30,    31,    12,    25,    27,     8,     9,    10,
       8,     9,    10,    33,    34,     8,     9,    10,     8,     9,
      10,     8,     9,    10,    30,    34,    33,     8,     9,    10,
       8,     9,    10,    34
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
# define YYLLOC_DEFAULT(Current, Rhs, N)		\
   ((Current).first_line   = (Rhs)[1].first_line,	\
    (Current).first_column = (Rhs)[1].first_column,	\
    (Current).last_line    = (Rhs)[N].last_line,	\
    (Current).last_column  = (Rhs)[N].last_column)
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
| TOP (included).                                                   |
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

#if defined (YYMAXDEPTH) && YYMAXDEPTH == 0
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
#line 137 "./cond.y"
    {
            COND_input* cond = (COND_input*) info;
            cond->result = yyvsp[0].value;
        ;}
    break;

  case 3:
#line 145 "./cond.y"
    {
            yyval.value = yyvsp[0].value;
        ;}
    break;

  case 4:
#line 149 "./cond.y"
    {
            yyval.value = yyvsp[-2].value || yyvsp[0].value;
        ;}
    break;

  case 5:
#line 156 "./cond.y"
    {
            yyval.value = yyvsp[0].value;
        ;}
    break;

  case 6:
#line 160 "./cond.y"
    {
            yyval.value = yyvsp[-2].value && yyvsp[0].value;
        ;}
    break;

  case 7:
#line 167 "./cond.y"
    {
            yyval.value = yyvsp[0].value;
        ;}
    break;

  case 8:
#line 171 "./cond.y"
    {
            yyval.value = ! yyvsp[0].value;
        ;}
    break;

  case 9:
#line 179 "./cond.y"
    {
            yyval.value = yyvsp[0].value;
        ;}
    break;

  case 10:
#line 183 "./cond.y"
    {
            yyval.value = (yyvsp[0].string && yyvsp[0].string[0]) ? MSICONDITION_TRUE : MSICONDITION_FALSE;
            msi_free( yyvsp[0].string );
        ;}
    break;

  case 11:
#line 188 "./cond.y"
    {
            yyval.value = yyvsp[-1].fn_comp_int( yyvsp[-2].value, yyvsp[0].value );
        ;}
    break;

  case 12:
#line 192 "./cond.y"
    {
            yyval.value = yyvsp[-1].fn_comp_str( yyvsp[-2].string, yyvsp[0].string, FALSE );
            msi_free( yyvsp[-2].string );
            msi_free( yyvsp[0].string );
        ;}
    break;

  case 13:
#line 198 "./cond.y"
    {
            yyval.value = yyvsp[-1].fn_comp_str( yyvsp[-3].string, yyvsp[0].string, TRUE );
            msi_free( yyvsp[-3].string );
            msi_free( yyvsp[0].string );
        ;}
    break;

  case 14:
#line 204 "./cond.y"
    {
            yyval.value = yyvsp[-1].fn_comp_m1( yyvsp[-2].string, yyvsp[0].value );
            msi_free( yyvsp[-2].string );
        ;}
    break;

  case 15:
#line 209 "./cond.y"
    {
            yyval.value = yyvsp[-1].fn_comp_m2( yyvsp[-2].value, yyvsp[0].string );
            msi_free( yyvsp[0].string );
        ;}
    break;

  case 16:
#line 214 "./cond.y"
    {
            yyval.value = yyvsp[-1].value;
        ;}
    break;

  case 17:
#line 222 "./cond.y"
    {
            yyval.fn_comp_int = comp_eq_i;
        ;}
    break;

  case 18:
#line 226 "./cond.y"
    {
            yyval.fn_comp_int = comp_ne_i;
        ;}
    break;

  case 19:
#line 230 "./cond.y"
    {
            yyval.fn_comp_int = comp_lt_i;
        ;}
    break;

  case 20:
#line 234 "./cond.y"
    {
            yyval.fn_comp_int = comp_gt_i;
        ;}
    break;

  case 21:
#line 238 "./cond.y"
    {
            yyval.fn_comp_int = comp_le_i;
        ;}
    break;

  case 22:
#line 242 "./cond.y"
    {
            yyval.fn_comp_int = comp_ge_i;
        ;}
    break;

  case 23:
#line 247 "./cond.y"
    {
            yyval.fn_comp_int = comp_bitand;
        ;}
    break;

  case 24:
#line 251 "./cond.y"
    {
            yyval.fn_comp_int = comp_highcomp;
        ;}
    break;

  case 25:
#line 255 "./cond.y"
    {
            yyval.fn_comp_int = comp_lowcomp;
        ;}
    break;

  case 26:
#line 263 "./cond.y"
    {
            yyval.fn_comp_str = comp_eq_s;
        ;}
    break;

  case 27:
#line 267 "./cond.y"
    {
            yyval.fn_comp_str = comp_ne_s;
        ;}
    break;

  case 28:
#line 271 "./cond.y"
    {
            yyval.fn_comp_str = comp_lt_s;
        ;}
    break;

  case 29:
#line 275 "./cond.y"
    {
            yyval.fn_comp_str = comp_gt_s;
        ;}
    break;

  case 30:
#line 279 "./cond.y"
    {
            yyval.fn_comp_str = comp_le_s;
        ;}
    break;

  case 31:
#line 283 "./cond.y"
    {
            yyval.fn_comp_str = comp_ge_s;
        ;}
    break;

  case 32:
#line 288 "./cond.y"
    {
            yyval.fn_comp_str = comp_substring;
        ;}
    break;

  case 33:
#line 292 "./cond.y"
    {
            yyval.fn_comp_str = comp_start;
        ;}
    break;

  case 34:
#line 296 "./cond.y"
    {
            yyval.fn_comp_str = comp_end;
        ;}
    break;

  case 35:
#line 304 "./cond.y"
    {
            yyval.fn_comp_m1 = comp_eq_m1;
        ;}
    break;

  case 36:
#line 308 "./cond.y"
    {
            yyval.fn_comp_m1 = comp_ne_m1;
        ;}
    break;

  case 37:
#line 312 "./cond.y"
    {
            yyval.fn_comp_m1 = comp_lt_m1;
        ;}
    break;

  case 38:
#line 316 "./cond.y"
    {
            yyval.fn_comp_m1 = comp_gt_m1;
        ;}
    break;

  case 39:
#line 320 "./cond.y"
    {
            yyval.fn_comp_m1 = comp_le_m1;
        ;}
    break;

  case 40:
#line 324 "./cond.y"
    {
            yyval.fn_comp_m1 = comp_ge_m1;
        ;}
    break;

  case 41:
#line 329 "./cond.y"
    {
            yyval.fn_comp_m1 = 0;
        ;}
    break;

  case 42:
#line 333 "./cond.y"
    {
            yyval.fn_comp_m1 = 0;
        ;}
    break;

  case 43:
#line 337 "./cond.y"
    {
            yyval.fn_comp_m1 = 0;
        ;}
    break;

  case 44:
#line 345 "./cond.y"
    {
            yyval.fn_comp_m2 = comp_eq_m2;
        ;}
    break;

  case 45:
#line 349 "./cond.y"
    {
            yyval.fn_comp_m2 = comp_ne_m2;
        ;}
    break;

  case 46:
#line 353 "./cond.y"
    {
            yyval.fn_comp_m2 = comp_lt_m2;
        ;}
    break;

  case 47:
#line 357 "./cond.y"
    {
            yyval.fn_comp_m2 = comp_gt_m2;
        ;}
    break;

  case 48:
#line 361 "./cond.y"
    {
            yyval.fn_comp_m2 = comp_le_m2;
        ;}
    break;

  case 49:
#line 365 "./cond.y"
    {
            yyval.fn_comp_m2 = comp_ge_m2;
        ;}
    break;

  case 50:
#line 370 "./cond.y"
    {
            yyval.fn_comp_m2 = 0;
        ;}
    break;

  case 51:
#line 374 "./cond.y"
    {
            yyval.fn_comp_m2 = 0;
        ;}
    break;

  case 52:
#line 378 "./cond.y"
    {
            yyval.fn_comp_m2 = 0;
        ;}
    break;

  case 53:
#line 385 "./cond.y"
    {
            yyval.value = yyvsp[0].value;
        ;}
    break;

  case 54:
#line 389 "./cond.y"
    {
            yyval.value = yyvsp[0].value;
        ;}
    break;

  case 55:
#line 396 "./cond.y"
    {
        yyval.string = yyvsp[0].string;
    ;}
    break;

  case 56:
#line 400 "./cond.y"
    {
        yyval.string = yyvsp[0].string;
    ;}
    break;

  case 57:
#line 407 "./cond.y"
    {
            yyval.string = COND_GetLiteral(&yyvsp[0].str);
            if( !yyval.string )
                YYABORT;
        ;}
    break;

  case 58:
#line 416 "./cond.y"
    {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;

            MSI_GetComponentStateW(cond->package, yyvsp[0].string, &install, &action );
            yyval.value = action;
            msi_free( yyvsp[0].string );
        ;}
    break;

  case 59:
#line 425 "./cond.y"
    {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;

            MSI_GetComponentStateW(cond->package, yyvsp[0].string, &install, &action );
            yyval.value = install;
            msi_free( yyvsp[0].string );
        ;}
    break;

  case 60:
#line 434 "./cond.y"
    {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;

            MSI_GetFeatureStateW(cond->package, yyvsp[0].string, &install, &action );
            yyval.value = action;
            msi_free( yyvsp[0].string );
        ;}
    break;

  case 61:
#line 443 "./cond.y"
    {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;

            MSI_GetFeatureStateW(cond->package, yyvsp[0].string, &install, &action );
            yyval.value = install;
            msi_free( yyvsp[0].string );
        ;}
    break;

  case 62:
#line 455 "./cond.y"
    {
            DWORD sz;
            COND_input* cond = (COND_input*) info;

            sz = 0;
            MSI_GetPropertyW(cond->package, yyvsp[0].string, NULL, &sz);
            if (sz == 0)
            {
                yyval.string = msi_alloc( sizeof(WCHAR));
                yyval.string[0] = 0;
            }
            else
            {
                sz ++;
                yyval.string = msi_alloc( sz*sizeof (WCHAR) );

                /* Lookup the identifier */

                MSI_GetPropertyW(cond->package,yyvsp[0].string,yyval.string,&sz);
            }
            msi_free( yyvsp[0].string );
        ;}
    break;

  case 63:
#line 478 "./cond.y"
    {
            UINT len = GetEnvironmentVariableW( yyvsp[0].string, NULL, 0 );
            if( len++ )
            {
                yyval.string = msi_alloc( len*sizeof (WCHAR) );
                if( yyval.string )
                    GetEnvironmentVariableW( yyvsp[0].string, yyval.string, len );
            }
            msi_free( yyvsp[0].string );
        ;}
    break;

  case 64:
#line 492 "./cond.y"
    {
            yyval.string = COND_GetString(&yyvsp[0].str);
            if( !yyval.string )
                YYABORT;
        ;}
    break;

  case 65:
#line 501 "./cond.y"
    {
            LPWSTR szNum = COND_GetString(&yyvsp[0].str);
            if( !szNum )
                YYABORT;
            yyval.value = atoiW( szNum );
            msi_free( szNum );
        ;}
    break;


    }

/* Line 1000 of yacc.c.  */
#line 1733 "cond.tab.c"

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

      if (yychar <= YYEOF)
        {
          /* If at end of input, pop the error token,
	     then the rest of the stack, then return failure.  */
	  if (yychar == YYEOF)
	     for (;;)
	       {
		 YYPOPSTACK;
		 if (yyssp == yyss)
		   YYABORT;
		 YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
		 yydestruct (yystos[*yyssp], yyvsp);
	       }
        }
      else
	{
	  YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
	  yydestruct (yytoken, &yylval);
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

#ifdef __GNUC__
  /* Pacify GCC when the user code never invokes YYERROR and the label
     yyerrorlab therefore never appears in user code.  */
  if (0)
     goto yyerrorlab;
#endif

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

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
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


#line 510 "./cond.y"



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

    ret = msi_alloc( (str->len+1) * sizeof (WCHAR) );
    if( ret )
    {
        memcpy( ret, str->data, str->len * sizeof(WCHAR));
        ret[str->len]=0;
    }
    TRACE("Got identifier %s\n",debugstr_w(ret));
    return ret;
}

static LPWSTR COND_GetLiteral( struct cond_str *str )
{
    LPWSTR ret;

    ret = msi_alloc( (str->len-1) * sizeof (WCHAR) );
    if( ret )
    {
        memcpy( ret, str->data+1, (str->len-2) * sizeof(WCHAR) );
        ret[str->len - 2]=0;
    }
    TRACE("Got literal %s\n",debugstr_w(ret));
    return ret;
}

static int COND_error(const char *str)
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

    if ( szCondition == NULL || szCondition[0] == 0)
    	r = MSICONDITION_NONE;
    else if ( !COND_parse( &cond ) )
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
        szwCond = msi_alloc( len * sizeof (WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, szCondition, -1, szwCond, len );
    }

    r = MsiEvaluateConditionW( hInstall, szwCond );

    msi_free( szwCond );

    return r;
}

