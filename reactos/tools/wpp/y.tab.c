/* A Bison parser, made from ./ppy.y
   by GNU bison 1.35.  */

#define YYBISON 1  /* Identify Bison output.  */

#define yyparse ppparse
#define yylex pplex
#define yyerror pperror
#define yylval pplval
#define yychar ppchar
#define yydebug ppdebug
#define yynerrs ppnerrs
# define	tRCINCLUDE	257
# define	tIF	258
# define	tIFDEF	259
# define	tIFNDEF	260
# define	tELSE	261
# define	tELIF	262
# define	tENDIF	263
# define	tDEFINED	264
# define	tNL	265
# define	tINCLUDE	266
# define	tLINE	267
# define	tGCCLINE	268
# define	tERROR	269
# define	tWARNING	270
# define	tPRAGMA	271
# define	tPPIDENT	272
# define	tUNDEF	273
# define	tMACROEND	274
# define	tCONCAT	275
# define	tELIPSIS	276
# define	tSTRINGIZE	277
# define	tIDENT	278
# define	tLITERAL	279
# define	tMACRO	280
# define	tDEFINE	281
# define	tDQSTRING	282
# define	tSQSTRING	283
# define	tIQSTRING	284
# define	tUINT	285
# define	tSINT	286
# define	tULONG	287
# define	tSLONG	288
# define	tULONGLONG	289
# define	tSLONGLONG	290
# define	tRCINCLUDEPATH	291
# define	tLOGOR	292
# define	tLOGAND	293
# define	tEQ	294
# define	tNE	295
# define	tLTE	296
# define	tGTE	297
# define	tLSHIFT	298
# define	tRSHIFT	299

#line 30 "./ppy.y"

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "wpp_private.h"


#define UNARY_OP(r, v, OP)					\
	switch(v.type)						\
	{							\
	case cv_sint:	r.val.si  = OP v.val.si; break;		\
	case cv_uint:	r.val.ui  = OP v.val.ui; break;		\
	case cv_slong:	r.val.sl  = OP v.val.sl; break;		\
	case cv_ulong:	r.val.ul  = OP v.val.ul; break;		\
	case cv_sll:	r.val.sll = OP v.val.sll; break;	\
	case cv_ull:	r.val.ull = OP v.val.ull; break;	\
	}

#define cv_signed(v)	((v.type & FLAG_SIGNED) != 0)

#define BIN_OP_INT(r, v1, v2, OP)			\
	r.type = v1.type;				\
	if(cv_signed(v1) && cv_signed(v2))		\
		r.val.si = v1.val.si OP v2.val.si;	\
	else if(cv_signed(v1) && !cv_signed(v2))	\
		r.val.si = v1.val.si OP v2.val.ui;	\
	else if(!cv_signed(v1) && cv_signed(v2))	\
		r.val.ui = v1.val.ui OP v2.val.si;	\
	else						\
		r.val.ui = v1.val.ui OP v2.val.ui;

#define BIN_OP_LONG(r, v1, v2, OP)			\
	r.type = v1.type;				\
	if(cv_signed(v1) && cv_signed(v2))		\
		r.val.sl = v1.val.sl OP v2.val.sl;	\
	else if(cv_signed(v1) && !cv_signed(v2))	\
		r.val.sl = v1.val.sl OP v2.val.ul;	\
	else if(!cv_signed(v1) && cv_signed(v2))	\
		r.val.ul = v1.val.ul OP v2.val.sl;	\
	else						\
		r.val.ul = v1.val.ul OP v2.val.ul;

#define BIN_OP_LONGLONG(r, v1, v2, OP)			\
	r.type = v1.type;				\
	if(cv_signed(v1) && cv_signed(v2))		\
		r.val.sll = v1.val.sll OP v2.val.sll;	\
	else if(cv_signed(v1) && !cv_signed(v2))	\
		r.val.sll = v1.val.sll OP v2.val.ull;	\
	else if(!cv_signed(v1) && cv_signed(v2))	\
		r.val.ull = v1.val.ull OP v2.val.sll;	\
	else						\
		r.val.ull = v1.val.ull OP v2.val.ull;

#define BIN_OP(r, v1, v2, OP)						\
	switch(v1.type & SIZE_MASK)					\
	{								\
	case SIZE_INT:		BIN_OP_INT(r, v1, v2, OP); break;	\
	case SIZE_LONG:		BIN_OP_LONG(r, v1, v2, OP); break;	\
	case SIZE_LONGLONG:	BIN_OP_LONGLONG(r, v1, v2, OP); break;	\
	default: pp_internal_error(__FILE__, __LINE__, "Invalid type indicator (0x%04x)", v1.type);	\
	}


/*
 * Prototypes
 */
static int boolean(cval_t *v);
static void promote_equal_size(cval_t *v1, cval_t *v2);
static void cast_to_sint(cval_t *v);
static void cast_to_uint(cval_t *v);
static void cast_to_slong(cval_t *v);
static void cast_to_ulong(cval_t *v);
static void cast_to_sll(cval_t *v);
static void cast_to_ull(cval_t *v);
static marg_t *new_marg(char *str, def_arg_t type);
static marg_t *add_new_marg(char *str, def_arg_t type);
static int marg_index(char *id);
static mtext_t *new_mtext(char *str, int idx, def_exp_t type);
static mtext_t *combine_mtext(mtext_t *tail, mtext_t *mtp);
static char *merge_text(char *s1, char *s2);

/*
 * Local variables
 */
static marg_t **macro_args;	/* Macro parameters array while parsing */
static int	nmacro_args;


#line 126 "./ppy.y"
#ifndef YYSTYPE
typedef union{
	int		sint;
	unsigned int	uint;
	long		slong;
	unsigned long	ulong;
	wrc_sll_t	sll;
	wrc_ull_t	ull;
	int		*iptr;
	char		*cptr;
	cval_t		cval;
	marg_t		*marg;
	mtext_t		*mtext;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
#ifndef YYDEBUG
# define YYDEBUG 1
#endif



#define	YYFINAL		153
#define	YYFLAG		-32768
#define	YYNTBASE	62

/* YYTRANSLATE(YYLEX) -- Bison token number corresponding to YYLEX. */
#define YYTRANSLATE(x) ((unsigned)(x) <= 299 ? yytranslate[x] : 74)

/* YYTRANSLATE[YYLEX] -- Bison token number corresponding to YYLEX. */
static const char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    58,     2,     2,     2,     2,    44,     2,
      60,    61,    55,    53,    59,    54,     2,    56,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    39,     2,
      47,     2,    49,    38,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    43,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    42,     2,    57,     2,     2,     2,
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
      36,    37,    40,    41,    45,    46,    48,    50,    51,    52
};

#if YYDEBUG
static const short yyprhs[] =
{
       0,     0,     1,     4,     8,    12,    16,    20,    24,    28,
      31,    34,    38,    42,    49,    54,    59,    65,    72,    80,
      89,    92,    96,   100,   104,   108,   111,   114,   115,   117,
     119,   121,   123,   126,   129,   132,   133,   134,   136,   138,
     142,   146,   148,   149,   151,   153,   156,   158,   160,   162,
     164,   167,   169,   171,   173,   175,   177,   179,   181,   184,
     189,   191,   195,   199,   203,   207,   211,   215,   219,   223,
     227,   231,   235,   239,   243,   247,   251,   255,   259,   262,
     265,   268,   271,   275
};
static const short yyrhs[] =
{
      -1,    62,    63,     0,    12,    28,    11,     0,    12,    30,
      11,     0,     4,    73,    11,     0,     5,    24,    11,     0,
       6,    24,    11,     0,     8,    73,    11,     0,     7,    11,
       0,     9,    11,     0,    19,    24,    11,     0,    27,    64,
      11,     0,    26,    66,    67,    20,    70,    11,     0,    13,
      32,    28,    11,     0,    14,    32,    28,    11,     0,    14,
      32,    28,    32,    11,     0,    14,    32,    28,    32,    32,
      11,     0,    14,    32,    28,    32,    32,    32,    11,     0,
      14,    32,    28,    32,    32,    32,    32,    11,     0,    14,
      11,     0,    15,    64,    11,     0,    16,    64,    11,     0,
      17,    64,    11,     0,    18,    64,    11,     0,     3,    37,
       0,     3,    28,     0,     0,    65,     0,    25,     0,    28,
       0,    29,     0,    65,    25,     0,    65,    28,     0,    65,
      29,     0,     0,     0,    68,     0,    69,     0,    69,    59,
      22,     0,    69,    59,    24,     0,    24,     0,     0,    71,
       0,    72,     0,    71,    72,     0,    25,     0,    28,     0,
      29,     0,    21,     0,    23,    24,     0,    24,     0,    32,
       0,    31,     0,    34,     0,    33,     0,    36,     0,    35,
       0,    10,    24,     0,    10,    60,    24,    61,     0,    24,
       0,    73,    40,    73,     0,    73,    41,    73,     0,    73,
      45,    73,     0,    73,    46,    73,     0,    73,    47,    73,
       0,    73,    49,    73,     0,    73,    48,    73,     0,    73,
      50,    73,     0,    73,    53,    73,     0,    73,    54,    73,
       0,    73,    43,    73,     0,    73,    44,    73,     0,    73,
      42,    73,     0,    73,    55,    73,     0,    73,    56,    73,
       0,    73,    51,    73,     0,    73,    52,    73,     0,    53,
      73,     0,    54,    73,     0,    57,    73,     0,    58,    73,
       0,    60,    73,    61,     0,    73,    38,    73,    39,    73,
       0
};

#endif

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined. */
static const short yyrline[] =
{
       0,   181,   182,   186,   187,   188,   189,   190,   210,   231,
     254,   269,   270,   271,   274,   275,   276,   278,   280,   282,
     284,   285,   286,   287,   288,   289,   296,   302,   303,   306,
     307,   308,   309,   310,   311,   314,   317,   318,   321,   322,
     325,   326,   330,   331,   337,   338,   341,   342,   343,   344,
     345,   351,   360,   361,   362,   363,   364,   365,   366,   367,
     368,   369,   370,   371,   372,   373,   374,   375,   376,   377,
     378,   379,   380,   381,   382,   383,   384,   385,   386,   387,
     388,   389,   390,   391
};
#endif


#if (YYDEBUG) || defined YYERROR_VERBOSE

/* YYTNAME[TOKEN_NUM] -- String name of the token TOKEN_NUM. */
static const char *const yytname[] =
{
  "$", "error", "$undefined.", "tRCINCLUDE", "tIF", "tIFDEF", "tIFNDEF", 
  "tELSE", "tELIF", "tENDIF", "tDEFINED", "tNL", "tINCLUDE", "tLINE", 
  "tGCCLINE", "tERROR", "tWARNING", "tPRAGMA", "tPPIDENT", "tUNDEF", 
  "tMACROEND", "tCONCAT", "tELIPSIS", "tSTRINGIZE", "tIDENT", "tLITERAL", 
  "tMACRO", "tDEFINE", "tDQSTRING", "tSQSTRING", "tIQSTRING", "tUINT", 
  "tSINT", "tULONG", "tSLONG", "tULONGLONG", "tSLONGLONG", 
  "tRCINCLUDEPATH", "'?'", "':'", "tLOGOR", "tLOGAND", "'|'", "'^'", 
  "'&'", "tEQ", "tNE", "'<'", "tLTE", "'>'", "tGTE", "tLSHIFT", "tRSHIFT", 
  "'+'", "'-'", "'*'", "'/'", "'~'", "'!'", "','", "'('", "')'", 
  "pp_file", "preprocessor", "opt_text", "text", "res_arg", "allmargs", 
  "emargs", "margs", "opt_mtexts", "mtexts", "mtext", "pp_expr", 0
};
#endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives. */
static const short yyr1[] =
{
       0,    62,    62,    63,    63,    63,    63,    63,    63,    63,
      63,    63,    63,    63,    63,    63,    63,    63,    63,    63,
      63,    63,    63,    63,    63,    63,    63,    64,    64,    65,
      65,    65,    65,    65,    65,    66,    67,    67,    68,    68,
      69,    69,    70,    70,    71,    71,    72,    72,    72,    72,
      72,    72,    73,    73,    73,    73,    73,    73,    73,    73,
      73,    73,    73,    73,    73,    73,    73,    73,    73,    73,
      73,    73,    73,    73,    73,    73,    73,    73,    73,    73,
      73,    73,    73,    73
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN. */
static const short yyr2[] =
{
       0,     0,     2,     3,     3,     3,     3,     3,     3,     2,
       2,     3,     3,     6,     4,     4,     5,     6,     7,     8,
       2,     3,     3,     3,     3,     2,     2,     0,     1,     1,
       1,     1,     2,     2,     2,     0,     0,     1,     1,     3,
       3,     1,     0,     1,     1,     2,     1,     1,     1,     1,
       2,     1,     1,     1,     1,     1,     1,     1,     2,     4,
       1,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     2,     2,
       2,     2,     3,     5
};

/* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
   doesn't specify something else to do.  Zero means the default is an
   error. */
static const short yydefact[] =
{
       1,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    27,    27,    27,    27,     0,    35,    27,     2,
      26,    25,     0,    60,    53,    52,    55,    54,    57,    56,
       0,     0,     0,     0,     0,     0,     0,     0,     9,     0,
      10,     0,     0,     0,    20,     0,    29,    30,    31,     0,
      28,     0,     0,     0,     0,    36,     0,    58,     0,    78,
      79,    80,    81,     0,     5,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     6,     7,     8,     3,     4,     0,     0,
      21,    32,    33,    34,    22,    23,    24,    11,    41,     0,
      37,    38,    12,     0,    82,     0,    61,    62,    73,    71,
      72,    63,    64,    65,    67,    66,    68,    76,    77,    69,
      70,    74,    75,    14,    15,     0,    42,     0,    59,     0,
      16,     0,    49,     0,    51,    46,    47,    48,     0,    43,
      44,    39,    40,    83,    17,     0,    50,    13,    45,    18,
       0,    19,     0,     0
};

static const short yydefgoto[] =
{
       1,    19,    49,    50,    55,    99,   100,   101,   138,   139,
     140,    35
};

static const short yypact[] =
{
  -32768,   142,   -26,    -3,   -12,    -2,    30,    -3,    34,    91,
      14,     2,   -19,   -19,   -19,   -19,    28,-32768,   -19,-32768,
  -32768,-32768,   -23,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
      -3,    -3,    -3,    -3,    -3,    38,    42,    45,-32768,    85,
  -32768,    66,   109,    96,-32768,   124,-32768,-32768,-32768,   179,
      -9,   278,   280,   281,   282,   129,   283,-32768,   271,    88,
      88,-32768,-32768,    57,-32768,    -3,    -3,    -3,    -3,    -3,
      -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,
      -3,    -3,    -3,-32768,-32768,-32768,-32768,-32768,   285,     3,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,   277,
  -32768,   239,-32768,   238,-32768,   132,   167,   182,   196,   209,
     221,   231,   231,   111,   111,   111,   111,    61,    61,    88,
      88,-32768,-32768,-32768,-32768,     4,    19,   266,-32768,    -3,
  -32768,     6,-32768,   276,-32768,-32768,-32768,-32768,   290,    19,
  -32768,-32768,-32768,   151,-32768,     7,-32768,-32768,-32768,-32768,
     291,-32768,   303,-32768
};

static const short yypgoto[] =
{
  -32768,-32768,   -10,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
     165,    -7
};


#define	YYLAST		304


static const short yytable[] =
{
      39,    57,    20,    51,    52,    53,    46,    22,    56,    47,
      48,    21,    36,    44,   124,   130,    91,   144,   149,    92,
      93,    23,    37,    59,    60,    61,    62,    63,    24,    25,
      26,    27,    28,    29,    45,   125,   131,    58,   145,   150,
     132,    38,   133,   134,   135,    40,    43,   136,   137,    64,
      30,    31,    54,    83,    32,    33,    84,    34,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,    65,    86,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    65,    85,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    79,    80,    81,    82,   104,    41,
      87,    42,   143,    65,    88,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,   152,    81,    82,     2,     3,     4,     5,     6,
       7,     8,    89,    98,     9,    10,    11,    12,    13,    14,
      15,    16,    77,    78,    79,    80,    81,    82,    17,    18,
      65,   129,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    65,
      90,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,   141,    94,
     142,    95,    96,    97,   102,   103,   123,   126,   127,   128,
     146,   147,   151,   153,   148
};

static const short yycheck[] =
{
       7,    24,    28,    13,    14,    15,    25,    10,    18,    28,
      29,    37,    24,    11,    11,    11,    25,    11,    11,    28,
      29,    24,    24,    30,    31,    32,    33,    34,    31,    32,
      33,    34,    35,    36,    32,    32,    32,    60,    32,    32,
      21,    11,    23,    24,    25,    11,    32,    28,    29,    11,
      53,    54,    24,    11,    57,    58,    11,    60,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    38,    11,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    38,    11,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    53,    54,    55,    56,    61,    28,
      11,    30,   129,    38,    28,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,     0,    55,    56,     3,     4,     5,     6,     7,
       8,     9,    28,    24,    12,    13,    14,    15,    16,    17,
      18,    19,    51,    52,    53,    54,    55,    56,    26,    27,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    38,
      11,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    22,    11,
      24,    11,    11,    11,    11,    24,    11,    20,    59,    61,
      24,    11,    11,     0,   139
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "c:\\MinGW\\bin\\bison.simple"

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

#line 315 "c:\\MinGW\\bin\\bison.simple"


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

case 3:
#line 186 "./ppy.y"
{ pp_do_include(yyvsp[-1].cptr, 1); }
    break;
case 4:
#line 187 "./ppy.y"
{ pp_do_include(yyvsp[-1].cptr, 0); }
    break;
case 5:
#line 188 "./ppy.y"
{ pp_next_if_state(boolean(&yyvsp[-1].cval)); }
    break;
case 6:
#line 189 "./ppy.y"
{ pp_next_if_state(pplookup(yyvsp[-1].cptr) != NULL); free(yyvsp[-1].cptr); }
    break;
case 7:
#line 190 "./ppy.y"
{
		int t = pplookup(yyvsp[-1].cptr) == NULL;
		if(pp_incl_state.state == 0 && t && !pp_incl_state.seen_junk)
		{
			pp_incl_state.state	= 1;
			pp_incl_state.ppp	= yyvsp[-1].cptr;
			pp_incl_state.ifdepth	= pp_get_if_depth();
		}
		else if(pp_incl_state.state != 1)
		{
			pp_incl_state.state = -1;
			free(yyvsp[-1].cptr);
		}
		else
			free(yyvsp[-1].cptr);
		pp_next_if_state(t);
		if(pp_status.debug)
			fprintf(stderr, "tIFNDEF: %s:%d: include_state=%d, include_ppp='%s', include_ifdepth=%d\n",
                                pp_status.input, pp_status.line_number, pp_incl_state.state, pp_incl_state.ppp, pp_incl_state.ifdepth);
		}
    break;
case 8:
#line 210 "./ppy.y"
{
		pp_if_state_t s = pp_pop_if();
		switch(s)
		{
		case if_true:
		case if_elif:
			pp_push_if(if_elif);
			break;
		case if_false:
			pp_push_if(boolean(&yyvsp[-1].cval) ? if_true : if_false);
			break;
		case if_ignore:
			pp_push_if(if_ignore);
			break;
		case if_elsetrue:
		case if_elsefalse:
			pperror("#elif cannot follow #else");
		default:
			pp_internal_error(__FILE__, __LINE__, "Invalid pp_if_state (%d) in #elif directive", s);
		}
		}
    break;
case 9:
#line 231 "./ppy.y"
{
		pp_if_state_t s = pp_pop_if();
		switch(s)
		{
		case if_true:
			pp_push_if(if_elsefalse);
			break;
		case if_elif:
			pp_push_if(if_elif);
			break;
		case if_false:
			pp_push_if(if_elsetrue);
			break;
		case if_ignore:
			pp_push_if(if_ignore);
			break;
		case if_elsetrue:
		case if_elsefalse:
			pperror("#else clause already defined");
		default:
			pp_internal_error(__FILE__, __LINE__, "Invalid pp_if_state (%d) in #else directive", s);
		}
		}
    break;
case 10:
#line 254 "./ppy.y"
{
		pp_pop_if();
		if(pp_incl_state.ifdepth == pp_get_if_depth() && pp_incl_state.state == 1)
		{
			pp_incl_state.state = 2;
			pp_incl_state.seen_junk = 0;
		}
		else if(pp_incl_state.state != 1)
		{
			pp_incl_state.state = -1;
		}
		if(pp_status.debug)
			fprintf(stderr, "tENDIF: %s:%d: include_state=%d, include_ppp='%s', include_ifdepth=%d\n",
                                pp_status.input, pp_status.line_number, pp_incl_state.state, pp_incl_state.ppp, pp_incl_state.ifdepth);
		}
    break;
case 11:
#line 269 "./ppy.y"
{ pp_del_define(yyvsp[-1].cptr); free(yyvsp[-1].cptr); }
    break;
case 12:
#line 270 "./ppy.y"
{ pp_add_define(yyvsp[-2].cptr, yyvsp[-1].cptr); }
    break;
case 13:
#line 271 "./ppy.y"
{
		pp_add_macro(yyvsp[-5].cptr, macro_args, nmacro_args, yyvsp[-1].mtext);
		}
    break;
case 14:
#line 274 "./ppy.y"
{ fprintf(ppout, "# %d %s\n", yyvsp[-2].sint , yyvsp[-1].cptr); free(yyvsp[-1].cptr); }
    break;
case 15:
#line 275 "./ppy.y"
{ fprintf(ppout, "# %d %s\n", yyvsp[-2].sint , yyvsp[-1].cptr); free(yyvsp[-1].cptr); }
    break;
case 16:
#line 277 "./ppy.y"
{ fprintf(ppout, "# %d %s %d\n", yyvsp[-3].sint, yyvsp[-2].cptr, yyvsp[-1].sint); free(yyvsp[-2].cptr); }
    break;
case 17:
#line 279 "./ppy.y"
{ fprintf(ppout, "# %d %s %d %d\n", yyvsp[-4].sint ,yyvsp[-3].cptr, yyvsp[-2].sint, yyvsp[-1].sint); free(yyvsp[-3].cptr); }
    break;
case 18:
#line 281 "./ppy.y"
{ fprintf(ppout, "# %d %s %d %d %d\n", yyvsp[-5].sint ,yyvsp[-4].cptr ,yyvsp[-3].sint ,yyvsp[-2].sint, yyvsp[-1].sint); free(yyvsp[-4].cptr); }
    break;
case 19:
#line 283 "./ppy.y"
{ fprintf(ppout, "# %d %s %d %d %d %d\n", yyvsp[-6].sint ,yyvsp[-5].cptr ,yyvsp[-4].sint ,yyvsp[-3].sint, yyvsp[-2].sint, yyvsp[-1].sint); free(yyvsp[-5].cptr); }
    break;
case 21:
#line 285 "./ppy.y"
{ pperror("#error directive: '%s'", yyvsp[-1].cptr); if(yyvsp[-1].cptr) free(yyvsp[-1].cptr); }
    break;
case 22:
#line 286 "./ppy.y"
{ ppwarning("#warning directive: '%s'", yyvsp[-1].cptr); if(yyvsp[-1].cptr) free(yyvsp[-1].cptr); }
    break;
case 23:
#line 287 "./ppy.y"
{ fprintf(ppout, "#pragma %s\n", yyvsp[-1].cptr ? yyvsp[-1].cptr : ""); if (yyvsp[-1].cptr) free(yyvsp[-1].cptr); }
    break;
case 24:
#line 288 "./ppy.y"
{ if(pp_status.pedantic) ppwarning("#ident ignored (arg: '%s')", yyvsp[-1].cptr); if(yyvsp[-1].cptr) free(yyvsp[-1].cptr); }
    break;
case 25:
#line 289 "./ppy.y"
{
                int nl=strlen(yyvsp[0].cptr) +3;
                char *fn=pp_xmalloc(nl);
                sprintf(fn,"\"%s\"",yyvsp[0].cptr);
		free(yyvsp[0].cptr);
		pp_do_include(fn,1);
	}
    break;
case 26:
#line 296 "./ppy.y"
{
		pp_do_include(yyvsp[0].cptr,1);
	}
    break;
case 27:
#line 302 "./ppy.y"
{ yyval.cptr = NULL; }
    break;
case 28:
#line 303 "./ppy.y"
{ yyval.cptr = yyvsp[0].cptr; }
    break;
case 29:
#line 306 "./ppy.y"
{ yyval.cptr = yyvsp[0].cptr; }
    break;
case 30:
#line 307 "./ppy.y"
{ yyval.cptr = yyvsp[0].cptr; }
    break;
case 31:
#line 308 "./ppy.y"
{ yyval.cptr = yyvsp[0].cptr; }
    break;
case 32:
#line 309 "./ppy.y"
{ yyval.cptr = merge_text(yyvsp[-1].cptr, yyvsp[0].cptr); }
    break;
case 33:
#line 310 "./ppy.y"
{ yyval.cptr = merge_text(yyvsp[-1].cptr, yyvsp[0].cptr); }
    break;
case 34:
#line 311 "./ppy.y"
{ yyval.cptr = merge_text(yyvsp[-1].cptr, yyvsp[0].cptr); }
    break;
case 35:
#line 314 "./ppy.y"
{ macro_args = NULL; nmacro_args = 0; }
    break;
case 36:
#line 317 "./ppy.y"
{ yyval.sint = 0; macro_args = NULL; nmacro_args = 0; }
    break;
case 37:
#line 318 "./ppy.y"
{ yyval.sint = nmacro_args; }
    break;
case 38:
#line 321 "./ppy.y"
{ yyval.marg = yyvsp[0].marg; }
    break;
case 39:
#line 322 "./ppy.y"
{ yyval.marg = add_new_marg(NULL, arg_list); nmacro_args *= -1; }
    break;
case 40:
#line 325 "./ppy.y"
{ yyval.marg = add_new_marg(yyvsp[0].cptr, arg_single); }
    break;
case 41:
#line 326 "./ppy.y"
{ yyval.marg = add_new_marg(yyvsp[0].cptr, arg_single); }
    break;
case 42:
#line 330 "./ppy.y"
{ yyval.mtext = NULL; }
    break;
case 43:
#line 331 "./ppy.y"
{
		for(yyval.mtext = yyvsp[0].mtext; yyval.mtext && yyval.mtext->prev; yyval.mtext = yyval.mtext->prev)
			;
		}
    break;
case 44:
#line 337 "./ppy.y"
{ yyval.mtext = yyvsp[0].mtext; }
    break;
case 45:
#line 338 "./ppy.y"
{ yyval.mtext = combine_mtext(yyvsp[-1].mtext, yyvsp[0].mtext); }
    break;
case 46:
#line 341 "./ppy.y"
{ yyval.mtext = new_mtext(yyvsp[0].cptr, 0, exp_text); }
    break;
case 47:
#line 342 "./ppy.y"
{ yyval.mtext = new_mtext(yyvsp[0].cptr, 0, exp_text); }
    break;
case 48:
#line 343 "./ppy.y"
{ yyval.mtext = new_mtext(yyvsp[0].cptr, 0, exp_text); }
    break;
case 49:
#line 344 "./ppy.y"
{ yyval.mtext = new_mtext(NULL, 0, exp_concat); }
    break;
case 50:
#line 345 "./ppy.y"
{
		int mat = marg_index(yyvsp[0].cptr);
		if(mat < 0)
			pperror("Stringification identifier must be an argument parameter");
		yyval.mtext = new_mtext(NULL, mat, exp_stringize);
		}
    break;
case 51:
#line 351 "./ppy.y"
{
		int mat = marg_index(yyvsp[0].cptr);
		if(mat >= 0)
			yyval.mtext = new_mtext(NULL, mat, exp_subst);
		else
			yyval.mtext = new_mtext(yyvsp[0].cptr, 0, exp_text);
		}
    break;
case 52:
#line 360 "./ppy.y"
{ yyval.cval.type = cv_sint;  yyval.cval.val.si = yyvsp[0].sint; }
    break;
case 53:
#line 361 "./ppy.y"
{ yyval.cval.type = cv_uint;  yyval.cval.val.ui = yyvsp[0].uint; }
    break;
case 54:
#line 362 "./ppy.y"
{ yyval.cval.type = cv_slong; yyval.cval.val.sl = yyvsp[0].slong; }
    break;
case 55:
#line 363 "./ppy.y"
{ yyval.cval.type = cv_ulong; yyval.cval.val.ul = yyvsp[0].ulong; }
    break;
case 56:
#line 364 "./ppy.y"
{ yyval.cval.type = cv_sll;   yyval.cval.val.sl = yyvsp[0].sll; }
    break;
case 57:
#line 365 "./ppy.y"
{ yyval.cval.type = cv_ull;   yyval.cval.val.ul = yyvsp[0].ull; }
    break;
case 58:
#line 366 "./ppy.y"
{ yyval.cval.type = cv_sint;  yyval.cval.val.si = pplookup(yyvsp[0].cptr) != NULL; }
    break;
case 59:
#line 367 "./ppy.y"
{ yyval.cval.type = cv_sint;  yyval.cval.val.si = pplookup(yyvsp[-1].cptr) != NULL; }
    break;
case 60:
#line 368 "./ppy.y"
{ yyval.cval.type = cv_sint;  yyval.cval.val.si = 0; }
    break;
case 61:
#line 369 "./ppy.y"
{ yyval.cval.type = cv_sint; yyval.cval.val.si = boolean(&yyvsp[-2].cval) || boolean(&yyvsp[0].cval); }
    break;
case 62:
#line 370 "./ppy.y"
{ yyval.cval.type = cv_sint; yyval.cval.val.si = boolean(&yyvsp[-2].cval) && boolean(&yyvsp[0].cval); }
    break;
case 63:
#line 371 "./ppy.y"
{ promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval, ==) }
    break;
case 64:
#line 372 "./ppy.y"
{ promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval, !=) }
    break;
case 65:
#line 373 "./ppy.y"
{ promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval,  <) }
    break;
case 66:
#line 374 "./ppy.y"
{ promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval,  >) }
    break;
case 67:
#line 375 "./ppy.y"
{ promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval, <=) }
    break;
case 68:
#line 376 "./ppy.y"
{ promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval, >=) }
    break;
case 69:
#line 377 "./ppy.y"
{ promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval,  +) }
    break;
case 70:
#line 378 "./ppy.y"
{ promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval,  -) }
    break;
case 71:
#line 379 "./ppy.y"
{ promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval,  ^) }
    break;
case 72:
#line 380 "./ppy.y"
{ promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval,  &) }
    break;
case 73:
#line 381 "./ppy.y"
{ promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval,  |) }
    break;
case 74:
#line 382 "./ppy.y"
{ promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval,  *) }
    break;
case 75:
#line 383 "./ppy.y"
{ promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval,  /) }
    break;
case 76:
#line 384 "./ppy.y"
{ promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval, <<) }
    break;
case 77:
#line 385 "./ppy.y"
{ promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval, >>) }
    break;
case 78:
#line 386 "./ppy.y"
{ yyval.cval =  yyvsp[0].cval; }
    break;
case 79:
#line 387 "./ppy.y"
{ UNARY_OP(yyval.cval, yyvsp[0].cval, -) }
    break;
case 80:
#line 388 "./ppy.y"
{ UNARY_OP(yyval.cval, yyvsp[0].cval, ~) }
    break;
case 81:
#line 389 "./ppy.y"
{ yyval.cval.type = cv_sint; yyval.cval.val.si = !boolean(&yyvsp[0].cval); }
    break;
case 82:
#line 390 "./ppy.y"
{ yyval.cval =  yyvsp[-1].cval; }
    break;
case 83:
#line 391 "./ppy.y"
{ yyval.cval = boolean(&yyvsp[-4].cval) ? yyvsp[-2].cval : yyvsp[0].cval; }
    break;
}

#line 705 "c:\\MinGW\\bin\\bison.simple"


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
#line 394 "./ppy.y"


/*
 **************************************************************************
 * Support functions
 **************************************************************************
 */

static void cast_to_sint(cval_t *v)
{
	switch(v->type)
	{
	case cv_sint:	break;
	case cv_uint:	break;
	case cv_slong:	v->val.si = v->val.sl;	break;
	case cv_ulong:	v->val.si = v->val.ul;	break;
	case cv_sll:	v->val.si = v->val.sll;	break;
	case cv_ull:	v->val.si = v->val.ull;	break;
	}
	v->type = cv_sint;
}

static void cast_to_uint(cval_t *v)
{
	switch(v->type)
	{
	case cv_sint:	break;
	case cv_uint:	break;
	case cv_slong:	v->val.ui = v->val.sl;	break;
	case cv_ulong:	v->val.ui = v->val.ul;	break;
	case cv_sll:	v->val.ui = v->val.sll;	break;
	case cv_ull:	v->val.ui = v->val.ull;	break;
	}
	v->type = cv_uint;
}

static void cast_to_slong(cval_t *v)
{
	switch(v->type)
	{
	case cv_sint:	v->val.sl = v->val.si;	break;
	case cv_uint:	v->val.sl = v->val.ui;	break;
	case cv_slong:	break;
	case cv_ulong:	break;
	case cv_sll:	v->val.sl = v->val.sll;	break;
	case cv_ull:	v->val.sl = v->val.ull;	break;
	}
	v->type = cv_slong;
}

static void cast_to_ulong(cval_t *v)
{
	switch(v->type)
	{
	case cv_sint:	v->val.ul = v->val.si;	break;
	case cv_uint:	v->val.ul = v->val.ui;	break;
	case cv_slong:	break;
	case cv_ulong:	break;
	case cv_sll:	v->val.ul = v->val.sll;	break;
	case cv_ull:	v->val.ul = v->val.ull;	break;
	}
	v->type = cv_ulong;
}

static void cast_to_sll(cval_t *v)
{
	switch(v->type)
	{
	case cv_sint:	v->val.sll = v->val.si;	break;
	case cv_uint:	v->val.sll = v->val.ui;	break;
	case cv_slong:	v->val.sll = v->val.sl;	break;
	case cv_ulong:	v->val.sll = v->val.ul;	break;
	case cv_sll:	break;
	case cv_ull:	break;
	}
	v->type = cv_sll;
}

static void cast_to_ull(cval_t *v)
{
	switch(v->type)
	{
	case cv_sint:	v->val.ull = v->val.si;	break;
	case cv_uint:	v->val.ull = v->val.ui;	break;
	case cv_slong:	v->val.ull = v->val.sl;	break;
	case cv_ulong:	v->val.ull = v->val.ul;	break;
	case cv_sll:	break;
	case cv_ull:	break;
	}
	v->type = cv_ull;
}


static void promote_equal_size(cval_t *v1, cval_t *v2)
{
#define cv_sizeof(v)	((int)(v->type & SIZE_MASK))
	int s1 = cv_sizeof(v1);
	int s2 = cv_sizeof(v2);
#undef cv_sizeof

	if(s1 == s2)
		return;
	else if(s1 > s2)
	{
		switch(v1->type)
		{
		case cv_sint:	cast_to_sint(v2); break;
		case cv_uint:	cast_to_uint(v2); break;
		case cv_slong:	cast_to_slong(v2); break;
		case cv_ulong:	cast_to_ulong(v2); break;
		case cv_sll:	cast_to_sll(v2); break;
		case cv_ull:	cast_to_ull(v2); break;
		}
	}
	else
	{
		switch(v2->type)
		{
		case cv_sint:	cast_to_sint(v1); break;
		case cv_uint:	cast_to_uint(v1); break;
		case cv_slong:	cast_to_slong(v1); break;
		case cv_ulong:	cast_to_ulong(v1); break;
		case cv_sll:	cast_to_sll(v1); break;
		case cv_ull:	cast_to_ull(v1); break;
		}
	}
}


static int boolean(cval_t *v)
{
	switch(v->type)
	{
	case cv_sint:	return v->val.si != (int)0;
	case cv_uint:	return v->val.ui != (unsigned int)0;
	case cv_slong:	return v->val.sl != (long)0;
	case cv_ulong:	return v->val.ul != (unsigned long)0;
	case cv_sll:	return v->val.sll != (wrc_sll_t)0;
	case cv_ull:	return v->val.ull != (wrc_ull_t)0;
	}
	return 0;
}

static marg_t *new_marg(char *str, def_arg_t type)
{
	marg_t *ma = pp_xmalloc(sizeof(marg_t));
	ma->arg = str;
	ma->type = type;
	ma->nnl = 0;
	return ma;
}

static marg_t *add_new_marg(char *str, def_arg_t type)
{
	marg_t *ma = new_marg(str, type);
	nmacro_args++;
	macro_args = pp_xrealloc(macro_args, nmacro_args * sizeof(macro_args[0]));
	macro_args[nmacro_args-1] = ma;
	return ma;
}

static int marg_index(char *id)
{
	int t;
	for(t = 0; t < nmacro_args; t++)
	{
		if(!strcmp(id, macro_args[t]->arg))
			break;
	}
	return t < nmacro_args ? t : -1;
}

static mtext_t *new_mtext(char *str, int idx, def_exp_t type)
{
	mtext_t *mt = pp_xmalloc(sizeof(mtext_t));
	if(str == NULL)
		mt->subst.argidx = idx;
	else
		mt->subst.text = str;
	mt->type = type;
	mt->next = mt->prev = NULL;
	return mt;
}

static mtext_t *combine_mtext(mtext_t *tail, mtext_t *mtp)
{
	if(!tail)
		return mtp;

	if(!mtp)
		return tail;

	if(tail->type == exp_text && mtp->type == exp_text)
	{
		tail->subst.text = pp_xrealloc(tail->subst.text, strlen(tail->subst.text)+strlen(mtp->subst.text)+1);
		strcat(tail->subst.text, mtp->subst.text);
		free(mtp->subst.text);
		free(mtp);
		return tail;
	}

	if(tail->type == exp_concat && mtp->type == exp_concat)
	{
		free(mtp);
		return tail;
	}

	if(tail->type == exp_concat && mtp->type == exp_text)
	{
		int len = strlen(mtp->subst.text);
		while(len)
		{
/* FIXME: should delete space from head of string */
			if(isspace(mtp->subst.text[len-1] & 0xff))
				mtp->subst.text[--len] = '\0';
			else
				break;
		}

		if(!len)
		{
			free(mtp->subst.text);
			free(mtp);
			return tail;
		}
	}

	if(tail->type == exp_text && mtp->type == exp_concat)
	{
		int len = strlen(tail->subst.text);
		while(len)
		{
			if(isspace(tail->subst.text[len-1] & 0xff))
				tail->subst.text[--len] = '\0';
			else
				break;
		}

		if(!len)
		{
			mtp->prev = tail->prev;
			mtp->next = tail->next;
			if(tail->prev)
				tail->prev->next = mtp;
			free(tail->subst.text);
			free(tail);
			return mtp;
		}
	}

	tail->next = mtp;
	mtp->prev = tail;

	return mtp;
}

static char *merge_text(char *s1, char *s2)
{
	int l1 = strlen(s1);
	int l2 = strlen(s2);
	s1 = pp_xrealloc(s1, l1+l2+1);
	memcpy(s1+l1, s2, l2+1);
	free(s2);
	return s1;
}
