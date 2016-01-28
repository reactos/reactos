/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

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
#define YYBISON_VERSION "3.0.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         wql_parse
#define yylex           wql_lex
#define yyerror         wql_error
#define yydebug         wql_debug
#define yynerrs         wql_nerrs


/* Copy the first part of user declarations.  */
#line 1 "wql.y" /* yacc.c:339  */


/*
 * Copyright 2012 Hans Leidekker for CodeWeavers
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

#include "wbemprox_private.h"

struct parser
{
    const WCHAR *cmd;
    UINT idx;
    UINT len;
    HRESULT error;
    struct view **view;
    struct list *mem;
};

struct string
{
    const WCHAR *data;
    int len;
};

static void *alloc_mem( struct parser *parser, UINT size )
{
    struct list *mem = heap_alloc( sizeof(struct list) + size );
    list_add_tail( parser->mem, mem );
    return &mem[1];
}

static struct property *alloc_property( struct parser *parser, const WCHAR *class, const WCHAR *name )
{
    struct property *prop = alloc_mem( parser, sizeof(*prop) );
    if (prop)
    {
        prop->name  = name;
        prop->class = class;
        prop->next  = NULL;
    }
    return prop;
}

static WCHAR *get_string( struct parser *parser, const struct string *str )
{
    const WCHAR *p = str->data;
    int len = str->len;
    WCHAR *ret;

    if ((p[0] == '\"' && p[len - 1] != '\"') ||
        (p[0] == '\'' && p[len - 1] != '\'')) return NULL;
    if ((p[0] == '\"' && p[len - 1] == '\"') ||
        (p[0] == '\'' && p[len - 1] == '\''))
    {
        p++;
        len -= 2;
    }
    if (!(ret = alloc_mem( parser, (len + 1) * sizeof(WCHAR) ))) return NULL;
    memcpy( ret, p, len * sizeof(WCHAR) );
    ret[len] = 0;
    return ret;
}

static int get_int( struct parser *parser )
{
    const WCHAR *p = &parser->cmd[parser->idx];
    int i, ret = 0;

    for (i = 0; i < parser->len; i++)
    {
        if (p[i] < '0' || p[i] > '9')
        {
            ERR("should only be numbers here!\n");
            break;
        }
        ret = (p[i] - '0') + ret * 10;
    }
    return ret;
}

static struct expr *expr_complex( struct parser *parser, struct expr *l, UINT op, struct expr *r )
{
    struct expr *e = alloc_mem( parser, sizeof(*e) );
    if (e)
    {
        e->type = EXPR_COMPLEX;
        e->u.expr.left = l;
        e->u.expr.op = op;
        e->u.expr.right = r;
    }
    return e;
}

static struct expr *expr_unary( struct parser *parser, struct expr *l, UINT op )
{
    struct expr *e = alloc_mem( parser, sizeof(*e) );
    if (e)
    {
        e->type = EXPR_UNARY;
        e->u.expr.left = l;
        e->u.expr.op = op;
        e->u.expr.right = NULL;
    }
    return e;
}

static struct expr *expr_ival( struct parser *parser, int val )
{
    struct expr *e = alloc_mem( parser, sizeof *e );
    if (e)
    {
        e->type = EXPR_IVAL;
        e->u.ival = val;
    }
    return e;
}

static struct expr *expr_sval( struct parser *parser, const struct string *str )
{
    struct expr *e = alloc_mem( parser, sizeof *e );
    if (e)
    {
        e->type = EXPR_SVAL;
        e->u.sval = get_string( parser, str );
        if (!e->u.sval)
            return NULL; /* e will be freed by query destructor */
    }
    return e;
}

static struct expr *expr_bval( struct parser *parser, int val )
{
    struct expr *e = alloc_mem( parser, sizeof *e );
    if (e)
    {
        e->type = EXPR_BVAL;
        e->u.ival = val;
    }
    return e;
}

static struct expr *expr_propval( struct parser *parser, const struct property *prop )
{
    struct expr *e = alloc_mem( parser, sizeof *e );
    if (e)
    {
        e->type = EXPR_PROPVAL;
        e->u.propval = prop;
    }
    return e;
}

static int wql_error( struct parser *parser, const char *str );
static int wql_lex( void *val, struct parser *parser );

#define PARSER_BUBBLE_UP_VIEW( parser, result, current_view ) \
    *parser->view = current_view; \
    result = current_view


#line 259 "wql.tab.c" /* yacc.c:339  */

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
# define YYERROR_VERBOSE 1
#endif


/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int wql_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    TK_SELECT = 258,
    TK_FROM = 259,
    TK_STAR = 260,
    TK_COMMA = 261,
    TK_DOT = 262,
    TK_IS = 263,
    TK_LP = 264,
    TK_RP = 265,
    TK_NULL = 266,
    TK_FALSE = 267,
    TK_TRUE = 268,
    TK_INTEGER = 269,
    TK_WHERE = 270,
    TK_SPACE = 271,
    TK_MINUS = 272,
    TK_ILLEGAL = 273,
    TK_BY = 274,
    TK_STRING = 275,
    TK_ID = 276,
    TK_OR = 277,
    TK_AND = 278,
    TK_NOT = 279,
    TK_EQ = 280,
    TK_NE = 281,
    TK_LT = 282,
    TK_GT = 283,
    TK_LE = 284,
    TK_GE = 285,
    TK_LIKE = 286
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 194 "wql.y" /* yacc.c:355  */

    struct string str;
    WCHAR *string;
    struct property *proplist;
    struct view *view;
    struct expr *expr;
    int integer;

#line 337 "wql.tab.c" /* yacc.c:355  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int wql_parse (struct parser *ctx);



/* Copy the second part of user declarations.  */

#line 351 "wql.tab.c" /* yacc.c:358  */

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
# elif ! defined YYSIZE_T
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

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
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
#define YYFINAL  9
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   68

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  32
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  10
/* YYNRULES -- Number of rules.  */
#define YYNRULES  36
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  67

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   286

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

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
      25,    26,    27,    28,    29,    30,    31
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   221,   221,   233,   245,   260,   261,   265,   272,   278,
     287,   296,   303,   309,   315,   321,   327,   333,   339,   345,
     351,   357,   363,   369,   375,   381,   387,   393,   399,   405,
     411,   420,   429,   438,   444,   450,   456
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "TK_SELECT", "TK_FROM", "TK_STAR",
  "TK_COMMA", "TK_DOT", "TK_IS", "TK_LP", "TK_RP", "TK_NULL", "TK_FALSE",
  "TK_TRUE", "TK_INTEGER", "TK_WHERE", "TK_SPACE", "TK_MINUS",
  "TK_ILLEGAL", "TK_BY", "TK_STRING", "TK_ID", "TK_OR", "TK_AND", "TK_NOT",
  "TK_EQ", "TK_NE", "TK_LT", "TK_GT", "TK_LE", "TK_GE", "TK_LIKE",
  "$accept", "select", "proplist", "prop", "id", "number", "expr",
  "string_val", "prop_val", "const_val", YY_NULLPTR
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
     285,   286
};
# endif

#define YYPACT_NINF -18

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-18)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int8 yypact[] =
{
       7,     3,    12,    -5,   -18,   -18,    24,    29,    35,   -18,
     -18,    -5,     4,    -5,    21,   -18,   -18,    17,    17,   -18,
     -18,   -18,   -18,    17,   -18,   -18,    11,    -8,    25,    22,
     -18,    17,    17,    16,    -9,    -9,    -9,    -9,    -9,    -9,
      23,    -5,    -5,    -5,    -5,    -5,    -5,   -18,    45,   -18,
     -18,    37,   -18,   -18,   -18,   -18,   -18,   -18,   -18,   -18,
     -18,   -18,   -18,   -18,   -18,   -18,   -18
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     0,     7,    10,     0,     5,     9,     1,
       2,     0,     0,     0,     3,     6,     8,     0,     0,    36,
      35,    11,    34,     0,    32,    33,     4,     0,     0,     0,
      15,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    12,    14,    13,
      29,     0,    16,    21,    18,    17,    19,    20,    31,    28,
      22,    27,    24,    23,    25,    26,    30
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -18,   -18,    34,     1,    36,   -18,   -17,   -18,    15,    28
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     2,     6,    24,     8,    25,    26,    59,    27,    28
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      33,    29,     7,    19,    20,    21,    30,     3,     4,     4,
       1,    22,     9,     7,    48,    49,     5,    34,    35,    36,
      37,    38,    39,    40,     5,     5,    18,    50,    11,    19,
      20,    21,    47,    31,    32,    12,    17,    22,     5,    10,
      51,    23,    13,    58,    31,    32,    15,    14,    66,    16,
      41,    42,    43,    44,    45,    46,    60,    61,    62,    63,
      64,    65,    52,    53,    54,    55,    56,    57,    32
};

static const yytype_uint8 yycheck[] =
{
       8,    18,     1,    12,    13,    14,    23,     4,     5,     5,
       3,    20,     0,    12,    31,    32,    21,    25,    26,    27,
      28,    29,    30,    31,    21,    21,     9,    11,     4,    12,
      13,    14,    10,    22,    23,     6,    15,    20,    21,     3,
      24,    24,     7,    20,    22,    23,    12,    11,    11,    13,
      25,    26,    27,    28,    29,    30,    41,    42,    43,    44,
      45,    46,    34,    35,    36,    37,    38,    39,    23
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,    33,     4,     5,    21,    34,    35,    36,     0,
      36,     4,     6,     7,    36,    34,    36,    15,     9,    12,
      13,    14,    20,    24,    35,    37,    38,    40,    41,    38,
      38,    22,    23,     8,    25,    26,    27,    28,    29,    30,
      31,    25,    26,    27,    28,    29,    30,    10,    38,    38,
      11,    24,    41,    41,    41,    41,    41,    41,    20,    39,
      40,    40,    40,    40,    40,    40,    11
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    32,    33,    33,    33,    34,    34,    34,    35,    35,
      36,    37,    38,    38,    38,    38,    38,    38,    38,    38,
      38,    38,    38,    38,    38,    38,    38,    38,    38,    38,
      38,    39,    40,    41,    41,    41,    41
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     3,     4,     6,     1,     3,     1,     3,     1,
       1,     1,     3,     3,     3,     2,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       4,     1,     1,     1,     1,     1,     1
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
      yyerror (ctx, YY_("syntax error: cannot back up")); \
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
                  Type, Value, ctx); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, struct parser *ctx)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  YYUSE (ctx);
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
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, struct parser *ctx)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, ctx);
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
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule, struct parser *ctx)
{
  unsigned long int yylno = yyrline[yyrule];
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
                                              , ctx);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, ctx); \
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
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, struct parser *ctx)
{
  YYUSE (yyvaluep);
  YYUSE (ctx);
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
yyparse (struct parser *ctx)
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
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex (&yylval, ctx);
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
#line 222 "wql.y" /* yacc.c:1646  */
    {
            HRESULT hr;
            struct parser *parser = ctx;
            struct view *view;

            hr = create_view( NULL, (yyvsp[0].string), NULL, &view );
            if (hr != S_OK)
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( parser, (yyval.view), view );
        }
#line 1482 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 3:
#line 234 "wql.y" /* yacc.c:1646  */
    {
            HRESULT hr;
            struct parser *parser = ctx;
            struct view *view;

            hr = create_view( (yyvsp[-2].proplist), (yyvsp[0].string), NULL, &view );
            if (hr != S_OK)
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( parser, (yyval.view), view );
        }
#line 1498 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 4:
#line 246 "wql.y" /* yacc.c:1646  */
    {
            HRESULT hr;
            struct parser *parser = ctx;
            struct view *view;

            hr = create_view( (yyvsp[-4].proplist), (yyvsp[-2].string), (yyvsp[0].expr), &view );
            if (hr != S_OK)
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( parser, (yyval.view), view );
        }
#line 1514 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 6:
#line 262 "wql.y" /* yacc.c:1646  */
    {
            (yyvsp[-2].proplist)->next = (yyvsp[0].proplist);
        }
#line 1522 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 7:
#line 266 "wql.y" /* yacc.c:1646  */
    {
            (yyval.proplist) = NULL;
        }
#line 1530 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 8:
#line 273 "wql.y" /* yacc.c:1646  */
    {
            (yyval.proplist) = alloc_property( ctx, (yyvsp[-2].string), (yyvsp[0].string) );
            if (!(yyval.proplist))
                YYABORT;
        }
#line 1540 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 9:
#line 279 "wql.y" /* yacc.c:1646  */
    {
            (yyval.proplist) = alloc_property( ctx, NULL, (yyvsp[0].string) );
            if (!(yyval.proplist))
                YYABORT;
        }
#line 1550 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 10:
#line 288 "wql.y" /* yacc.c:1646  */
    {
            (yyval.string) = get_string( ctx, &(yyvsp[0].str) );
            if (!(yyval.string))
                YYABORT;
        }
#line 1560 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 11:
#line 297 "wql.y" /* yacc.c:1646  */
    {
            (yyval.integer) = get_int( ctx );
        }
#line 1568 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 12:
#line 304 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = (yyvsp[-1].expr);
            if (!(yyval.expr))
                YYABORT;
        }
#line 1578 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 13:
#line 310 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = expr_complex( ctx, (yyvsp[-2].expr), OP_AND, (yyvsp[0].expr) );
            if (!(yyval.expr))
                YYABORT;
        }
#line 1588 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 14:
#line 316 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = expr_complex( ctx, (yyvsp[-2].expr), OP_OR, (yyvsp[0].expr) );
            if (!(yyval.expr))
                YYABORT;
        }
#line 1598 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 15:
#line 322 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = expr_unary( ctx, (yyvsp[0].expr), OP_NOT );
            if (!(yyval.expr))
                YYABORT;
        }
#line 1608 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 16:
#line 328 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = expr_complex( ctx, (yyvsp[-2].expr), OP_EQ, (yyvsp[0].expr) );
            if (!(yyval.expr))
                YYABORT;
        }
#line 1618 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 17:
#line 334 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = expr_complex( ctx, (yyvsp[-2].expr), OP_GT, (yyvsp[0].expr) );
            if (!(yyval.expr))
                YYABORT;
        }
#line 1628 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 18:
#line 340 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = expr_complex( ctx, (yyvsp[-2].expr), OP_LT, (yyvsp[0].expr) );
            if (!(yyval.expr))
                YYABORT;
        }
#line 1638 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 19:
#line 346 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = expr_complex( ctx, (yyvsp[-2].expr), OP_LE, (yyvsp[0].expr) );
            if (!(yyval.expr))
                YYABORT;
        }
#line 1648 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 20:
#line 352 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = expr_complex( ctx, (yyvsp[-2].expr), OP_GE, (yyvsp[0].expr) );
            if (!(yyval.expr))
                YYABORT;
        }
#line 1658 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 21:
#line 358 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = expr_complex( ctx, (yyvsp[-2].expr), OP_NE, (yyvsp[0].expr) );
            if (!(yyval.expr))
                YYABORT;
        }
#line 1668 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 22:
#line 364 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = expr_complex( ctx, (yyvsp[-2].expr), OP_EQ, (yyvsp[0].expr) );
            if (!(yyval.expr))
                YYABORT;
        }
#line 1678 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 23:
#line 370 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = expr_complex( ctx, (yyvsp[-2].expr), OP_GT, (yyvsp[0].expr) );
            if (!(yyval.expr))
                YYABORT;
        }
#line 1688 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 24:
#line 376 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = expr_complex( ctx, (yyvsp[-2].expr), OP_LT, (yyvsp[0].expr) );
            if (!(yyval.expr))
                YYABORT;
        }
#line 1698 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 25:
#line 382 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = expr_complex( ctx, (yyvsp[-2].expr), OP_LE, (yyvsp[0].expr) );
            if (!(yyval.expr))
                YYABORT;
        }
#line 1708 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 26:
#line 388 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = expr_complex( ctx, (yyvsp[-2].expr), OP_GE, (yyvsp[0].expr) );
            if (!(yyval.expr))
                YYABORT;
        }
#line 1718 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 27:
#line 394 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = expr_complex( ctx, (yyvsp[-2].expr), OP_NE, (yyvsp[0].expr) );
            if (!(yyval.expr))
                YYABORT;
        }
#line 1728 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 28:
#line 400 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = expr_complex( ctx, (yyvsp[-2].expr), OP_LIKE, (yyvsp[0].expr) );
            if (!(yyval.expr))
                YYABORT;
        }
#line 1738 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 29:
#line 406 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = expr_unary( ctx, (yyvsp[-2].expr), OP_ISNULL );
            if (!(yyval.expr))
                YYABORT;
        }
#line 1748 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 30:
#line 412 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = expr_unary( ctx, (yyvsp[-3].expr), OP_NOTNULL );
            if (!(yyval.expr))
                YYABORT;
        }
#line 1758 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 31:
#line 421 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = expr_sval( ctx, &(yyvsp[0].str) );
            if (!(yyval.expr))
                YYABORT;
        }
#line 1768 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 32:
#line 430 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = expr_propval( ctx, (yyvsp[0].proplist) );
            if (!(yyval.expr))
                YYABORT;
        }
#line 1778 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 33:
#line 439 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = expr_ival( ctx, (yyvsp[0].integer) );
            if (!(yyval.expr))
                YYABORT;
        }
#line 1788 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 34:
#line 445 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = expr_sval( ctx, &(yyvsp[0].str) );
            if (!(yyval.expr))
                YYABORT;
        }
#line 1798 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 35:
#line 451 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = expr_bval( ctx, -1 );
            if (!(yyval.expr))
                YYABORT;
        }
#line 1808 "wql.tab.c" /* yacc.c:1646  */
    break;

  case 36:
#line 457 "wql.y" /* yacc.c:1646  */
    {
            (yyval.expr) = expr_bval( ctx, 0 );
            if (!(yyval.expr))
                YYABORT;
        }
#line 1818 "wql.tab.c" /* yacc.c:1646  */
    break;


#line 1822 "wql.tab.c" /* yacc.c:1646  */
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
      yyerror (ctx, YY_("syntax error"));
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
        yyerror (ctx, yymsgp);
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
                      yytoken, &yylval, ctx);
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
                  yystos[yystate], yyvsp, ctx);
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
  yyerror (ctx, YY_("memory exhausted"));
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
                  yytoken, &yylval, ctx);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, ctx);
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
#line 464 "wql.y" /* yacc.c:1906  */


HRESULT parse_query( const WCHAR *str, struct view **view, struct list *mem )
{
    struct parser parser;
    int ret;

    *view = NULL;

    parser.cmd   = str;
    parser.idx   = 0;
    parser.len   = 0;
    parser.error = WBEM_E_INVALID_QUERY;
    parser.view  = view;
    parser.mem   = mem;

    ret = wql_parse( &parser );
    TRACE("wql_parse returned %d\n", ret);
    if (ret)
    {
        if (*parser.view)
        {
            destroy_view( *parser.view );
            *parser.view = NULL;
        }
        return parser.error;
    }
    return S_OK;
}

static const char id_char[] =
{
    0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

struct keyword
{
    const WCHAR *name;
    unsigned int len;
    int type;
};

#define MAX_TOKEN_LEN 6

static const WCHAR andW[] = {'A','N','D'};
static const WCHAR byW[] = {'B','Y'};
static const WCHAR falseW[] = {'F','A','L','S','E'};
static const WCHAR fromW[] = {'F','R','O','M'};
static const WCHAR isW[] = {'I','S'};
static const WCHAR likeW[] = {'L','I','K','E'};
static const WCHAR notW[] = {'N','O','T'};
static const WCHAR nullW[] = {'N','U','L','L'};
static const WCHAR orW[] = {'O','R'};
static const WCHAR selectW[] = {'S','E','L','E','C','T'};
static const WCHAR trueW[] = {'T','R','U','E'};
static const WCHAR whereW[] = {'W','H','E','R','E'};

static const struct keyword keyword_table[] =
{
  { andW,    SIZEOF(andW),    TK_AND },
  { byW,     SIZEOF(byW),     TK_BY },
  { falseW,  SIZEOF(falseW),  TK_FALSE },
  { fromW,   SIZEOF(fromW),   TK_FROM },
  { isW,     SIZEOF(isW),     TK_IS },
  { likeW,   SIZEOF(likeW),   TK_LIKE },
  { notW,    SIZEOF(notW),    TK_NOT },
  { nullW,   SIZEOF(nullW),   TK_NULL },
  { orW,     SIZEOF(orW),     TK_OR },
  { selectW, SIZEOF(selectW), TK_SELECT },
  { trueW,   SIZEOF(trueW),   TK_TRUE },
  { whereW,  SIZEOF(whereW),  TK_WHERE }
};

static int cmp_keyword( const void *arg1, const void *arg2 )
{
    const struct keyword *key1 = arg1, *key2 = arg2;
    int len = min( key1->len, key2->len );
    int ret;

    if ((ret = memicmpW( key1->name, key2->name, len ))) return ret;
    if (key1->len < key2->len) return -1;
    else if (key1->len > key2->len) return 1;
    return 0;
}

static int keyword_type( const WCHAR *str, unsigned int len )
{
    struct keyword key, *ret;

    if (len > MAX_TOKEN_LEN) return TK_ID;

    key.name = str;
    key.len  = len;
    key.type = 0;
    ret = bsearch( &key, keyword_table, SIZEOF(keyword_table), sizeof(struct keyword), cmp_keyword );
    if (ret) return ret->type;
    return TK_ID;
}

static int get_token( const WCHAR *s, int *token )
{
    int i;

    switch (*s)
    {
    case ' ':
    case '\t':
    case '\r':
    case '\n':
        for (i = 1; isspaceW( s[i] ); i++) {}
        *token = TK_SPACE;
        return i;
    case '-':
        if (!s[1]) return -1;
        *token = TK_MINUS;
        return 1;
    case '(':
        *token = TK_LP;
        return 1;
    case ')':
        *token = TK_RP;
        return 1;
    case '*':
        *token = TK_STAR;
        return 1;
    case '=':
        *token = TK_EQ;
        return 1;
    case '<':
        if (s[1] == '=' )
        {
            *token = TK_LE;
            return 2;
        }
        else if (s[1] == '>')
        {
            *token = TK_NE;
            return 2;
        }
        else
        {
            *token = TK_LT;
            return 1;
        }
    case '>':
        if (s[1] == '=')
        {
            *token = TK_GE;
            return 2;
        }
        else
        {
            *token = TK_GT;
            return 1;
        }
    case '!':
        if (s[1] != '=')
        {
            *token = TK_ILLEGAL;
            return 2;
        }
        else
        {
            *token = TK_NE;
            return 2;
        }
    case ',':
        *token = TK_COMMA;
        return 1;
    case '\"':
    case '\'':
        {
            for (i = 1; s[i]; i++)
            {
                if (s[i] == s[0]) break;
            }
            if (s[i]) i++;
            *token = TK_STRING;
            return i;
        }
    case '.':
        if (!isdigitW( s[1] ))
        {
            *token = TK_DOT;
            return 1;
        }
        /* fall through */
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        *token = TK_INTEGER;
        for (i = 1; isdigitW( s[i] ); i++) {}
        return i;
    default:
        if (!id_char[*s]) break;

        for (i = 1; id_char[s[i]]; i++) {}
        *token = keyword_type( s, i );
        return i;
    }
    *token = TK_ILLEGAL;
    return 1;
}

static int wql_lex( void *p, struct parser *parser )
{
    struct string *str = p;
    int token = -1;
    do
    {
        parser->idx += parser->len;
        if (!parser->cmd[parser->idx]) return 0;
        parser->len = get_token( &parser->cmd[parser->idx], &token );
        if (!parser->len) break;

        str->data = &parser->cmd[parser->idx];
        str->len = parser->len;
    } while (token == TK_SPACE);
    return token;
}

static int wql_error( struct parser *parser, const char *str )
{
    ERR("%s\n", str);
    return 0;
}
