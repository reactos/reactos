/* A Bison parser, made from ./parser.y
   by GNU bison 1.35.  */

#define YYBISON 1  /* Identify Bison output.  */

# define	aIDENTIFIER	257
# define	aKNOWNTYPE	258
# define	aNUM	259
# define	aHEXNUM	260
# define	aSTRING	261
# define	aUUID	262
# define	aEOF	263
# define	SHL	264
# define	SHR	265
# define	tAGGREGATABLE	266
# define	tALLOCATE	267
# define	tAPPOBJECT	268
# define	tARRAYS	269
# define	tASYNC	270
# define	tASYNCUUID	271
# define	tAUTOHANDLE	272
# define	tBINDABLE	273
# define	tBOOLEAN	274
# define	tBROADCAST	275
# define	tBYTE	276
# define	tBYTECOUNT	277
# define	tCALLAS	278
# define	tCALLBACK	279
# define	tCASE	280
# define	tCDECL	281
# define	tCHAR	282
# define	tCOCLASS	283
# define	tCODE	284
# define	tCOMMSTATUS	285
# define	tCONST	286
# define	tCONTEXTHANDLE	287
# define	tCONTEXTHANDLENOSERIALIZE	288
# define	tCONTEXTHANDLESERIALIZE	289
# define	tCONTROL	290
# define	tCPPQUOTE	291
# define	tDEFAULT	292
# define	tDEFAULTVALUE	293
# define	tDISPINTERFACE	294
# define	tDLLNAME	295
# define	tDOUBLE	296
# define	tDUAL	297
# define	tENDPOINT	298
# define	tENTRY	299
# define	tENUM	300
# define	tERRORSTATUST	301
# define	tEXPLICITHANDLE	302
# define	tEXTERN	303
# define	tFLOAT	304
# define	tHANDLE	305
# define	tHANDLET	306
# define	tHELPCONTEXT	307
# define	tHELPFILE	308
# define	tHELPSTRING	309
# define	tHELPSTRINGCONTEXT	310
# define	tHELPSTRINGDLL	311
# define	tHIDDEN	312
# define	tHYPER	313
# define	tID	314
# define	tIDEMPOTENT	315
# define	tIIDIS	316
# define	tIMPLICITHANDLE	317
# define	tIMPORT	318
# define	tIMPORTLIB	319
# define	tIN	320
# define	tINCLUDE	321
# define	tINLINE	322
# define	tINPUTSYNC	323
# define	tINT	324
# define	tINT64	325
# define	tINTERFACE	326
# define	tLENGTHIS	327
# define	tLIBRARY	328
# define	tLOCAL	329
# define	tLONG	330
# define	tMETHODS	331
# define	tMODULE	332
# define	tNONCREATABLE	333
# define	tOBJECT	334
# define	tODL	335
# define	tOLEAUTOMATION	336
# define	tOPTIONAL	337
# define	tOUT	338
# define	tPOINTERDEFAULT	339
# define	tPROPERTIES	340
# define	tPROPGET	341
# define	tPROPPUT	342
# define	tPROPPUTREF	343
# define	tPTR	344
# define	tPUBLIC	345
# define	tREADONLY	346
# define	tREF	347
# define	tRESTRICTED	348
# define	tRETVAL	349
# define	tSHORT	350
# define	tSIGNED	351
# define	tSINGLE	352
# define	tSIZEIS	353
# define	tSIZEOF	354
# define	tSMALL	355
# define	tSOURCE	356
# define	tSTDCALL	357
# define	tSTRING	358
# define	tSTRUCT	359
# define	tSWITCH	360
# define	tSWITCHIS	361
# define	tSWITCHTYPE	362
# define	tTRANSMITAS	363
# define	tTYPEDEF	364
# define	tUNION	365
# define	tUNIQUE	366
# define	tUNSIGNED	367
# define	tUUID	368
# define	tV1ENUM	369
# define	tVARARG	370
# define	tVERSION	371
# define	tVOID	372
# define	tWCHAR	373
# define	tWIREMARSHAL	374
# define	tPOINTERTYPE	375
# define	COND	376
# define	CAST	377
# define	PPTR	378
# define	NEG	379

#line 1 "./parser.y"

/*
 * IDL Compiler
 *
 * Copyright 2002 Ove Kaaven
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"
#include "typelib.h"

#if defined(YYBYACC)
	/* Berkeley yacc (byacc) doesn't seem to know about these */
	/* Some *BSD supplied versions do define these though */
# ifndef YYEMPTY
#  define YYEMPTY	(-1)	/* Empty lookahead value of yychar */
# endif
# ifndef YYLEX
#  define YYLEX		yylex()
# endif

#elif defined(YYBISON)
	/* Bison was used for original development */
	/* #define YYEMPTY -2 */
	/* #define YYLEX   yylex() */

#else
	/* No yacc we know yet */
# if !defined(YYEMPTY) || !defined(YYLEX)
#  error Yacc version/type unknown. This version needs to be verified for settings of YYEMPTY and YYLEX.
# elif defined(__GNUC__)	/* gcc defines the #warning directive */
#  warning Yacc version/type unknown. It defines YYEMPTY and YYLEX, but is not tested
  /* #else we just take a chance that it works... */
# endif
#endif

static attr_t *make_attr(enum attr_type type);
static attr_t *make_attrv(enum attr_type type, unsigned long val);
static attr_t *make_attrp(enum attr_type type, void *val);
static expr_t *make_expr(enum expr_type type);
static expr_t *make_exprl(enum expr_type type, long val);
static expr_t *make_exprs(enum expr_type type, char *val);
static expr_t *make_exprt(enum expr_type type, typeref_t *tref, expr_t *expr);
static expr_t *make_expr1(enum expr_type type, expr_t *expr);
static expr_t *make_expr2(enum expr_type type, expr_t *exp1, expr_t *exp2);
static expr_t *make_expr3(enum expr_type type, expr_t *expr1, expr_t *expr2, expr_t *expr3);
static type_t *make_type(unsigned char type, type_t *ref);
static typeref_t *make_tref(char *name, type_t *ref);
static typeref_t *uniq_tref(typeref_t *ref);
static type_t *type_ref(typeref_t *ref);
static void set_type(var_t *v, typeref_t *ref, expr_t *arr);
static ifref_t *make_ifref(type_t *iface);
static var_t *make_var(char *name);
static func_t *make_func(var_t *def, var_t *args);
static class_t *make_class(char *name);

static type_t *reg_type(type_t *type, char *name, int t);
static type_t *reg_types(type_t *type, var_t *names, int t);
static type_t *find_type(char *name, int t);
static type_t *find_type2(char *name, int t);
static type_t *get_type(unsigned char type, char *name, int t);
static type_t *get_typev(unsigned char type, var_t *name, int t);
static int get_struct_type(var_t *fields);

static var_t *reg_const(var_t *var);
static var_t *find_const(char *name, int f);

#define tsENUM   1
#define tsSTRUCT 2
#define tsUNION  3

static type_t std_bool = { "boolean" };
static type_t std_int = { "int" };
static type_t std_int64 = { "__int64" };
static type_t std_uhyper = { "MIDL_uhyper" };


#line 106 "./parser.y"
#ifndef YYSTYPE
typedef union {
	attr_t *attr;
	expr_t *expr;
	type_t *type;
	typeref_t *tref;
	var_t *var;
	func_t *func;
	ifref_t *ifref;
	class_t *clas;
	char *str;
	UUID *uuid;
	unsigned int num;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
#ifndef YYDEBUG
# define YYDEBUG 1
#endif



#define	YYFINAL		464
#define	YYFLAG		-32768
#define	YYNTBASE	145

/* YYTRANSLATE(YYLEX) -- Bison token number corresponding to YYLEX. */
#define YYTRANSLATE(x) ((unsigned)(x) <= 379 ? yytranslate[x] : 217)

/* YYTRANSLATE[YYLEX] -- Bison token number corresponding to YYLEX. */
static const short yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   125,     2,
     135,   136,   128,   127,   122,   126,   144,   129,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   141,   134,
       2,   142,     2,   143,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   139,     2,   140,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   137,   124,   138,   130,     2,     2,     2,
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
     116,   117,   118,   119,   120,   121,   123,   131,   132,   133
};

#if YYDEBUG
static const short yyprhs[] =
{
       0,     0,     2,     3,     6,     9,    12,    15,    18,    21,
      22,    25,    28,    31,    34,    37,    38,    42,    45,    47,
      50,    52,    55,    58,    60,    63,    66,    69,    74,    78,
      82,    85,    89,    93,    94,    96,    98,   100,   104,   106,
     111,   115,   122,   128,   129,   133,   137,   139,   143,   148,
     149,   151,   155,   157,   161,   166,   168,   170,   175,   180,
     182,   184,   186,   188,   190,   195,   200,   205,   207,   212,
     217,   222,   224,   226,   231,   236,   241,   246,   251,   253,
     258,   260,   265,   271,   273,   275,   280,   282,   284,   286,
     288,   290,   292,   294,   299,   301,   303,   305,   307,   309,
     311,   313,   315,   317,   322,   324,   326,   331,   336,   341,
     343,   348,   350,   352,   357,   362,   364,   365,   367,   368,
     371,   376,   380,   386,   387,   390,   392,   394,   398,   402,
     404,   410,   412,   416,   417,   419,   421,   423,   425,   431,
     435,   439,   443,   447,   451,   455,   459,   463,   466,   469,
     472,   477,   482,   486,   488,   492,   494,   499,   500,   503,
     506,   510,   513,   515,   520,   528,   529,   531,   532,   534,
     536,   538,   540,   542,   544,   546,   548,   550,   552,   554,
     557,   560,   562,   564,   566,   568,   570,   572,   574,   575,
     577,   579,   582,   585,   588,   591,   593,   595,   598,   601,
     604,   609,   610,   613,   616,   619,   622,   625,   628,   632,
     635,   639,   645,   646,   649,   652,   655,   658,   664,   672,
     674,   677,   680,   683,   686,   689,   694,   697,   700,   702,
     704,   708,   710,   714,   716,   718,   724,   726,   728,   730,
     733,   735,   738,   740,   743,   745,   748,   753,   759,   770,
     772
};
static const short yyrhs[] =
{
     146,     0,     0,   146,   204,     0,   146,   203,     0,   146,
     192,     0,   146,   207,     0,   146,   155,     0,   146,   149,
       0,     0,   147,   204,     0,   147,   203,     0,   147,   192,
       0,   147,   207,     0,   147,   149,     0,     0,   148,   183,
     134,     0,   148,   149,     0,   134,     0,   169,   134,     0,
     150,     0,   173,   134,     0,   179,   134,     0,   152,     0,
     212,   134,     0,   214,   134,     0,   215,   134,     0,    37,
     135,     7,   136,     0,    64,     7,   134,     0,   151,   147,
       9,     0,    74,     3,     0,   163,   153,   137,     0,   154,
     147,   138,     0,     0,   158,     0,   118,     0,   159,     0,
     158,   122,   159,     0,   157,     0,   163,   213,   209,   160,
       0,   213,   209,   160,     0,   163,   213,   209,   135,   156,
     136,     0,   213,   209,   135,   156,   136,     0,     0,   139,
     161,   140,     0,   139,   128,   140,     0,   175,     0,   161,
     122,   176,     0,   161,   140,   139,   176,     0,     0,   163,
       0,   139,   164,   140,     0,   165,     0,   164,   122,   165,
       0,   164,   140,   139,   165,     0,    16,     0,    18,     0,
      24,   135,   186,   136,     0,    26,   135,   177,   136,     0,
      33,     0,    34,     0,    35,     0,    36,     0,    38,     0,
      39,   135,   178,   136,     0,    39,   135,     7,   136,     0,
      41,   135,     7,   136,     0,    43,     0,    44,   135,     7,
     136,     0,    45,   135,     7,   136,     0,    45,   135,   178,
     136,     0,    48,     0,    51,     0,    53,   135,   178,   136,
       0,    54,   135,     7,   136,     0,    55,   135,     7,   136,
       0,    56,   135,   178,   136,     0,    57,   135,     7,   136,
       0,    58,     0,    60,   135,   178,   136,     0,    61,     0,
      62,   135,   186,   136,     0,    63,   135,    52,     3,   136,
       0,    66,     0,    69,     0,    73,   135,   174,   136,     0,
      75,     0,    79,     0,    80,     0,    81,     0,    82,     0,
      83,     0,    84,     0,    85,   135,   211,   136,     0,    87,
       0,    88,     0,    89,     0,    90,     0,    91,     0,    92,
       0,    93,     0,    94,     0,    95,     0,    99,   135,   174,
     136,     0,   102,     0,   104,     0,   107,   135,   176,   136,
       0,   108,   135,   213,   136,     0,   109,   135,   213,   136,
       0,   112,     0,   114,   135,     8,   136,     0,   115,     0,
     116,     0,   117,   135,   216,   136,     0,   120,   135,   213,
     136,     0,   211,     0,     0,   103,     0,     0,   167,   168,
       0,    26,   176,   141,   181,     0,    38,   141,   181,     0,
      32,   213,   186,   142,   178,     0,     0,   171,   122,     0,
     171,     0,   172,     0,   171,   122,   172,     0,   186,   142,
     178,     0,   186,     0,    46,   185,   137,   170,   138,     0,
     175,     0,   174,   122,   175,     0,     0,   176,     0,     5,
       0,     6,     0,     3,     0,   176,   143,   176,   141,   176,
       0,   176,   124,   176,     0,   176,   125,   176,     0,   176,
     127,   176,     0,   176,   126,   176,     0,   176,   128,   176,
       0,   176,   129,   176,     0,   176,    10,   176,     0,   176,
      11,   176,     0,   130,   176,     0,   126,   176,     0,   128,
     176,     0,   135,   213,   136,   176,     0,   100,   135,   213,
     136,     0,   135,   176,   136,     0,   178,     0,   177,   122,
     178,     0,   176,     0,    49,    32,   213,   186,     0,     0,
     180,   181,     0,   182,   134,     0,   162,   215,   134,     0,
     163,   134,     0,   134,     0,   162,   213,   209,   160,     0,
     162,   213,   166,   209,   135,   156,   136,     0,     0,   186,
       0,     0,     3,     0,     4,     0,     3,     0,     4,     0,
      16,     0,    60,     0,    95,     0,   117,     0,    22,     0,
     119,     0,   189,     0,    97,   189,     0,   113,   189,     0,
     113,     0,    50,     0,    98,     0,    42,     0,    20,     0,
      47,     0,    52,     0,     0,    70,     0,    70,     0,   101,
     188,     0,    96,   188,     0,    76,   188,     0,    59,   188,
       0,    71,     0,    28,     0,    29,     3,     0,    29,     4,
       0,   163,   190,     0,   191,   137,   193,   138,     0,     0,
     193,   194,     0,   162,   204,     0,    40,     3,     0,    40,
       4,     0,   163,   195,     0,    86,   141,     0,   197,   182,
     134,     0,    77,   141,     0,   198,   183,   134,     0,   196,
     137,   197,   198,   138,     0,     0,   141,     4,     0,    72,
       3,     0,    72,     4,     0,   163,   201,     0,   202,   200,
     137,   148,   138,     0,   202,   141,     3,   137,   152,   148,
     138,     0,   199,     0,   201,   134,     0,   195,   134,     0,
      78,     3,     0,    78,     4,     0,   163,   205,     0,   206,
     137,   148,   138,     0,   128,   209,     0,    32,   208,     0,
     186,     0,   208,     0,   135,   209,   136,     0,   209,     0,
     210,   122,   209,     0,    93,     0,   112,     0,   105,   185,
     137,   180,   138,     0,   118,     0,     4,     0,   187,     0,
      32,   213,     0,   173,     0,    46,     3,     0,   212,     0,
     105,     3,     0,   215,     0,   111,     3,     0,   110,   162,
     213,   210,     0,   111,   185,   137,   180,   138,     0,   111,
     185,   106,   135,   182,   136,   184,   137,   167,   138,     0,
       5,     0,     5,   144,     5,     0
};

#endif

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined. */
static const short yyrline[] =
{
       0,   228,   231,   232,   233,   234,   235,   236,   237,   240,
     241,   242,   243,   244,   245,   248,   249,   250,   253,   254,
     255,   256,   257,   258,   259,   260,   261,   264,   266,   269,
     272,   274,   276,   279,   280,   283,   286,   287,   288,   292,
     296,   299,   305,   312,   313,   314,   317,   318,   319,   322,
     323,   326,   330,   331,   332,   335,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,   356,   357,   358,   359,   360,
     361,   362,   363,   364,   365,   366,   367,   368,   369,   370,
     371,   372,   373,   374,   375,   376,   377,   378,   379,   380,
     381,   382,   383,   384,   385,   386,   387,   388,   389,   390,
     391,   392,   393,   394,   395,   396,   399,   400,   403,   404,
     409,   413,   419,   426,   427,   428,   431,   432,   438,   443,
     449,   457,   458,   471,   472,   475,   476,   477,   478,   479,
     480,   481,   482,   483,   484,   485,   486,   487,   488,   489,
     490,   491,   492,   495,   496,   499,   505,   510,   511,   516,
     517,   518,   519,   522,   525,   536,   537,   540,   541,   542,
     545,   547,   548,   549,   550,   551,   554,   555,   556,   557,
     558,   570,   571,   572,   573,   574,   575,   576,   579,   580,
     583,   584,   585,   586,   587,   588,   589,   592,   593,   596,
     603,   608,   609,   612,   616,   617,   620,   632,   633,   636,
     637,   640,   655,   656,   659,   660,   663,   671,   679,   686,
     689,   691,   694,   695,   698,   703,   709,   710,   713,   714,
     715,   718,   720,   723,   725,   728,   738,   739,   740,   741,
     742,   743,   744,   745,   746,   747,   750,   763,   767,   780,
     782
};
#endif


#if (YYDEBUG) || defined YYERROR_VERBOSE

/* YYTNAME[TOKEN_NUM] -- String name of the token TOKEN_NUM. */
static const char *const yytname[] =
{
  "$", "error", "$undefined.", "aIDENTIFIER", "aKNOWNTYPE", "aNUM", 
  "aHEXNUM", "aSTRING", "aUUID", "aEOF", "SHL", "SHR", "tAGGREGATABLE", 
  "tALLOCATE", "tAPPOBJECT", "tARRAYS", "tASYNC", "tASYNCUUID", 
  "tAUTOHANDLE", "tBINDABLE", "tBOOLEAN", "tBROADCAST", "tBYTE", 
  "tBYTECOUNT", "tCALLAS", "tCALLBACK", "tCASE", "tCDECL", "tCHAR", 
  "tCOCLASS", "tCODE", "tCOMMSTATUS", "tCONST", "tCONTEXTHANDLE", 
  "tCONTEXTHANDLENOSERIALIZE", "tCONTEXTHANDLESERIALIZE", "tCONTROL", 
  "tCPPQUOTE", "tDEFAULT", "tDEFAULTVALUE", "tDISPINTERFACE", "tDLLNAME", 
  "tDOUBLE", "tDUAL", "tENDPOINT", "tENTRY", "tENUM", "tERRORSTATUST", 
  "tEXPLICITHANDLE", "tEXTERN", "tFLOAT", "tHANDLE", "tHANDLET", 
  "tHELPCONTEXT", "tHELPFILE", "tHELPSTRING", "tHELPSTRINGCONTEXT", 
  "tHELPSTRINGDLL", "tHIDDEN", "tHYPER", "tID", "tIDEMPOTENT", "tIIDIS", 
  "tIMPLICITHANDLE", "tIMPORT", "tIMPORTLIB", "tIN", "tINCLUDE", 
  "tINLINE", "tINPUTSYNC", "tINT", "tINT64", "tINTERFACE", "tLENGTHIS", 
  "tLIBRARY", "tLOCAL", "tLONG", "tMETHODS", "tMODULE", "tNONCREATABLE", 
  "tOBJECT", "tODL", "tOLEAUTOMATION", "tOPTIONAL", "tOUT", 
  "tPOINTERDEFAULT", "tPROPERTIES", "tPROPGET", "tPROPPUT", "tPROPPUTREF", 
  "tPTR", "tPUBLIC", "tREADONLY", "tREF", "tRESTRICTED", "tRETVAL", 
  "tSHORT", "tSIGNED", "tSINGLE", "tSIZEIS", "tSIZEOF", "tSMALL", 
  "tSOURCE", "tSTDCALL", "tSTRING", "tSTRUCT", "tSWITCH", "tSWITCHIS", 
  "tSWITCHTYPE", "tTRANSMITAS", "tTYPEDEF", "tUNION", "tUNIQUE", 
  "tUNSIGNED", "tUUID", "tV1ENUM", "tVARARG", "tVERSION", "tVOID", 
  "tWCHAR", "tWIREMARSHAL", "tPOINTERTYPE", "','", "COND", "'|'", "'&'", 
  "'-'", "'+'", "'*'", "'/'", "'~'", "CAST", "PPTR", "NEG", "';'", "'('", 
  "')'", "'{'", "'}'", "'['", "']'", "':'", "'='", "'?'", "'.'", "input", 
  "gbl_statements", "imp_statements", "int_statements", "statement", 
  "cppquote", "import_start", "import", "libraryhdr", "library_start", 
  "librarydef", "m_args", "no_args", "args", "arg", "array", "array_list", 
  "m_attributes", "attributes", "attrib_list", "attribute", "callconv", 
  "cases", "case", "constdef", "enums", "enum_list", "enum", "enumdef", 
  "m_exprs", "m_expr", "expr", "expr_list_const", "expr_const", 
  "externdef", "fields", "field", "s_field", "funcdef", "m_ident", 
  "t_ident", "ident", "base_type", "m_int", "int_std", "coclass", 
  "coclasshdr", "coclassdef", "coclass_ints", "coclass_int", 
  "dispinterface", "dispinterfacehdr", "dispint_props", "dispint_meths", 
  "dispinterfacedef", "inherit", "interface", "interfacehdr", 
  "interfacedef", "interfacedec", "module", "modulehdr", "moduledef", 
  "p_ident", "pident", "pident_list", "pointer_type", "structdef", "type", 
  "typedef", "uniondef", "version", 0
};
#endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives. */
static const short yyr1[] =
{
       0,   145,   146,   146,   146,   146,   146,   146,   146,   147,
     147,   147,   147,   147,   147,   148,   148,   148,   149,   149,
     149,   149,   149,   149,   149,   149,   149,   150,   151,   152,
     153,   154,   155,   156,   156,   157,   158,   158,   158,   159,
     159,   159,   159,   160,   160,   160,   161,   161,   161,   162,
     162,   163,   164,   164,   164,   165,   165,   165,   165,   165,
     165,   165,   165,   165,   165,   165,   165,   165,   165,   165,
     165,   165,   165,   165,   165,   165,   165,   165,   165,   165,
     165,   165,   165,   165,   165,   165,   165,   165,   165,   165,
     165,   165,   165,   165,   165,   165,   165,   165,   165,   165,
     165,   165,   165,   165,   165,   165,   165,   165,   165,   165,
     165,   165,   165,   165,   165,   165,   166,   166,   167,   167,
     168,   168,   169,   170,   170,   170,   171,   171,   172,   172,
     173,   174,   174,   175,   175,   176,   176,   176,   176,   176,
     176,   176,   176,   176,   176,   176,   176,   176,   176,   176,
     176,   176,   176,   177,   177,   178,   179,   180,   180,   181,
     181,   181,   181,   182,   183,   184,   184,   185,   185,   185,
     186,   186,   186,   186,   186,   186,   187,   187,   187,   187,
     187,   187,   187,   187,   187,   187,   187,   187,   188,   188,
     189,   189,   189,   189,   189,   189,   189,   190,   190,   191,
     192,   193,   193,   194,   195,   195,   196,   197,   197,   198,
     198,   199,   200,   200,   201,   201,   202,   203,   203,   203,
     204,   204,   205,   205,   206,   207,   208,   208,   209,   209,
     209,   210,   210,   211,   211,   212,   213,   213,   213,   213,
     213,   213,   213,   213,   213,   213,   214,   215,   215,   216,
     216
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN. */
static const short yyr2[] =
{
       0,     1,     0,     2,     2,     2,     2,     2,     2,     0,
       2,     2,     2,     2,     2,     0,     3,     2,     1,     2,
       1,     2,     2,     1,     2,     2,     2,     4,     3,     3,
       2,     3,     3,     0,     1,     1,     1,     3,     1,     4,
       3,     6,     5,     0,     3,     3,     1,     3,     4,     0,
       1,     3,     1,     3,     4,     1,     1,     4,     4,     1,
       1,     1,     1,     1,     4,     4,     4,     1,     4,     4,
       4,     1,     1,     4,     4,     4,     4,     4,     1,     4,
       1,     4,     5,     1,     1,     4,     1,     1,     1,     1,
       1,     1,     1,     4,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     4,     1,     1,     4,     4,     4,     1,
       4,     1,     1,     4,     4,     1,     0,     1,     0,     2,
       4,     3,     5,     0,     2,     1,     1,     3,     3,     1,
       5,     1,     3,     0,     1,     1,     1,     1,     5,     3,
       3,     3,     3,     3,     3,     3,     3,     2,     2,     2,
       4,     4,     3,     1,     3,     1,     4,     0,     2,     2,
       3,     2,     1,     4,     7,     0,     1,     0,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       2,     1,     1,     1,     1,     1,     1,     1,     0,     1,
       1,     2,     2,     2,     2,     1,     1,     2,     2,     2,
       4,     0,     2,     2,     2,     2,     2,     2,     3,     2,
       3,     5,     0,     2,     2,     2,     2,     5,     7,     1,
       2,     2,     2,     2,     2,     4,     2,     2,     1,     1,
       3,     1,     3,     1,     1,     5,     1,     1,     1,     2,
       1,     2,     1,     2,     1,     2,     4,     5,    10,     1,
       3
};

/* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
   doesn't specify something else to do.  Zero means the default is an
   error. */
static const short yydefact[] =
{
       2,     1,     0,     0,     0,   167,     0,     0,     0,   167,
      49,   167,    18,     0,     8,    20,     9,    23,     9,     7,
       0,     0,     0,     0,     0,     5,     0,     0,   219,     0,
     212,     4,     3,     0,     6,     0,     0,     0,   237,   185,
     176,   196,     0,   184,   167,   186,   182,   187,   188,   190,
     195,   188,   188,     0,   183,   188,   167,   167,   181,   236,
     177,   240,   238,   178,   242,     0,   244,     0,   204,   205,
     168,   169,     0,     0,     0,   214,   215,     0,     0,    50,
       0,    55,    56,     0,     0,    59,    60,    61,    62,    63,
       0,     0,    67,     0,     0,    71,    72,     0,     0,     0,
       0,     0,    78,     0,    80,     0,     0,    83,    84,     0,
      86,    87,    88,    89,    90,    91,    92,     0,    94,    95,
      96,    97,    98,    99,   100,   101,   102,     0,   104,   105,
       0,     0,     0,   109,     0,   111,   112,     0,     0,     0,
      52,   115,     0,     0,     0,     0,     0,     0,   199,   206,
     216,   224,    19,    21,    22,   201,   221,     0,   220,     0,
       0,    15,    24,    25,    26,   239,   241,   189,   194,   193,
     192,   179,   191,   243,   245,   180,   170,   171,   172,   173,
     174,   175,     0,     0,   123,     0,    28,   157,     0,     0,
     157,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   133,     0,   133,     0,     0,
       0,     0,     0,     0,     0,    51,    29,    14,     0,    12,
      11,    10,    13,    32,   197,   198,    30,   222,   223,    31,
      49,     0,    49,     0,   213,    15,    49,     0,    27,     0,
     125,   126,   129,   156,    49,     0,     0,     0,   228,   229,
     231,   246,    49,    49,     0,   137,   135,   136,     0,     0,
       0,     0,     0,   155,     0,   153,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   131,   134,   233,   234,     0,     0,     0,     0,     0,
       0,   249,     0,     0,    53,     0,   200,     0,   202,   207,
       0,     0,     0,    49,     0,    49,   225,    17,     0,     0,
     122,   130,   124,     0,   162,   235,     0,    50,   158,     0,
     227,   226,     0,     0,     0,   247,    57,     0,   148,   149,
     147,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    58,    65,    64,    66,    68,    69,    70,
      73,    74,    75,    76,    77,    79,    81,     0,   133,    85,
      93,   103,   106,   107,   108,   110,     0,   113,   114,    54,
     203,   209,     0,   208,   211,     0,    15,   217,   116,    16,
     127,   128,   244,   161,   159,   230,   232,   165,     0,   152,
       0,   145,   146,   139,   140,   142,   141,   143,   144,     0,
     154,    82,   132,   250,    43,   210,    49,   117,     0,   160,
       0,   166,   151,   150,     0,   133,   163,   218,     0,   118,
     138,     0,     0,    46,    33,     0,    45,     0,    44,   236,
       0,    38,    34,    36,     0,     0,     0,     0,   248,   119,
      47,     0,   164,     0,     0,    43,     0,    49,    48,    37,
      43,    33,    40,    49,   121,    33,    39,     0,   120,     0,
      42,    41,     0,     0,     0
};

static const short yydefgoto[] =
{
     462,     1,   142,   236,   307,    15,    16,    17,   147,    18,
      19,   430,   431,   432,   433,   416,   422,   308,    79,   139,
     140,   408,   425,   439,    21,   239,   240,   241,    61,   280,
     281,   263,   264,   265,    23,   244,   318,   319,   309,   410,
      72,   248,    62,   168,    63,   148,    24,   219,   230,   298,
      26,    27,   232,   303,    28,   160,    29,    30,   220,   221,
     151,    33,   222,   249,   250,   251,   141,    64,   435,    36,
      66,   292
};

static const short yypact[] =
{
  -32768,   686,   644,   -99,    30,    93,    13,    51,   119,    93,
     -32,    93,-32768,   814,-32768,-32768,-32768,-32768,-32768,-32768,
      11,    -6,     6,     9,   -25,-32768,    34,     7,-32768,    36,
      43,-32768,-32768,    14,-32768,    52,    54,    56,-32768,-32768,
  -32768,-32768,   644,-32768,   146,-32768,-32768,-32768,     2,-32768,
  -32768,     2,     2,    35,-32768,     2,   197,   199,    35,-32768,
  -32768,-32768,-32768,-32768,-32768,    21,-32768,   186,-32768,-32768,
  -32768,-32768,    61,   644,    70,-32768,-32768,    69,   644,-32768,
     -76,-32768,-32768,    74,    80,-32768,-32768,-32768,-32768,-32768,
      88,    99,-32768,   107,   116,-32768,-32768,   120,   122,   124,
     130,   132,-32768,   134,-32768,   135,   136,-32768,-32768,   137,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,   138,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,   139,-32768,-32768,
     141,   142,   150,-32768,   152,-32768,-32768,   154,   155,   -81,
  -32768,-32768,   568,   655,   209,   224,   222,   144,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,   205,-32768,   234,
     157,-32768,-32768,-32768,-32768,-32768,   160,-32768,-32768,-32768,
  -32768,-32768,-32768,   160,   -71,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,   156,   163,    21,    21,-32768,-32768,   327,   165,
  -32768,    21,   423,   469,   285,   296,   527,   423,   297,   298,
     423,   306,   423,    21,   262,   423,   -62,   423,   423,   644,
     644,   308,   313,   644,   814,   181,-32768,-32768,    46,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
     111,   180,   -59,   185,-32768,-32768,   247,   423,-32768,   187,
     202,-32768,   184,-32768,   -47,    -9,   327,   327,-32768,-32768,
  -32768,   207,   -32,   -30,   191,-32768,-32768,-32768,   200,   423,
     423,   423,   515,    38,   -69,-32768,   203,   204,   206,   208,
     210,   212,   214,   215,   217,   220,   225,   226,   227,   333,
     -67,-32768,    38,-32768,-32768,   229,   -65,    92,   230,   233,
     235,   194,   240,   241,-32768,   814,-32768,   -18,-32768,-32768,
     219,   644,   211,   123,   314,   697,-32768,-32768,   644,   238,
  -32768,-32768,    21,   423,-32768,-32768,   644,   245,-32768,   249,
  -32768,-32768,   248,   327,   253,-32768,-32768,   644,   252,   252,
     252,   357,   254,   423,   423,   423,   423,   423,   423,   423,
     423,   423,   423,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,   255,   423,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,   365,-32768,-32768,-32768,
  -32768,-32768,   327,-32768,-32768,   263,-32768,-32768,   293,-32768,
  -32768,-32768,   266,-32768,-32768,-32768,-32768,    21,   265,-32768,
     423,   252,   252,   388,   104,    28,    28,    17,    17,   363,
  -32768,-32768,-32768,-32768,   264,-32768,   707,-32768,   327,-32768,
     267,-32768,-32768,   252,   423,   535,-32768,-32768,   270,-32768,
      38,    59,   -80,-32768,   236,   -12,-32768,   423,   269,   -54,
     277,-32768,   292,-32768,   644,   327,   423,   274,-32768,-32768,
      38,   423,-32768,   360,   327,   -22,   383,   -61,    38,-32768,
      -5,   236,-32768,   -61,-32768,   236,-32768,   281,-32768,   282,
  -32768,-32768,   420,   421,-32768
};

static const short yypgoto[] =
{
  -32768,-32768,   406,  -218,    12,-32768,-32768,   121,-32768,-32768,
  -32768,  -326,-32768,-32768,   -16,  -357,-32768,    -8,    -1,-32768,
    -185,-32768,-32768,-32768,-32768,-32768,-32768,   125,     5,   231,
    -338,  -161,-32768,  -181,-32768,   242,  -377,  -200,   131,-32768,
     126,   -64,-32768,   140,    37,-32768,-32768,   432,-32768,-32768,
     -11,-32768,-32768,-32768,-32768,-32768,   -10,-32768,   439,     4,
  -32768,-32768,   440,   198,  -239,-32768,   243,    10,     1,-32768,
       3,-32768
};


#define	YYLAST		934


static const short yytable[] =
{
      20,   182,    78,    65,    37,    32,    22,   321,   322,   149,
     150,    35,   267,    14,   436,   271,   272,   305,   300,   275,
     402,   277,     4,   245,   176,   177,   437,   333,   334,   294,
     189,   283,   302,    68,    69,  -168,    67,   178,   333,   334,
     144,   214,   427,   165,   282,    73,   282,   287,   333,   334,
     284,     4,   324,   342,     8,   358,   310,   358,    74,   215,
     428,   190,   255,    41,   256,   257,  -168,   343,   -35,   359,
     454,   361,   167,   314,   185,   144,   458,   423,    13,   188,
      13,   179,   -35,     8,   386,   145,     4,   314,   452,   146,
     171,   315,    13,   456,    48,   175,    70,    71,   328,   329,
     330,   331,   333,   334,   314,    49,    50,    13,   325,    13,
     369,    51,   155,   451,   333,   334,   180,   415,     8,   246,
     242,   243,    75,    76,   146,   457,   438,   254,   152,   459,
     455,    52,   381,   404,   415,    77,    55,    80,   181,   278,
     153,   218,   218,   154,   157,    37,    37,    22,    22,   166,
      71,   161,    35,    35,   217,   217,   339,   340,   406,   258,
     341,   400,   335,   336,   337,   338,   339,   340,   156,   418,
     158,   341,   391,   392,   393,   394,   395,   396,   397,   398,
     399,   341,    77,    80,   159,   259,   162,   260,   163,   261,
     164,   169,   170,   183,   262,   172,   445,   282,   184,   426,
     173,    71,   174,    71,   186,   450,   187,   149,   150,   191,
     288,   289,   224,   225,   293,   192,   335,   336,   337,   338,
     339,   340,   297,   193,   301,   227,   228,   226,   362,   413,
     337,   338,   339,   340,   194,   341,   316,   233,   234,    37,
      38,    22,   195,   317,   301,   316,    35,   341,   242,   296,
      13,   196,   317,   420,   282,   197,    39,   198,    40,   199,
     329,   374,    13,   332,    41,   200,   440,   201,    42,   202,
     203,   204,   205,   206,   207,   446,   208,   209,    43,     2,
     448,   229,    44,    45,     3,   210,    46,   211,    47,   212,
     213,   231,   268,     5,   235,    48,     6,  -168,   237,   238,
     252,   370,   372,   269,   273,   274,    49,    50,    37,   378,
      22,     7,    51,   276,   279,    35,   290,   372,   291,   382,
     295,   299,   304,   411,   312,   311,   313,   326,   388,   323,
     176,   177,    52,    53,    54,   327,   357,    55,   366,   344,
     345,    56,   346,   178,   347,   373,   348,    57,   349,    58,
     350,   351,     9,   352,   429,    60,   353,    10,    11,   245,
     371,   354,   355,   356,    38,   360,   363,   333,   334,   364,
     403,   365,   379,   333,   334,    13,   367,   368,     7,   383,
      39,    12,    40,   384,   385,   306,    13,   179,    41,   387,
     390,   401,    42,   333,   334,   341,   407,   405,   333,   334,
     409,   412,    43,   415,   419,   424,    44,    45,   441,    37,
      46,    22,    47,   442,   443,   447,    35,   460,   461,    48,
     463,   464,   180,   434,   143,   376,   255,   449,   256,   257,
      49,    50,   253,    25,   375,   444,    51,   380,   286,   316,
      31,    34,   434,   320,   181,   316,   317,     0,     0,   285,
     434,     0,   317,     0,   434,   246,    52,    53,    54,     0,
       0,    55,   247,     0,     0,    56,     0,     0,     0,     0,
       0,    57,   255,    58,   256,   257,   266,     0,    59,    60,
       0,   335,   336,   337,   338,   339,   340,   335,   336,   337,
     338,   339,   340,   389,     0,     0,     0,     0,     0,    13,
     341,     0,     0,     0,   414,     0,   341,   335,   336,   337,
     338,   339,   340,   336,   337,   338,   339,   340,   255,    38,
     256,   257,     0,   258,   453,     0,   341,     0,     0,     0,
     255,   341,   256,   257,   270,    39,     0,    40,   255,     0,
     256,   257,     0,    41,     0,     0,     0,    42,     0,   259,
       0,   260,     0,   261,     0,     0,     0,    43,   262,     0,
       0,    44,    45,     0,     0,    46,     0,    47,     0,   258,
       0,     0,     0,     0,    48,     0,     0,   216,     0,     0,
       0,     0,     0,     0,     0,    49,    50,     0,     0,     0,
       0,    51,     0,     0,     0,   259,     0,   260,     0,   261,
       2,     0,     0,     0,   262,     3,     0,     0,     4,     0,
       0,    52,    53,    54,     5,   258,    55,     6,     0,     0,
      56,     0,     0,     0,     0,     0,    57,   258,    58,     0,
       0,     0,     7,    59,    60,   258,     0,     0,     0,     0,
       8,   259,     0,   260,     0,   261,     0,     0,    38,     0,
     262,     0,     0,   259,     0,   260,     0,   261,     0,     0,
       0,   259,   262,   421,    39,   261,    40,     0,     0,     0,
     262,     0,    41,     9,     0,     0,    42,     0,    10,    11,
       0,     0,     0,     0,     0,     0,    43,     2,     0,     0,
      44,    45,     3,     0,    46,     4,    47,     0,     0,     0,
       0,     5,    12,    48,     6,     0,     0,    13,     0,     0,
       0,     0,     0,     0,    49,    50,     0,     0,     2,     7,
      51,     0,     0,     3,     0,     0,     4,     8,     0,     2,
       0,     0,     5,     0,     3,     6,     0,     0,     0,     2,
      52,    53,    54,     5,     3,    55,     6,     0,     0,    56,
       7,     0,     0,     5,     0,    57,     6,    58,     8,     0,
       9,     7,    59,    60,     0,    10,    11,     0,     0,     0,
       0,     7,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    12,
       0,     9,     0,   223,    13,     0,    10,    11,     0,     0,
       0,     0,     9,     0,     0,     0,     0,    10,    11,     0,
       0,     0,     9,     0,     0,     0,     0,    10,    11,     0,
      12,     0,     0,     0,     0,    13,     0,     0,     0,     0,
      81,    12,    82,     0,     0,   377,    13,     0,    83,     0,
      84,    12,     0,     0,     0,   417,    13,    85,    86,    87,
      88,     0,    89,    90,     0,    91,     0,    92,    93,    94,
       0,     0,    95,     0,     0,    96,     0,    97,    98,    99,
     100,   101,   102,     0,   103,   104,   105,   106,     0,     0,
     107,     0,     0,   108,     0,     0,     0,   109,     0,   110,
       0,     0,     0,   111,   112,   113,   114,   115,   116,   117,
       0,   118,   119,   120,   121,   122,   123,   124,   125,   126,
       0,     0,     0,   127,     0,     0,   128,     0,   129,     0,
       0,   130,   131,   132,     0,     0,   133,     0,   134,   135,
     136,   137,     0,     0,   138
};

static const short yycheck[] =
{
       1,    65,    10,     2,     1,     1,     1,   246,   247,    20,
      20,     1,   193,     1,    26,   196,   197,   235,    77,   200,
     358,   202,    40,    32,     3,     4,    38,    10,    11,   214,
     106,    93,   232,     3,     4,   106,   135,    16,    10,    11,
      29,   122,   122,    42,   205,    32,   207,   208,    10,    11,
     112,    40,   252,   122,    72,   122,   237,   122,     7,   140,
     140,   137,     3,    28,     5,     6,   137,   136,   122,   136,
     447,   136,    70,   134,    73,    29,   453,   415,   139,    78,
     139,    60,   136,    72,   323,    74,    40,   134,   445,    78,
      53,   138,   139,   450,    59,    58,     3,     4,   259,   260,
     261,   262,    10,    11,   134,    70,    71,   139,   138,   139,
     295,    76,   137,   135,    10,    11,    95,   139,    72,   128,
     184,   185,     3,     4,    78,   451,   138,   191,   134,   455,
     135,    96,   313,   372,   139,     9,   101,    11,   117,   203,
     134,   142,   143,   134,   137,   142,   143,   142,   143,     3,
       4,   137,   142,   143,   142,   143,   128,   129,   376,   100,
     143,   342,   124,   125,   126,   127,   128,   129,   134,   408,
     134,   143,   333,   334,   335,   336,   337,   338,   339,   340,
     341,   143,    56,    57,   141,   126,   134,   128,   134,   130,
     134,    51,    52,     7,   135,    55,   435,   358,   137,   140,
       3,     4,     3,     4,   134,   444,   137,   218,   218,   135,
     209,   210,     3,     4,   213,   135,   124,   125,   126,   127,
     128,   129,   230,   135,   232,     3,     4,     3,   136,   390,
     126,   127,   128,   129,   135,   143,   244,     3,     4,   236,
       4,   236,   135,   244,   252,   253,   236,   143,   312,   138,
     139,   135,   253,   414,   415,   135,    20,   135,    22,   135,
     421,   138,   139,   262,    28,   135,   427,   135,    32,   135,
     135,   135,   135,   135,   135,   436,   135,   135,    42,    32,
     441,   137,    46,    47,    37,   135,    50,   135,    52,   135,
     135,    86,     7,    46,   137,    59,    49,   137,   142,   136,
     135,   297,   301,     7,     7,     7,    70,    71,   305,   308,
     305,    64,    76,     7,    52,   305,     8,   316,     5,   316,
     139,   141,   137,   387,   122,   138,   142,   136,   327,   122,
       3,     4,    96,    97,    98,   135,     3,   101,   144,   136,
     136,   105,   136,    16,   136,   134,   136,   111,   136,   113,
     136,   136,   105,   136,   118,   119,   136,   110,   111,    32,
     141,   136,   136,   136,     4,   136,   136,    10,    11,   136,
       5,   136,   134,    10,    11,   139,   136,   136,    64,   134,
      20,   134,    22,   134,   136,   138,   139,    60,    28,   136,
     136,   136,    32,    10,    11,   143,   103,   134,    10,    11,
     134,   136,    42,   139,   137,   135,    46,    47,   139,   406,
      50,   406,    52,   136,   122,   141,   406,   136,   136,    59,
       0,     0,    95,   424,    18,   304,     3,   443,     5,     6,
      70,    71,   190,     1,   303,   434,    76,   312,   207,   447,
       1,     1,   443,   245,   117,   453,   447,    -1,    -1,   206,
     451,    -1,   453,    -1,   455,   128,    96,    97,    98,    -1,
      -1,   101,   135,    -1,    -1,   105,    -1,    -1,    -1,    -1,
      -1,   111,     3,   113,     5,     6,     7,    -1,   118,   119,
      -1,   124,   125,   126,   127,   128,   129,   124,   125,   126,
     127,   128,   129,   136,    -1,    -1,    -1,    -1,    -1,   139,
     143,    -1,    -1,    -1,   141,    -1,   143,   124,   125,   126,
     127,   128,   129,   125,   126,   127,   128,   129,     3,     4,
       5,     6,    -1,   100,   141,    -1,   143,    -1,    -1,    -1,
       3,   143,     5,     6,     7,    20,    -1,    22,     3,    -1,
       5,     6,    -1,    28,    -1,    -1,    -1,    32,    -1,   126,
      -1,   128,    -1,   130,    -1,    -1,    -1,    42,   135,    -1,
      -1,    46,    47,    -1,    -1,    50,    -1,    52,    -1,   100,
      -1,    -1,    -1,    -1,    59,    -1,    -1,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    70,    71,    -1,    -1,    -1,
      -1,    76,    -1,    -1,    -1,   126,    -1,   128,    -1,   130,
      32,    -1,    -1,    -1,   135,    37,    -1,    -1,    40,    -1,
      -1,    96,    97,    98,    46,   100,   101,    49,    -1,    -1,
     105,    -1,    -1,    -1,    -1,    -1,   111,   100,   113,    -1,
      -1,    -1,    64,   118,   119,   100,    -1,    -1,    -1,    -1,
      72,   126,    -1,   128,    -1,   130,    -1,    -1,     4,    -1,
     135,    -1,    -1,   126,    -1,   128,    -1,   130,    -1,    -1,
      -1,   126,   135,   128,    20,   130,    22,    -1,    -1,    -1,
     135,    -1,    28,   105,    -1,    -1,    32,    -1,   110,   111,
      -1,    -1,    -1,    -1,    -1,    -1,    42,    32,    -1,    -1,
      46,    47,    37,    -1,    50,    40,    52,    -1,    -1,    -1,
      -1,    46,   134,    59,    49,    -1,    -1,   139,    -1,    -1,
      -1,    -1,    -1,    -1,    70,    71,    -1,    -1,    32,    64,
      76,    -1,    -1,    37,    -1,    -1,    40,    72,    -1,    32,
      -1,    -1,    46,    -1,    37,    49,    -1,    -1,    -1,    32,
      96,    97,    98,    46,    37,   101,    49,    -1,    -1,   105,
      64,    -1,    -1,    46,    -1,   111,    49,   113,    72,    -1,
     105,    64,   118,   119,    -1,   110,   111,    -1,    -1,    -1,
      -1,    64,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   134,
      -1,   105,    -1,   138,   139,    -1,   110,   111,    -1,    -1,
      -1,    -1,   105,    -1,    -1,    -1,    -1,   110,   111,    -1,
      -1,    -1,   105,    -1,    -1,    -1,    -1,   110,   111,    -1,
     134,    -1,    -1,    -1,    -1,   139,    -1,    -1,    -1,    -1,
      16,   134,    18,    -1,    -1,   138,   139,    -1,    24,    -1,
      26,   134,    -1,    -1,    -1,   138,   139,    33,    34,    35,
      36,    -1,    38,    39,    -1,    41,    -1,    43,    44,    45,
      -1,    -1,    48,    -1,    -1,    51,    -1,    53,    54,    55,
      56,    57,    58,    -1,    60,    61,    62,    63,    -1,    -1,
      66,    -1,    -1,    69,    -1,    -1,    -1,    73,    -1,    75,
      -1,    -1,    -1,    79,    80,    81,    82,    83,    84,    85,
      -1,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      -1,    -1,    -1,    99,    -1,    -1,   102,    -1,   104,    -1,
      -1,   107,   108,   109,    -1,    -1,   112,    -1,   114,   115,
     116,   117,    -1,    -1,   120
};
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
#line 228 "./parser.y"
{ write_proxies(yyvsp[0].ifref); write_client(yyvsp[0].ifref); write_server(yyvsp[0].ifref); }
    break;
case 2:
#line 231 "./parser.y"
{ yyval.ifref = NULL; }
    break;
case 3:
#line 232 "./parser.y"
{ yyval.ifref = yyvsp[-1].ifref; }
    break;
case 4:
#line 233 "./parser.y"
{ yyval.ifref = make_ifref(yyvsp[0].type); LINK(yyval.ifref, yyvsp[-1].ifref); }
    break;
case 5:
#line 234 "./parser.y"
{ yyval.ifref = yyvsp[-1].ifref; add_coclass(yyvsp[0].clas); }
    break;
case 6:
#line 235 "./parser.y"
{ yyval.ifref = yyvsp[-1].ifref; add_module(yyvsp[0].type); }
    break;
case 7:
#line 236 "./parser.y"
{ yyval.ifref = yyvsp[-1].ifref; }
    break;
case 8:
#line 237 "./parser.y"
{ yyval.ifref = yyvsp[-1].ifref; }
    break;
case 9:
#line 240 "./parser.y"
{}
    break;
case 10:
#line 241 "./parser.y"
{ if (!parse_only) add_interface(yyvsp[0].type); }
    break;
case 11:
#line 242 "./parser.y"
{ if (!parse_only) add_interface(yyvsp[0].type); }
    break;
case 12:
#line 243 "./parser.y"
{ if (!parse_only) add_coclass(yyvsp[0].clas); }
    break;
case 13:
#line 244 "./parser.y"
{ if (!parse_only) add_module(yyvsp[0].type); }
    break;
case 14:
#line 245 "./parser.y"
{}
    break;
case 15:
#line 248 "./parser.y"
{ yyval.func = NULL; }
    break;
case 16:
#line 249 "./parser.y"
{ yyval.func = yyvsp[-1].func; LINK(yyval.func, yyvsp[-2].func); }
    break;
case 17:
#line 250 "./parser.y"
{ yyval.func = yyvsp[-1].func; }
    break;
case 18:
#line 253 "./parser.y"
{}
    break;
case 19:
#line 254 "./parser.y"
{ if (!parse_only && do_header) { write_constdef(yyvsp[-1].var); } }
    break;
case 20:
#line 255 "./parser.y"
{}
    break;
case 21:
#line 256 "./parser.y"
{ if (!parse_only && do_header) { write_type(header, yyvsp[-1].type, NULL, NULL); fprintf(header, ";\n\n"); } }
    break;
case 22:
#line 257 "./parser.y"
{ if (!parse_only && do_header) { write_externdef(yyvsp[-1].var); } }
    break;
case 23:
#line 258 "./parser.y"
{}
    break;
case 24:
#line 259 "./parser.y"
{ if (!parse_only && do_header) { write_type(header, yyvsp[-1].type, NULL, NULL); fprintf(header, ";\n\n"); } }
    break;
case 25:
#line 260 "./parser.y"
{}
    break;
case 26:
#line 261 "./parser.y"
{ if (!parse_only && do_header) { write_type(header, yyvsp[-1].type, NULL, NULL); fprintf(header, ";\n\n"); } }
    break;
case 27:
#line 264 "./parser.y"
{ if (!parse_only && do_header) fprintf(header, "%s\n", yyvsp[-1].str); }
    break;
case 28:
#line 266 "./parser.y"
{ assert(yychar == YYEMPTY);
						  if (!do_import(yyvsp[-1].str)) yychar = aEOF; }
    break;
case 29:
#line 269 "./parser.y"
{}
    break;
case 30:
#line 272 "./parser.y"
{ yyval.str = yyvsp[0].str; }
    break;
case 31:
#line 274 "./parser.y"
{ start_typelib(yyvsp[-1].str, yyvsp[-2].attr); }
    break;
case 32:
#line 276 "./parser.y"
{ end_typelib(); }
    break;
case 33:
#line 279 "./parser.y"
{ yyval.var = NULL; }
    break;
case 35:
#line 283 "./parser.y"
{ yyval.var = NULL; }
    break;
case 37:
#line 287 "./parser.y"
{ LINK(yyvsp[0].var, yyvsp[-2].var); yyval.var = yyvsp[0].var; }
    break;
case 39:
#line 292 "./parser.y"
{ yyval.var = yyvsp[-1].var;
						  set_type(yyval.var, yyvsp[-2].tref, yyvsp[0].expr);
						  yyval.var->attrs = yyvsp[-3].attr;
						}
    break;
case 40:
#line 296 "./parser.y"
{ yyval.var = yyvsp[-1].var;
						  set_type(yyval.var, yyvsp[-2].tref, yyvsp[0].expr);
						}
    break;
case 41:
#line 299 "./parser.y"
{ yyval.var = yyvsp[-3].var;
						  yyval.var->ptr_level--;
						  set_type(yyval.var, yyvsp[-4].tref, NULL);
						  yyval.var->attrs = yyvsp[-5].attr;
						  yyval.var->args = yyvsp[-1].var;
						}
    break;
case 42:
#line 305 "./parser.y"
{ yyval.var = yyvsp[-3].var;
						  yyval.var->ptr_level--;
						  set_type(yyval.var, yyvsp[-4].tref, NULL);
						  yyval.var->args = yyvsp[-1].var;
						}
    break;
case 43:
#line 312 "./parser.y"
{ yyval.expr = NULL; }
    break;
case 44:
#line 313 "./parser.y"
{ yyval.expr = yyvsp[-1].expr; }
    break;
case 45:
#line 314 "./parser.y"
{ yyval.expr = make_expr(EXPR_VOID); }
    break;
case 47:
#line 318 "./parser.y"
{ LINK(yyvsp[0].expr, yyvsp[-2].expr); yyval.expr = yyvsp[0].expr; }
    break;
case 48:
#line 319 "./parser.y"
{ LINK(yyvsp[0].expr, yyvsp[-3].expr); yyval.expr = yyvsp[0].expr; }
    break;
case 49:
#line 322 "./parser.y"
{ yyval.attr = NULL; }
    break;
case 51:
#line 327 "./parser.y"
{ yyval.attr = yyvsp[-1].attr; }
    break;
case 53:
#line 331 "./parser.y"
{ LINK(yyvsp[0].attr, yyvsp[-2].attr); yyval.attr = yyvsp[0].attr; }
    break;
case 54:
#line 332 "./parser.y"
{ LINK(yyvsp[0].attr, yyvsp[-3].attr); yyval.attr = yyvsp[0].attr; }
    break;
case 55:
#line 336 "./parser.y"
{ yyval.attr = make_attr(ATTR_ASYNC); }
    break;
case 56:
#line 337 "./parser.y"
{ yyval.attr = make_attr(ATTR_AUTO_HANDLE); }
    break;
case 57:
#line 338 "./parser.y"
{ yyval.attr = make_attrp(ATTR_CALLAS, yyvsp[-1].var); }
    break;
case 58:
#line 339 "./parser.y"
{ yyval.attr = make_attrp(ATTR_CASE, yyvsp[-1].expr); }
    break;
case 59:
#line 340 "./parser.y"
{ yyval.attr = make_attrv(ATTR_CONTEXTHANDLE, 0); }
    break;
case 60:
#line 341 "./parser.y"
{ yyval.attr = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_DONT_SERIALIZE */ }
    break;
case 61:
#line 342 "./parser.y"
{ yyval.attr = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_SERIALIZE */ }
    break;
case 62:
#line 343 "./parser.y"
{ yyval.attr = make_attr(ATTR_CONTROL); }
    break;
case 63:
#line 344 "./parser.y"
{ yyval.attr = make_attr(ATTR_DEFAULT); }
    break;
case 64:
#line 345 "./parser.y"
{ yyval.attr = make_attrp(ATTR_DEFAULTVALUE_EXPR, yyvsp[-1].expr); }
    break;
case 65:
#line 346 "./parser.y"
{ yyval.attr = make_attrp(ATTR_DEFAULTVALUE_STRING, yyvsp[-1].str); }
    break;
case 66:
#line 347 "./parser.y"
{ yyval.attr = make_attrp(ATTR_DLLNAME, yyvsp[-1].str); }
    break;
case 67:
#line 348 "./parser.y"
{ yyval.attr = make_attr(ATTR_DUAL); }
    break;
case 68:
#line 349 "./parser.y"
{ yyval.attr = make_attrp(ATTR_ENDPOINT, yyvsp[-1].str); }
    break;
case 69:
#line 350 "./parser.y"
{ yyval.attr = make_attrp(ATTR_ENTRY_STRING, yyvsp[-1].str); }
    break;
case 70:
#line 351 "./parser.y"
{ yyval.attr = make_attrp(ATTR_ENTRY_ORDINAL, yyvsp[-1].expr); }
    break;
case 71:
#line 352 "./parser.y"
{ yyval.attr = make_attr(ATTR_EXPLICIT_HANDLE); }
    break;
case 72:
#line 353 "./parser.y"
{ yyval.attr = make_attr(ATTR_HANDLE); }
    break;
case 73:
#line 354 "./parser.y"
{ yyval.attr = make_attrp(ATTR_HELPCONTEXT, yyvsp[-1].expr); }
    break;
case 74:
#line 355 "./parser.y"
{ yyval.attr = make_attrp(ATTR_HELPFILE, yyvsp[-1].str); }
    break;
case 75:
#line 356 "./parser.y"
{ yyval.attr = make_attrp(ATTR_HELPSTRING, yyvsp[-1].str); }
    break;
case 76:
#line 357 "./parser.y"
{ yyval.attr = make_attrp(ATTR_HELPSTRINGCONTEXT, yyvsp[-1].expr); }
    break;
case 77:
#line 358 "./parser.y"
{ yyval.attr = make_attrp(ATTR_HELPSTRINGDLL, yyvsp[-1].str); }
    break;
case 78:
#line 359 "./parser.y"
{ yyval.attr = make_attr(ATTR_HIDDEN); }
    break;
case 79:
#line 360 "./parser.y"
{ yyval.attr = make_attrp(ATTR_ID, yyvsp[-1].expr); }
    break;
case 80:
#line 361 "./parser.y"
{ yyval.attr = make_attr(ATTR_IDEMPOTENT); }
    break;
case 81:
#line 362 "./parser.y"
{ yyval.attr = make_attrp(ATTR_IIDIS, yyvsp[-1].var); }
    break;
case 82:
#line 363 "./parser.y"
{ yyval.attr = make_attrp(ATTR_IMPLICIT_HANDLE, yyvsp[-1].str); }
    break;
case 83:
#line 364 "./parser.y"
{ yyval.attr = make_attr(ATTR_IN); }
    break;
case 84:
#line 365 "./parser.y"
{ yyval.attr = make_attr(ATTR_INPUTSYNC); }
    break;
case 85:
#line 366 "./parser.y"
{ yyval.attr = make_attrp(ATTR_LENGTHIS, yyvsp[-1].expr); }
    break;
case 86:
#line 367 "./parser.y"
{ yyval.attr = make_attr(ATTR_LOCAL); }
    break;
case 87:
#line 368 "./parser.y"
{ yyval.attr = make_attr(ATTR_NONCREATABLE); }
    break;
case 88:
#line 369 "./parser.y"
{ yyval.attr = make_attr(ATTR_OBJECT); }
    break;
case 89:
#line 370 "./parser.y"
{ yyval.attr = make_attr(ATTR_ODL); }
    break;
case 90:
#line 371 "./parser.y"
{ yyval.attr = make_attr(ATTR_OLEAUTOMATION); }
    break;
case 91:
#line 372 "./parser.y"
{ yyval.attr = make_attr(ATTR_OPTIONAL); }
    break;
case 92:
#line 373 "./parser.y"
{ yyval.attr = make_attr(ATTR_OUT); }
    break;
case 93:
#line 374 "./parser.y"
{ yyval.attr = make_attrv(ATTR_POINTERDEFAULT, yyvsp[-1].num); }
    break;
case 94:
#line 375 "./parser.y"
{ yyval.attr = make_attr(ATTR_PROPGET); }
    break;
case 95:
#line 376 "./parser.y"
{ yyval.attr = make_attr(ATTR_PROPPUT); }
    break;
case 96:
#line 377 "./parser.y"
{ yyval.attr = make_attr(ATTR_PROPPUTREF); }
    break;
case 97:
#line 378 "./parser.y"
{ yyval.attr = make_attr(ATTR_PTR); }
    break;
case 98:
#line 379 "./parser.y"
{ yyval.attr = make_attr(ATTR_PUBLIC); }
    break;
case 99:
#line 380 "./parser.y"
{ yyval.attr = make_attr(ATTR_READONLY); }
    break;
case 100:
#line 381 "./parser.y"
{ yyval.attr = make_attr(ATTR_REF); }
    break;
case 101:
#line 382 "./parser.y"
{ yyval.attr = make_attr(ATTR_RESTRICTED); }
    break;
case 102:
#line 383 "./parser.y"
{ yyval.attr = make_attr(ATTR_RETVAL); }
    break;
case 103:
#line 384 "./parser.y"
{ yyval.attr = make_attrp(ATTR_SIZEIS, yyvsp[-1].expr); }
    break;
case 104:
#line 385 "./parser.y"
{ yyval.attr = make_attr(ATTR_SOURCE); }
    break;
case 105:
#line 386 "./parser.y"
{ yyval.attr = make_attr(ATTR_STRING); }
    break;
case 106:
#line 387 "./parser.y"
{ yyval.attr = make_attrp(ATTR_SWITCHIS, yyvsp[-1].expr); }
    break;
case 107:
#line 388 "./parser.y"
{ yyval.attr = make_attrp(ATTR_SWITCHTYPE, type_ref(yyvsp[-1].tref)); }
    break;
case 108:
#line 389 "./parser.y"
{ yyval.attr = make_attrp(ATTR_TRANSMITAS, type_ref(yyvsp[-1].tref)); }
    break;
case 109:
#line 390 "./parser.y"
{ yyval.attr = make_attr(ATTR_UNIQUE); }
    break;
case 110:
#line 391 "./parser.y"
{ yyval.attr = make_attrp(ATTR_UUID, yyvsp[-1].uuid); }
    break;
case 111:
#line 392 "./parser.y"
{ yyval.attr = make_attr(ATTR_V1ENUM); }
    break;
case 112:
#line 393 "./parser.y"
{ yyval.attr = make_attr(ATTR_VARARG); }
    break;
case 113:
#line 394 "./parser.y"
{ yyval.attr = make_attrv(ATTR_VERSION, yyvsp[-1].num); }
    break;
case 114:
#line 395 "./parser.y"
{ yyval.attr = make_attrp(ATTR_WIREMARSHAL, type_ref(yyvsp[-1].tref)); }
    break;
case 115:
#line 396 "./parser.y"
{ yyval.attr = make_attrv(ATTR_POINTERTYPE, yyvsp[0].num); }
    break;
case 118:
#line 403 "./parser.y"
{ yyval.var = NULL; }
    break;
case 119:
#line 404 "./parser.y"
{ if (yyvsp[0].var) { LINK(yyvsp[0].var, yyvsp[-1].var); yyval.var = yyvsp[0].var; }
						  else { yyval.var = yyvsp[-1].var; }
						}
    break;
case 120:
#line 409 "./parser.y"
{ attr_t *a = make_attrp(ATTR_CASE, yyvsp[-2].expr);
						  yyval.var = yyvsp[0].var; if (!yyval.var) yyval.var = make_var(NULL);
						  LINK(a, yyval.var->attrs); yyval.var->attrs = a;
						}
    break;
case 121:
#line 413 "./parser.y"
{ attr_t *a = make_attr(ATTR_DEFAULT);
						  yyval.var = yyvsp[0].var; if (!yyval.var) yyval.var = make_var(NULL);
						  LINK(a, yyval.var->attrs); yyval.var->attrs = a;
						}
    break;
case 122:
#line 419 "./parser.y"
{ yyval.var = reg_const(yyvsp[-2].var);
						  set_type(yyval.var, yyvsp[-3].tref, NULL);
						  yyval.var->eval = yyvsp[0].expr;
						  yyval.var->lval = yyvsp[0].expr->cval;
						}
    break;
case 123:
#line 426 "./parser.y"
{ yyval.var = NULL; }
    break;
case 124:
#line 427 "./parser.y"
{ yyval.var = yyvsp[-1].var; }
    break;
case 127:
#line 432 "./parser.y"
{ LINK(yyvsp[0].var, yyvsp[-2].var); yyval.var = yyvsp[0].var;
						  if (yyvsp[-2].var && !yyvsp[0].var->eval)
						    yyvsp[0].var->lval = yyvsp[-2].var->lval + 1;
						}
    break;
case 128:
#line 438 "./parser.y"
{ yyval.var = reg_const(yyvsp[-2].var);
						  yyval.var->eval = yyvsp[0].expr;
						  yyval.var->lval = yyvsp[0].expr->cval;
						  yyval.var->type = make_type(RPC_FC_LONG, &std_int);
						}
    break;
case 129:
#line 443 "./parser.y"
{ yyval.var = reg_const(yyvsp[0].var);
						  yyval.var->lval = 0; /* default for first enum entry */
						  yyval.var->type = make_type(RPC_FC_LONG, &std_int);
						}
    break;
case 130:
#line 449 "./parser.y"
{ yyval.type = get_typev(RPC_FC_ENUM16, yyvsp[-3].var, tsENUM);
						  yyval.type->fields = yyvsp[-1].var;
						  yyval.type->defined = TRUE;
						  if(in_typelib)
						      add_enum(yyval.type);
						}
    break;
case 132:
#line 458 "./parser.y"
{ LINK(yyvsp[0].expr, yyvsp[-2].expr); yyval.expr = yyvsp[0].expr; }
    break;
case 133:
#line 471 "./parser.y"
{ yyval.expr = make_expr(EXPR_VOID); }
    break;
case 135:
#line 475 "./parser.y"
{ yyval.expr = make_exprl(EXPR_NUM, yyvsp[0].num); }
    break;
case 136:
#line 476 "./parser.y"
{ yyval.expr = make_exprl(EXPR_HEXNUM, yyvsp[0].num); }
    break;
case 137:
#line 477 "./parser.y"
{ yyval.expr = make_exprs(EXPR_IDENTIFIER, yyvsp[0].str); }
    break;
case 138:
#line 478 "./parser.y"
{ yyval.expr = make_expr3(EXPR_COND, yyvsp[-4].expr, yyvsp[-2].expr, yyvsp[0].expr); }
    break;
case 139:
#line 479 "./parser.y"
{ yyval.expr = make_expr2(EXPR_OR , yyvsp[-2].expr, yyvsp[0].expr); }
    break;
case 140:
#line 480 "./parser.y"
{ yyval.expr = make_expr2(EXPR_AND, yyvsp[-2].expr, yyvsp[0].expr); }
    break;
case 141:
#line 481 "./parser.y"
{ yyval.expr = make_expr2(EXPR_ADD, yyvsp[-2].expr, yyvsp[0].expr); }
    break;
case 142:
#line 482 "./parser.y"
{ yyval.expr = make_expr2(EXPR_SUB, yyvsp[-2].expr, yyvsp[0].expr); }
    break;
case 143:
#line 483 "./parser.y"
{ yyval.expr = make_expr2(EXPR_MUL, yyvsp[-2].expr, yyvsp[0].expr); }
    break;
case 144:
#line 484 "./parser.y"
{ yyval.expr = make_expr2(EXPR_DIV, yyvsp[-2].expr, yyvsp[0].expr); }
    break;
case 145:
#line 485 "./parser.y"
{ yyval.expr = make_expr2(EXPR_SHL, yyvsp[-2].expr, yyvsp[0].expr); }
    break;
case 146:
#line 486 "./parser.y"
{ yyval.expr = make_expr2(EXPR_SHR, yyvsp[-2].expr, yyvsp[0].expr); }
    break;
case 147:
#line 487 "./parser.y"
{ yyval.expr = make_expr1(EXPR_NOT, yyvsp[0].expr); }
    break;
case 148:
#line 488 "./parser.y"
{ yyval.expr = make_expr1(EXPR_NEG, yyvsp[0].expr); }
    break;
case 149:
#line 489 "./parser.y"
{ yyval.expr = make_expr1(EXPR_PPTR, yyvsp[0].expr); }
    break;
case 150:
#line 490 "./parser.y"
{ yyval.expr = make_exprt(EXPR_CAST, yyvsp[-2].tref, yyvsp[0].expr); }
    break;
case 151:
#line 491 "./parser.y"
{ yyval.expr = make_exprt(EXPR_SIZEOF, yyvsp[-1].tref, NULL); }
    break;
case 152:
#line 492 "./parser.y"
{ yyval.expr = yyvsp[-1].expr; }
    break;
case 154:
#line 496 "./parser.y"
{ LINK(yyvsp[0].expr, yyvsp[-2].expr); yyval.expr = yyvsp[0].expr; }
    break;
case 155:
#line 499 "./parser.y"
{ yyval.expr = yyvsp[0].expr;
						  if (!yyval.expr->is_const)
						      yyerror("expression is not constant\n");
						}
    break;
case 156:
#line 505 "./parser.y"
{ yyval.var = yyvsp[0].var;
						  set_type(yyval.var, yyvsp[-1].tref, NULL);
						}
    break;
case 157:
#line 510 "./parser.y"
{ yyval.var = NULL; }
    break;
case 158:
#line 511 "./parser.y"
{ if (yyvsp[0].var) { LINK(yyvsp[0].var, yyvsp[-1].var); yyval.var = yyvsp[0].var; }
						  else { yyval.var = yyvsp[-1].var; }
						}
    break;
case 159:
#line 516 "./parser.y"
{ yyval.var = yyvsp[-1].var; }
    break;
case 160:
#line 517 "./parser.y"
{ yyval.var = make_var(NULL); yyval.var->type = yyvsp[-1].type; yyval.var->attrs = yyvsp[-2].attr; }
    break;
case 161:
#line 518 "./parser.y"
{ yyval.var = make_var(NULL); yyval.var->attrs = yyvsp[-1].attr; }
    break;
case 162:
#line 519 "./parser.y"
{ yyval.var = NULL; }
    break;
case 163:
#line 522 "./parser.y"
{ yyval.var = yyvsp[-1].var; set_type(yyval.var, yyvsp[-2].tref, yyvsp[0].expr); yyval.var->attrs = yyvsp[-3].attr; }
    break;
case 164:
#line 527 "./parser.y"
{ set_type(yyvsp[-3].var, yyvsp[-5].tref, NULL);
						  yyvsp[-3].var->attrs = yyvsp[-6].attr;
						  yyval.func = make_func(yyvsp[-3].var, yyvsp[-1].var);
						  if (is_attr(yyvsp[-3].var->attrs, ATTR_IN)) {
						    yyerror("Inapplicable attribute");
						  }
						}
    break;
case 165:
#line 536 "./parser.y"
{ yyval.var = NULL; }
    break;
case 167:
#line 540 "./parser.y"
{ yyval.var = NULL; }
    break;
case 168:
#line 541 "./parser.y"
{ yyval.var = make_var(yyvsp[0].str); }
    break;
case 169:
#line 542 "./parser.y"
{ yyval.var = make_var(yyvsp[0].str); }
    break;
case 170:
#line 545 "./parser.y"
{ yyval.var = make_var(yyvsp[0].str); }
    break;
case 171:
#line 547 "./parser.y"
{ yyval.var = make_var(yyvsp[0].str); }
    break;
case 172:
#line 548 "./parser.y"
{ yyval.var = make_var(yyvsp[0].str); }
    break;
case 173:
#line 549 "./parser.y"
{ yyval.var = make_var(yyvsp[0].str); }
    break;
case 174:
#line 550 "./parser.y"
{ yyval.var = make_var(yyvsp[0].str); }
    break;
case 175:
#line 551 "./parser.y"
{ yyval.var = make_var(yyvsp[0].str); }
    break;
case 176:
#line 554 "./parser.y"
{ yyval.type = make_type(RPC_FC_BYTE, NULL); }
    break;
case 177:
#line 555 "./parser.y"
{ yyval.type = make_type(RPC_FC_WCHAR, NULL); }
    break;
case 179:
#line 557 "./parser.y"
{ yyval.type = yyvsp[0].type; yyval.type->sign = 1; }
    break;
case 180:
#line 558 "./parser.y"
{ yyval.type = yyvsp[0].type; yyval.type->sign = -1;
						  switch (yyval.type->type) {
						  case RPC_FC_CHAR: break;
						  case RPC_FC_SMALL: yyval.type->type = RPC_FC_USMALL; break;
						  case RPC_FC_SHORT: yyval.type->type = RPC_FC_USHORT; break;
						  case RPC_FC_LONG:  yyval.type->type = RPC_FC_ULONG;  break;
						  case RPC_FC_HYPER:
						    if (!yyval.type->ref) { yyval.type->ref = &std_uhyper; yyval.type->sign = 0; }
						    break;
						  default: break;
						  }
						}
    break;
case 181:
#line 570 "./parser.y"
{ yyval.type = make_type(RPC_FC_ULONG, &std_int); yyval.type->sign = -1; }
    break;
case 182:
#line 571 "./parser.y"
{ yyval.type = make_type(RPC_FC_FLOAT, NULL); }
    break;
case 183:
#line 572 "./parser.y"
{ yyval.type = make_type(RPC_FC_FLOAT, NULL); }
    break;
case 184:
#line 573 "./parser.y"
{ yyval.type = make_type(RPC_FC_DOUBLE, NULL); }
    break;
case 185:
#line 574 "./parser.y"
{ yyval.type = make_type(RPC_FC_SMALL, &std_bool); }
    break;
case 186:
#line 575 "./parser.y"
{ yyval.type = make_type(RPC_FC_ERROR_STATUS_T, NULL); }
    break;
case 187:
#line 576 "./parser.y"
{ yyval.type = make_type(RPC_FC_IGNORE, NULL); }
    break;
case 190:
#line 583 "./parser.y"
{ yyval.type = make_type(RPC_FC_LONG, &std_int); }
    break;
case 191:
#line 584 "./parser.y"
{ yyval.type = make_type(RPC_FC_SMALL, NULL); }
    break;
case 192:
#line 585 "./parser.y"
{ yyval.type = make_type(RPC_FC_SHORT, NULL); }
    break;
case 193:
#line 586 "./parser.y"
{ yyval.type = make_type(RPC_FC_LONG, NULL); }
    break;
case 194:
#line 587 "./parser.y"
{ yyval.type = make_type(RPC_FC_HYPER, NULL); }
    break;
case 195:
#line 588 "./parser.y"
{ yyval.type = make_type(RPC_FC_HYPER, &std_int64); }
    break;
case 196:
#line 589 "./parser.y"
{ yyval.type = make_type(RPC_FC_CHAR, NULL); }
    break;
case 197:
#line 592 "./parser.y"
{ yyval.clas = make_class(yyvsp[0].str); }
    break;
case 198:
#line 593 "./parser.y"
{ yyval.clas = make_class(yyvsp[0].str); }
    break;
case 199:
#line 596 "./parser.y"
{ yyval.clas = yyvsp[0].clas;
						  yyval.clas->attrs = yyvsp[-1].attr;
						  if (!parse_only && do_header)
						    write_coclass(yyval.clas);
						}
    break;
case 200:
#line 603 "./parser.y"
{ yyval.clas = yyvsp[-3].clas;
						  yyval.clas->ifaces = yyvsp[-1].ifref;
						}
    break;
case 201:
#line 608 "./parser.y"
{ yyval.ifref = NULL; }
    break;
case 202:
#line 609 "./parser.y"
{ LINK(yyvsp[0].ifref, yyvsp[-1].ifref); yyval.ifref = yyvsp[0].ifref; }
    break;
case 203:
#line 613 "./parser.y"
{ yyval.ifref = make_ifref(yyvsp[0].type); yyval.ifref->attrs = yyvsp[-1].attr; }
    break;
case 204:
#line 616 "./parser.y"
{ yyval.type = get_type(0, yyvsp[0].str, 0); }
    break;
case 205:
#line 617 "./parser.y"
{ yyval.type = get_type(0, yyvsp[0].str, 0); }
    break;
case 206:
#line 620 "./parser.y"
{ yyval.type = yyvsp[0].type;
						  if (yyval.type->defined) yyerror("multiple definition error\n");
						  yyval.type->attrs = yyvsp[-1].attr;
						  yyval.type->attrs = make_attr(ATTR_DISPINTERFACE);
						  LINK(yyval.type->attrs, yyvsp[-1].attr);
						  yyval.type->ref = find_type("IDispatch", 0);
						  if (!yyval.type->ref) yyerror("IDispatch is undefined\n");
						  yyval.type->defined = TRUE;
						  if (!parse_only && do_header) write_forward(yyval.type);
						}
    break;
case 207:
#line 632 "./parser.y"
{ yyval.var = NULL; }
    break;
case 208:
#line 633 "./parser.y"
{ LINK(yyvsp[-1].var, yyvsp[-2].var); yyval.var = yyvsp[-1].var; }
    break;
case 209:
#line 636 "./parser.y"
{ yyval.func = NULL; }
    break;
case 210:
#line 637 "./parser.y"
{ LINK(yyvsp[-1].func, yyvsp[-2].func); yyval.func = yyvsp[-1].func; }
    break;
case 211:
#line 643 "./parser.y"
{ yyval.type = yyvsp[-4].type;
						  yyval.type->fields = yyvsp[-2].var;
						  yyval.type->funcs = yyvsp[-1].func;
						  if (!parse_only && do_header) write_dispinterface(yyval.type);
						}
    break;
case 212:
#line 655 "./parser.y"
{ yyval.type = NULL; }
    break;
case 213:
#line 656 "./parser.y"
{ yyval.type = find_type2(yyvsp[0].str, 0); }
    break;
case 214:
#line 659 "./parser.y"
{ yyval.type = get_type(RPC_FC_IP, yyvsp[0].str, 0); }
    break;
case 215:
#line 660 "./parser.y"
{ yyval.type = get_type(RPC_FC_IP, yyvsp[0].str, 0); }
    break;
case 216:
#line 663 "./parser.y"
{ yyval.type = yyvsp[0].type;
						  if (yyval.type->defined) yyerror("multiple definition error\n");
						  yyval.type->attrs = yyvsp[-1].attr;
						  yyval.type->defined = TRUE;
						  if (!parse_only && do_header) write_forward(yyval.type);
						}
    break;
case 217:
#line 672 "./parser.y"
{ yyval.type = yyvsp[-4].type;
						  yyval.type->ref = yyvsp[-3].type;
						  yyval.type->funcs = yyvsp[-1].func;
						  if (!parse_only && do_header) write_interface(yyval.type);
						}
    break;
case 218:
#line 680 "./parser.y"
{ yyval.type = yyvsp[-6].type;
						  yyval.type->ref = find_type2(yyvsp[-4].str, 0);
						  if (!yyval.type->ref) yyerror("base class %s not found in import\n", yyvsp[-4].str);
						  yyval.type->funcs = yyvsp[-1].func;
						  if (!parse_only && do_header) write_interface(yyval.type);
						}
    break;
case 219:
#line 686 "./parser.y"
{ yyval.type = yyvsp[0].type; }
    break;
case 220:
#line 690 "./parser.y"
{ yyval.type = yyvsp[-1].type; if (!parse_only && do_header) write_forward(yyval.type); }
    break;
case 221:
#line 691 "./parser.y"
{ yyval.type = yyvsp[-1].type; if (!parse_only && do_header) write_forward(yyval.type); }
    break;
case 222:
#line 694 "./parser.y"
{ yyval.type = make_type(0, NULL); yyval.type->name = yyvsp[0].str; }
    break;
case 223:
#line 695 "./parser.y"
{ yyval.type = make_type(0, NULL); yyval.type->name = yyvsp[0].str; }
    break;
case 224:
#line 698 "./parser.y"
{ yyval.type = yyvsp[0].type;
						  yyval.type->attrs = yyvsp[-1].attr;
						}
    break;
case 225:
#line 703 "./parser.y"
{ yyval.type = yyvsp[-3].type;
						  yyval.type->funcs = yyvsp[-1].func;
						  /* FIXME: if (!parse_only && do_header) write_module($$); */
						}
    break;
case 226:
#line 709 "./parser.y"
{ yyval.var = yyvsp[0].var; yyval.var->ptr_level++; }
    break;
case 227:
#line 710 "./parser.y"
{ yyval.var = yyvsp[0].var; /* FIXME */ }
    break;
case 230:
#line 715 "./parser.y"
{ yyval.var = yyvsp[-1].var; }
    break;
case 232:
#line 720 "./parser.y"
{ LINK(yyvsp[0].var, yyvsp[-2].var); yyval.var = yyvsp[0].var; }
    break;
case 233:
#line 724 "./parser.y"
{ yyval.num = RPC_FC_RP; }
    break;
case 234:
#line 725 "./parser.y"
{ yyval.num = RPC_FC_UP; }
    break;
case 235:
#line 728 "./parser.y"
{ yyval.type = get_typev(RPC_FC_STRUCT, yyvsp[-3].var, tsSTRUCT);
						  /* overwrite RPC_FC_STRUCT with a more exact type */
						  yyval.type->type = get_struct_type( yyvsp[-1].var );
						  yyval.type->fields = yyvsp[-1].var;
						  yyval.type->defined = TRUE;
						  if(in_typelib)
						    add_struct(yyval.type);
						}
    break;
case 236:
#line 738 "./parser.y"
{ yyval.tref = make_tref(NULL, make_type(0, NULL)); }
    break;
case 237:
#line 739 "./parser.y"
{ yyval.tref = make_tref(yyvsp[0].str, find_type(yyvsp[0].str, 0)); }
    break;
case 238:
#line 740 "./parser.y"
{ yyval.tref = make_tref(NULL, yyvsp[0].type); }
    break;
case 239:
#line 741 "./parser.y"
{ yyval.tref = uniq_tref(yyvsp[0].tref); yyval.tref->ref->is_const = TRUE; }
    break;
case 240:
#line 742 "./parser.y"
{ yyval.tref = make_tref(NULL, yyvsp[0].type); }
    break;
case 241:
#line 743 "./parser.y"
{ yyval.tref = make_tref(NULL, find_type2(yyvsp[0].str, tsENUM)); }
    break;
case 242:
#line 744 "./parser.y"
{ yyval.tref = make_tref(NULL, yyvsp[0].type); }
    break;
case 243:
#line 745 "./parser.y"
{ yyval.tref = make_tref(NULL, get_type(RPC_FC_STRUCT, yyvsp[0].str, tsSTRUCT)); }
    break;
case 244:
#line 746 "./parser.y"
{ yyval.tref = make_tref(NULL, yyvsp[0].type); }
    break;
case 245:
#line 747 "./parser.y"
{ yyval.tref = make_tref(NULL, find_type2(yyvsp[0].str, tsUNION)); }
    break;
case 246:
#line 750 "./parser.y"
{ typeref_t *tref = uniq_tref(yyvsp[-1].tref);
						  yyvsp[0].var->tname = tref->name;
						  tref->name = NULL;
						  yyval.type = type_ref(tref);
						  yyval.type->attrs = yyvsp[-2].attr;
						  if (!parse_only && do_header)
						    write_typedef(yyval.type, yyvsp[0].var);
						  if (in_typelib && yyval.type->attrs)
						    add_typedef(yyval.type, yyvsp[0].var);
						  reg_types(yyval.type, yyvsp[0].var, 0);
						}
    break;
case 247:
#line 763 "./parser.y"
{ yyval.type = get_typev(RPC_FC_NON_ENCAPSULATED_UNION, yyvsp[-3].var, tsUNION);
						  yyval.type->fields = yyvsp[-1].var;
						  yyval.type->defined = TRUE;
						}
    break;
case 248:
#line 769 "./parser.y"
{ var_t *u = yyvsp[-3].var;
						  yyval.type = get_typev(RPC_FC_ENCAPSULATED_UNION, yyvsp[-8].var, tsUNION);
						  if (!u) u = make_var("tagged_union");
						  u->type = make_type(RPC_FC_NON_ENCAPSULATED_UNION, NULL);
						  u->type->fields = yyvsp[-1].var;
						  u->type->defined = TRUE;
						  LINK(u, yyvsp[-5].var); yyval.type->fields = u;
						  yyval.type->defined = TRUE;
						}
    break;
case 249:
#line 781 "./parser.y"
{ yyval.num = MAKELONG(yyvsp[0].num, 0); }
    break;
case 250:
#line 782 "./parser.y"
{ yyval.num = MAKELONG(yyvsp[-2].num, yyvsp[0].num); }
    break;
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
#line 785 "./parser.y"


static attr_t *make_attr(enum attr_type type)
{
  attr_t *a = xmalloc(sizeof(attr_t));
  a->type = type;
  a->u.ival = 0;
  INIT_LINK(a);
  return a;
}

static attr_t *make_attrv(enum attr_type type, unsigned long val)
{
  attr_t *a = xmalloc(sizeof(attr_t));
  a->type = type;
  a->u.ival = val;
  INIT_LINK(a);
  return a;
}

static attr_t *make_attrp(enum attr_type type, void *val)
{
  attr_t *a = xmalloc(sizeof(attr_t));
  a->type = type;
  a->u.pval = val;
  INIT_LINK(a);
  return a;
}

static expr_t *make_expr(enum expr_type type)
{
  expr_t *e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = NULL;
  e->u.lval = 0;
  e->is_const = FALSE;
  INIT_LINK(e);
  return e;
}

static expr_t *make_exprl(enum expr_type type, long val)
{
  expr_t *e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = NULL;
  e->u.lval = val;
  e->is_const = FALSE;
  INIT_LINK(e);
  /* check for numeric constant */
  if (type == EXPR_NUM || type == EXPR_HEXNUM) {
    e->is_const = TRUE;
    e->cval = val;
  }
  return e;
}

static expr_t *make_exprs(enum expr_type type, char *val)
{
  expr_t *e;
  e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = NULL;
  e->u.sval = val;
  e->is_const = FALSE;
  INIT_LINK(e);
  /* check for predefined constants */
  if (type == EXPR_IDENTIFIER) {
    var_t *c = find_const(val, 0);
    if (c) {
      e->u.sval = c->name;
      free(val);
      e->is_const = TRUE;
      e->cval = c->lval;
    }
  }
  return e;
}

static expr_t *make_exprt(enum expr_type type, typeref_t *tref, expr_t *expr)
{
  expr_t *e;
  e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = expr;
  e->u.tref = tref;
  e->is_const = FALSE;
  INIT_LINK(e);
  /* check for cast of constant expression */
  if (type == EXPR_CAST && expr->is_const) {
    e->is_const = TRUE;
    e->cval = expr->cval;
  }
  return e;
}

static expr_t *make_expr1(enum expr_type type, expr_t *expr)
{
  expr_t *e;
  e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = expr;
  e->u.lval = 0;
  e->is_const = FALSE;
  INIT_LINK(e);
  /* check for compile-time optimization */
  if (expr->is_const) {
    e->is_const = TRUE;
    switch (type) {
    case EXPR_NEG:
      e->cval = -expr->cval;
      break;
    case EXPR_NOT:
      e->cval = ~expr->cval;
      break;
    default:
      e->is_const = FALSE;
      break;
    }
  }
  return e;
}

static expr_t *make_expr2(enum expr_type type, expr_t *expr1, expr_t *expr2)
{
  expr_t *e;
  e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = expr1;
  e->u.ext = expr2;
  e->is_const = FALSE;
  INIT_LINK(e);
  /* check for compile-time optimization */
  if (expr1->is_const && expr2->is_const) {
    e->is_const = TRUE;
    switch (type) {
    case EXPR_ADD:
      e->cval = expr1->cval + expr2->cval;
      break;
    case EXPR_SUB:
      e->cval = expr1->cval - expr2->cval;
      break;
    case EXPR_MUL:
      e->cval = expr1->cval * expr2->cval;
      break;
    case EXPR_DIV:
      e->cval = expr1->cval / expr2->cval;
      break;
    case EXPR_OR:
      e->cval = expr1->cval | expr2->cval;
      break;
    case EXPR_AND:
      e->cval = expr1->cval & expr2->cval;
      break;
    case EXPR_SHL:
      e->cval = expr1->cval << expr2->cval;
      break;
    case EXPR_SHR:
      e->cval = expr1->cval >> expr2->cval;
      break;
    default:
      e->is_const = FALSE;
      break;
    }
  }
  return e;
}

static expr_t *make_expr3(enum expr_type type, expr_t *expr1, expr_t *expr2, expr_t *expr3)
{
  expr_t *e;
  e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = expr1;
  e->u.ext = expr2;
  e->ext2 = expr3;
  e->is_const = FALSE;
  INIT_LINK(e);
  /* check for compile-time optimization */
  if (expr1->is_const && expr2->is_const && expr3->is_const) {
    e->is_const = TRUE;
    switch (type) {
    case EXPR_COND:
      e->cval = expr1->cval ? expr2->cval : expr3->cval;
      break;
    default:
      e->is_const = FALSE;
      break;
    }
  }
  return e;
}

static type_t *make_type(unsigned char type, type_t *ref)
{
  type_t *t = xmalloc(sizeof(type_t));
  t->name = NULL;
  t->type = type;
  t->ref = ref;
  t->rname = NULL;
  t->attrs = NULL;
  t->funcs = NULL;
  t->fields = NULL;
  t->ignore = parse_only;
  t->is_const = FALSE;
  t->sign = 0;
  t->defined = FALSE;
  t->written = FALSE;
  t->typelib_idx = -1;
  INIT_LINK(t);
  return t;
}

static typeref_t *make_tref(char *name, type_t *ref)
{
  typeref_t *t = xmalloc(sizeof(typeref_t));
  t->name = name;
  t->ref = ref;
  t->uniq = ref ? 0 : 1;
  return t;
}

static typeref_t *uniq_tref(typeref_t *ref)
{
  typeref_t *t = ref;
  type_t *tp;
  if (t->uniq) return t;
  tp = make_type(0, t->ref);
  tp->name = t->name;
  t->name = NULL;
  t->ref = tp;
  t->uniq = 1;
  return t;
}

static type_t *type_ref(typeref_t *ref)
{
  type_t *t = ref->ref;
  if (ref->name) free(ref->name);
  free(ref);
  return t;
}

static void set_type(var_t *v, typeref_t *ref, expr_t *arr)
{
  v->type = ref->ref;
  v->tname = ref->name;
  ref->name = NULL;
  free(ref);
  v->array = arr;
}

static ifref_t *make_ifref(type_t *iface)
{
  ifref_t *l = xmalloc(sizeof(ifref_t));
  l->iface = iface;
  l->attrs = NULL;
  INIT_LINK(l);
  return l;
}

static var_t *make_var(char *name)
{
  var_t *v = xmalloc(sizeof(var_t));
  v->name = name;
  v->ptr_level = 0;
  v->type = NULL;
  v->tname = NULL;
  v->attrs = NULL;
  v->array = NULL;
  v->eval = NULL;
  v->lval = 0;
  INIT_LINK(v);
  return v;
}

static func_t *make_func(var_t *def, var_t *args)
{
  func_t *f = xmalloc(sizeof(func_t));
  f->def = def;
  f->args = args;
  f->ignore = parse_only;
  f->idx = -1;
  INIT_LINK(f);
  return f;
}

static class_t *make_class(char *name)
{
  class_t *c = xmalloc(sizeof(class_t));
  c->name = name;
  c->attrs = NULL;
  c->ifaces = NULL;
  INIT_LINK(c);
  return c;
}

#define HASHMAX 64

static int hash_ident(const char *name)
{
  const char *p = name;
  int sum = 0;
  /* a simple sum hash is probably good enough */
  while (*p) {
    sum += *p;
    p++;
  }
  return sum & (HASHMAX-1);
}

/***** type repository *****/

struct rtype {
  char *name;
  type_t *type;
  int t;
  struct rtype *next;
};

struct rtype *type_hash[HASHMAX];

static type_t *reg_type(type_t *type, char *name, int t)
{
  struct rtype *nt;
  int hash;
  if (!name) {
    yyerror("registering named type without name\n");
    return type;
  }
  hash = hash_ident(name);
  nt = xmalloc(sizeof(struct rtype));
  nt->name = name;
  nt->type = type;
  nt->t = t;
  nt->next = type_hash[hash];
  type_hash[hash] = nt;
  type->name = name;
  return type;
}

/* determine pointer type from attrs */
static unsigned char get_pointer_type( type_t *type )
{
  int t;
  if (is_attr( type->attrs, ATTR_STRING ))
  {
    type_t *t = type;
    while( t->type == 0 && t->ref )
      t = t->ref;
    switch( t->type )
    {
    case RPC_FC_CHAR:
      return RPC_FC_C_CSTRING;
    case RPC_FC_WCHAR:
      return RPC_FC_C_WSTRING;
    }
  }
  t = get_attrv( type->attrs, ATTR_POINTERTYPE );
  if (t) return t;

  if(is_attr( type->attrs, ATTR_PTR ))
    return RPC_FC_FP;

  if(is_attr( type->attrs, ATTR_UNIQUE ))
    return RPC_FC_UP;

  return RPC_FC_RP;
}

static type_t *reg_types(type_t *type, var_t *names, int t)
{
  type_t *ptr = type;
  int ptrc = 0;

  while (names) {
    var_t *next = NEXT_LINK(names);
    if (names->name) {
      type_t *cur = ptr;
      int cptr = names->ptr_level;
      if (cptr > ptrc) {
        while (cptr > ptrc) {
          int t = get_pointer_type( cur );
          cur = ptr = make_type(t, cur);
          ptrc++;
        }
      } else {
        while (cptr < ptrc) {
          cur = cur->ref;
          cptr++;
        }
      }
      reg_type(cur, names->name, t);
    }
    free(names);
    names = next;
  }
  return type;
}

static type_t *find_type(char *name, int t)
{
  struct rtype *cur = type_hash[hash_ident(name)];
  while (cur && (cur->t != t || strcmp(cur->name, name)))
    cur = cur->next;
  if (!cur) {
    yyerror("type %s not found\n", name);
    return NULL;
  }
  return cur->type;
}

static type_t *find_type2(char *name, int t)
{
  type_t *tp = find_type(name, t);
  free(name);
  return tp;
}

int is_type(const char *name)
{
  struct rtype *cur = type_hash[hash_ident(name)];
  while (cur && (cur->t || strcmp(cur->name, name)))
    cur = cur->next;
  if (cur) return TRUE;
  return FALSE;
}

static type_t *get_type(unsigned char type, char *name, int t)
{
  struct rtype *cur = NULL;
  type_t *tp;
  if (name) {
    cur = type_hash[hash_ident(name)];
    while (cur && (cur->t != t || strcmp(cur->name, name)))
      cur = cur->next;
  }
  if (cur) {
    free(name);
    return cur->type;
  }
  tp = make_type(type, NULL);
  tp->name = name;
  if (!name) return tp;
  return reg_type(tp, name, t);
}

static type_t *get_typev(unsigned char type, var_t *name, int t)
{
  char *sname = NULL;
  if (name) {
    sname = name->name;
    free(name);
  }
  return get_type(type, sname, t);
}

static int get_struct_type(var_t *field)
{
  int has_pointer = 0;
  int has_conformant_array = 0;
  int has_conformant_string = 0;

  while (field)
  {
    type_t *t = field->type;

    /* get the base type */
    while( (t->type == 0) && t->ref )
      t = t->ref;

    switch (t->type)
    {
    /*
     * RPC_FC_BYTE, RPC_FC_STRUCT, etc
     *  Simple types don't effect the type of struct.
     *  A struct containing a simple struct is still a simple struct.
     *  So long as we can block copy the data, we return RPC_FC_STRUCT.
     */
    case 0: /* void pointer */
    case RPC_FC_BYTE:
    case RPC_FC_CHAR:
    case RPC_FC_SMALL:
    case RPC_FC_USMALL:
    case RPC_FC_WCHAR:
    case RPC_FC_SHORT:
    case RPC_FC_USHORT:
    case RPC_FC_LONG:
    case RPC_FC_ULONG:
    case RPC_FC_INT3264:
    case RPC_FC_UINT3264:
    case RPC_FC_HYPER:
    case RPC_FC_FLOAT:
    case RPC_FC_DOUBLE:
    case RPC_FC_STRUCT:
    case RPC_FC_ENUM16:
    case RPC_FC_ENUM32:
      break;

    case RPC_FC_UP:
    case RPC_FC_FP:
      has_pointer = 1;
      break;
    case RPC_FC_CARRAY:
      has_conformant_array = 1;
      break;
    case RPC_FC_C_CSTRING:
    case RPC_FC_C_WSTRING:
      has_conformant_string = 1;
      break;

    /*
     * Propagate member attributes
     *  a struct should be at least as complex as its member
     */
    case RPC_FC_CVSTRUCT:
      has_conformant_string = 1;
      has_pointer = 1;
      break;

    case RPC_FC_CPSTRUCT:
      has_conformant_array = 1;
      has_pointer = 1;
      break;

    case RPC_FC_CSTRUCT:
      has_conformant_array = 1;
      break;

    case RPC_FC_PSTRUCT:
      has_pointer = 1;
      break;

    default:
      fprintf(stderr,"Unknown struct member %s with type (0x%02x)\n",
              field->name, t->type);
      /* fallthru - treat it as complex */

    /* as soon as we see one of these these members, it's bogus... */
    case RPC_FC_IP:
    case RPC_FC_ENCAPSULATED_UNION:
    case RPC_FC_NON_ENCAPSULATED_UNION:
    case RPC_FC_TRANSMIT_AS:
    case RPC_FC_REPRESENT_AS:
    case RPC_FC_PAD:
    case RPC_FC_EMBEDDED_COMPLEX:
    case RPC_FC_BOGUS_STRUCT:
      return RPC_FC_BOGUS_STRUCT;
    }
    field = NEXT_LINK(field);
  }

  if( has_conformant_string && has_pointer )
    return RPC_FC_CVSTRUCT;
  if( has_conformant_array && has_pointer )
    return RPC_FC_CPSTRUCT;
  if( has_conformant_array )
    return RPC_FC_CSTRUCT;
  if( has_pointer )
    return RPC_FC_PSTRUCT;
  return RPC_FC_STRUCT;
}

/***** constant repository *****/

struct rconst {
  char *name;
  var_t *var;
  struct rconst *next;
};

struct rconst *const_hash[HASHMAX];

static var_t *reg_const(var_t *var)
{
  struct rconst *nc;
  int hash;
  if (!var->name) {
    yyerror("registering constant without name\n");
    return var;
  }
  hash = hash_ident(var->name);
  nc = xmalloc(sizeof(struct rconst));
  nc->name = var->name;
  nc->var = var;
  nc->next = const_hash[hash];
  const_hash[hash] = nc;
  return var;
}

static var_t *find_const(char *name, int f)
{
  struct rconst *cur = const_hash[hash_ident(name)];
  while (cur && strcmp(cur->name, name))
    cur = cur->next;
  if (!cur) {
    if (f) yyerror("constant %s not found\n", name);
    return NULL;
  }
  return cur->var;
}
