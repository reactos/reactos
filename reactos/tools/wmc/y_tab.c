
/*  A Bison parser, made from ./mcy.y
 by  GNU Bison version 1.25
  */

#define YYBISON 1  /* Identify Bison output.  */

#define	tSEVNAMES	258
#define	tFACNAMES	259
#define	tLANNAMES	260
#define	tBASE	261
#define	tCODEPAGE	262
#define	tTYPEDEF	263
#define	tNL	264
#define	tSYMNAME	265
#define	tMSGEND	266
#define	tSEVERITY	267
#define	tFACILITY	268
#define	tLANGUAGE	269
#define	tMSGID	270
#define	tIDENT	271
#define	tLINE	272
#define	tFILE	273
#define	tCOMMENT	274
#define	tNUMBER	275
#define	tTOKEN	276

#line 23 "./mcy.y"


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "config.h"

#include "utils.h"
#include "wmc.h"
#include "lang.h"

static const char err_syntax[]	= "Syntax error";
static const char err_number[]	= "Number expected";
static const char err_ident[]	= "Identifier expected";
static const char err_assign[]	= "'=' expected";
static const char err_popen[]	= "'(' expected";
static const char err_pclose[]	= "')' expected";
static const char err_colon[]	= "':' expected";
static const char err_msg[]	= "Message expected";

/* Scanner switches */
int want_nl = 0;		/* Request next newlinw */
int want_line = 0;		/* Request next complete line */
int want_file = 0;		/* Request next ident as filename */

node_t *nodehead = NULL;	/* The list of all parsed elements */
static node_t *nodetail = NULL;
lan_blk_t *lanblockhead;	/* List of parsed elements transposed */

static int base = 16;		/* Current printout base to use (8, 10 or 16) */
static WCHAR *cast = NULL;	/* Current typecast to use */

static int last_id = 0;		/* The last message ID parsed */
static int last_sev = 0;	/* Last severity code parsed */
static int last_fac = 0;	/* Last facility code parsed */
static WCHAR *last_sym = NULL;/* Last alias symbol parsed */
static int have_sev;		/* Set if severity parsed for current message */
static int have_fac;		/* Set if facility parsed for current message */
static int have_sym;		/* Set is symbol parsed for current message */

static cp_xlat_t *cpxlattab = NULL;	/* Codepage translation table */
static int ncpxlattab = 0;

/* Prototypes */
static WCHAR *merge(WCHAR *s1, WCHAR *s2);
static lanmsg_t *new_lanmsg(lan_cp_t *lcp, WCHAR *msg);
static msg_t *add_lanmsg(msg_t *msg, lanmsg_t *lanmsg);
static msg_t *complete_msg(msg_t *msg, int id);
static void add_node(node_e type, void *p);
static void do_add_token(tok_e type, token_t *tok, const char *code);
static void test_id(int id);
static int check_languages(node_t *head);
static lan_blk_t *block_messages(node_t *head);
static void add_cpxlat(int lan, int cpin, int cpout);
static cp_xlat_t *find_cpxlat(int lan);


#line 83 "./mcy.y"
typedef union {
	WCHAR		*str;
	unsigned	num;
	token_t		*tok;
	lanmsg_t	*lmp;
	msg_t		*msg;
	lan_cp_t	lcp;
} YYSTYPE;
#ifndef YYDEBUG
#define YYDEBUG 1
#endif

#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		155
#define	YYFLAG		-32768
#define	YYNTBASE	27

#define YYTRANSLATE(x) ((unsigned)(x) <= 276 ? yytranslate[x] : 58)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,    23,
    24,     2,    26,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,    25,     2,     2,
    22,     2,     2,     2,     2,     2,     2,     2,     2,     2,
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
     2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
    16,    17,    18,    19,    20,    21
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     2,     4,     7,     9,    11,    13,    15,    21,    27,
    31,    34,    40,    46,    50,    53,    59,    65,    69,    72,
    78,    84,    88,    91,    95,    99,   102,   106,   110,   113,
   115,   118,   120,   125,   129,   132,   134,   137,   139,   144,
   148,   151,   152,   155,   158,   160,   163,   165,   173,   180,
   185,   189,   192,   193,   196,   199,   201,   204,   206,   212,
   218,   223,   227,   230,   232,   234,   235,   240,   244,   247,
   248,   250,   253,   256,   257,   260,   263,   266,   270,   274,
   277,   281,   285,   288,   292,   296,   299,   301,   304,   306,
   311,   317,   323,   328,   331,   333,   336,   338,   341,   343,
   345,   346,   347
};

static const short yyrhs[] = {    28,
     0,    29,     0,    28,    29,     0,    30,     0,    42,     0,
    19,     0,     1,     0,     3,    22,    23,    31,    24,     0,
     3,    22,    23,    31,     1,     0,     3,    22,     1,     0,
     3,     1,     0,     4,    22,    23,    33,    24,     0,     4,
    22,    23,    33,     1,     0,     4,    22,     1,     0,     4,
     1,     0,     5,    22,    23,    36,    24,     0,     5,    22,
    23,    36,     1,     0,     5,    22,     1,     0,     5,     1,
     0,     7,    22,    23,    39,    24,     0,     7,    22,    23,
    39,     1,     0,     7,    22,     1,     0,     7,     1,     0,
     8,    22,    16,     0,     8,    22,     1,     0,     8,     1,
     0,     6,    22,    20,     0,     6,    22,     1,     0,     6,
     1,     0,    32,     0,    31,    32,     0,     1,     0,    54,
    22,    20,    35,     0,    54,    22,     1,     0,    54,     1,
     0,    34,     0,    33,    34,     0,     1,     0,    54,    22,
    20,    35,     0,    54,    22,     1,     0,    54,     1,     0,
     0,    25,    16,     0,    25,     1,     0,    37,     0,    36,
    37,     0,     1,     0,    54,    22,    20,    57,    25,    18,
    38,     0,    54,    22,    20,    57,    25,     1,     0,    54,
    22,    20,     1,     0,    54,    22,     1,     0,    54,     1,
     0,     0,    25,    20,     0,    25,     1,     0,    40,     0,
    39,    40,     0,     1,     0,    41,    22,    20,    25,    20,
     0,    41,    22,    20,    25,     1,     0,    41,    22,    20,
     1,     0,    41,    22,     1,     0,    41,     1,     0,    20,
     0,    21,     0,     0,    44,    46,    43,    50,     0,    15,
    22,    45,     0,    15,     1,     0,     0,    20,     0,    26,
    20,     0,    26,     1,     0,     0,    46,    48,     0,    46,
    49,     0,    46,    47,     0,    10,    22,    16,     0,    10,
    22,     1,     0,    10,     1,     0,    12,    22,    54,     0,
    12,    22,     1,     0,    12,     1,     0,    13,    22,    54,
     0,    13,    22,     1,     0,    13,     1,     0,    51,     0,
    50,    51,     0,     1,     0,    52,    56,    53,    11,     0,
    14,    55,    22,    54,     9,     0,    14,    55,    22,    54,
     1,     0,    14,    55,    22,     1,     0,    14,     1,     0,
    17,     0,    53,    17,     0,     1,     0,    53,     1,     0,
    16,     0,    21,     0,     0,     0,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   108,   115,   116,   119,   120,   121,   122,   125,   126,   127,
   128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
   138,   139,   140,   141,   142,   143,   144,   156,   157,   163,
   164,   165,   168,   175,   176,   182,   183,   184,   187,   194,
   195,   198,   199,   200,   206,   207,   208,   211,   219,   220,
   221,   222,   225,   226,   227,   233,   234,   235,   238,   248,
   249,   250,   251,   254,   255,   265,   265,   268,   273,   276,
   277,   278,   279,   282,   283,   284,   285,   288,   289,   290,
   293,   301,   302,   305,   313,   314,   320,   321,   322,   325,
   333,   362,   363,   364,   367,   368,   369,   370,   376,   377,
   380,   383,   386
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","tSEVNAMES",
"tFACNAMES","tLANNAMES","tBASE","tCODEPAGE","tTYPEDEF","tNL","tSYMNAME","tMSGEND",
"tSEVERITY","tFACILITY","tLANGUAGE","tMSGID","tIDENT","tLINE","tFILE","tCOMMENT",
"tNUMBER","tTOKEN","'='","'('","')'","':'","'+'","file","items","decl","global",
"smaps","smap","fmaps","fmap","alias","lmaps","lmap","optcp","cmaps","cmap",
"clan","msg","@1","msgid","id","sevfacsym","sym","sev","fac","bodies","body",
"lang","lines","token","setnl","setline","setfile", NULL
};
#endif

static const short yyr1[] = {     0,
    27,    28,    28,    29,    29,    29,    29,    30,    30,    30,
    30,    30,    30,    30,    30,    30,    30,    30,    30,    30,
    30,    30,    30,    30,    30,    30,    30,    30,    30,    31,
    31,    31,    32,    32,    32,    33,    33,    33,    34,    34,
    34,    35,    35,    35,    36,    36,    36,    37,    37,    37,
    37,    37,    38,    38,    38,    39,    39,    39,    40,    40,
    40,    40,    40,    41,    41,    43,    42,    44,    44,    45,
    45,    45,    45,    46,    46,    46,    46,    47,    47,    47,
    48,    48,    48,    49,    49,    49,    50,    50,    50,    51,
    52,    52,    52,    52,    53,    53,    53,    53,    54,    54,
    55,    56,    57
};

static const short yyr2[] = {     0,
     1,     1,     2,     1,     1,     1,     1,     5,     5,     3,
     2,     5,     5,     3,     2,     5,     5,     3,     2,     5,
     5,     3,     2,     3,     3,     2,     3,     3,     2,     1,
     2,     1,     4,     3,     2,     1,     2,     1,     4,     3,
     2,     0,     2,     2,     1,     2,     1,     7,     6,     4,
     3,     2,     0,     2,     2,     1,     2,     1,     5,     5,
     4,     3,     2,     1,     1,     0,     4,     3,     2,     0,
     1,     2,     2,     0,     2,     2,     2,     3,     3,     2,
     3,     3,     2,     3,     3,     2,     1,     2,     1,     4,
     5,     5,     4,     2,     1,     2,     1,     2,     1,     1,
     0,     0,     0
};

static const short yydefact[] = {     0,
     7,     0,     0,     0,     0,     0,     0,     0,     6,     0,
     2,     4,     5,    74,    11,     0,    15,     0,    19,     0,
    29,     0,    23,     0,    26,     0,    69,    70,     3,    66,
    10,     0,    14,     0,    18,     0,    28,    27,    22,     0,
    25,    24,    71,     0,    68,     0,     0,     0,     0,    77,
    75,    76,    32,    99,   100,     0,    30,     0,    38,     0,
    36,     0,    47,     0,    45,     0,    58,    64,    65,     0,
    56,     0,    73,    72,    80,     0,    83,     0,    86,     0,
    89,     0,    67,    87,   102,     9,     8,    31,    35,     0,
    13,    12,    37,    41,     0,    17,    16,    46,    52,     0,
    21,    20,    57,    63,     0,    79,    78,    82,    81,    85,
    84,    94,     0,    88,     0,    34,    42,    40,    42,    51,
     0,    62,     0,     0,    97,    95,     0,     0,    33,    39,
    50,     0,    61,     0,    93,     0,    98,    90,    96,    44,
    43,     0,    60,    59,    92,    91,    49,    53,     0,    48,
    55,    54,     0,     0,     0
};

static const short yydefgoto[] = {   153,
    10,    11,    12,    56,    57,    60,    61,   129,    64,    65,
   150,    70,    71,    72,    13,    49,    14,    45,    30,    50,
    51,    52,    83,    84,    85,   127,    58,   113,   115,   132
};

static const short yypact[] = {   132,
-32768,    18,    44,    46,    47,    48,    49,    50,-32768,   113,
-32768,-32768,-32768,-32768,-32768,    11,-32768,    14,-32768,    19,
-32768,    85,-32768,    20,-32768,   147,-32768,   -13,-32768,    87,
-32768,    66,-32768,    80,-32768,    82,-32768,-32768,-32768,    64,
-32768,-32768,-32768,   107,-32768,    51,    52,    53,     3,-32768,
-32768,-32768,-32768,-32768,-32768,     7,-32768,    54,-32768,     8,
-32768,    55,-32768,    17,-32768,    56,-32768,-32768,-32768,    15,
-32768,    57,-32768,-32768,-32768,   148,-32768,    88,-32768,    90,
-32768,    58,    -4,-32768,-32768,-32768,-32768,-32768,-32768,   109,
-32768,-32768,-32768,-32768,   114,-32768,-32768,-32768,-32768,   121,
-32768,-32768,-32768,-32768,   122,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,   -11,-32768,   129,-32768,    35,-32768,    35,-32768,
     0,-32768,     2,    91,-32768,-32768,   144,   149,-32768,-32768,
-32768,    36,-32768,   123,-32768,     5,-32768,-32768,-32768,-32768,
-32768,     4,-32768,-32768,-32768,-32768,-32768,    37,   124,-32768,
-32768,-32768,    63,    93,-32768
};

static const short yypgoto[] = {-32768,
-32768,    78,-32768,-32768,    38,-32768,    42,   -55,-32768,    31,
-32768,-32768,    61,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,    43,-32768,-32768,   -34,-32768,-32768,-32768
};


#define	YYLAST		165


static const short yytable[] = {    62,
   131,    66,   133,    81,   147,   145,    43,    86,    91,    82,
   124,    31,    44,   146,    33,   101,    82,    96,    15,    35,
    39,   148,    54,    54,  -103,    62,   134,    55,    55,    66,
    87,    92,    54,    32,    68,    69,    34,    55,   102,    16,
    97,    36,    40,   109,    17,   111,    19,    21,    23,    25,
    27,    75,    77,    79,    89,    94,    99,   104,   112,   128,
   142,   149,   154,   130,    67,    18,    53,    20,    22,    24,
    26,    28,    76,    78,    80,    90,    95,   100,   105,  -101,
    59,    54,    63,    68,    69,    37,    55,    29,   108,   136,
   110,   135,   155,    88,    98,    54,    46,    54,    47,    48,
    55,    93,    55,    54,    38,    54,    54,    73,    55,   116,
    55,    55,    -1,     1,   118,     2,     3,     4,     5,     6,
     7,   120,   122,   143,   151,   114,    74,     8,   117,   125,
   103,     9,     1,   119,     2,     3,     4,     5,     6,     7,
   121,   123,   144,   152,   137,   126,     8,    41,   106,   140,
     9,     0,     0,     0,   138,     0,     0,     0,     0,     0,
   139,     0,    42,   107,   141
};

static const short yycheck[] = {    34,
     1,    36,     1,     1,     1,     1,    20,     1,     1,    14,
    22,     1,    26,     9,     1,     1,    14,     1,     1,     1,
     1,    18,    16,    16,    25,    60,    25,    21,    21,    64,
    24,    24,    16,    23,    20,    21,    23,    21,    24,    22,
    24,    23,    23,    78,     1,    80,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     1,     1,    25,
    25,    25,     0,   119,     1,    22,     1,    22,    22,    22,
    22,    22,    22,    22,    22,    22,    22,    22,    22,    22,
     1,    16,     1,    20,    21,     1,    21,    10,     1,   124,
     1,     1,     0,    56,    64,    16,    10,    16,    12,    13,
    21,    60,    21,    16,    20,    16,    16,     1,    21,     1,
    21,    21,     0,     1,     1,     3,     4,     5,     6,     7,
     8,     1,     1,     1,     1,    83,    20,    15,    20,     1,
    70,    19,     1,    20,     3,     4,     5,     6,     7,     8,
    20,    20,    20,    20,     1,    17,    15,     1,     1,     1,
    19,    -1,    -1,    -1,    11,    -1,    -1,    -1,    -1,    -1,
    17,    -1,    16,    16,    16
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/share/bison.simple"

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

#ifndef alloca
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi)
#include <alloca.h>
#else /* not sparc */
#if defined (MSDOS) && !defined (__TURBOC__)
#include <malloc.h>
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
#include <malloc.h>
 #pragma alloca
#else /* not MSDOS, __TURBOC__, or _AIX */
#ifdef __hpux
#ifdef __cplusplus
extern "C" {
void *alloca (unsigned int);
};
#else /* not __cplusplus */
void *alloca ();
#endif /* not __cplusplus */
#endif /* __hpux */
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc.  */
#endif /* not GNU C.  */
#endif /* alloca not defined.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	return(0)
#define YYABORT 	return(1)
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

#ifndef YYPARSE_RETURN_TYPE
#define YYPARSE_RETURN_TYPE int
#endif

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
YYPARSE_RETURN_TYPE yyparse (void);
#endif

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, int count)
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 196 "/usr/share/bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

YYPARSE_RETURN_TYPE
yyparse(YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
      yyss = (short *) _alloca (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss, (char *)yyss1, size * sizeof (*yyssp));
      yyvs = (YYSTYPE *) _alloca (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs, (char *)yyvs1, size * sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) _alloca (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls, (char *)yyls1, size * sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
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
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
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

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 1:
#line 108 "./mcy.y"
{
		if(!check_languages(nodehead))
			xyyerror("No messages defined");
		lanblockhead = block_messages(nodehead);
	;
    break;}
case 5:
#line 120 "./mcy.y"
{ add_node(nd_msg, yyvsp[0].msg); ;
    break;}
case 6:
#line 121 "./mcy.y"
{ add_node(nd_comment, yyvsp[0].str); ;
    break;}
case 7:
#line 122 "./mcy.y"
{ xyyerror(err_syntax); /* `Catch all' error */ ;
    break;}
case 9:
#line 126 "./mcy.y"
{ xyyerror(err_pclose); ;
    break;}
case 10:
#line 127 "./mcy.y"
{ xyyerror(err_popen); ;
    break;}
case 11:
#line 128 "./mcy.y"
{ xyyerror(err_assign); ;
    break;}
case 13:
#line 130 "./mcy.y"
{ xyyerror(err_pclose); ;
    break;}
case 14:
#line 131 "./mcy.y"
{ xyyerror(err_popen); ;
    break;}
case 15:
#line 132 "./mcy.y"
{ xyyerror(err_assign); ;
    break;}
case 17:
#line 134 "./mcy.y"
{ xyyerror(err_pclose); ;
    break;}
case 18:
#line 135 "./mcy.y"
{ xyyerror(err_popen); ;
    break;}
case 19:
#line 136 "./mcy.y"
{ xyyerror(err_assign); ;
    break;}
case 21:
#line 138 "./mcy.y"
{ xyyerror(err_pclose); ;
    break;}
case 22:
#line 139 "./mcy.y"
{ xyyerror(err_popen); ;
    break;}
case 23:
#line 140 "./mcy.y"
{ xyyerror(err_assign); ;
    break;}
case 24:
#line 141 "./mcy.y"
{ cast = yyvsp[0].str; ;
    break;}
case 25:
#line 142 "./mcy.y"
{ xyyerror(err_number); ;
    break;}
case 26:
#line 143 "./mcy.y"
{ xyyerror(err_assign); ;
    break;}
case 27:
#line 144 "./mcy.y"
{
		switch(base)
		{
		case 8:
		case 10:
		case 16:
			base = yyvsp[0].num;
			break;
		default:
			xyyerror("Numberbase must be 8, 10 or 16");
		}
	;
    break;}
case 28:
#line 156 "./mcy.y"
{ xyyerror(err_number); ;
    break;}
case 29:
#line 157 "./mcy.y"
{ xyyerror(err_assign); ;
    break;}
case 32:
#line 165 "./mcy.y"
{ xyyerror(err_ident); ;
    break;}
case 33:
#line 168 "./mcy.y"
{
		yyvsp[-3].tok->token = yyvsp[-1].num;
		yyvsp[-3].tok->alias = yyvsp[0].str;
		if(yyvsp[-1].num & (~0x3))
			xyyerror("Severity value out of range (0x%08x > 0x3)", yyvsp[-1].num);
		do_add_token(tok_severity, yyvsp[-3].tok, "severity");
	;
    break;}
case 34:
#line 175 "./mcy.y"
{ xyyerror(err_number); ;
    break;}
case 35:
#line 176 "./mcy.y"
{ xyyerror(err_assign); ;
    break;}
case 38:
#line 184 "./mcy.y"
{ xyyerror(err_ident); ;
    break;}
case 39:
#line 187 "./mcy.y"
{
		yyvsp[-3].tok->token = yyvsp[-1].num;
		yyvsp[-3].tok->alias = yyvsp[0].str;
		if(yyvsp[-1].num & (~0xfff))
			xyyerror("Facility value out of range (0x%08x > 0xfff)", yyvsp[-1].num);
		do_add_token(tok_facility, yyvsp[-3].tok, "facility");
	;
    break;}
case 40:
#line 194 "./mcy.y"
{ xyyerror(err_number); ;
    break;}
case 41:
#line 195 "./mcy.y"
{ xyyerror(err_assign); ;
    break;}
case 42:
#line 198 "./mcy.y"
{ yyval.str = NULL; ;
    break;}
case 43:
#line 199 "./mcy.y"
{ yyval.str = yyvsp[0].str; ;
    break;}
case 44:
#line 200 "./mcy.y"
{ xyyerror(err_ident); ;
    break;}
case 47:
#line 208 "./mcy.y"
{ xyyerror(err_ident); ;
    break;}
case 48:
#line 211 "./mcy.y"
{
		yyvsp[-6].tok->token = yyvsp[-4].num;
		yyvsp[-6].tok->alias = yyvsp[-1].str;
		yyvsp[-6].tok->codepage = yyvsp[0].num;
		do_add_token(tok_language, yyvsp[-6].tok, "language");
		if(!find_language(yyvsp[-6].tok->token) && !find_cpxlat(yyvsp[-6].tok->token))
			yywarning("Language 0x%x not built-in, using codepage %d; use explicit codepage to override", yyvsp[-6].tok->token, WMC_DEFAULT_CODEPAGE);
	;
    break;}
case 49:
#line 219 "./mcy.y"
{ xyyerror("Filename expected"); ;
    break;}
case 50:
#line 220 "./mcy.y"
{ xyyerror(err_colon); ;
    break;}
case 51:
#line 221 "./mcy.y"
{ xyyerror(err_number); ;
    break;}
case 52:
#line 222 "./mcy.y"
{ xyyerror(err_assign); ;
    break;}
case 53:
#line 225 "./mcy.y"
{ yyval.num = 0; ;
    break;}
case 54:
#line 226 "./mcy.y"
{ yyval.num = yyvsp[0].num; ;
    break;}
case 55:
#line 227 "./mcy.y"
{ xyyerror("Codepage-number expected"); ;
    break;}
case 58:
#line 235 "./mcy.y"
{ xyyerror(err_ident); ;
    break;}
case 59:
#line 238 "./mcy.y"
{
		static const char err_nocp[] = "Codepage %d not builtin; cannot convert";
		if(find_cpxlat(yyvsp[-4].num))
			xyyerror("Codepage translation already defined for language 0x%x", yyvsp[-4].num);
//		if(yyvsp[-2].num && !find_codepage(yyvsp[-2].num))
		if(yyvsp[-2].num)
			xyyerror(err_nocp, yyvsp[-2].num);
//		if(yyvsp[0].num && !find_codepage(yyvsp[0].num))
		if(yyvsp[0].num)
			xyyerror(err_nocp, yyvsp[0].num);
		add_cpxlat(yyvsp[-4].num, yyvsp[-2].num, yyvsp[0].num);
	;
    break;}
case 60:
#line 248 "./mcy.y"
{ xyyerror(err_number); ;
    break;}
case 61:
#line 249 "./mcy.y"
{ xyyerror(err_colon); ;
    break;}
case 62:
#line 250 "./mcy.y"
{ xyyerror(err_number); ;
    break;}
case 63:
#line 251 "./mcy.y"
{ xyyerror(err_assign); ;
    break;}
case 64:
#line 254 "./mcy.y"
{ yyval.num = yyvsp[0].num; ;
    break;}
case 65:
#line 255 "./mcy.y"
{
		if(yyvsp[0].tok->type != tok_language)
			xyyerror("Language name or code expected");
		yyval.num = yyvsp[0].tok->token;
	;
    break;}
case 66:
#line 265 "./mcy.y"
{ test_id(yyvsp[-1].num); ;
    break;}
case 67:
#line 265 "./mcy.y"
{ yyval.msg = complete_msg(yyvsp[0].msg, yyvsp[-3].num); ;
    break;}
case 68:
#line 268 "./mcy.y"
{
		if(yyvsp[0].num & (~0xffff))
			xyyerror("Message ID value out of range (0x%08x > 0xffff)", yyvsp[0].num);
		yyval.num = yyvsp[0].num;
	;
    break;}
case 69:
#line 273 "./mcy.y"
{ xyyerror(err_assign); ;
    break;}
case 70:
#line 276 "./mcy.y"
{ yyval.num = ++last_id; ;
    break;}
case 71:
#line 277 "./mcy.y"
{ yyval.num = last_id = yyvsp[0].num; ;
    break;}
case 72:
#line 278 "./mcy.y"
{ yyval.num = last_id += yyvsp[0].num; ;
    break;}
case 73:
#line 279 "./mcy.y"
{ xyyerror(err_number); ;
    break;}
case 74:
#line 282 "./mcy.y"
{ have_sev = have_fac = have_sym = 0; ;
    break;}
case 75:
#line 283 "./mcy.y"
{ if(have_sev) xyyerror("Severity already defined"); have_sev = 1; ;
    break;}
case 76:
#line 284 "./mcy.y"
{ if(have_fac) xyyerror("Facility already defined"); have_fac = 1; ;
    break;}
case 77:
#line 285 "./mcy.y"
{ if(have_sym) xyyerror("Symbolname already defined"); have_sym = 1; ;
    break;}
case 78:
#line 288 "./mcy.y"
{ last_sym = yyvsp[0].str; ;
    break;}
case 79:
#line 289 "./mcy.y"
{ xyyerror(err_ident); ;
    break;}
case 80:
#line 290 "./mcy.y"
{ xyyerror(err_assign); ;
    break;}
case 81:
#line 293 "./mcy.y"
{
		token_t *tok = lookup_token(yyvsp[0].tok->name);
		if(!tok)
			xyyerror("Undefined severityname");
		if(tok->type != tok_severity)
			xyyerror("Identifier is not of class 'severity'");
		last_sev = tok->token;
	;
    break;}
case 82:
#line 301 "./mcy.y"
{ xyyerror(err_ident); ;
    break;}
case 83:
#line 302 "./mcy.y"
{ xyyerror(err_assign); ;
    break;}
case 84:
#line 305 "./mcy.y"
{
		token_t *tok = lookup_token(yyvsp[0].tok->name);
		if(!tok)
			xyyerror("Undefined facilityname");
		if(tok->type != tok_facility)
			xyyerror("Identifier is not of class 'facility'");
		last_fac = tok->token;
	;
    break;}
case 85:
#line 313 "./mcy.y"
{ xyyerror(err_ident); ;
    break;}
case 86:
#line 314 "./mcy.y"
{ xyyerror(err_assign); ;
    break;}
case 87:
#line 320 "./mcy.y"
{ yyval.msg = add_lanmsg(NULL, yyvsp[0].lmp); ;
    break;}
case 88:
#line 321 "./mcy.y"
{ yyval.msg = add_lanmsg(yyvsp[-1].msg, yyvsp[0].lmp); ;
    break;}
case 89:
#line 322 "./mcy.y"
{ xyyerror("'Language=...' (start of message text-definition) expected"); ;
    break;}
case 90:
#line 325 "./mcy.y"
{ yyval.lmp = new_lanmsg(&yyvsp[-3].lcp, yyvsp[-1].str); ;
    break;}
case 91:
#line 333 "./mcy.y"
{
		token_t *tok = lookup_token(yyvsp[-1].tok->name);
		cp_xlat_t *cpx;
		if(!tok)
			xyyerror("Undefined language");
		if(tok->type != tok_language)
			xyyerror("Identifier is not of class 'language'");
		if((cpx = find_cpxlat(tok->token)))
		{
			set_codepage(yyval.lcp.codepage = cpx->cpin);
		}
		else if(!tok->codepage)
		{
			const language_t *lan = find_language(tok->token);
			if(!lan)
			{
				/* Just set default; warning was given while parsing languagenames */
				set_codepage(yyval.lcp.codepage = WMC_DEFAULT_CODEPAGE);
			}
			else
			{
				/* The default seems to be to use the DOS codepage... */
				set_codepage(yyval.lcp.codepage = lan->doscp);
			}
		}
		else
			set_codepage(yyval.lcp.codepage = tok->codepage);
		yyval.lcp.language = tok->token;
	;
    break;}
case 92:
#line 362 "./mcy.y"
{ xyyerror("Missing newline"); ;
    break;}
case 93:
#line 363 "./mcy.y"
{ xyyerror(err_ident); ;
    break;}
case 94:
#line 364 "./mcy.y"
{ xyyerror(err_assign); ;
    break;}
case 95:
#line 367 "./mcy.y"
{ yyval.str = yyvsp[0].str; ;
    break;}
case 96:
#line 368 "./mcy.y"
{ yyval.str = merge(yyvsp[-1].str, yyvsp[0].str); ;
    break;}
case 97:
#line 369 "./mcy.y"
{ xyyerror(err_msg); ;
    break;}
case 98:
#line 370 "./mcy.y"
{ xyyerror(err_msg); ;
    break;}
case 99:
#line 376 "./mcy.y"
{ yyval.tok = xmalloc(sizeof(token_t)); yyval.tok->name = yyvsp[0].str; ;
    break;}
case 100:
#line 377 "./mcy.y"
{ yyval.tok = yyvsp[0].tok; ;
    break;}
case 101:
#line 380 "./mcy.y"
{ want_nl = 1; ;
    break;}
case 102:
#line 383 "./mcy.y"
{ want_line = 1; ;
    break;}
case 103:
#line 386 "./mcy.y"
{ want_file = 1; ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 498 "/usr/share/bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

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

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;
}
#line 389 "./mcy.y"


static WCHAR *merge(WCHAR *s1, WCHAR *s2)
{
	int l1 = unistrlen(s1);
	int l2 = unistrlen(s2);
	s1 = xrealloc(s1, (l1 + l2 + 1) * sizeof(*s1));
	unistrcpy(s1+l1, s2);
	free(s2);
	return s1;
}

static void do_add_token(tok_e type, token_t *tok, const char *code)
{
	token_t *tp = lookup_token(tok->name);
	if(tp)
	{
		if(tok->type != type)
			yywarning("Type change in token");
		if(tp != tok)
			xyyerror("Overlapping token not the same");
		/* else its already defined and changed */
		if(tok->fixed)
			xyyerror("Redefinition of %s", code);
		tok->fixed = 1;
	}
	else
	{
		add_token(type, tok->name, tok->token, tok->codepage, tok->alias, 1);
		free(tok);
	}
}

static lanmsg_t *new_lanmsg(lan_cp_t *lcp, WCHAR *msg)
{
	lanmsg_t *lmp = (lanmsg_t *)xmalloc(sizeof(lanmsg_t));
	lmp->lan = lcp->language;
	lmp->cp  = lcp->codepage;
	lmp->msg = msg;
	lmp->len = unistrlen(msg) + 1;	/* Include termination */
	if(lmp->len > 4096)
		yywarning("Message exceptionally long; might be a missing termination");
	return lmp;
}

static msg_t *add_lanmsg(msg_t *msg, lanmsg_t *lanmsg)
{
	int i;
	if(!msg)
		msg = xmalloc(sizeof(msg_t));
	msg->msgs = xrealloc(msg->msgs, (msg->nmsgs+1) * sizeof(*(msg->msgs)));
	msg->msgs[msg->nmsgs] = lanmsg;
	msg->nmsgs++;
	for(i = 0; i < msg->nmsgs-1; i++)
	{
		if(msg->msgs[i]->lan == lanmsg->lan)
			xyyerror("Message for language 0x%x already defined", lanmsg->lan);
	}
	return msg;
}

static int sort_lanmsg(const void *p1, const void *p2)
{
	return (*(lanmsg_t **)p1)->lan - (*(lanmsg_t **)p2)->lan;
}

static msg_t *complete_msg(msg_t *mp, int id)
{
	assert(mp != NULL);
	mp->id = id;
	if(have_sym)
		mp->sym = last_sym;
	else
		xyyerror("No symbolic name defined for message id %d", id);
	mp->sev = last_sev;
	mp->fac = last_fac;
	qsort(mp->msgs, mp->nmsgs, sizeof(*(mp->msgs)), sort_lanmsg);
	mp->realid = id | (last_sev << 30) | (last_fac << 16);
	if(custombit)
		mp->realid |= 1 << 29;
	mp->base = base;
	mp->cast = cast;
	return mp;
}

static void add_node(node_e type, void *p)
{
	node_t *ndp = (node_t *)xmalloc(sizeof(node_t));
	ndp->type = type;
	ndp->u.all = p;

	if(nodetail)
	{
		ndp->prev = nodetail;
		nodetail->next = ndp;
		nodetail = ndp;
	}
	else
	{
		nodehead = nodetail = ndp;
	}
}

static void test_id(int id)
{
	node_t *ndp;
	for(ndp = nodehead; ndp; ndp = ndp->next)
	{
		if(ndp->type != nd_msg)
			continue;
		if(ndp->u.msg->id == id && ndp->u.msg->sev == last_sev && ndp->u.msg->fac == last_fac)
			xyyerror("MessageId %d with facility 0x%x and severity 0x%x already defined", id, last_fac, last_sev);
	}
}

static int check_languages(node_t *head)
{
	static char err_missing[] = "Missing definition for language 0x%x; MessageID %d, facility 0x%x, severity 0x%x";
	node_t *ndp;
	int nm = 0;
	msg_t *msg = NULL;

	for(ndp = head; ndp; ndp = ndp->next)
	{
		if(ndp->type != nd_msg)
			continue;
		if(!nm)
		{
			msg = ndp->u.msg;
		}
		else
		{
			int i;
			msg_t *m1;
			msg_t *m2;
			if(ndp->u.msg->nmsgs > msg->nmsgs)
			{
				m1 = ndp->u.msg;
				m2 = msg;
			}
			else
			{
				m1 = msg;
				m2 = ndp->u.msg;
			}

			for(i = 0; i < m1->nmsgs; i++)
			{
				if(i > m2->nmsgs)
					error(err_missing, m1->msgs[i]->lan, m2->id, m2->fac, m2->sev);
				else if(m1->msgs[i]->lan < m2->msgs[i]->lan)
					error(err_missing, m1->msgs[i]->lan, m2->id, m2->fac, m2->sev);
				else if(m1->msgs[i]->lan > m2->msgs[i]->lan)
					error(err_missing, m2->msgs[i]->lan, m1->id, m1->fac, m1->sev);
			}
		}
		nm++;
	}
	return nm;
}

#define MSGRID(x)	((*(msg_t **)(x))->realid)
static int sort_msg(const void *p1, const void *p2)
{
	return MSGRID(p1) > MSGRID(p2) ? 1 : (MSGRID(p1) == MSGRID(p2) ? 0 : -1);
	/* return (*(msg_t **)p1)->realid - (*(msg_t **)p1)->realid; */
}

/*
 * block_messages() basically transposes the messages
 * from ID/language based list to a language/ID
 * based list.
 */
static lan_blk_t *block_messages(node_t *head)
{
	lan_blk_t *lbp;
	lan_blk_t *lblktail = NULL;
	lan_blk_t *lblkhead = NULL;
	msg_t **msgtab = NULL;
	node_t *ndp;
	int nmsg = 0;
	int i;
	int nl;
	int factor = unicodeout ? 2 : 1;

	for(ndp = head; ndp; ndp = ndp->next)
	{
		if(ndp->type != nd_msg)
			continue;
		msgtab = xrealloc(msgtab, (nmsg+1) * sizeof(*msgtab));
		msgtab[nmsg++] = ndp->u.msg;
	}

	assert(nmsg != 0);
	qsort(msgtab, nmsg, sizeof(*msgtab), sort_msg);

	for(nl = 0; nl < msgtab[0]->nmsgs; nl++)	/* This should be equal for all after check_languages() */
	{
		lbp = xmalloc(sizeof(lan_blk_t));

		if(!lblktail)
		{
			lblkhead = lblktail = lbp;
		}
		else
		{
			lblktail->next = lbp;
			lbp->prev = lblktail;
			lblktail = lbp;
		}
		lbp->nblk = 1;
		lbp->blks = xmalloc(sizeof(*lbp->blks));
		lbp->blks[0].idlo = msgtab[0]->realid;
		lbp->blks[0].idhi = msgtab[0]->realid;
		/* The plus 4 is the entry header; (+3)&~3 is DWORD alignment */
		lbp->blks[0].size = ((factor * msgtab[0]->msgs[nl]->len + 3) & ~3) + 4;
		lbp->blks[0].msgs = xmalloc(sizeof(*lbp->blks[0].msgs));
		lbp->blks[0].nmsg = 1;
		lbp->blks[0].msgs[0] = msgtab[0]->msgs[nl];
		lbp->lan = msgtab[0]->msgs[nl]->lan;

		for(i = 1; i < nmsg; i++)
		{
			block_t *blk = &(lbp->blks[lbp->nblk-1]);
			if(msgtab[i]->realid == blk->idhi+1)
			{
				blk->size += ((factor * msgtab[i]->msgs[nl]->len + 3) & ~3) + 4;
				blk->idhi++;
				blk->msgs = xrealloc(blk->msgs, (blk->nmsg+1) * sizeof(*blk->msgs));
				blk->msgs[blk->nmsg++] = msgtab[i]->msgs[nl];
			}
			else
			{
				lbp->nblk++;
				lbp->blks = xrealloc(lbp->blks, lbp->nblk * sizeof(*lbp->blks));
				blk = &(lbp->blks[lbp->nblk-1]);
				blk->idlo = msgtab[i]->realid;
				blk->idhi = msgtab[i]->realid;
				blk->size = ((factor * msgtab[i]->msgs[nl]->len + 3) & ~3) + 4;
				blk->msgs = xmalloc(sizeof(*blk->msgs));
				blk->nmsg = 1;
				blk->msgs[0] = msgtab[i]->msgs[nl];
			}
		}
	}
	free(msgtab);
	return lblkhead;
}

static int sc_xlat(const void *p1, const void *p2)
{
	return ((cp_xlat_t *)p1)->lan - ((cp_xlat_t *)p2)->lan;
}

static void add_cpxlat(int lan, int cpin, int cpout)
{
	cpxlattab = xrealloc(cpxlattab, (ncpxlattab+1) * sizeof(*cpxlattab));
	cpxlattab[ncpxlattab].lan   = lan;
	cpxlattab[ncpxlattab].cpin  = cpin;
	cpxlattab[ncpxlattab].cpout = cpout;
	ncpxlattab++;
	qsort(cpxlattab, ncpxlattab, sizeof(*cpxlattab), sc_xlat);
}

static cp_xlat_t *find_cpxlat(int lan)
{
	cp_xlat_t t;

	if(!cpxlattab) return NULL;

	t.lan = lan;
	return (cp_xlat_t *)bsearch(&t, cpxlattab, ncpxlattab, sizeof(*cpxlattab), sc_xlat);
}

