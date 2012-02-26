/* A Bison parser, made by GNU Bison 2.4.2.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2006, 2009-2010 Free Software
   Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse         ppy_parse
#define yylex           ppy_lex
#define yyerror         ppy_error
#define yylval          ppy_lval
#define yychar          ppy_char
#define yydebug         ppy_debug
#define yynerrs         ppy_nerrs


/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 30 "ppy.y"

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
		r.val.si = v1.val.si OP (signed) v2.val.ui; \
	else if(!cv_signed(v1) && cv_signed(v2))	\
		r.val.si = (signed) v1.val.ui OP v2.val.si; \
	else						\
		r.val.ui = v1.val.ui OP v2.val.ui;

#define BIN_OP_LONG(r, v1, v2, OP)			\
	r.type = v1.type;				\
	if(cv_signed(v1) && cv_signed(v2))		\
		r.val.sl = v1.val.sl OP v2.val.sl;	\
	else if(cv_signed(v1) && !cv_signed(v2))	\
		r.val.sl = v1.val.sl OP (signed long) v2.val.ul; \
	else if(!cv_signed(v1) && cv_signed(v2))	\
		r.val.sl = (signed long) v1.val.ul OP v2.val.sl; \
	else						\
		r.val.ul = v1.val.ul OP v2.val.ul;

#define BIN_OP_LONGLONG(r, v1, v2, OP)			\
	r.type = v1.type;				\
	if(cv_signed(v1) && cv_signed(v2))		\
		r.val.sll = v1.val.sll OP v2.val.sll;	\
	else if(cv_signed(v1) && !cv_signed(v2))	\
		r.val.sll = v1.val.sll OP (wrc_sll_t) v2.val.ull; \
	else if(!cv_signed(v1) && cv_signed(v2))	\
		r.val.sll = (wrc_sll_t) v1.val.ull OP v2.val.sll; \
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



/* Line 189 of yacc.c  */
#line 177 "ppy.tab.c"

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
     tLINE = 268,
     tGCCLINE = 269,
     tERROR = 270,
     tWARNING = 271,
     tPRAGMA = 272,
     tPPIDENT = 273,
     tUNDEF = 274,
     tMACROEND = 275,
     tCONCAT = 276,
     tELIPSIS = 277,
     tSTRINGIZE = 278,
     tIDENT = 279,
     tLITERAL = 280,
     tMACRO = 281,
     tDEFINE = 282,
     tDQSTRING = 283,
     tSQSTRING = 284,
     tIQSTRING = 285,
     tUINT = 286,
     tSINT = 287,
     tULONG = 288,
     tSLONG = 289,
     tULONGLONG = 290,
     tSLONGLONG = 291,
     tRCINCLUDEPATH = 292,
     tLOGOR = 293,
     tLOGAND = 294,
     tNE = 295,
     tEQ = 296,
     tGTE = 297,
     tLTE = 298,
     tRSHIFT = 299,
     tLSHIFT = 300
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 126 "ppy.y"

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



/* Line 214 of yacc.c  */
#line 274 "ppy.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 286 "ppy.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
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
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   303

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  62
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  13
/* YYNRULES -- Number of rules.  */
#define YYNRULES  84
/* YYNRULES -- Number of states.  */
#define YYNSTATES  153

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   300

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    58,     2,     2,     2,     2,    44,     2,
      60,    61,    55,    53,    59,    54,     2,    56,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    39,     2,
      47,     2,    48,    38,     2,     2,     2,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    40,    41,    45,    46,    49,    50,    51,
      52
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     7,    11,    15,    19,    23,    27,
      31,    34,    37,    41,    45,    52,    57,    62,    68,    75,
      83,    92,    95,    99,   103,   107,   111,   114,   117,   118,
     120,   122,   124,   126,   129,   132,   135,   136,   137,   139,
     141,   145,   149,   151,   152,   154,   156,   159,   161,   163,
     165,   167,   170,   172,   174,   176,   178,   180,   182,   184,
     187,   192,   194,   198,   202,   206,   210,   214,   218,   222,
     226,   230,   234,   238,   242,   246,   250,   254,   258,   262,
     265,   268,   271,   274,   278
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      63,     0,    -1,    -1,    63,    64,    -1,    12,    28,    11,
      -1,    12,    30,    11,    -1,     4,    74,    11,    -1,     5,
      24,    11,    -1,     6,    24,    11,    -1,     8,    74,    11,
      -1,     7,    11,    -1,     9,    11,    -1,    19,    24,    11,
      -1,    27,    65,    11,    -1,    26,    67,    68,    20,    71,
      11,    -1,    13,    32,    28,    11,    -1,    14,    32,    28,
      11,    -1,    14,    32,    28,    32,    11,    -1,    14,    32,
      28,    32,    32,    11,    -1,    14,    32,    28,    32,    32,
      32,    11,    -1,    14,    32,    28,    32,    32,    32,    32,
      11,    -1,    14,    11,    -1,    15,    65,    11,    -1,    16,
      65,    11,    -1,    17,    65,    11,    -1,    18,    65,    11,
      -1,     3,    37,    -1,     3,    28,    -1,    -1,    66,    -1,
      25,    -1,    28,    -1,    29,    -1,    66,    25,    -1,    66,
      28,    -1,    66,    29,    -1,    -1,    -1,    69,    -1,    70,
      -1,    70,    59,    22,    -1,    70,    59,    24,    -1,    24,
      -1,    -1,    72,    -1,    73,    -1,    72,    73,    -1,    25,
      -1,    28,    -1,    29,    -1,    21,    -1,    23,    24,    -1,
      24,    -1,    32,    -1,    31,    -1,    34,    -1,    33,    -1,
      36,    -1,    35,    -1,    10,    24,    -1,    10,    60,    24,
      61,    -1,    24,    -1,    74,    40,    74,    -1,    74,    41,
      74,    -1,    74,    46,    74,    -1,    74,    45,    74,    -1,
      74,    47,    74,    -1,    74,    48,    74,    -1,    74,    50,
      74,    -1,    74,    49,    74,    -1,    74,    53,    74,    -1,
      74,    54,    74,    -1,    74,    43,    74,    -1,    74,    44,
      74,    -1,    74,    42,    74,    -1,    74,    55,    74,    -1,
      74,    56,    74,    -1,    74,    52,    74,    -1,    74,    51,
      74,    -1,    53,    74,    -1,    54,    74,    -1,    57,    74,
      -1,    58,    74,    -1,    60,    74,    61,    -1,    74,    38,
      74,    39,    74,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   181,   181,   182,   186,   187,   188,   189,   190,   210,
     234,   260,   277,   278,   279,   282,   283,   284,   286,   288,
     290,   292,   293,   294,   295,   296,   297,   310,   316,   317,
     320,   321,   322,   323,   324,   325,   328,   331,   332,   335,
     336,   339,   340,   344,   345,   351,   352,   355,   356,   357,
     358,   359,   366,   375,   376,   377,   378,   379,   380,   381,
     382,   383,   384,   385,   386,   387,   388,   389,   390,   391,
     392,   393,   394,   395,   396,   397,   398,   399,   400,   401,
     402,   403,   404,   405,   406
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "tRCINCLUDE", "tIF", "tIFDEF", "tIFNDEF",
  "tELSE", "tELIF", "tENDIF", "tDEFINED", "tNL", "tINCLUDE", "tLINE",
  "tGCCLINE", "tERROR", "tWARNING", "tPRAGMA", "tPPIDENT", "tUNDEF",
  "tMACROEND", "tCONCAT", "tELIPSIS", "tSTRINGIZE", "tIDENT", "tLITERAL",
  "tMACRO", "tDEFINE", "tDQSTRING", "tSQSTRING", "tIQSTRING", "tUINT",
  "tSINT", "tULONG", "tSLONG", "tULONGLONG", "tSLONGLONG",
  "tRCINCLUDEPATH", "'?'", "':'", "tLOGOR", "tLOGAND", "'|'", "'^'", "'&'",
  "tNE", "tEQ", "'<'", "'>'", "tGTE", "tLTE", "tRSHIFT", "tLSHIFT", "'+'",
  "'-'", "'*'", "'/'", "'~'", "'!'", "','", "'('", "')'", "$accept",
  "pp_file", "preprocessor", "opt_text", "text", "res_arg", "allmargs",
  "emargs", "margs", "opt_mtexts", "mtexts", "mtext", "pp_expr", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,    63,    58,
     293,   294,   124,    94,    38,   295,   296,    60,    62,   297,
     298,   299,   300,    43,    45,    42,    47,   126,    33,    44,
      40,    41
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    62,    63,    63,    64,    64,    64,    64,    64,    64,
      64,    64,    64,    64,    64,    64,    64,    64,    64,    64,
      64,    64,    64,    64,    64,    64,    64,    64,    65,    65,
      66,    66,    66,    66,    66,    66,    67,    68,    68,    69,
      69,    70,    70,    71,    71,    72,    72,    73,    73,    73,
      73,    73,    73,    74,    74,    74,    74,    74,    74,    74,
      74,    74,    74,    74,    74,    74,    74,    74,    74,    74,
      74,    74,    74,    74,    74,    74,    74,    74,    74,    74,
      74,    74,    74,    74,    74
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     3,     3,     3,     3,     3,     3,
       2,     2,     3,     3,     6,     4,     4,     5,     6,     7,
       8,     2,     3,     3,     3,     3,     2,     2,     0,     1,
       1,     1,     1,     2,     2,     2,     0,     0,     1,     1,
       3,     3,     1,     0,     1,     1,     2,     1,     1,     1,
       1,     2,     1,     1,     1,     1,     1,     1,     1,     2,
       4,     1,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     2,
       2,     2,     2,     3,     5
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     1,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    28,    28,    28,    28,     0,    36,    28,
       3,    27,    26,     0,    61,    54,    53,    56,    55,    58,
      57,     0,     0,     0,     0,     0,     0,     0,     0,    10,
       0,    11,     0,     0,     0,    21,     0,    30,    31,    32,
       0,    29,     0,     0,     0,     0,    37,     0,    59,     0,
      79,    80,    81,    82,     0,     6,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     7,     8,     9,     4,     5,     0,
       0,    22,    33,    34,    35,    23,    24,    25,    12,    42,
       0,    38,    39,    13,     0,    83,     0,    62,    63,    74,
      72,    73,    65,    64,    66,    67,    69,    68,    78,    77,
      70,    71,    75,    76,    15,    16,     0,    43,     0,    60,
       0,    17,     0,    50,     0,    52,    47,    48,    49,     0,
      44,    45,    40,    41,    84,    18,     0,    51,    14,    46,
      19,     0,    20
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    20,    50,    51,    56,   100,   101,   102,   139,
     140,   141,    36
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -27
static const yytype_int16 yypact[] =
{
     -27,   142,   -27,   -26,    -3,   -12,    -2,    30,    -3,    41,
      91,    21,     2,   -19,   -19,   -19,   -19,    32,   -27,   -19,
     -27,   -27,   -27,   -23,   -27,   -27,   -27,   -27,   -27,   -27,
     -27,    -3,    -3,    -3,    -3,    -3,    38,    66,   109,   -27,
      85,   -27,   113,   133,   115,   -27,   124,   -27,   -27,   -27,
     179,    -9,   278,   280,   281,   282,   129,   283,   -27,   271,
     -10,   -10,   -27,   -27,    57,   -27,    -3,    -3,    -3,    -3,
      -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,
      -3,    -3,    -3,    -3,   -27,   -27,   -27,   -27,   -27,   285,
       3,   -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,
     277,   -27,   239,   -27,   238,   -27,   132,   167,   182,   196,
     209,   221,   231,   231,   111,   111,   111,   111,    61,    61,
     -10,   -10,   -27,   -27,   -27,   -27,     4,    19,   266,   -27,
      -3,   -27,     6,   -27,   276,   -27,   -27,   -27,   -27,   290,
      19,   -27,   -27,   -27,   151,   -27,     7,   -27,   -27,   -27,
     -27,   291,   -27
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -27,   -27,   -27,   -11,   -27,   -27,   -27,   -27,   -27,   -27,
     -27,   163,    -8
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      40,    58,    21,    52,    53,    54,    47,    23,    57,    48,
      49,    22,    37,    45,   125,   131,    92,   145,   150,    93,
      94,    24,    38,    60,    61,    62,    63,    64,    25,    26,
      27,    28,    29,    30,    46,   126,   132,    59,   146,   151,
     133,    39,   134,   135,   136,    82,    83,   137,   138,    65,
      31,    32,    41,    44,    33,    34,    55,    35,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,    66,    84,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    66,    86,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    80,    81,    82,    83,   105,    42,
      85,    43,   144,    66,    87,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,     2,    89,    88,     3,     4,     5,     6,     7,
       8,     9,    90,    99,    10,    11,    12,    13,    14,    15,
      16,    17,    78,    79,    80,    81,    82,    83,    18,    19,
      66,   130,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    66,
      91,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,   142,    95,
     143,    96,    97,    98,   103,   104,   124,   127,   128,   129,
     147,   148,   152,   149
};

static const yytype_uint8 yycheck[] =
{
       8,    24,    28,    14,    15,    16,    25,    10,    19,    28,
      29,    37,    24,    11,    11,    11,    25,    11,    11,    28,
      29,    24,    24,    31,    32,    33,    34,    35,    31,    32,
      33,    34,    35,    36,    32,    32,    32,    60,    32,    32,
      21,    11,    23,    24,    25,    55,    56,    28,    29,    11,
      53,    54,    11,    32,    57,    58,    24,    60,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    38,    11,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    38,    11,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    53,    54,    55,    56,    61,    28,
      11,    30,   130,    38,    11,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,     0,    28,    11,     3,     4,     5,     6,     7,
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
      24,    11,    11,   140
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    63,     0,     3,     4,     5,     6,     7,     8,     9,
      12,    13,    14,    15,    16,    17,    18,    19,    26,    27,
      64,    28,    37,    10,    24,    31,    32,    33,    34,    35,
      36,    53,    54,    57,    58,    60,    74,    24,    24,    11,
      74,    11,    28,    30,    32,    11,    32,    25,    28,    29,
      65,    66,    65,    65,    65,    24,    67,    65,    24,    60,
      74,    74,    74,    74,    74,    11,    38,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    11,    11,    11,    11,    11,    28,
      28,    11,    25,    28,    29,    11,    11,    11,    11,    24,
      68,    69,    70,    11,    24,    61,    74,    74,    74,    74,
      74,    74,    74,    74,    74,    74,    74,    74,    74,    74,
      74,    74,    74,    74,    11,    11,    32,    20,    59,    61,
      39,    11,    32,    21,    23,    24,    25,    28,    29,    71,
      72,    73,    22,    24,    74,    11,    32,    24,    11,    73,
      11,    32,    11
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
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
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
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
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
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

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
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
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
      YYSIZE_T yyn = 0;
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

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
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
      int yychecklim = YYLAST - yyn + 1;
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
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
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
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
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
  YYUSE (yyvaluep);

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
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
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



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

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
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

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
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
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

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

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

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

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

/* Line 1464 of yacc.c  */
#line 186 "ppy.y"
    { pp_do_include((yyvsp[(2) - (3)].cptr), 1); ;}
    break;

  case 5:

/* Line 1464 of yacc.c  */
#line 187 "ppy.y"
    { pp_do_include((yyvsp[(2) - (3)].cptr), 0); ;}
    break;

  case 6:

/* Line 1464 of yacc.c  */
#line 188 "ppy.y"
    { pp_next_if_state(boolean(&(yyvsp[(2) - (3)].cval))); ;}
    break;

  case 7:

/* Line 1464 of yacc.c  */
#line 189 "ppy.y"
    { pp_next_if_state(pplookup((yyvsp[(2) - (3)].cptr)) != NULL); free((yyvsp[(2) - (3)].cptr)); ;}
    break;

  case 8:

/* Line 1464 of yacc.c  */
#line 190 "ppy.y"
    {
		int t = pplookup((yyvsp[(2) - (3)].cptr)) == NULL;
		if(pp_incl_state.state == 0 && t && !pp_incl_state.seen_junk)
		{
			pp_incl_state.state	= 1;
			pp_incl_state.ppp	= (yyvsp[(2) - (3)].cptr);
			pp_incl_state.ifdepth	= pp_get_if_depth();
		}
		else if(pp_incl_state.state != 1)
		{
			pp_incl_state.state = -1;
			free((yyvsp[(2) - (3)].cptr));
		}
		else
			free((yyvsp[(2) - (3)].cptr));
		pp_next_if_state(t);
		if(pp_status.debug)
			fprintf(stderr, "tIFNDEF: %s:%d: include_state=%d, include_ppp='%s', include_ifdepth=%d\n",
                                pp_status.input, pp_status.line_number, pp_incl_state.state, pp_incl_state.ppp, pp_incl_state.ifdepth);
		;}
    break;

  case 9:

/* Line 1464 of yacc.c  */
#line 210 "ppy.y"
    {
		pp_if_state_t s = pp_pop_if();
		switch(s)
		{
		case if_true:
		case if_elif:
			pp_push_if(if_elif);
			break;
		case if_false:
			pp_push_if(boolean(&(yyvsp[(2) - (3)].cval)) ? if_true : if_false);
			break;
		case if_ignore:
			pp_push_if(if_ignore);
			break;
		case if_elsetrue:
		case if_elsefalse:
			ppy_error("#elif cannot follow #else");
			break;
		case if_error:
			break;
		default:
			pp_internal_error(__FILE__, __LINE__, "Invalid pp_if_state (%d) in #elif directive", s);
		}
		;}
    break;

  case 10:

/* Line 1464 of yacc.c  */
#line 234 "ppy.y"
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
			ppy_error("#else clause already defined");
			break;
		case if_error:
			break;
		default:
			pp_internal_error(__FILE__, __LINE__, "Invalid pp_if_state (%d) in #else directive", s);
		}
		;}
    break;

  case 11:

/* Line 1464 of yacc.c  */
#line 260 "ppy.y"
    {
		if(pp_pop_if() != if_error)
		{
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
		;}
    break;

  case 12:

/* Line 1464 of yacc.c  */
#line 277 "ppy.y"
    { pp_del_define((yyvsp[(2) - (3)].cptr)); free((yyvsp[(2) - (3)].cptr)); ;}
    break;

  case 13:

/* Line 1464 of yacc.c  */
#line 278 "ppy.y"
    { pp_add_define((yyvsp[(1) - (3)].cptr), (yyvsp[(2) - (3)].cptr)); free((yyvsp[(1) - (3)].cptr)); free((yyvsp[(2) - (3)].cptr)); ;}
    break;

  case 14:

/* Line 1464 of yacc.c  */
#line 279 "ppy.y"
    {
		pp_add_macro((yyvsp[(1) - (6)].cptr), macro_args, nmacro_args, (yyvsp[(5) - (6)].mtext));
		;}
    break;

  case 15:

/* Line 1464 of yacc.c  */
#line 282 "ppy.y"
    { if((yyvsp[(3) - (4)].cptr)) pp_writestring("# %d %s\n", (yyvsp[(2) - (4)].sint) , (yyvsp[(3) - (4)].cptr)); free((yyvsp[(3) - (4)].cptr)); ;}
    break;

  case 16:

/* Line 1464 of yacc.c  */
#line 283 "ppy.y"
    { if((yyvsp[(3) - (4)].cptr)) pp_writestring("# %d %s\n", (yyvsp[(2) - (4)].sint) , (yyvsp[(3) - (4)].cptr)); free((yyvsp[(3) - (4)].cptr)); ;}
    break;

  case 17:

/* Line 1464 of yacc.c  */
#line 285 "ppy.y"
    { if((yyvsp[(3) - (5)].cptr)) pp_writestring("# %d %s %d\n", (yyvsp[(2) - (5)].sint), (yyvsp[(3) - (5)].cptr), (yyvsp[(4) - (5)].sint)); free((yyvsp[(3) - (5)].cptr)); ;}
    break;

  case 18:

/* Line 1464 of yacc.c  */
#line 287 "ppy.y"
    { if((yyvsp[(3) - (6)].cptr)) pp_writestring("# %d %s %d %d\n", (yyvsp[(2) - (6)].sint) ,(yyvsp[(3) - (6)].cptr), (yyvsp[(4) - (6)].sint), (yyvsp[(5) - (6)].sint)); free((yyvsp[(3) - (6)].cptr)); ;}
    break;

  case 19:

/* Line 1464 of yacc.c  */
#line 289 "ppy.y"
    { if((yyvsp[(3) - (7)].cptr)) pp_writestring("# %d %s %d %d %d\n", (yyvsp[(2) - (7)].sint) ,(yyvsp[(3) - (7)].cptr) ,(yyvsp[(4) - (7)].sint) ,(yyvsp[(5) - (7)].sint), (yyvsp[(6) - (7)].sint)); free((yyvsp[(3) - (7)].cptr)); ;}
    break;

  case 20:

/* Line 1464 of yacc.c  */
#line 291 "ppy.y"
    { if((yyvsp[(3) - (8)].cptr)) pp_writestring("# %d %s %d %d %d %d\n", (yyvsp[(2) - (8)].sint) ,(yyvsp[(3) - (8)].cptr) ,(yyvsp[(4) - (8)].sint) ,(yyvsp[(5) - (8)].sint), (yyvsp[(6) - (8)].sint), (yyvsp[(7) - (8)].sint)); free((yyvsp[(3) - (8)].cptr)); ;}
    break;

  case 22:

/* Line 1464 of yacc.c  */
#line 293 "ppy.y"
    { ppy_error("#error directive: '%s'", (yyvsp[(2) - (3)].cptr)); free((yyvsp[(2) - (3)].cptr)); ;}
    break;

  case 23:

/* Line 1464 of yacc.c  */
#line 294 "ppy.y"
    { ppy_warning("#warning directive: '%s'", (yyvsp[(2) - (3)].cptr)); free((yyvsp[(2) - (3)].cptr)); ;}
    break;

  case 24:

/* Line 1464 of yacc.c  */
#line 295 "ppy.y"
    { pp_writestring("#pragma %s\n", (yyvsp[(2) - (3)].cptr) ? (yyvsp[(2) - (3)].cptr) : ""); free((yyvsp[(2) - (3)].cptr)); ;}
    break;

  case 25:

/* Line 1464 of yacc.c  */
#line 296 "ppy.y"
    { if(pp_status.pedantic) ppy_warning("#ident ignored (arg: '%s')", (yyvsp[(2) - (3)].cptr)); free((yyvsp[(2) - (3)].cptr)); ;}
    break;

  case 26:

/* Line 1464 of yacc.c  */
#line 297 "ppy.y"
    {
                if((yyvsp[(2) - (2)].cptr))
                {
                        int nl=strlen((yyvsp[(2) - (2)].cptr)) +3;
                        char *fn=pp_xmalloc(nl);
                        if(fn)
                        {
                                sprintf(fn,"\"%s\"",(yyvsp[(2) - (2)].cptr));
                                pp_do_include(fn,1);
                        }
                        free((yyvsp[(2) - (2)].cptr));
                }
	;}
    break;

  case 27:

/* Line 1464 of yacc.c  */
#line 310 "ppy.y"
    {
		pp_do_include((yyvsp[(2) - (2)].cptr),1);
	;}
    break;

  case 28:

/* Line 1464 of yacc.c  */
#line 316 "ppy.y"
    { (yyval.cptr) = NULL; ;}
    break;

  case 29:

/* Line 1464 of yacc.c  */
#line 317 "ppy.y"
    { (yyval.cptr) = (yyvsp[(1) - (1)].cptr); ;}
    break;

  case 30:

/* Line 1464 of yacc.c  */
#line 320 "ppy.y"
    { (yyval.cptr) = (yyvsp[(1) - (1)].cptr); ;}
    break;

  case 31:

/* Line 1464 of yacc.c  */
#line 321 "ppy.y"
    { (yyval.cptr) = (yyvsp[(1) - (1)].cptr); ;}
    break;

  case 32:

/* Line 1464 of yacc.c  */
#line 322 "ppy.y"
    { (yyval.cptr) = (yyvsp[(1) - (1)].cptr); ;}
    break;

  case 33:

/* Line 1464 of yacc.c  */
#line 323 "ppy.y"
    { (yyval.cptr) = merge_text((yyvsp[(1) - (2)].cptr), (yyvsp[(2) - (2)].cptr)); ;}
    break;

  case 34:

/* Line 1464 of yacc.c  */
#line 324 "ppy.y"
    { (yyval.cptr) = merge_text((yyvsp[(1) - (2)].cptr), (yyvsp[(2) - (2)].cptr)); ;}
    break;

  case 35:

/* Line 1464 of yacc.c  */
#line 325 "ppy.y"
    { (yyval.cptr) = merge_text((yyvsp[(1) - (2)].cptr), (yyvsp[(2) - (2)].cptr)); ;}
    break;

  case 36:

/* Line 1464 of yacc.c  */
#line 328 "ppy.y"
    { macro_args = NULL; nmacro_args = 0; ;}
    break;

  case 37:

/* Line 1464 of yacc.c  */
#line 331 "ppy.y"
    { (yyval.sint) = 0; macro_args = NULL; nmacro_args = 0; ;}
    break;

  case 38:

/* Line 1464 of yacc.c  */
#line 332 "ppy.y"
    { (yyval.sint) = nmacro_args; ;}
    break;

  case 39:

/* Line 1464 of yacc.c  */
#line 335 "ppy.y"
    { (yyval.marg) = (yyvsp[(1) - (1)].marg); ;}
    break;

  case 40:

/* Line 1464 of yacc.c  */
#line 336 "ppy.y"
    { (yyval.marg) = add_new_marg(NULL, arg_list); nmacro_args *= -1; ;}
    break;

  case 41:

/* Line 1464 of yacc.c  */
#line 339 "ppy.y"
    { (yyval.marg) = add_new_marg((yyvsp[(3) - (3)].cptr), arg_single); ;}
    break;

  case 42:

/* Line 1464 of yacc.c  */
#line 340 "ppy.y"
    { (yyval.marg) = add_new_marg((yyvsp[(1) - (1)].cptr), arg_single); ;}
    break;

  case 43:

/* Line 1464 of yacc.c  */
#line 344 "ppy.y"
    { (yyval.mtext) = NULL; ;}
    break;

  case 44:

/* Line 1464 of yacc.c  */
#line 345 "ppy.y"
    {
		for((yyval.mtext) = (yyvsp[(1) - (1)].mtext); (yyval.mtext) && (yyval.mtext)->prev; (yyval.mtext) = (yyval.mtext)->prev)
			;
		;}
    break;

  case 45:

/* Line 1464 of yacc.c  */
#line 351 "ppy.y"
    { (yyval.mtext) = (yyvsp[(1) - (1)].mtext); ;}
    break;

  case 46:

/* Line 1464 of yacc.c  */
#line 352 "ppy.y"
    { (yyval.mtext) = combine_mtext((yyvsp[(1) - (2)].mtext), (yyvsp[(2) - (2)].mtext)); ;}
    break;

  case 47:

/* Line 1464 of yacc.c  */
#line 355 "ppy.y"
    { (yyval.mtext) = new_mtext((yyvsp[(1) - (1)].cptr), 0, exp_text); ;}
    break;

  case 48:

/* Line 1464 of yacc.c  */
#line 356 "ppy.y"
    { (yyval.mtext) = new_mtext((yyvsp[(1) - (1)].cptr), 0, exp_text); ;}
    break;

  case 49:

/* Line 1464 of yacc.c  */
#line 357 "ppy.y"
    { (yyval.mtext) = new_mtext((yyvsp[(1) - (1)].cptr), 0, exp_text); ;}
    break;

  case 50:

/* Line 1464 of yacc.c  */
#line 358 "ppy.y"
    { (yyval.mtext) = new_mtext(NULL, 0, exp_concat); ;}
    break;

  case 51:

/* Line 1464 of yacc.c  */
#line 359 "ppy.y"
    {
		int mat = marg_index((yyvsp[(2) - (2)].cptr));
		if(mat < 0)
			ppy_error("Stringification identifier must be an argument parameter");
		else
			(yyval.mtext) = new_mtext(NULL, mat, exp_stringize);
		;}
    break;

  case 52:

/* Line 1464 of yacc.c  */
#line 366 "ppy.y"
    {
		int mat = marg_index((yyvsp[(1) - (1)].cptr));
		if(mat >= 0)
			(yyval.mtext) = new_mtext(NULL, mat, exp_subst);
		else if((yyvsp[(1) - (1)].cptr))
			(yyval.mtext) = new_mtext((yyvsp[(1) - (1)].cptr), 0, exp_text);
		;}
    break;

  case 53:

/* Line 1464 of yacc.c  */
#line 375 "ppy.y"
    { (yyval.cval).type = cv_sint;  (yyval.cval).val.si = (yyvsp[(1) - (1)].sint); ;}
    break;

  case 54:

/* Line 1464 of yacc.c  */
#line 376 "ppy.y"
    { (yyval.cval).type = cv_uint;  (yyval.cval).val.ui = (yyvsp[(1) - (1)].uint); ;}
    break;

  case 55:

/* Line 1464 of yacc.c  */
#line 377 "ppy.y"
    { (yyval.cval).type = cv_slong; (yyval.cval).val.sl = (yyvsp[(1) - (1)].slong); ;}
    break;

  case 56:

/* Line 1464 of yacc.c  */
#line 378 "ppy.y"
    { (yyval.cval).type = cv_ulong; (yyval.cval).val.ul = (yyvsp[(1) - (1)].ulong); ;}
    break;

  case 57:

/* Line 1464 of yacc.c  */
#line 379 "ppy.y"
    { (yyval.cval).type = cv_sll;   (yyval.cval).val.sll = (yyvsp[(1) - (1)].sll); ;}
    break;

  case 58:

/* Line 1464 of yacc.c  */
#line 380 "ppy.y"
    { (yyval.cval).type = cv_ull;   (yyval.cval).val.ull = (yyvsp[(1) - (1)].ull); ;}
    break;

  case 59:

/* Line 1464 of yacc.c  */
#line 381 "ppy.y"
    { (yyval.cval).type = cv_sint;  (yyval.cval).val.si = pplookup((yyvsp[(2) - (2)].cptr)) != NULL; ;}
    break;

  case 60:

/* Line 1464 of yacc.c  */
#line 382 "ppy.y"
    { (yyval.cval).type = cv_sint;  (yyval.cval).val.si = pplookup((yyvsp[(3) - (4)].cptr)) != NULL; ;}
    break;

  case 61:

/* Line 1464 of yacc.c  */
#line 383 "ppy.y"
    { (yyval.cval).type = cv_sint;  (yyval.cval).val.si = 0; ;}
    break;

  case 62:

/* Line 1464 of yacc.c  */
#line 384 "ppy.y"
    { (yyval.cval).type = cv_sint; (yyval.cval).val.si = boolean(&(yyvsp[(1) - (3)].cval)) || boolean(&(yyvsp[(3) - (3)].cval)); ;}
    break;

  case 63:

/* Line 1464 of yacc.c  */
#line 385 "ppy.y"
    { (yyval.cval).type = cv_sint; (yyval.cval).val.si = boolean(&(yyvsp[(1) - (3)].cval)) && boolean(&(yyvsp[(3) - (3)].cval)); ;}
    break;

  case 64:

/* Line 1464 of yacc.c  */
#line 386 "ppy.y"
    { promote_equal_size(&(yyvsp[(1) - (3)].cval), &(yyvsp[(3) - (3)].cval)); BIN_OP((yyval.cval), (yyvsp[(1) - (3)].cval), (yyvsp[(3) - (3)].cval), ==) ;}
    break;

  case 65:

/* Line 1464 of yacc.c  */
#line 387 "ppy.y"
    { promote_equal_size(&(yyvsp[(1) - (3)].cval), &(yyvsp[(3) - (3)].cval)); BIN_OP((yyval.cval), (yyvsp[(1) - (3)].cval), (yyvsp[(3) - (3)].cval), !=) ;}
    break;

  case 66:

/* Line 1464 of yacc.c  */
#line 388 "ppy.y"
    { promote_equal_size(&(yyvsp[(1) - (3)].cval), &(yyvsp[(3) - (3)].cval)); BIN_OP((yyval.cval), (yyvsp[(1) - (3)].cval), (yyvsp[(3) - (3)].cval),  <) ;}
    break;

  case 67:

/* Line 1464 of yacc.c  */
#line 389 "ppy.y"
    { promote_equal_size(&(yyvsp[(1) - (3)].cval), &(yyvsp[(3) - (3)].cval)); BIN_OP((yyval.cval), (yyvsp[(1) - (3)].cval), (yyvsp[(3) - (3)].cval),  >) ;}
    break;

  case 68:

/* Line 1464 of yacc.c  */
#line 390 "ppy.y"
    { promote_equal_size(&(yyvsp[(1) - (3)].cval), &(yyvsp[(3) - (3)].cval)); BIN_OP((yyval.cval), (yyvsp[(1) - (3)].cval), (yyvsp[(3) - (3)].cval), <=) ;}
    break;

  case 69:

/* Line 1464 of yacc.c  */
#line 391 "ppy.y"
    { promote_equal_size(&(yyvsp[(1) - (3)].cval), &(yyvsp[(3) - (3)].cval)); BIN_OP((yyval.cval), (yyvsp[(1) - (3)].cval), (yyvsp[(3) - (3)].cval), >=) ;}
    break;

  case 70:

/* Line 1464 of yacc.c  */
#line 392 "ppy.y"
    { promote_equal_size(&(yyvsp[(1) - (3)].cval), &(yyvsp[(3) - (3)].cval)); BIN_OP((yyval.cval), (yyvsp[(1) - (3)].cval), (yyvsp[(3) - (3)].cval),  +) ;}
    break;

  case 71:

/* Line 1464 of yacc.c  */
#line 393 "ppy.y"
    { promote_equal_size(&(yyvsp[(1) - (3)].cval), &(yyvsp[(3) - (3)].cval)); BIN_OP((yyval.cval), (yyvsp[(1) - (3)].cval), (yyvsp[(3) - (3)].cval),  -) ;}
    break;

  case 72:

/* Line 1464 of yacc.c  */
#line 394 "ppy.y"
    { promote_equal_size(&(yyvsp[(1) - (3)].cval), &(yyvsp[(3) - (3)].cval)); BIN_OP((yyval.cval), (yyvsp[(1) - (3)].cval), (yyvsp[(3) - (3)].cval),  ^) ;}
    break;

  case 73:

/* Line 1464 of yacc.c  */
#line 395 "ppy.y"
    { promote_equal_size(&(yyvsp[(1) - (3)].cval), &(yyvsp[(3) - (3)].cval)); BIN_OP((yyval.cval), (yyvsp[(1) - (3)].cval), (yyvsp[(3) - (3)].cval),  &) ;}
    break;

  case 74:

/* Line 1464 of yacc.c  */
#line 396 "ppy.y"
    { promote_equal_size(&(yyvsp[(1) - (3)].cval), &(yyvsp[(3) - (3)].cval)); BIN_OP((yyval.cval), (yyvsp[(1) - (3)].cval), (yyvsp[(3) - (3)].cval),  |) ;}
    break;

  case 75:

/* Line 1464 of yacc.c  */
#line 397 "ppy.y"
    { promote_equal_size(&(yyvsp[(1) - (3)].cval), &(yyvsp[(3) - (3)].cval)); BIN_OP((yyval.cval), (yyvsp[(1) - (3)].cval), (yyvsp[(3) - (3)].cval),  *) ;}
    break;

  case 76:

/* Line 1464 of yacc.c  */
#line 398 "ppy.y"
    { promote_equal_size(&(yyvsp[(1) - (3)].cval), &(yyvsp[(3) - (3)].cval)); BIN_OP((yyval.cval), (yyvsp[(1) - (3)].cval), (yyvsp[(3) - (3)].cval),  /) ;}
    break;

  case 77:

/* Line 1464 of yacc.c  */
#line 399 "ppy.y"
    { promote_equal_size(&(yyvsp[(1) - (3)].cval), &(yyvsp[(3) - (3)].cval)); BIN_OP((yyval.cval), (yyvsp[(1) - (3)].cval), (yyvsp[(3) - (3)].cval), <<) ;}
    break;

  case 78:

/* Line 1464 of yacc.c  */
#line 400 "ppy.y"
    { promote_equal_size(&(yyvsp[(1) - (3)].cval), &(yyvsp[(3) - (3)].cval)); BIN_OP((yyval.cval), (yyvsp[(1) - (3)].cval), (yyvsp[(3) - (3)].cval), >>) ;}
    break;

  case 79:

/* Line 1464 of yacc.c  */
#line 401 "ppy.y"
    { (yyval.cval) =  (yyvsp[(2) - (2)].cval); ;}
    break;

  case 80:

/* Line 1464 of yacc.c  */
#line 402 "ppy.y"
    { UNARY_OP((yyval.cval), (yyvsp[(2) - (2)].cval), -) ;}
    break;

  case 81:

/* Line 1464 of yacc.c  */
#line 403 "ppy.y"
    { UNARY_OP((yyval.cval), (yyvsp[(2) - (2)].cval), ~) ;}
    break;

  case 82:

/* Line 1464 of yacc.c  */
#line 404 "ppy.y"
    { (yyval.cval).type = cv_sint; (yyval.cval).val.si = !boolean(&(yyvsp[(2) - (2)].cval)); ;}
    break;

  case 83:

/* Line 1464 of yacc.c  */
#line 405 "ppy.y"
    { (yyval.cval) =  (yyvsp[(2) - (3)].cval); ;}
    break;

  case 84:

/* Line 1464 of yacc.c  */
#line 406 "ppy.y"
    { (yyval.cval) = boolean(&(yyvsp[(1) - (5)].cval)) ? (yyvsp[(3) - (5)].cval) : (yyvsp[(5) - (5)].cval); ;}
    break;



/* Line 1464 of yacc.c  */
#line 2333 "ppy.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
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
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
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

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
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


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
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

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1684 of yacc.c  */
#line 409 "ppy.y"


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
	if(!ma)
		return NULL;
	ma->arg = str;
	ma->type = type;
	ma->nnl = 0;
	return ma;
}

static marg_t *add_new_marg(char *str, def_arg_t type)
{
	marg_t **new_macro_args;
	marg_t *ma;
	if(!str)
		return NULL;
	new_macro_args = pp_xrealloc(macro_args, (nmacro_args+1) * sizeof(macro_args[0]));
	if(!new_macro_args)
		return NULL;
	macro_args = new_macro_args;
	ma = new_marg(str, type);
	if(!ma)
		return NULL;
	macro_args[nmacro_args] = ma;
	nmacro_args++;
	return ma;
}

static int marg_index(char *id)
{
	int t;
	if(!id)
		return -1;
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
	if(!mt)
		return NULL;
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
		char *new_text;
		new_text = pp_xrealloc(tail->subst.text, strlen(tail->subst.text)+strlen(mtp->subst.text)+1);
		if(!new_text)
			return mtp;
		tail->subst.text = new_text;
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
	int l1;
	int l2;
	char *snew;
	if(!s1)
		return s2;
	if(!s2)
		return s1;
	l1 = strlen(s1);
	l2 = strlen(s2);
	snew = pp_xrealloc(s1, l1+l2+1);
	if(!snew)
	{
		free(s2);
		return s1;
	}
	s1 = snew;
	memcpy(s1+l1, s2, l2+1);
	free(s2);
	return s1;
}

