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
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0

/* If NAME_PREFIX is specified substitute the variables and functions
   names.  */
#define yyparse ppparse
#define yylex   pplex
#define yyerror pperror
#define yylval  pplval
#define yychar  ppchar
#define yydebug ppdebug
#define yynerrs ppnerrs


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     tRCINCLUDE = 258,
     tIF = 259,
     tIFDEF = 260,
     tIFNDEF = 261,
     tELSE = 262,
     tELIF = 263,
     tENDIF = 264,
     tDEFINED = 265,
     tNL = 266,
     tINCLUDE = 267,
     tINCLUDE_NEXT = 268,
     tLINE = 269,
     tGCCLINE = 270,
     tERROR = 271,
     tWARNING = 272,
     tPRAGMA = 273,
     tPPIDENT = 274,
     tUNDEF = 275,
     tMACROEND = 276,
     tCONCAT = 277,
     tELIPSIS = 278,
     tSTRINGIZE = 279,
     tIDENT = 280,
     tLITERAL = 281,
     tMACRO = 282,
     tDEFINE = 283,
     tDQSTRING = 284,
     tSQSTRING = 285,
     tIQSTRING = 286,
     tUINT = 287,
     tSINT = 288,
     tULONG = 289,
     tSLONG = 290,
     tULONGLONG = 291,
     tSLONGLONG = 292,
     tRCINCLUDEPATH = 293,
     tLOGOR = 294,
     tLOGAND = 295,
     tNE = 296,
     tEQ = 297,
     tGTE = 298,
     tLTE = 299,
     tRSHIFT = 300,
     tLSHIFT = 301
   };
#endif
#define tRCINCLUDE 258
#define tIF 259
#define tIFDEF 260
#define tIFNDEF 261
#define tELSE 262
#define tELIF 263
#define tENDIF 264
#define tDEFINED 265
#define tNL 266
#define tINCLUDE 267
#define tINCLUDE_NEXT 268
#define tLINE 269
#define tGCCLINE 270
#define tERROR 271
#define tWARNING 272
#define tPRAGMA 273
#define tPPIDENT 274
#define tUNDEF 275
#define tMACROEND 276
#define tCONCAT 277
#define tELIPSIS 278
#define tSTRINGIZE 279
#define tIDENT 280
#define tLITERAL 281
#define tMACRO 282
#define tDEFINE 283
#define tDQSTRING 284
#define tSQSTRING 285
#define tIQSTRING 286
#define tUINT 287
#define tSINT 288
#define tULONG 289
#define tSLONG 290
#define tULONGLONG 291
#define tSLONGLONG 292
#define tRCINCLUDEPATH 293
#define tLOGOR 294
#define tLOGAND 295
#define tNE 296
#define tEQ 297
#define tGTE 298
#define tLTE 299
#define tRSHIFT 300
#define tLSHIFT 301




/* Copy the first part of user declarations.  */
#line 30 "wpp/ppy.y"

#include "config.h"

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



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 126 "wpp/ppy.y"
typedef union YYSTYPE {
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
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 287 "wpp/wpp.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 299 "wpp/wpp.tab.c"

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
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   308

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  63
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  13
/* YYNRULES -- Number of rules. */
#define YYNRULES  86
/* YYNRULES -- Number of states. */
#define YYNSTATES  158

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   301

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    59,     2,     2,     2,     2,    45,     2,
      61,    62,    56,    54,    60,    55,     2,    57,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    40,     2,
      48,     2,    49,    39,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    44,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    43,     2,    58,     2,     2,     2,
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
      35,    36,    37,    38,    41,    42,    46,    47,    50,    51,
      52,    53
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short yyprhs[] =
{
       0,     0,     3,     4,     7,    11,    15,    19,    23,    27,
      31,    35,    39,    42,    45,    49,    53,    60,    65,    70,
      76,    83,    91,   100,   103,   107,   111,   115,   119,   122,
     125,   126,   128,   130,   132,   134,   137,   140,   143,   144,
     145,   147,   149,   153,   157,   159,   160,   162,   164,   167,
     169,   171,   173,   175,   178,   180,   182,   184,   186,   188,
     190,   192,   195,   200,   202,   206,   210,   214,   218,   222,
     226,   230,   234,   238,   242,   246,   250,   254,   258,   262,
     266,   270,   273,   276,   279,   282,   286
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      64,     0,    -1,    -1,    64,    65,    -1,    12,    29,    11,
      -1,    12,    31,    11,    -1,    13,    29,    11,    -1,    13,
      31,    11,    -1,     4,    75,    11,    -1,     5,    25,    11,
      -1,     6,    25,    11,    -1,     8,    75,    11,    -1,     7,
      11,    -1,     9,    11,    -1,    20,    25,    11,    -1,    28,
      66,    11,    -1,    27,    68,    69,    21,    72,    11,    -1,
      14,    33,    29,    11,    -1,    15,    33,    29,    11,    -1,
      15,    33,    29,    33,    11,    -1,    15,    33,    29,    33,
      33,    11,    -1,    15,    33,    29,    33,    33,    33,    11,
      -1,    15,    33,    29,    33,    33,    33,    33,    11,    -1,
      15,    11,    -1,    16,    66,    11,    -1,    17,    66,    11,
      -1,    18,    66,    11,    -1,    19,    66,    11,    -1,     3,
      38,    -1,     3,    29,    -1,    -1,    67,    -1,    26,    -1,
      29,    -1,    30,    -1,    67,    26,    -1,    67,    29,    -1,
      67,    30,    -1,    -1,    -1,    70,    -1,    71,    -1,    71,
      60,    23,    -1,    71,    60,    25,    -1,    25,    -1,    -1,
      73,    -1,    74,    -1,    73,    74,    -1,    26,    -1,    29,
      -1,    30,    -1,    22,    -1,    24,    25,    -1,    25,    -1,
      33,    -1,    32,    -1,    35,    -1,    34,    -1,    37,    -1,
      36,    -1,    10,    25,    -1,    10,    61,    25,    62,    -1,
      25,    -1,    75,    41,    75,    -1,    75,    42,    75,    -1,
      75,    47,    75,    -1,    75,    46,    75,    -1,    75,    48,
      75,    -1,    75,    49,    75,    -1,    75,    51,    75,    -1,
      75,    50,    75,    -1,    75,    54,    75,    -1,    75,    55,
      75,    -1,    75,    44,    75,    -1,    75,    45,    75,    -1,
      75,    43,    75,    -1,    75,    56,    75,    -1,    75,    57,
      75,    -1,    75,    53,    75,    -1,    75,    52,    75,    -1,
      54,    75,    -1,    55,    75,    -1,    58,    75,    -1,    59,
      75,    -1,    61,    75,    62,    -1,    75,    39,    75,    40,
      75,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   181,   181,   182,   186,   187,   188,   189,   190,   191,
     192,   212,   233,   256,   271,   272,   273,   276,   277,   278,
     280,   282,   284,   286,   287,   288,   289,   290,   291,   298,
     304,   305,   308,   309,   310,   311,   312,   313,   316,   319,
     320,   323,   324,   327,   328,   332,   333,   339,   340,   343,
     344,   345,   346,   347,   353,   362,   363,   364,   365,   366,
     367,   368,   369,   370,   371,   372,   373,   374,   375,   376,
     377,   378,   379,   380,   381,   382,   383,   384,   385,   386,
     387,   388,   389,   390,   391,   392,   393
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "tRCINCLUDE", "tIF", "tIFDEF", "tIFNDEF", 
  "tELSE", "tELIF", "tENDIF", "tDEFINED", "tNL", "tINCLUDE", 
  "tINCLUDE_NEXT", "tLINE", "tGCCLINE", "tERROR", "tWARNING", "tPRAGMA", 
  "tPPIDENT", "tUNDEF", "tMACROEND", "tCONCAT", "tELIPSIS", "tSTRINGIZE", 
  "tIDENT", "tLITERAL", "tMACRO", "tDEFINE", "tDQSTRING", "tSQSTRING", 
  "tIQSTRING", "tUINT", "tSINT", "tULONG", "tSLONG", "tULONGLONG", 
  "tSLONGLONG", "tRCINCLUDEPATH", "'?'", "':'", "tLOGOR", "tLOGAND", 
  "'|'", "'^'", "'&'", "tNE", "tEQ", "'<'", "'>'", "tGTE", "tLTE", 
  "tRSHIFT", "tLSHIFT", "'+'", "'-'", "'*'", "'/'", "'~'", "'!'", "','", 
  "'('", "')'", "$accept", "pp_file", "preprocessor", "opt_text", "text", 
  "res_arg", "allmargs", "emargs", "margs", "opt_mtexts", "mtexts", 
  "mtext", "pp_expr", 0
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
     285,   286,   287,   288,   289,   290,   291,   292,   293,    63,
      58,   294,   295,   124,    94,    38,   296,   297,    60,    62,
     298,   299,   300,   301,    43,    45,    42,    47,   126,    33,
      44,    40,    41
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    63,    64,    64,    65,    65,    65,    65,    65,    65,
      65,    65,    65,    65,    65,    65,    65,    65,    65,    65,
      65,    65,    65,    65,    65,    65,    65,    65,    65,    65,
      66,    66,    67,    67,    67,    67,    67,    67,    68,    69,
      69,    70,    70,    71,    71,    72,    72,    73,    73,    74,
      74,    74,    74,    74,    74,    75,    75,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    75
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     0,     2,     3,     3,     3,     3,     3,     3,
       3,     3,     2,     2,     3,     3,     6,     4,     4,     5,
       6,     7,     8,     2,     3,     3,     3,     3,     2,     2,
       0,     1,     1,     1,     1,     2,     2,     2,     0,     0,
       1,     1,     3,     3,     1,     0,     1,     1,     2,     1,
       1,     1,     1,     2,     1,     1,     1,     1,     1,     1,
       1,     2,     4,     1,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     2,     2,     2,     2,     3,     5
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       2,     0,     1,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    30,    30,    30,    30,     0,    38,
      30,     3,    29,    28,     0,    63,    56,    55,    58,    57,
      60,    59,     0,     0,     0,     0,     0,     0,     0,     0,
      12,     0,    13,     0,     0,     0,     0,     0,    23,     0,
      32,    33,    34,     0,    31,     0,     0,     0,     0,    39,
       0,    61,     0,    81,    82,    83,    84,     0,     8,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     9,    10,    11,
       4,     5,     6,     7,     0,     0,    24,    35,    36,    37,
      25,    26,    27,    14,    44,     0,    40,    41,    15,     0,
      85,     0,    64,    65,    76,    74,    75,    67,    66,    68,
      69,    71,    70,    80,    79,    72,    73,    77,    78,    17,
      18,     0,    45,     0,    62,     0,    19,     0,    52,     0,
      54,    49,    50,    51,     0,    46,    47,    42,    43,    86,
      20,     0,    53,    16,    48,    21,     0,    22
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,     1,    21,    53,    54,    59,   105,   106,   107,   144,
     145,   146,    37
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -28
static const short yypact[] =
{
     -28,   126,   -28,   -27,    -3,   -13,    -5,     4,    -3,    12,
     -10,    28,    21,     2,   -20,   -20,   -20,   -20,    35,   -28,
     -20,   -28,   -28,   -28,   -24,   -28,   -28,   -28,   -28,   -28,
     -28,   -28,    -3,    -3,    -3,    -3,    -3,    42,    95,    97,
     -28,    68,   -28,   117,   137,   138,   141,   127,   -28,   170,
     -28,   -28,   -28,   277,   121,   281,   282,   285,   286,   273,
     288,   -28,   275,    80,    80,   -28,   -28,   116,   -28,    -3,
      -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,
      -3,    -3,    -3,    -3,    -3,    -3,    -3,   -28,   -28,   -28,
     -28,   -28,   -28,   -28,   290,     3,   -28,   -28,   -28,   -28,
     -28,   -28,   -28,   -28,   -28,   283,   -28,   242,   -28,   241,
     -28,   140,   175,   190,   204,   217,   229,    -7,    -7,    48,
      48,    48,    48,   120,   120,    80,    80,   -28,   -28,   -28,
     -28,     5,   265,    57,   -28,    -3,   -28,     6,   -28,   280,
     -28,   -28,   -28,   -28,   295,   265,   -28,   -28,   -28,   159,
     -28,     7,   -28,   -28,   -28,   -28,   296,   -28
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
     -28,   -28,   -28,   -12,   -28,   -28,   -28,   -28,   -28,   -28,
     -28,   163,    -8
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
      41,    61,    22,    55,    56,    57,    50,    24,    60,    51,
      52,    23,    38,    48,   130,    40,   136,   150,   155,    43,
      39,    44,    25,    42,    63,    64,    65,    66,    67,    26,
      27,    28,    29,    30,    31,    49,   131,    62,   137,   151,
     156,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    32,    33,    68,    47,    34,    35,    45,    36,    46,
      58,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,   128,    89,
     147,    69,   148,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      81,    82,    83,    84,    85,    86,    87,    69,    88,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,     2,   149,    90,     3,
       4,     5,     6,     7,     8,     9,    85,    86,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    97,    91,    92,
      98,    99,    93,    19,    20,    69,    94,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    83,    84,    85,    86,   110,    69,
     135,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    69,    95,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,   138,    96,   139,
     140,   141,   100,   101,   142,   143,   102,   103,   104,   108,
     109,   129,   133,   134,   132,   152,   153,   157,   154
};

static const unsigned char yycheck[] =
{
       8,    25,    29,    15,    16,    17,    26,    10,    20,    29,
      30,    38,    25,    11,    11,    11,    11,    11,    11,    29,
      25,    31,    25,    11,    32,    33,    34,    35,    36,    32,
      33,    34,    35,    36,    37,    33,    33,    61,    33,    33,
      33,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    54,    55,    11,    33,    58,    59,    29,    61,    31,
      25,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    11,
      23,    39,    25,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      52,    53,    54,    55,    56,    57,    11,    39,    11,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,     0,   135,    11,     3,
       4,     5,     6,     7,     8,     9,    56,    57,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    26,    11,    11,
      29,    30,    11,    27,    28,    39,    29,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    54,    55,    56,    57,    62,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    39,    29,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    22,    11,    24,
      25,    26,    11,    11,    29,    30,    11,    11,    25,    11,
      25,    11,    60,    62,    21,    25,    11,    11,   145
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    64,     0,     3,     4,     5,     6,     7,     8,     9,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    27,
      28,    65,    29,    38,    10,    25,    32,    33,    34,    35,
      36,    37,    54,    55,    58,    59,    61,    75,    25,    25,
      11,    75,    11,    29,    31,    29,    31,    33,    11,    33,
      26,    29,    30,    66,    67,    66,    66,    66,    25,    68,
      66,    25,    61,    75,    75,    75,    75,    75,    11,    39,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    11,    11,    11,
      11,    11,    11,    11,    29,    29,    11,    26,    29,    30,
      11,    11,    11,    11,    25,    69,    70,    71,    11,    25,
      62,    75,    75,    75,    75,    75,    75,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    11,
      11,    33,    21,    60,    62,    40,    11,    33,    22,    24,
      25,    26,    29,    30,    72,    73,    74,    23,    25,    75,
      11,    33,    25,    11,    74,    11,    33,    11
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
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
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



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



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
        case 4:
#line 186 "wpp/ppy.y"
    { pp_do_include(yyvsp[-1].cptr, LOCAL_INCLUDE); ;}
    break;

  case 5:
#line 187 "wpp/ppy.y"
    { pp_do_include(yyvsp[-1].cptr, GLOBAL_INCLUDE); ;}
    break;

  case 6:
#line 188 "wpp/ppy.y"
    { pp_do_include(yyvsp[-1].cptr, INCLUDE_NEXT); ;}
    break;

  case 7:
#line 189 "wpp/ppy.y"
    { pp_do_include(yyvsp[-1].cptr, INCLUDE_NEXT); ;}
    break;

  case 8:
#line 190 "wpp/ppy.y"
    { pp_next_if_state(boolean(&yyvsp[-1].cval)); ;}
    break;

  case 9:
#line 191 "wpp/ppy.y"
    { pp_next_if_state(pplookup(yyvsp[-1].cptr) != NULL); free(yyvsp[-1].cptr); ;}
    break;

  case 10:
#line 192 "wpp/ppy.y"
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
		;}
    break;

  case 11:
#line 212 "wpp/ppy.y"
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
		;}
    break;

  case 12:
#line 233 "wpp/ppy.y"
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
		;}
    break;

  case 13:
#line 256 "wpp/ppy.y"
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
		;}
    break;

  case 14:
#line 271 "wpp/ppy.y"
    { pp_del_define(yyvsp[-1].cptr); free(yyvsp[-1].cptr); ;}
    break;

  case 15:
#line 272 "wpp/ppy.y"
    { pp_add_define(yyvsp[-2].cptr, yyvsp[-1].cptr); ;}
    break;

  case 16:
#line 273 "wpp/ppy.y"
    {
		pp_add_macro(yyvsp[-5].cptr, macro_args, nmacro_args, yyvsp[-1].mtext);
		;}
    break;

  case 17:
#line 276 "wpp/ppy.y"
    { fprintf(ppout, "# %d %s\n", yyvsp[-2].sint , yyvsp[-1].cptr); free(yyvsp[-1].cptr); ;}
    break;

  case 18:
#line 277 "wpp/ppy.y"
    { fprintf(ppout, "# %d %s\n", yyvsp[-2].sint , yyvsp[-1].cptr); free(yyvsp[-1].cptr); ;}
    break;

  case 19:
#line 279 "wpp/ppy.y"
    { fprintf(ppout, "# %d %s %d\n", yyvsp[-3].sint, yyvsp[-2].cptr, yyvsp[-1].sint); free(yyvsp[-2].cptr); ;}
    break;

  case 20:
#line 281 "wpp/ppy.y"
    { fprintf(ppout, "# %d %s %d %d\n", yyvsp[-4].sint ,yyvsp[-3].cptr, yyvsp[-2].sint, yyvsp[-1].sint); free(yyvsp[-3].cptr); ;}
    break;

  case 21:
#line 283 "wpp/ppy.y"
    { fprintf(ppout, "# %d %s %d %d %d\n", yyvsp[-5].sint ,yyvsp[-4].cptr ,yyvsp[-3].sint ,yyvsp[-2].sint, yyvsp[-1].sint); free(yyvsp[-4].cptr); ;}
    break;

  case 22:
#line 285 "wpp/ppy.y"
    { fprintf(ppout, "# %d %s %d %d %d %d\n", yyvsp[-6].sint ,yyvsp[-5].cptr ,yyvsp[-4].sint ,yyvsp[-3].sint, yyvsp[-2].sint, yyvsp[-1].sint); free(yyvsp[-5].cptr); ;}
    break;

  case 24:
#line 287 "wpp/ppy.y"
    { pperror("#error directive: '%s'", yyvsp[-1].cptr); if(yyvsp[-1].cptr) free(yyvsp[-1].cptr); ;}
    break;

  case 25:
#line 288 "wpp/ppy.y"
    { ppwarning("#warning directive: '%s'", yyvsp[-1].cptr); if(yyvsp[-1].cptr) free(yyvsp[-1].cptr); ;}
    break;

  case 26:
#line 289 "wpp/ppy.y"
    { fprintf(ppout, "#pragma %s\n", yyvsp[-1].cptr ? yyvsp[-1].cptr : ""); if (yyvsp[-1].cptr) free(yyvsp[-1].cptr); ;}
    break;

  case 27:
#line 290 "wpp/ppy.y"
    { if(pp_status.pedantic) ppwarning("#ident ignored (arg: '%s')", yyvsp[-1].cptr); if(yyvsp[-1].cptr) free(yyvsp[-1].cptr); ;}
    break;

  case 28:
#line 291 "wpp/ppy.y"
    {
                int nl=strlen(yyvsp[0].cptr) +3;
                char *fn=pp_xmalloc(nl);
                sprintf(fn,"\"%s\"",yyvsp[0].cptr);
		free(yyvsp[0].cptr);
		pp_do_include(fn,LOCAL_INCLUDE);
	;}
    break;

  case 29:
#line 298 "wpp/ppy.y"
    {
		pp_do_include(yyvsp[0].cptr,LOCAL_INCLUDE);
	;}
    break;

  case 30:
#line 304 "wpp/ppy.y"
    { yyval.cptr = NULL; ;}
    break;

  case 31:
#line 305 "wpp/ppy.y"
    { yyval.cptr = yyvsp[0].cptr; ;}
    break;

  case 32:
#line 308 "wpp/ppy.y"
    { yyval.cptr = yyvsp[0].cptr; ;}
    break;

  case 33:
#line 309 "wpp/ppy.y"
    { yyval.cptr = yyvsp[0].cptr; ;}
    break;

  case 34:
#line 310 "wpp/ppy.y"
    { yyval.cptr = yyvsp[0].cptr; ;}
    break;

  case 35:
#line 311 "wpp/ppy.y"
    { yyval.cptr = merge_text(yyvsp[-1].cptr, yyvsp[0].cptr); ;}
    break;

  case 36:
#line 312 "wpp/ppy.y"
    { yyval.cptr = merge_text(yyvsp[-1].cptr, yyvsp[0].cptr); ;}
    break;

  case 37:
#line 313 "wpp/ppy.y"
    { yyval.cptr = merge_text(yyvsp[-1].cptr, yyvsp[0].cptr); ;}
    break;

  case 38:
#line 316 "wpp/ppy.y"
    { macro_args = NULL; nmacro_args = 0; ;}
    break;

  case 39:
#line 319 "wpp/ppy.y"
    { yyval.sint = 0; macro_args = NULL; nmacro_args = 0; ;}
    break;

  case 40:
#line 320 "wpp/ppy.y"
    { yyval.sint = nmacro_args; ;}
    break;

  case 41:
#line 323 "wpp/ppy.y"
    { yyval.marg = yyvsp[0].marg; ;}
    break;

  case 42:
#line 324 "wpp/ppy.y"
    { yyval.marg = add_new_marg(NULL, arg_list); nmacro_args *= -1; ;}
    break;

  case 43:
#line 327 "wpp/ppy.y"
    { yyval.marg = add_new_marg(yyvsp[0].cptr, arg_single); ;}
    break;

  case 44:
#line 328 "wpp/ppy.y"
    { yyval.marg = add_new_marg(yyvsp[0].cptr, arg_single); ;}
    break;

  case 45:
#line 332 "wpp/ppy.y"
    { yyval.mtext = NULL; ;}
    break;

  case 46:
#line 333 "wpp/ppy.y"
    {
		for(yyval.mtext = yyvsp[0].mtext; yyval.mtext && yyval.mtext->prev; yyval.mtext = yyval.mtext->prev)
			;
		;}
    break;

  case 47:
#line 339 "wpp/ppy.y"
    { yyval.mtext = yyvsp[0].mtext; ;}
    break;

  case 48:
#line 340 "wpp/ppy.y"
    { yyval.mtext = combine_mtext(yyvsp[-1].mtext, yyvsp[0].mtext); ;}
    break;

  case 49:
#line 343 "wpp/ppy.y"
    { yyval.mtext = new_mtext(yyvsp[0].cptr, 0, exp_text); ;}
    break;

  case 50:
#line 344 "wpp/ppy.y"
    { yyval.mtext = new_mtext(yyvsp[0].cptr, 0, exp_text); ;}
    break;

  case 51:
#line 345 "wpp/ppy.y"
    { yyval.mtext = new_mtext(yyvsp[0].cptr, 0, exp_text); ;}
    break;

  case 52:
#line 346 "wpp/ppy.y"
    { yyval.mtext = new_mtext(NULL, 0, exp_concat); ;}
    break;

  case 53:
#line 347 "wpp/ppy.y"
    {
		int mat = marg_index(yyvsp[0].cptr);
		if(mat < 0)
			pperror("Stringification identifier must be an argument parameter");
		yyval.mtext = new_mtext(NULL, mat, exp_stringize);
		;}
    break;

  case 54:
#line 353 "wpp/ppy.y"
    {
		int mat = marg_index(yyvsp[0].cptr);
		if(mat >= 0)
			yyval.mtext = new_mtext(NULL, mat, exp_subst);
		else
			yyval.mtext = new_mtext(yyvsp[0].cptr, 0, exp_text);
		;}
    break;

  case 55:
#line 362 "wpp/ppy.y"
    { yyval.cval.type = cv_sint;  yyval.cval.val.si = yyvsp[0].sint; ;}
    break;

  case 56:
#line 363 "wpp/ppy.y"
    { yyval.cval.type = cv_uint;  yyval.cval.val.ui = yyvsp[0].uint; ;}
    break;

  case 57:
#line 364 "wpp/ppy.y"
    { yyval.cval.type = cv_slong; yyval.cval.val.sl = yyvsp[0].slong; ;}
    break;

  case 58:
#line 365 "wpp/ppy.y"
    { yyval.cval.type = cv_ulong; yyval.cval.val.ul = yyvsp[0].ulong; ;}
    break;

  case 59:
#line 366 "wpp/ppy.y"
    { yyval.cval.type = cv_sll;   yyval.cval.val.sl = yyvsp[0].sll; ;}
    break;

  case 60:
#line 367 "wpp/ppy.y"
    { yyval.cval.type = cv_ull;   yyval.cval.val.ul = yyvsp[0].ull; ;}
    break;

  case 61:
#line 368 "wpp/ppy.y"
    { yyval.cval.type = cv_sint;  yyval.cval.val.si = pplookup(yyvsp[0].cptr) != NULL; ;}
    break;

  case 62:
#line 369 "wpp/ppy.y"
    { yyval.cval.type = cv_sint;  yyval.cval.val.si = pplookup(yyvsp[-1].cptr) != NULL; ;}
    break;

  case 63:
#line 370 "wpp/ppy.y"
    { yyval.cval.type = cv_sint;  yyval.cval.val.si = 0; ;}
    break;

  case 64:
#line 371 "wpp/ppy.y"
    { yyval.cval.type = cv_sint; yyval.cval.val.si = boolean(&yyvsp[-2].cval) || boolean(&yyvsp[0].cval); ;}
    break;

  case 65:
#line 372 "wpp/ppy.y"
    { yyval.cval.type = cv_sint; yyval.cval.val.si = boolean(&yyvsp[-2].cval) && boolean(&yyvsp[0].cval); ;}
    break;

  case 66:
#line 373 "wpp/ppy.y"
    { promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval, ==) ;}
    break;

  case 67:
#line 374 "wpp/ppy.y"
    { promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval, !=) ;}
    break;

  case 68:
#line 375 "wpp/ppy.y"
    { promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval,  <) ;}
    break;

  case 69:
#line 376 "wpp/ppy.y"
    { promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval,  >) ;}
    break;

  case 70:
#line 377 "wpp/ppy.y"
    { promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval, <=) ;}
    break;

  case 71:
#line 378 "wpp/ppy.y"
    { promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval, >=) ;}
    break;

  case 72:
#line 379 "wpp/ppy.y"
    { promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval,  +) ;}
    break;

  case 73:
#line 380 "wpp/ppy.y"
    { promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval,  -) ;}
    break;

  case 74:
#line 381 "wpp/ppy.y"
    { promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval,  ^) ;}
    break;

  case 75:
#line 382 "wpp/ppy.y"
    { promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval,  &) ;}
    break;

  case 76:
#line 383 "wpp/ppy.y"
    { promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval,  |) ;}
    break;

  case 77:
#line 384 "wpp/ppy.y"
    { promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval,  *) ;}
    break;

  case 78:
#line 385 "wpp/ppy.y"
    { promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval,  /) ;}
    break;

  case 79:
#line 386 "wpp/ppy.y"
    { promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval, <<) ;}
    break;

  case 80:
#line 387 "wpp/ppy.y"
    { promote_equal_size(&yyvsp[-2].cval, &yyvsp[0].cval); BIN_OP(yyval.cval, yyvsp[-2].cval, yyvsp[0].cval, >>) ;}
    break;

  case 81:
#line 388 "wpp/ppy.y"
    { yyval.cval =  yyvsp[0].cval; ;}
    break;

  case 82:
#line 389 "wpp/ppy.y"
    { UNARY_OP(yyval.cval, yyvsp[0].cval, -) ;}
    break;

  case 83:
#line 390 "wpp/ppy.y"
    { UNARY_OP(yyval.cval, yyvsp[0].cval, ~) ;}
    break;

  case 84:
#line 391 "wpp/ppy.y"
    { yyval.cval.type = cv_sint; yyval.cval.val.si = !boolean(&yyvsp[0].cval); ;}
    break;

  case 85:
#line 392 "wpp/ppy.y"
    { yyval.cval =  yyvsp[-1].cval; ;}
    break;

  case 86:
#line 393 "wpp/ppy.y"
    { yyval.cval = boolean(&yyvsp[-4].cval) ? yyvsp[-2].cval : yyvsp[0].cval; ;}
    break;


    }

/* Line 999 of yacc.c.  */
#line 1858 "wpp/wpp.tab.c"

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


#line 396 "wpp/ppy.y"


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

