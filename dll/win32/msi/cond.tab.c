/* A Bison parser, made by GNU Bison 3.1.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018 Free Software Foundation, Inc.

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
#define YYBISON_VERSION "3.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         cond_parse
#define yylex           cond_lex
#define yyerror         cond_error
#define yydebug         cond_debug
#define yynerrs         cond_nerrs


/* Copy the first part of user declarations.  */
#line 1 "cond.y" /* yacc.c:339  */


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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define COBJMACROS

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "msi.h"
#include "msiquery.h"
#include "objbase.h"
#include "oleauto.h"

#include "msipriv.h"
#include "msiserver.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

typedef struct tag_yyinput
{
    MSIPACKAGE *package;
    LPCWSTR str;
    INT    n;
    MSICONDITION result;
    struct list mem;
} COND_input;

struct cond_str {
    LPCWSTR data;
    INT len;
};

struct value {
    enum value_type {
        VALUE_INTEGER,
        VALUE_LITERAL,
        VALUE_SYMBOL
    } type;
    union {
        INT integer;
        WCHAR *string;
    } u;
};

static LPWSTR COND_GetString( COND_input *info, const struct cond_str *str );
static LPWSTR COND_GetLiteral( COND_input *info, const struct cond_str *str );
static int cond_lex( void *COND_lval, COND_input *info);
static int cond_error( COND_input *info, const char *str);

static void *cond_alloc( COND_input *cond, unsigned int sz );
static void *cond_track_mem( COND_input *cond, void *ptr, unsigned int sz );
static void cond_free( void *ptr );

static INT compare_int( INT a, INT operator, INT b );
static INT compare_string( LPCWSTR a, INT operator, LPCWSTR b, BOOL convert );

static BOOL num_from_prop( LPCWSTR p, INT *val )
{
    INT ret = 0, sign = 1;

    if (!p)
        return FALSE;
    if (*p == '-')
    {
        sign = -1;
        p++;
    }
    if (!*p)
        return FALSE;
    while (*p)
    {
        if( *p < '0' || *p > '9' )
            return FALSE;
        ret = ret*10 + (*p - '0');
        p++;
    }
    *val = ret*sign;
    return TRUE;
}

static void value_free( struct value val )
{
    if (val.type != VALUE_INTEGER)
        cond_free( val.u.string );
}


#line 188 "cond.tab.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif


/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int cond_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    COND_SPACE = 258,
    COND_EOF = 259,
    COND_OR = 260,
    COND_AND = 261,
    COND_NOT = 262,
    COND_XOR = 263,
    COND_IMP = 264,
    COND_EQV = 265,
    COND_LT = 266,
    COND_GT = 267,
    COND_EQ = 268,
    COND_NE = 269,
    COND_GE = 270,
    COND_LE = 271,
    COND_ILT = 272,
    COND_IGT = 273,
    COND_IEQ = 274,
    COND_INE = 275,
    COND_IGE = 276,
    COND_ILE = 277,
    COND_LPAR = 278,
    COND_RPAR = 279,
    COND_TILDA = 280,
    COND_SS = 281,
    COND_ISS = 282,
    COND_ILHS = 283,
    COND_IRHS = 284,
    COND_LHS = 285,
    COND_RHS = 286,
    COND_PERCENT = 287,
    COND_DOLLARS = 288,
    COND_QUESTION = 289,
    COND_AMPER = 290,
    COND_EXCLAM = 291,
    COND_IDENT = 292,
    COND_NUMBER = 293,
    COND_LITER = 294,
    COND_ERROR = 295
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 122 "cond.y" /* yacc.c:355  */

    struct cond_str str;
    struct value value;
    LPWSTR identifier;
    INT operator;
    BOOL bool;

#line 274 "cond.tab.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int cond_parse (COND_input *info);



/* Copy the second part of user declarations.  */

#line 290 "cond.tab.c" /* yacc.c:358  */

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
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
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
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
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

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  24
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   67

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  41
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  8
/* YYNRULES -- Number of rules.  */
#define YYNRULES  41
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  56

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   295

#define YYTRANSLATE(YYX)                                                \
  ((unsigned) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
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
      35,    36,    37,    38,    39,    40
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   149,   149,   155,   162,   166,   170,   174,   178,   185,
     189,   196,   200,   208,   243,   251,   252,   253,   254,   255,
     256,   257,   258,   259,   260,   261,   262,   263,   264,   265,
     266,   267,   268,   272,   286,   301,   309,   319,   336,   353,
     370,   390
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "COND_SPACE", "COND_EOF", "COND_OR",
  "COND_AND", "COND_NOT", "COND_XOR", "COND_IMP", "COND_EQV", "COND_LT",
  "COND_GT", "COND_EQ", "COND_NE", "COND_GE", "COND_LE", "COND_ILT",
  "COND_IGT", "COND_IEQ", "COND_INE", "COND_IGE", "COND_ILE", "COND_LPAR",
  "COND_RPAR", "COND_TILDA", "COND_SS", "COND_ISS", "COND_ILHS",
  "COND_IRHS", "COND_LHS", "COND_RHS", "COND_PERCENT", "COND_DOLLARS",
  "COND_QUESTION", "COND_AMPER", "COND_EXCLAM", "COND_IDENT",
  "COND_NUMBER", "COND_LITER", "COND_ERROR", "$accept", "condition",
  "expression", "boolean_term", "boolean_factor", "operator", "value",
  "identifier", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295
};
# endif

#define YYPACT_NINF -45

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-45)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int8 yypact[] =
{
      -7,    -7,    -7,   -36,   -36,   -36,   -36,   -36,   -45,   -45,
     -45,     2,     5,     3,   -45,    23,   -45,   -45,    -2,   -45,
     -45,   -45,   -45,   -45,   -45,    -7,    -7,    -7,    -7,    -7,
     -45,   -45,   -45,   -45,   -45,   -45,   -45,   -45,   -45,   -45,
     -45,   -45,   -45,   -45,   -45,   -45,   -45,   -45,    24,   -45,
       3,     3,     3,     3,   -45,   -45
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       3,     0,     0,     0,     0,     0,     0,     0,    41,    36,
      35,     0,     2,     4,     9,    12,    33,    11,     0,    34,
      37,    38,    39,    40,     1,     0,     0,     0,     0,     0,
      17,    18,    15,    16,    20,    19,    24,    25,    22,    23,
      27,    26,    21,    28,    31,    32,    29,    30,     0,    14,
       5,     7,     6,     8,    10,    13
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -45,   -45,     9,    39,     4,   -45,   -44,    14
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,    11,    12,    13,    14,    48,    15,    16
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
       1,     8,    24,    25,    55,    17,    26,    27,    28,    29,
      25,    18,     0,    26,    27,    28,     2,    19,    20,    21,
      22,    23,    49,     0,     0,     3,     4,     5,     6,     7,
       8,     9,    10,    54,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,     0,     0,     0,    42,
      43,    44,    45,    46,    47,     0,     3,     4,     5,     6,
       7,     8,     9,    10,    50,    51,    52,    53
};

static const yytype_int8 yycheck[] =
{
       7,    37,     0,     5,    48,     1,     8,     9,    10,     6,
       5,     2,    -1,     8,     9,    10,    23,     3,     4,     5,
       6,     7,    24,    -1,    -1,    32,    33,    34,    35,    36,
      37,    38,    39,    29,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    -1,    -1,    -1,    26,
      27,    28,    29,    30,    31,    -1,    32,    33,    34,    35,
      36,    37,    38,    39,    25,    26,    27,    28
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     7,    23,    32,    33,    34,    35,    36,    37,    38,
      39,    42,    43,    44,    45,    47,    48,    45,    43,    48,
      48,    48,    48,    48,     0,     5,     8,     9,    10,     6,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    26,    27,    28,    29,    30,    31,    46,    24,
      44,    44,    44,    44,    45,    47
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    41,    42,    42,    43,    43,    43,    43,    43,    44,
      44,    45,    45,    45,    45,    46,    46,    46,    46,    46,
      46,    46,    46,    46,    46,    46,    46,    46,    46,    46,
      46,    46,    46,    47,    47,    47,    47,    47,    47,    47,
      47,    48
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     0,     1,     3,     3,     3,     3,     1,
       3,     2,     1,     3,     3,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     2,     1,     1,     2,     2,     2,
       2,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (info, YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value, info); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, COND_input *info)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  YYUSE (info);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, COND_input *info)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, info);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule, COND_input *info)
{
  unsigned long yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              , info);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, info); \
} while (0)

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
#ifndef YYINITDEPTH
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
static YYSIZE_T
yystrlen (const char *yystr)
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
static char *
yystpcpy (char *yydest, const char *yysrc)
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

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, COND_input *info)
{
  YYUSE (yyvaluep);
  YYUSE (info);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/*----------.
| yyparse.  |
`----------*/

int
yyparse (COND_input *info)
{
/* The lookahead symbol.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
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
  int yytoken = 0;
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

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
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
                  (unsigned long) yystacksize));

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
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex (&yylval, info);
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
      if (yytable_value_is_error (yyn))
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
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

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
     '$$ = $1'.

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
#line 150 "cond.y" /* yacc.c:1651  */
    {
            COND_input* cond = (COND_input*) info;
            cond->result = (yyvsp[0].bool);
        }
#line 1420 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 3:
#line 155 "cond.y" /* yacc.c:1651  */
    {
            COND_input* cond = (COND_input*) info;
            cond->result = MSICONDITION_NONE;
        }
#line 1429 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 4:
#line 163 "cond.y" /* yacc.c:1651  */
    {
            (yyval.bool) = (yyvsp[0].bool);
        }
#line 1437 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 5:
#line 167 "cond.y" /* yacc.c:1651  */
    {
            (yyval.bool) = (yyvsp[-2].bool) || (yyvsp[0].bool);
        }
#line 1445 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 6:
#line 171 "cond.y" /* yacc.c:1651  */
    {
            (yyval.bool) = !(yyvsp[-2].bool) || (yyvsp[0].bool);
        }
#line 1453 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 7:
#line 175 "cond.y" /* yacc.c:1651  */
    {
            (yyval.bool) = ( (yyvsp[-2].bool) || (yyvsp[0].bool) ) && !( (yyvsp[-2].bool) && (yyvsp[0].bool) );
        }
#line 1461 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 8:
#line 179 "cond.y" /* yacc.c:1651  */
    {
            (yyval.bool) = ( (yyvsp[-2].bool) && (yyvsp[0].bool) ) || ( !(yyvsp[-2].bool) && !(yyvsp[0].bool) );
        }
#line 1469 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 9:
#line 186 "cond.y" /* yacc.c:1651  */
    {
            (yyval.bool) = (yyvsp[0].bool);
        }
#line 1477 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 10:
#line 190 "cond.y" /* yacc.c:1651  */
    {
            (yyval.bool) = (yyvsp[-2].bool) && (yyvsp[0].bool);
        }
#line 1485 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 11:
#line 197 "cond.y" /* yacc.c:1651  */
    {
            (yyval.bool) = !(yyvsp[0].bool);
        }
#line 1493 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 12:
#line 201 "cond.y" /* yacc.c:1651  */
    {
            if ((yyvsp[0].value).type == VALUE_INTEGER)
                (yyval.bool) = (yyvsp[0].value).u.integer ? 1 : 0;
            else
                (yyval.bool) = (yyvsp[0].value).u.string && (yyvsp[0].value).u.string[0];
            value_free( (yyvsp[0].value) );
        }
#line 1505 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 13:
#line 209 "cond.y" /* yacc.c:1651  */
    {
            if ((yyvsp[-2].value).type == VALUE_INTEGER && (yyvsp[0].value).type == VALUE_INTEGER)
            {
                (yyval.bool) = compare_int((yyvsp[-2].value).u.integer, (yyvsp[-1].operator), (yyvsp[0].value).u.integer);
            }
            else if ((yyvsp[-2].value).type != VALUE_INTEGER && (yyvsp[0].value).type != VALUE_INTEGER)
            {
                (yyval.bool) = compare_string((yyvsp[-2].value).u.string, (yyvsp[-1].operator), (yyvsp[0].value).u.string,
                        (yyvsp[-2].value).type == VALUE_SYMBOL || (yyvsp[0].value).type == VALUE_SYMBOL);
            }
            else if ((yyvsp[-2].value).type == VALUE_LITERAL || (yyvsp[0].value).type == VALUE_LITERAL)
            {
                (yyval.bool) = ((yyvsp[-1].operator) == COND_NE || (yyvsp[-1].operator) == COND_INE );
            }
            else if ((yyvsp[-2].value).type == VALUE_SYMBOL) /* symbol operator integer */
            {
                int num;
                if (num_from_prop( (yyvsp[-2].value).u.string, &num ))
                    (yyval.bool) = compare_int( num, (yyvsp[-1].operator), (yyvsp[0].value).u.integer );
                else
                    (yyval.bool) = ((yyvsp[-1].operator) == COND_NE || (yyvsp[-1].operator) == COND_INE );
            }
            else /* integer operator symbol */
            {
                int num;
                if (num_from_prop( (yyvsp[0].value).u.string, &num ))
                    (yyval.bool) = compare_int( (yyvsp[-2].value).u.integer, (yyvsp[-1].operator), num );
                else
                    (yyval.bool) = ((yyvsp[-1].operator) == COND_NE || (yyvsp[-1].operator) == COND_INE );
            }

            value_free( (yyvsp[-2].value) );
            value_free( (yyvsp[0].value) );
        }
#line 1544 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 14:
#line 244 "cond.y" /* yacc.c:1651  */
    {
            (yyval.bool) = (yyvsp[-1].bool);
        }
#line 1552 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 15:
#line 251 "cond.y" /* yacc.c:1651  */
    { (yyval.operator) = COND_EQ; }
#line 1558 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 16:
#line 252 "cond.y" /* yacc.c:1651  */
    { (yyval.operator) = COND_NE; }
#line 1564 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 17:
#line 253 "cond.y" /* yacc.c:1651  */
    { (yyval.operator) = COND_LT; }
#line 1570 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 18:
#line 254 "cond.y" /* yacc.c:1651  */
    { (yyval.operator) = COND_GT; }
#line 1576 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 19:
#line 255 "cond.y" /* yacc.c:1651  */
    { (yyval.operator) = COND_LE; }
#line 1582 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 20:
#line 256 "cond.y" /* yacc.c:1651  */
    { (yyval.operator) = COND_GE; }
#line 1588 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 21:
#line 257 "cond.y" /* yacc.c:1651  */
    { (yyval.operator) = COND_SS; }
#line 1594 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 22:
#line 258 "cond.y" /* yacc.c:1651  */
    { (yyval.operator) = COND_IEQ; }
#line 1600 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 23:
#line 259 "cond.y" /* yacc.c:1651  */
    { (yyval.operator) = COND_INE; }
#line 1606 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 24:
#line 260 "cond.y" /* yacc.c:1651  */
    { (yyval.operator) = COND_ILT; }
#line 1612 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 25:
#line 261 "cond.y" /* yacc.c:1651  */
    { (yyval.operator) = COND_IGT; }
#line 1618 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 26:
#line 262 "cond.y" /* yacc.c:1651  */
    { (yyval.operator) = COND_ILE; }
#line 1624 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 27:
#line 263 "cond.y" /* yacc.c:1651  */
    { (yyval.operator) = COND_IGE; }
#line 1630 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 28:
#line 264 "cond.y" /* yacc.c:1651  */
    { (yyval.operator) = COND_ISS; }
#line 1636 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 29:
#line 265 "cond.y" /* yacc.c:1651  */
    { (yyval.operator) = COND_LHS; }
#line 1642 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 30:
#line 266 "cond.y" /* yacc.c:1651  */
    { (yyval.operator) = COND_RHS; }
#line 1648 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 31:
#line 267 "cond.y" /* yacc.c:1651  */
    { (yyval.operator) = COND_ILHS; }
#line 1654 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 32:
#line 268 "cond.y" /* yacc.c:1651  */
    { (yyval.operator) = COND_IRHS; }
#line 1660 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 33:
#line 273 "cond.y" /* yacc.c:1651  */
    {
            COND_input* cond = (COND_input*) info;
            UINT len;

            (yyval.value).type = VALUE_SYMBOL;
            (yyval.value).u.string = msi_dup_property( cond->package->db, (yyvsp[0].identifier) );
            if ((yyval.value).u.string)
            {
                len = (lstrlenW((yyval.value).u.string) + 1) * sizeof (WCHAR);
                (yyval.value).u.string = cond_track_mem( cond, (yyval.value).u.string, len );
            }
            cond_free( (yyvsp[0].identifier) );
        }
#line 1678 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 34:
#line 287 "cond.y" /* yacc.c:1651  */
    {
            COND_input* cond = (COND_input*) info;
            UINT len = GetEnvironmentVariableW( (yyvsp[0].identifier), NULL, 0 );
            (yyval.value).type = VALUE_SYMBOL;
            (yyval.value).u.string = NULL;
            if (len++)
            {
                (yyval.value).u.string = cond_alloc( cond, len*sizeof (WCHAR) );
                if( !(yyval.value).u.string )
                    YYABORT;
                GetEnvironmentVariableW( (yyvsp[0].identifier), (yyval.value).u.string, len );
            }
            cond_free( (yyvsp[0].identifier) );
        }
#line 1697 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 35:
#line 302 "cond.y" /* yacc.c:1651  */
    {
            COND_input* cond = (COND_input*) info;
            (yyval.value).type = VALUE_LITERAL;
            (yyval.value).u.string = COND_GetLiteral( cond, &(yyvsp[0].str) );
            if( !(yyval.value).u.string )
                YYABORT;
        }
#line 1709 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 36:
#line 310 "cond.y" /* yacc.c:1651  */
    {
            COND_input* cond = (COND_input*) info;
            LPWSTR szNum = COND_GetString( cond, &(yyvsp[0].str) );
            if( !szNum )
                YYABORT;
            (yyval.value).type = VALUE_INTEGER;
            (yyval.value).u.integer = atoiW( szNum );
            cond_free( szNum );
        }
#line 1723 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 37:
#line 320 "cond.y" /* yacc.c:1651  */
    {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;
      
            if(MSI_GetComponentStateW(cond->package, (yyvsp[0].identifier), &install, &action ) != ERROR_SUCCESS)
            {
                (yyval.value).type = VALUE_LITERAL;
                (yyval.value).u.string = NULL;
            }
            else
            {
                (yyval.value).type = VALUE_INTEGER;
                (yyval.value).u.integer = action;
            }
            cond_free( (yyvsp[0].identifier) );
        }
#line 1744 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 38:
#line 337 "cond.y" /* yacc.c:1651  */
    {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;
      
            if(MSI_GetComponentStateW(cond->package, (yyvsp[0].identifier), &install, &action ) != ERROR_SUCCESS)
            {
                (yyval.value).type = VALUE_LITERAL;
                (yyval.value).u.string = NULL;
            }
            else
            {
                (yyval.value).type = VALUE_INTEGER;
                (yyval.value).u.integer = install;
            }
            cond_free( (yyvsp[0].identifier) );
        }
#line 1765 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 39:
#line 354 "cond.y" /* yacc.c:1651  */
    {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install, action;
      
            if (MSI_GetFeatureStateW(cond->package, (yyvsp[0].identifier), &install, &action ) != ERROR_SUCCESS)
            {
                (yyval.value).type = VALUE_LITERAL;
                (yyval.value).u.string = NULL;
            }
            else
            {
                (yyval.value).type = VALUE_INTEGER;
                (yyval.value).u.integer = action;
            }
            cond_free( (yyvsp[0].identifier) );
        }
#line 1786 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 40:
#line 371 "cond.y" /* yacc.c:1651  */
    {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;
      
            if(MSI_GetFeatureStateW(cond->package, (yyvsp[0].identifier), &install, &action ) != ERROR_SUCCESS)
            {
                (yyval.value).type = VALUE_LITERAL;
                (yyval.value).u.string = NULL;
            }
            else
            {
                (yyval.value).type = VALUE_INTEGER;
                (yyval.value).u.integer = install;
            }
            cond_free( (yyvsp[0].identifier) );
        }
#line 1807 "cond.tab.c" /* yacc.c:1651  */
    break;

  case 41:
#line 391 "cond.y" /* yacc.c:1651  */
    {
            COND_input* cond = (COND_input*) info;
            (yyval.identifier) = COND_GetString( cond, &(yyvsp[0].str) );
            if( !(yyval.identifier) )
                YYABORT;
        }
#line 1818 "cond.tab.c" /* yacc.c:1651  */
    break;


#line 1822 "cond.tab.c" /* yacc.c:1651  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (info, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (info, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
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
                      yytoken, &yylval, info);
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

  /* Do not reclaim the symbols of the rule whose action triggered
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
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
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
                  yystos[yystate], yyvsp, info);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


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

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (info, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, info);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, info);
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
  return yyresult;
}
#line 399 "cond.y" /* yacc.c:1910  */



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

static WCHAR *strstriW( const WCHAR *str, const WCHAR *sub )
{
    LPWSTR strlower, sublower, r;
    strlower = CharLowerW( strdupW( str ) );
    sublower = CharLowerW( strdupW( sub ) );
    r = strstrW( strlower, sublower );
    if (r)
        r = (LPWSTR)str + (r - strlower);
    msi_free( strlower );
    msi_free( sublower );
    return r;
}

static BOOL str_is_number( LPCWSTR str )
{
    int i;

    if (!*str)
        return FALSE;

    for (i = 0; i < lstrlenW( str ); i++)
        if (!isdigitW(str[i]))
            return FALSE;

    return TRUE;
}

static INT compare_substring( LPCWSTR a, INT operator, LPCWSTR b )
{
    int lhs, rhs;

    /* substring operators return 0 if LHS is missing */
    if (!a || !*a)
        return 0;

    /* substring operators return 1 if RHS is missing */
    if (!b || !*b)
        return 1;

    /* if both strings contain only numbers, use integer comparison */
    lhs = atoiW(a);
    rhs = atoiW(b);
    if (str_is_number(a) && str_is_number(b))
        return compare_int( lhs, operator, rhs );

    switch (operator)
    {
    case COND_SS:
        return strstrW( a, b ) != 0;
    case COND_ISS:
        return strstriW( a, b ) != 0;
    case COND_LHS:
    {
        int l = strlenW( a );
        int r = strlenW( b );
        if (r > l) return 0;
        return !strncmpW( a, b, r );
    }
    case COND_RHS:
    {
        int l = strlenW( a );
        int r = strlenW( b );
        if (r > l) return 0;
        return !strncmpW( a + (l - r), b, r );
    }
    case COND_ILHS:
    {
        int l = strlenW( a );
        int r = strlenW( b );
        if (r > l) return 0;
        return !strncmpiW( a, b, r );
    }
    case COND_IRHS:
    {
        int l = strlenW( a );
        int r = strlenW( b );
        if (r > l) return 0;
        return !strncmpiW( a + (l - r), b, r );
    }
    default:
        ERR("invalid substring operator\n");
        return 0;
    }
    return 0;
}

static INT compare_string( LPCWSTR a, INT operator, LPCWSTR b, BOOL convert )
{
    if (operator >= COND_SS && operator <= COND_RHS)
        return compare_substring( a, operator, b );

    /* null and empty string are equivalent */
    if (!a) a = szEmpty;
    if (!b) b = szEmpty;

    if (convert && str_is_number(a) && str_is_number(b))
        return compare_int( atoiW(a), operator, atoiW(b) );

    /* a or b may be NULL */
    switch (operator)
    {
    case COND_LT:
        return strcmpW( a, b ) < 0;
    case COND_GT:
        return strcmpW( a, b ) > 0;
    case COND_EQ:
        return strcmpW( a, b ) == 0;
    case COND_NE:
        return strcmpW( a, b ) != 0;
    case COND_GE:
        return strcmpW( a, b ) >= 0;
    case COND_LE:
        return strcmpW( a, b ) <= 0;
    case COND_ILT:
        return strcmpiW( a, b ) < 0;
    case COND_IGT:
        return strcmpiW( a, b ) > 0;
    case COND_IEQ:
        return strcmpiW( a, b ) == 0;
    case COND_INE:
        return strcmpiW( a, b ) != 0;
    case COND_IGE:
        return strcmpiW( a, b ) >= 0;
    case COND_ILE:
        return strcmpiW( a, b ) <= 0;
    default:
        ERR("invalid string operator\n");
        return 0;
    }
    return 0;
}


static INT compare_int( INT a, INT operator, INT b )
{
    switch (operator)
    {
    case COND_LT:
    case COND_ILT:
        return a < b;
    case COND_GT:
    case COND_IGT:
        return a > b;
    case COND_EQ:
    case COND_IEQ:
        return a == b;
    case COND_NE:
    case COND_INE:
        return a != b;
    case COND_GE:
    case COND_IGE:
        return a >= b;
    case COND_LE:
    case COND_ILE:
        return a <= b;
    case COND_SS:
    case COND_ISS:
        return ( a & b ) ? 1 : 0;
    case COND_RHS:
        return ( ( a & 0xffff ) == b ) ? 1 : 0;
    case COND_LHS:
        return ( ( (a>>16) & 0xffff ) == b ) ? 1 : 0;
    default:
        ERR("invalid integer operator\n");
        return 0;
    }
    return 0;
}


static int COND_IsIdent( WCHAR x )
{
    return( COND_IsAlpha( x ) || COND_IsNumber( x ) || ( x == '_' ) 
            || ( x == '#' ) || (x == '.') );
}

static int COND_GetOperator( COND_input *cond )
{
    static const struct {
        const WCHAR str[4];
        int id;
    } table[] = {
        { {'~','<','=',0}, COND_ILE },
        { {'~','>','<',0}, COND_ISS },
        { {'~','>','>',0}, COND_IRHS },
        { {'~','<','>',0}, COND_INE },
        { {'~','>','=',0}, COND_IGE },
        { {'~','<','<',0}, COND_ILHS },
        { {'~','=',0},     COND_IEQ },
        { {'~','<',0},     COND_ILT },
        { {'~','>',0},     COND_IGT },
        { {'>','=',0},     COND_GE  },
        { {'>','<',0},     COND_SS  },
        { {'<','<',0},     COND_LHS },
        { {'<','>',0},     COND_NE  },
        { {'<','=',0},     COND_LE  },
        { {'>','>',0},     COND_RHS },
        { {'>',0},         COND_GT  },
        { {'<',0},         COND_LT  },
        { {0},             0        }
    };
    LPCWSTR p = &cond->str[cond->n];
    int i = 0, len;

    while ( 1 )
    {
        len = lstrlenW( table[i].str );
        if ( !len || 0 == strncmpW( table[i].str, p, len ) )
            break;
        i++;
    }
    cond->n += len;
    return table[i].id;
}

static int COND_GetOne( struct cond_str *str, COND_input *cond )
{
    int rc, len = 1;
    WCHAR ch;

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

    case '~':
    case '<':
    case '>':
        rc = COND_GetOperator( cond );
        if (!rc)
            rc = COND_ERROR;
        return rc;
    default:
        rc = 0;
    }

    if ( rc )
    {
        cond->n += len;
        return rc;
    }

    if (ch == '"' )
    {
        LPCWSTR p = strchrW( str->data + 1, '"' );
        if (!p) return COND_ERROR;
        len = p - str->data + 1;
        rc = COND_LITER;
    }
    else if( COND_IsAlpha( ch ) )
    {
        static const WCHAR szNot[] = {'N','O','T',0};
        static const WCHAR szAnd[] = {'A','N','D',0};
        static const WCHAR szXor[] = {'X','O','R',0};
        static const WCHAR szEqv[] = {'E','Q','V',0};
        static const WCHAR szImp[] = {'I','M','P',0};
        static const WCHAR szOr[] = {'O','R',0};

        while( COND_IsIdent( str->data[len] ) )
            len++;
        rc = COND_IDENT;

        if ( len == 3 )
        {
            if ( !strncmpiW( str->data, szNot, len ) )
                rc = COND_NOT;
            else if( !strncmpiW( str->data, szAnd, len ) )
                rc = COND_AND;
            else if( !strncmpiW( str->data, szXor, len ) )
                rc = COND_XOR;
            else if( !strncmpiW( str->data, szEqv, len ) )
                rc = COND_EQV;
            else if( !strncmpiW( str->data, szImp, len ) )
                rc = COND_IMP;
        }
        else if( (len == 2) && !strncmpiW( str->data, szOr, len ) )
            rc = COND_OR;
    }
    else if( COND_IsNumber( ch ) )
    {
        while( COND_IsNumber( str->data[len] ) )
            len++;
        rc = COND_NUMBER;
    }
    else
    {
        ERR("Got unknown character %c(%x)\n",ch,ch);
        return COND_ERROR;
    }

    cond->n += len;
    str->len = len;

    return rc;
}

static int cond_lex( void *COND_lval, COND_input *cond )
{
    int rc;
    struct cond_str *str = COND_lval;

    do {
        rc = COND_GetOne( str, cond );
    } while (rc == COND_SPACE);
    
    return rc;
}

static LPWSTR COND_GetString( COND_input *cond, const struct cond_str *str )
{
    LPWSTR ret;

    ret = cond_alloc( cond, (str->len+1) * sizeof (WCHAR) );
    if( ret )
    {
        memcpy( ret, str->data, str->len * sizeof(WCHAR));
        ret[str->len]=0;
    }
    TRACE("Got identifier %s\n",debugstr_w(ret));
    return ret;
}

static LPWSTR COND_GetLiteral( COND_input *cond, const struct cond_str *str )
{
    LPWSTR ret;

    ret = cond_alloc( cond, (str->len-1) * sizeof (WCHAR) );
    if( ret )
    {
        memcpy( ret, str->data+1, (str->len-2) * sizeof(WCHAR) );
        ret[str->len - 2]=0;
    }
    TRACE("Got literal %s\n",debugstr_w(ret));
    return ret;
}

static void *cond_alloc( COND_input *cond, unsigned int sz )
{
    struct list *mem;

    mem = msi_alloc( sizeof (struct list) + sz );
    if( !mem )
        return NULL;

    list_add_head( &(cond->mem), mem );
    return mem + 1;
}

static void *cond_track_mem( COND_input *cond, void *ptr, unsigned int sz )
{
    void *new_ptr;

    if( !ptr )
        return ptr;

    new_ptr = cond_alloc( cond, sz );
    if( !new_ptr )
    {
        msi_free( ptr );
        return NULL;
    }

    memcpy( new_ptr, ptr, sz );
    msi_free( ptr );
    return new_ptr;
}

static void cond_free( void *ptr )
{
    struct list *mem = (struct list *)ptr - 1;

    if( ptr )
    {
        list_remove( mem );
        msi_free( mem );
    }
}

static int cond_error( COND_input *info, const char *str )
{
    TRACE("%s\n", str );
    return 0;
}

MSICONDITION MSI_EvaluateConditionW( MSIPACKAGE *package, LPCWSTR szCondition )
{
    COND_input cond;
    MSICONDITION r;
    struct list *mem, *safety;

    TRACE("%s\n", debugstr_w( szCondition ) );

    if (szCondition == NULL) return MSICONDITION_NONE;

    cond.package = package;
    cond.str   = szCondition;
    cond.n     = 0;
    cond.result = MSICONDITION_ERROR;

    list_init( &cond.mem );

    if ( !cond_parse( &cond ) )
        r = cond.result;
    else
        r = MSICONDITION_ERROR;

    LIST_FOR_EACH_SAFE( mem, safety, &cond.mem )
    {
        /* The tracked memory lives directly after the list struct */
        void *ptr = mem + 1;
        if ( r != MSICONDITION_ERROR )
            WARN( "condition parser failed to free up some memory: %p\n", ptr );
        cond_free( ptr );
    }

    TRACE("%i <- %s\n", r, debugstr_w(szCondition));
    return r;
}

MSICONDITION WINAPI MsiEvaluateConditionW( MSIHANDLE hInstall, LPCWSTR szCondition )
{
    MSIPACKAGE *package;
    UINT ret;

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE);
    if( !package )
    {
        HRESULT hr;
        BSTR condition;
        IWineMsiRemotePackage *remote_package;

        remote_package = (IWineMsiRemotePackage *)msi_get_remote( hInstall );
        if (!remote_package)
            return MSICONDITION_ERROR;

        condition = SysAllocString( szCondition );
        if (!condition)
        {
            IWineMsiRemotePackage_Release( remote_package );
            return ERROR_OUTOFMEMORY;
        }

        hr = IWineMsiRemotePackage_EvaluateCondition( remote_package, condition );

        SysFreeString( condition );
        IWineMsiRemotePackage_Release( remote_package );

        if (FAILED(hr))
        {
            if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
                return HRESULT_CODE(hr);

            return ERROR_FUNCTION_FAILED;
        }

        return ERROR_SUCCESS;
    }

    ret = MSI_EvaluateConditionW( package, szCondition );
    msiobj_release( &package->hdr );
    return ret;
}

MSICONDITION WINAPI MsiEvaluateConditionA( MSIHANDLE hInstall, LPCSTR szCondition )
{
    LPWSTR szwCond = NULL;
    MSICONDITION r;

    szwCond = strdupAtoW( szCondition );
    if( szCondition && !szwCond )
        return MSICONDITION_ERROR;

    r = MsiEvaluateConditionW( hInstall, szwCond );
    msi_free( szwCond );
    return r;
}
