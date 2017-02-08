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
#define yyparse         parser_parse
#define yylex           parser_lex
#define yyerror         parser_error
#define yydebug         parser_debug
#define yynerrs         parser_nerrs


/* Copy the first part of user declarations.  */
#line 19 "parser.y" /* yacc.c:339  */


#include "jscript.h"

static int parser_error(parser_ctx_t*,const char*);
static void set_error(parser_ctx_t*,UINT);
static BOOL explicit_error(parser_ctx_t*,void*,WCHAR);
static BOOL allow_auto_semicolon(parser_ctx_t*);
static void program_parsed(parser_ctx_t*,source_elements_t*);

typedef struct _statement_list_t {
    statement_t *head;
    statement_t *tail;
} statement_list_t;

static literal_t *new_string_literal(parser_ctx_t*,const WCHAR*);
static literal_t *new_null_literal(parser_ctx_t*);

typedef struct _property_list_t {
    prop_val_t *head;
    prop_val_t *tail;
} property_list_t;

static property_list_t *new_property_list(parser_ctx_t*,literal_t*,expression_t*);
static property_list_t *property_list_add(parser_ctx_t*,property_list_t*,literal_t*,expression_t*);

typedef struct _element_list_t {
    array_element_t *head;
    array_element_t *tail;
} element_list_t;

static element_list_t *new_element_list(parser_ctx_t*,int,expression_t*);
static element_list_t *element_list_add(parser_ctx_t*,element_list_t*,int,expression_t*);

typedef struct _argument_list_t {
    argument_t *head;
    argument_t *tail;
} argument_list_t;

static argument_list_t *new_argument_list(parser_ctx_t*,expression_t*);
static argument_list_t *argument_list_add(parser_ctx_t*,argument_list_t*,expression_t*);

typedef struct _case_list_t {
    case_clausule_t *head;
    case_clausule_t *tail;
} case_list_t;

static catch_block_t *new_catch_block(parser_ctx_t*,const WCHAR*,statement_t*);
static case_clausule_t *new_case_clausule(parser_ctx_t*,expression_t*,statement_list_t*);
static case_list_t *new_case_list(parser_ctx_t*,case_clausule_t*);
static case_list_t *case_list_add(parser_ctx_t*,case_list_t*,case_clausule_t*);
static case_clausule_t *new_case_block(parser_ctx_t*,case_list_t*,case_clausule_t*,case_list_t*);

typedef struct _variable_list_t {
    variable_declaration_t *head;
    variable_declaration_t *tail;
} variable_list_t;

static variable_declaration_t *new_variable_declaration(parser_ctx_t*,const WCHAR*,expression_t*);
static variable_list_t *new_variable_list(parser_ctx_t*,variable_declaration_t*);
static variable_list_t *variable_list_add(parser_ctx_t*,variable_list_t*,variable_declaration_t*);

static void *new_statement(parser_ctx_t*,statement_type_t,size_t);
static statement_t *new_block_statement(parser_ctx_t*,statement_list_t*);
static statement_t *new_var_statement(parser_ctx_t*,variable_list_t*);
static statement_t *new_expression_statement(parser_ctx_t*,expression_t*);
static statement_t *new_if_statement(parser_ctx_t*,expression_t*,statement_t*,statement_t*);
static statement_t *new_while_statement(parser_ctx_t*,BOOL,expression_t*,statement_t*);
static statement_t *new_for_statement(parser_ctx_t*,variable_list_t*,expression_t*,expression_t*,
        expression_t*,statement_t*);
static statement_t *new_forin_statement(parser_ctx_t*,variable_declaration_t*,expression_t*,expression_t*,statement_t*);
static statement_t *new_continue_statement(parser_ctx_t*,const WCHAR*);
static statement_t *new_break_statement(parser_ctx_t*,const WCHAR*);
static statement_t *new_return_statement(parser_ctx_t*,expression_t*);
static statement_t *new_with_statement(parser_ctx_t*,expression_t*,statement_t*);
static statement_t *new_labelled_statement(parser_ctx_t*,const WCHAR*,statement_t*);
static statement_t *new_switch_statement(parser_ctx_t*,expression_t*,case_clausule_t*);
static statement_t *new_throw_statement(parser_ctx_t*,expression_t*);
static statement_t *new_try_statement(parser_ctx_t*,statement_t*,catch_block_t*,statement_t*);

struct statement_list_t {
   statement_t *head;
   statement_t *tail;
};

static statement_list_t *new_statement_list(parser_ctx_t*,statement_t*);
static statement_list_t *statement_list_add(statement_list_t*,statement_t*);

typedef struct _parameter_list_t {
    parameter_t *head;
    parameter_t *tail;
} parameter_list_t;

static parameter_list_t *new_parameter_list(parser_ctx_t*,const WCHAR*);
static parameter_list_t *parameter_list_add(parser_ctx_t*,parameter_list_t*,const WCHAR*);

static void *new_expression(parser_ctx_t *ctx,expression_type_t,size_t);
static expression_t *new_function_expression(parser_ctx_t*,const WCHAR*,parameter_list_t*,
        source_elements_t*,const WCHAR*,const WCHAR*,DWORD);
static expression_t *new_binary_expression(parser_ctx_t*,expression_type_t,expression_t*,expression_t*);
static expression_t *new_unary_expression(parser_ctx_t*,expression_type_t,expression_t*);
static expression_t *new_conditional_expression(parser_ctx_t*,expression_t*,expression_t*,expression_t*);
static expression_t *new_member_expression(parser_ctx_t*,expression_t*,const WCHAR*);
static expression_t *new_new_expression(parser_ctx_t*,expression_t*,argument_list_t*);
static expression_t *new_call_expression(parser_ctx_t*,expression_t*,argument_list_t*);
static expression_t *new_identifier_expression(parser_ctx_t*,const WCHAR*);
static expression_t *new_literal_expression(parser_ctx_t*,literal_t*);
static expression_t *new_array_literal_expression(parser_ctx_t*,element_list_t*,int);
static expression_t *new_prop_and_value_expression(parser_ctx_t*,property_list_t*);

static source_elements_t *new_source_elements(parser_ctx_t*);
static source_elements_t *source_elements_add_statement(source_elements_t*,statement_t*);


#line 193 "parser.tab.c" /* yacc.c:339  */

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
extern int parser_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    kBREAK = 258,
    kCASE = 259,
    kCATCH = 260,
    kCONTINUE = 261,
    kDEFAULT = 262,
    kDELETE = 263,
    kDO = 264,
    kELSE = 265,
    kIF = 266,
    kFINALLY = 267,
    kFOR = 268,
    kIN = 269,
    kINSTANCEOF = 270,
    kNEW = 271,
    kNULL = 272,
    kRETURN = 273,
    kSWITCH = 274,
    kTHIS = 275,
    kTHROW = 276,
    kTRUE = 277,
    kFALSE = 278,
    kTRY = 279,
    kTYPEOF = 280,
    kVAR = 281,
    kVOID = 282,
    kWHILE = 283,
    kWITH = 284,
    tANDAND = 285,
    tOROR = 286,
    tINC = 287,
    tDEC = 288,
    tHTMLCOMMENT = 289,
    kDIVEQ = 290,
    kDCOL = 291,
    kFUNCTION = 292,
    tIdentifier = 293,
    tAssignOper = 294,
    tEqOper = 295,
    tShiftOper = 296,
    tRelOper = 297,
    tNumericLiteral = 298,
    tBooleanLiteral = 299,
    tStringLiteral = 300,
    tEOF = 301,
    LOWER_THAN_ELSE = 302
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 145 "parser.y" /* yacc.c:355  */

    int                     ival;
    const WCHAR             *srcptr;
    LPCWSTR                 wstr;
    literal_t               *literal;
    struct _argument_list_t *argument_list;
    case_clausule_t         *case_clausule;
    struct _case_list_t     *case_list;
    catch_block_t           *catch_block;
    struct _element_list_t  *element_list;
    expression_t            *expr;
    const WCHAR            *identifier;
    struct _parameter_list_t *parameter_list;
    struct _property_list_t *property_list;
    source_elements_t       *source_elements;
    statement_t             *statement;
    struct _statement_list_t *statement_list;
    struct _variable_list_t *variable_list;
    variable_declaration_t  *variable_declaration;

#line 299 "parser.tab.c" /* yacc.c:355  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int parser_parse (parser_ctx_t *ctx);



/* Copy the second part of user declarations.  */

#line 313 "parser.tab.c" /* yacc.c:358  */

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
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1085

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  70
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  95
/* YYNRULES -- Number of rules.  */
#define YYNRULES  217
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  389

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   302

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    64,     2,     2,     2,    62,    57,     2,
      68,    69,    60,    58,    50,    59,    67,    61,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    53,    52,
       2,    51,     2,    54,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    65,     2,    66,    56,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    49,    55,    38,    63,     2,     2,     2,
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
      35,    36,    37,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   252,   252,   256,   257,   261,   262,   267,   269,   271,
     275,   279,   283,   284,   289,   290,   294,   295,   296,   297,
     298,   299,   300,   301,   302,   303,   304,   305,   306,   307,
     308,   312,   313,   318,   319,   323,   324,   328,   333,   334,
     339,   341,   346,   351,   356,   357,   361,   366,   367,   371,
     376,   380,   385,   387,   392,   394,   397,   399,   396,   403,
     405,   402,   408,   410,   415,   420,   425,   430,   435,   440,
     445,   447,   452,   453,   457,   458,   463,   468,   473,   478,
     479,   480,   485,   490,   494,   495,   498,   499,   503,   504,
     509,   510,   514,   516,   520,   521,   525,   526,   528,   533,
     535,   537,   542,   543,   548,   550,   555,   556,   561,   563,
     568,   569,   574,   576,   581,   582,   587,   589,   594,   595,
     600,   602,   607,   608,   613,   615,   620,   621,   626,   627,
     632,   633,   635,   637,   642,   643,   645,   650,   651,   656,
     658,   660,   665,   666,   668,   670,   675,   676,   678,   679,
     681,   682,   683,   684,   685,   686,   690,   692,   694,   700,
     701,   705,   706,   710,   711,   712,   714,   716,   721,   723,
     725,   727,   732,   733,   737,   738,   743,   744,   745,   746,
     747,   748,   752,   753,   754,   755,   760,   762,   767,   768,
     772,   773,   777,   778,   783,   785,   790,   791,   792,   796,
     797,   801,   802,   803,   804,   805,   807,   812,   813,   814,
     817,   818,   821,   822,   825,   826,   829,   830
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "kBREAK", "kCASE", "kCATCH", "kCONTINUE",
  "kDEFAULT", "kDELETE", "kDO", "kELSE", "kIF", "kFINALLY", "kFOR", "kIN",
  "kINSTANCEOF", "kNEW", "kNULL", "kRETURN", "kSWITCH", "kTHIS", "kTHROW",
  "kTRUE", "kFALSE", "kTRY", "kTYPEOF", "kVAR", "kVOID", "kWHILE", "kWITH",
  "tANDAND", "tOROR", "tINC", "tDEC", "tHTMLCOMMENT", "kDIVEQ", "kDCOL",
  "kFUNCTION", "'}'", "tIdentifier", "tAssignOper", "tEqOper",
  "tShiftOper", "tRelOper", "tNumericLiteral", "tBooleanLiteral",
  "tStringLiteral", "tEOF", "LOWER_THAN_ELSE", "'{'", "','", "'='", "';'",
  "':'", "'?'", "'|'", "'^'", "'&'", "'+'", "'-'", "'*'", "'/'", "'%'",
  "'~'", "'!'", "'['", "']'", "'.'", "'('", "')'", "$accept", "Program",
  "HtmlComment", "SourceElements", "FunctionExpression", "KFunction",
  "FunctionBody", "FormalParameterList", "FormalParameterList_opt",
  "Statement", "StatementList", "StatementList_opt", "Block",
  "VariableStatement", "VariableDeclarationList",
  "VariableDeclarationListNoIn", "VariableDeclaration",
  "VariableDeclarationNoIn", "Initialiser_opt", "Initialiser",
  "InitialiserNoIn_opt", "InitialiserNoIn", "EmptyStatement",
  "ExpressionStatement", "IfStatement", "IterationStatement", "$@1", "$@2",
  "$@3", "$@4", "ContinueStatement", "BreakStatement", "ReturnStatement",
  "WithStatement", "LabelledStatement", "SwitchStatement", "CaseBlock",
  "CaseClausules_opt", "CaseClausules", "CaseClausule", "DefaultClausule",
  "ThrowStatement", "TryStatement", "Catch", "Finally", "Expression_opt",
  "Expression_err", "Expression", "ExpressionNoIn_opt", "ExpressionNoIn",
  "AssignOper", "AssignmentExpression", "AssignmentExpressionNoIn",
  "ConditionalExpression", "ConditionalExpressionNoIn",
  "LogicalORExpression", "LogicalORExpressionNoIn", "LogicalANDExpression",
  "LogicalANDExpressionNoIn", "BitwiseORExpression",
  "BitwiseORExpressionNoIn", "BitwiseXORExpression",
  "BitwiseXORExpressionNoIn", "BitwiseANDExpression",
  "BitwiseANDExpressionNoIn", "EqualityExpression",
  "EqualityExpressionNoIn", "RelationalExpression",
  "RelationalExpressionNoIn", "ShiftExpression", "AdditiveExpression",
  "MultiplicativeExpression", "UnaryExpression", "PostfixExpression",
  "LeftHandSideExpression", "NewExpression", "MemberExpression",
  "CallExpression", "Arguments", "ArgumentList", "PrimaryExpression",
  "ArrayLiteral", "ElementList", "Elision", "Elision_opt", "ObjectLiteral",
  "PropertyNameAndValueList", "PropertyName", "Identifier_opt", "Literal",
  "BooleanLiteral", "semicolon_opt", "left_bracket", "right_bracket",
  "semicolon", YY_NULLPTR
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
     285,   286,   287,   288,   289,   290,   291,   292,   125,   293,
     294,   295,   296,   297,   298,   299,   300,   301,   302,   123,
      44,    61,    59,    58,    63,   124,    94,    38,    43,    45,
      42,    47,    37,   126,    33,    91,    93,    46,    40,    41
};
# endif

#define YYPACT_NINF -336

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-336)))

#define YYTABLE_NINF -199

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -336,    33,   515,  -336,   -17,   -17,   983,   767,    37,    37,
    1017,  -336,   983,    37,  -336,   983,  -336,  -336,   -12,   983,
       9,   983,    37,    37,   983,   983,  -336,  -336,  -336,     8,
    -336,  -336,  -336,   578,  -336,   983,   983,  -336,   983,   983,
      57,   983,     4,   427,     6,  -336,  -336,  -336,  -336,  -336,
    -336,  -336,  -336,  -336,  -336,  -336,  -336,  -336,  -336,  -336,
      56,  -336,  -336,   -10,    90,    38,    46,    65,    83,    96,
      86,    94,   168,  -336,  -336,   114,  -336,   159,   169,  -336,
    -336,  -336,  -336,  -336,  -336,    14,    14,  -336,   122,  -336,
    -336,   180,   107,  -336,  -336,   388,   875,  -336,   159,    14,
     113,   983,    56,   641,    42,  -336,   124,    75,  -336,  -336,
     388,   983,  -336,  -336,   767,   460,     8,   120,   141,  -336,
     704,    79,   156,  -336,  -336,  -336,  -336,  -336,  -336,    84,
      91,   983,    63,  -336,    41,   177,  -336,   983,  -336,  -336,
     983,   983,   983,   983,   983,   983,   983,   983,   983,   983,
     983,   983,   983,   983,   983,   983,  -336,  -336,  -336,  -336,
     983,   983,   983,   199,   821,  -336,   983,   200,  -336,  -336,
    -336,  -336,  -336,  -336,  -336,    37,  -336,    22,   113,   201,
    -336,   192,  -336,  -336,    72,   213,   189,   190,   188,   206,
      17,    86,   105,  -336,  -336,    35,  -336,  -336,    37,   -12,
     236,  -336,   983,  -336,  -336,     9,  -336,    22,    35,  -336,
    -336,  -336,  -336,   125,   983,   202,  -336,  -336,  -336,  -336,
    -336,   211,   177,  -336,   204,    22,  -336,    90,   198,    38,
      46,    65,    83,    96,    86,    86,    86,    94,   168,   168,
    -336,  -336,  -336,  -336,  -336,    92,  -336,  -336,  -336,    64,
      93,  -336,   388,  -336,  -336,   767,   210,   205,   243,    45,
     983,   983,   983,   983,   983,   983,   983,   983,   983,   983,
     388,   983,   983,   214,   223,  -336,  -336,  -336,  -336,   767,
     767,   212,  -336,   216,   929,    37,    22,   225,   218,   983,
    -336,   983,  -336,  -336,    22,   258,   983,  -336,  -336,   201,
      45,   388,  -336,  -336,   983,  -336,   182,   213,   217,   189,
     190,   188,   206,    17,    86,    86,    22,  -336,  -336,   265,
    -336,    22,  -336,  -336,   983,  -336,  -336,   177,   222,  -336,
    -336,  -336,  -336,    14,   767,  -336,  -336,   983,    22,  -336,
     983,   767,   983,    18,   265,  -336,   -12,  -336,    22,  -336,
     767,   234,  -336,  -336,  -336,   767,    45,  -336,  -336,   117,
     220,  -336,   265,  -336,  -336,   226,   238,  -336,    45,  -336,
     983,   767,   767,   245,  -336,  -336,   983,    22,   767,  -336,
    -336,  -336,   246,    22,   767,  -336,   767,  -336,  -336
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       5,     0,     4,     1,   199,   199,     0,     0,     0,     0,
       0,   201,    84,     0,   176,     0,   207,   208,     0,     0,
       0,     0,     0,     0,     0,     0,     3,   206,    10,   177,
     203,   209,   204,     0,    50,     0,     0,   205,     0,     0,
     190,     0,     0,    19,     0,     6,    16,    17,    18,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
       0,    88,    96,   102,   106,   110,   114,   118,   122,   126,
     130,   137,   139,   142,   146,   156,   159,   161,   160,   163,
     179,   180,   178,   202,   200,     0,     0,   177,     0,   164,
     147,   156,     0,   213,   212,     0,    90,   162,   161,     0,
      85,     0,     0,     0,     0,   149,    44,     0,    38,   148,
       0,     0,   150,   151,     0,    36,   177,   203,   204,    31,
       0,     0,     0,   152,   153,   154,   155,   188,   182,     0,
     191,     0,     0,     2,     0,    14,   211,     0,   210,    51,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   157,   158,    95,    94,
       0,     0,     0,     0,     0,   168,     0,     0,   169,    65,
      64,   192,   196,   198,   197,     0,    87,     0,    86,     0,
      56,    91,    92,    99,   104,   108,   112,   116,   120,   124,
     128,   134,   156,   167,    66,     0,    78,    36,     0,     0,
      79,    80,     0,    42,    45,     0,    37,     0,     0,    68,
      35,    32,   193,     0,     0,   190,   184,   189,   183,   186,
     181,     0,    14,    12,    15,     0,    89,   107,     0,   111,
     115,   119,   123,   127,   133,   132,   131,   138,   140,   141,
     143,   144,   145,    97,    98,     0,   166,   172,   174,     0,
       0,   171,     0,   215,   214,     0,    47,    59,    40,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    83,    81,    46,    39,     0,
       0,     0,   194,   191,     0,     0,     0,     0,     0,     0,
     165,     0,   173,   170,     0,    53,     0,    43,    48,     0,
       0,     0,   217,   216,    84,    93,   156,   109,     0,   113,
     117,   121,   125,   129,   136,   135,     0,   100,   101,    72,
      69,     0,    55,    67,     0,   185,   187,    14,     0,    13,
       5,   103,   175,     0,     0,    49,    41,    84,     0,    57,
       0,     0,     0,     0,    73,    74,     0,   195,     0,     5,
      11,     0,    54,    52,    60,     0,     0,   105,    62,     0,
       0,    70,    72,    75,    82,     0,     0,     7,     0,    63,
      84,    33,    33,     0,     5,     8,    84,     0,    34,    76,
      77,    71,     0,     0,     0,     9,     0,    58,    61
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -336,  -336,  -336,   274,    -2,  -336,  -335,  -336,  -211,     1,
    -180,   -87,    -9,  -336,  -336,  -336,    81,    -8,  -336,  -336,
    -336,  -336,  -336,  -336,  -336,  -336,  -336,  -336,  -336,  -336,
    -336,  -336,  -336,  -336,  -336,  -336,  -336,   -75,  -336,   -56,
    -336,  -336,  -336,  -336,    89,  -287,  -108,   -11,  -336,  -336,
     229,   -66,  -244,  -336,  -336,  -336,  -336,   152,    47,   165,
      48,   166,    49,   170,    50,   167,    51,   173,    53,   -61,
     171,    69,    43,  -336,    34,   306,   312,  -336,   -35,  -336,
    -336,  -336,  -336,   108,   109,  -336,  -336,   112,   322,  -336,
    -336,   -73,    -3,    24,  -276
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    42,   350,    89,    44,   351,   224,   225,   119,
     120,   379,    46,    47,   107,   257,   108,   258,   203,   204,
     297,   298,    48,    49,    50,    51,   259,   356,   300,   368,
      52,    53,    54,    55,    56,    57,   320,   343,   344,   345,
     362,    58,    59,   200,   201,    99,   177,    60,   180,   181,
     272,    61,   182,    62,   183,    63,   184,    64,   185,    65,
     186,    66,   187,    67,   188,    68,   189,    69,   190,    70,
      71,    72,    73,    74,    75,    76,    77,    78,   165,   249,
      79,    80,   129,   130,   131,    81,   121,   122,    85,    82,
      83,   139,    95,   255,   304
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      43,   100,   207,    45,   102,    43,    96,    93,    92,   104,
     101,   286,   169,   170,   366,   136,   305,   339,   308,   110,
     111,   140,    84,   253,   337,   360,   194,   317,   318,   196,
     132,    43,   268,     3,   206,   191,   253,   103,    93,   382,
      91,   135,    93,   168,   141,   134,   302,   198,   106,    90,
     354,   133,   335,    91,   199,    91,   361,   136,    91,    91,
     269,   114,   105,   193,   109,   219,   138,   112,   113,    91,
      91,   226,    91,    91,    94,   228,   136,   221,   123,   124,
     370,   125,   126,   377,   178,   137,   234,   235,   236,   383,
     195,   254,   376,   143,   243,   244,   357,   303,   248,   178,
     208,    43,   144,   261,   254,    94,   137,   127,   138,    94,
     147,   148,    43,   137,   291,   209,   348,   212,    43,   270,
     142,   211,   145,   128,   146,   205,   262,   138,   150,   213,
     192,   222,   220,   292,   215,   175,   277,   156,   157,   149,
     158,   217,   137,   137,   294,   159,   156,   157,   282,   158,
     216,   245,   151,   152,   159,   250,   271,   218,   290,   293,
     171,   172,   316,   137,   172,   160,   173,   137,   174,   173,
     371,   174,   252,  -198,    91,   202,    91,    91,    91,    91,
      91,    91,    91,    91,    91,    91,    91,    91,    91,    91,
     275,   378,   378,   338,  -197,   274,   240,   241,   242,   191,
     191,   191,   191,   191,   191,   191,   191,   314,   315,   214,
     191,   191,   156,   157,   156,   157,   223,   158,   326,   273,
     238,   239,   159,   331,   162,   332,   163,   164,   153,   154,
     155,   279,   280,   271,   166,   191,   167,   164,   246,   251,
     256,   178,   260,   263,   264,   266,   265,   267,   199,   288,
     285,   289,   127,    43,   287,   299,   295,   301,   347,   178,
     352,   296,   321,   319,   329,   324,   217,   330,   334,   342,
     340,   349,   367,   372,     2,   374,   375,    43,    43,   191,
     322,   323,   327,   381,   385,   380,   278,   373,   363,   276,
     178,   336,   227,   100,   306,    91,   306,    91,    91,    91,
      91,    91,    91,    91,   161,   306,   306,   229,   307,   230,
     328,   309,   232,   310,   231,   311,    97,   312,   333,   233,
     313,   237,    98,   283,   284,   281,   100,    86,     0,     0,
     306,   359,    43,     0,     0,   353,     0,   364,     0,    43,
     341,     0,   358,     0,     0,   346,     0,     0,    43,     0,
       0,    45,     0,    43,     0,     0,   369,     0,     0,   100,
       0,     0,   355,     0,     0,   100,     0,     0,     0,    43,
      43,     0,   365,     0,   306,     0,    43,     0,     0,   211,
       0,     0,    43,     0,    43,   387,     0,   388,     0,   176,
       0,     0,     0,     0,     0,     0,     6,     0,     0,     0,
       0,   384,     0,     0,    10,    11,     0,   386,    14,     0,
      16,    17,     0,    19,     0,    21,     0,     0,     0,     0,
      24,    25,     0,    27,     0,    28,     0,    87,  -164,     0,
       0,     0,    30,    31,    32,     0,     0,    88,     0,     0,
       0,  -164,  -164,     0,     0,     0,    35,    36,     0,    37,
       0,    38,    39,    40,     0,     0,    41,  -164,  -164,     0,
       0,  -192,     0,     0,     0,     0,     0,  -164,  -164,  -164,
    -164,     0,     0,     0,  -192,  -192,     0,  -164,  -164,     0,
       0,  -164,  -164,  -164,  -164,     0,     0,  -164,     0,  -164,
    -192,  -192,     0,     0,  -164,     0,     0,     0,     0,     0,
    -192,  -192,  -192,  -192,     0,     0,     0,     0,     0,     0,
    -192,  -192,     0,     0,  -192,  -192,  -192,  -192,     4,     0,
    -192,     5,  -192,     6,     7,     0,     8,  -192,     9,     0,
       0,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,     0,     0,    24,    25,    26,
      27,     0,    28,     0,    29,     0,     0,     0,     0,    30,
      31,    32,     0,     0,    33,     0,     0,    34,     0,     0,
       0,     0,     0,    35,    36,     0,    37,     0,    38,    39,
      40,     4,     0,    41,     5,     0,     6,     7,     0,     8,
       0,     9,     0,     0,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,     0,     0,
      24,    25,     0,    27,     0,    28,   115,   116,     0,     0,
       0,     0,   117,    31,   118,     0,     0,    33,     0,     0,
      34,     0,     0,     0,     0,     0,    35,    36,     0,    37,
       0,    38,    39,    40,     4,     0,    41,     5,     0,     6,
       7,     0,     8,     0,     9,     0,     0,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,     0,     0,    24,    25,     0,    27,     0,    28,   197,
      29,     0,     0,     0,     0,    30,    31,    32,     0,     0,
      33,     0,     0,    34,     0,     0,     0,     0,     0,    35,
      36,     0,    37,     0,    38,    39,    40,     4,     0,    41,
       5,     0,     6,     7,     0,     8,     0,     9,     0,     0,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,     0,     0,    24,    25,     0,    27,
       0,    28,   210,    29,     0,     0,     0,     0,    30,    31,
      32,     0,     0,    33,     0,     0,    34,     0,     0,     0,
       0,     0,    35,    36,     0,    37,     0,    38,    39,    40,
       4,     0,    41,     5,     0,     6,     7,     0,     8,     0,
       9,     0,     0,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,     0,     0,    24,
      25,     0,    27,     0,    28,     0,    29,     0,     0,     0,
       0,    30,    31,    32,     0,     0,    33,     0,     0,    34,
       0,     0,     0,     0,     0,    35,    36,     0,    37,     6,
      38,    39,    40,     0,     0,    41,     0,    10,    11,     0,
       0,    14,     0,    16,    17,     0,    19,     0,    21,     0,
       0,     0,     0,    24,    25,     0,    27,     0,    28,     0,
      87,     0,     0,     0,     0,    30,    31,    32,     0,     0,
      88,     0,     0,     0,     0,     0,     0,     0,     0,    35,
      36,     0,    37,     6,    38,    39,    40,     0,     0,    41,
     247,    10,    11,     0,     0,    14,     0,    16,    17,     0,
      19,   179,    21,     0,     0,     0,     0,    24,    25,     0,
      27,     0,    28,     0,    87,     0,     0,     0,     0,    30,
      31,    32,     0,     0,    88,     0,     0,     0,     0,     0,
       0,     0,     0,    35,    36,     0,    37,     6,    38,    39,
      40,     0,     0,    41,     0,    10,    11,     0,     0,    14,
       0,    16,    17,     0,    19,     0,    21,     0,     0,     0,
       0,    24,    25,     0,    27,     0,    28,     0,    87,     0,
       0,     0,     0,    30,    31,    32,     0,     0,    88,     0,
       0,     0,     0,     0,     0,     0,     0,    35,    36,     0,
      37,     6,    38,    39,    40,   325,     0,    41,     0,    10,
      11,     0,     0,    14,     0,    16,    17,     0,    19,     0,
      21,     0,     0,     0,     0,    24,    25,     0,    27,     0,
      28,     0,    87,     0,     0,     0,     0,    30,    31,    32,
       0,     0,    88,    10,    11,     0,     0,    14,     0,    16,
      17,    35,    36,     0,    37,     0,    38,    39,    40,     0,
       0,    41,    27,     0,    28,     0,    87,     0,     0,     0,
       0,    30,    31,    32,     0,     0,    88,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    37,     0,
       0,     0,    40,     0,     0,    41
};

static const yytype_int16 yycheck[] =
{
       2,    12,   110,     2,    15,     7,     9,     1,     7,    18,
      13,   222,    85,    86,   349,     1,   260,   304,   262,    22,
      23,    31,    39,     1,   300,     7,    99,   271,   272,   102,
      41,    33,    15,     0,   107,    96,     1,    49,     1,   374,
       6,    44,     1,    78,    54,    39,     1,     5,    39,     6,
     337,    47,   296,    19,    12,    21,    38,     1,    24,    25,
      43,    53,    19,    98,    21,   131,    52,    24,    25,    35,
      36,   137,    38,    39,    68,   141,     1,    36,    35,    36,
     356,    38,    39,   370,    95,    50,   147,   148,   149,   376,
     101,    69,   368,    55,   160,   161,   340,    52,   164,   110,
     111,   103,    56,    31,    69,    68,    50,    50,    52,    68,
      14,    15,   114,    50,    50,   114,   327,    38,   120,    14,
      30,   120,    57,    66,    41,    50,    54,    52,    42,    50,
      96,   134,    69,    69,    50,    28,   202,    32,    33,    43,
      35,    50,    50,    50,   252,    40,    32,    33,   214,    35,
      66,   162,    58,    59,    40,   166,    51,    66,    66,    66,
      38,    39,   270,    50,    39,    51,    44,    50,    46,    44,
      53,    46,   175,    53,   140,    51,   142,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     199,   371,   372,   301,    53,   198,   153,   154,   155,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,    53,
     271,   272,    32,    33,    32,    33,    39,    35,   284,   195,
     151,   152,    40,   289,    65,   291,    67,    68,    60,    61,
      62,   207,   208,    51,    65,   296,    67,    68,    39,    39,
      39,   252,    50,    30,    55,    57,    56,    41,    12,   225,
      39,    53,    50,   255,    50,    50,   255,    14,   324,   270,
     333,    51,    39,    49,    39,    53,    50,    49,    10,     4,
      53,    49,    38,    53,     0,    49,    38,   279,   280,   340,
     279,   280,   285,    38,    38,   372,   205,   362,   344,   200,
     301,   299,   140,   304,   260,   261,   262,   263,   264,   265,
     266,   267,   268,   269,    75,   271,   272,   142,   261,   143,
     286,   263,   145,   264,   144,   265,    10,   266,   294,   146,
     267,   150,    10,   215,   215,   213,   337,     5,    -1,    -1,
     296,   342,   334,    -1,    -1,   334,    -1,   346,    -1,   341,
     316,    -1,   341,    -1,    -1,   321,    -1,    -1,   350,    -1,
      -1,   350,    -1,   355,    -1,    -1,   355,    -1,    -1,   370,
      -1,    -1,   338,    -1,    -1,   376,    -1,    -1,    -1,   371,
     372,    -1,   348,    -1,   340,    -1,   378,    -1,    -1,   378,
      -1,    -1,   384,    -1,   386,   384,    -1,   386,    -1,     1,
      -1,    -1,    -1,    -1,    -1,    -1,     8,    -1,    -1,    -1,
      -1,   377,    -1,    -1,    16,    17,    -1,   383,    20,    -1,
      22,    23,    -1,    25,    -1,    27,    -1,    -1,    -1,    -1,
      32,    33,    -1,    35,    -1,    37,    -1,    39,     1,    -1,
      -1,    -1,    44,    45,    46,    -1,    -1,    49,    -1,    -1,
      -1,    14,    15,    -1,    -1,    -1,    58,    59,    -1,    61,
      -1,    63,    64,    65,    -1,    -1,    68,    30,    31,    -1,
      -1,     1,    -1,    -1,    -1,    -1,    -1,    40,    41,    42,
      43,    -1,    -1,    -1,    14,    15,    -1,    50,    51,    -1,
      -1,    54,    55,    56,    57,    -1,    -1,    60,    -1,    62,
      30,    31,    -1,    -1,    67,    -1,    -1,    -1,    -1,    -1,
      40,    41,    42,    43,    -1,    -1,    -1,    -1,    -1,    -1,
      50,    51,    -1,    -1,    54,    55,    56,    57,     3,    -1,
      60,     6,    62,     8,     9,    -1,    11,    67,    13,    -1,
      -1,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    -1,    -1,    32,    33,    34,
      35,    -1,    37,    -1,    39,    -1,    -1,    -1,    -1,    44,
      45,    46,    -1,    -1,    49,    -1,    -1,    52,    -1,    -1,
      -1,    -1,    -1,    58,    59,    -1,    61,    -1,    63,    64,
      65,     3,    -1,    68,     6,    -1,     8,     9,    -1,    11,
      -1,    13,    -1,    -1,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    -1,    -1,
      32,    33,    -1,    35,    -1,    37,    38,    39,    -1,    -1,
      -1,    -1,    44,    45,    46,    -1,    -1,    49,    -1,    -1,
      52,    -1,    -1,    -1,    -1,    -1,    58,    59,    -1,    61,
      -1,    63,    64,    65,     3,    -1,    68,     6,    -1,     8,
       9,    -1,    11,    -1,    13,    -1,    -1,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    -1,    -1,    32,    33,    -1,    35,    -1,    37,    38,
      39,    -1,    -1,    -1,    -1,    44,    45,    46,    -1,    -1,
      49,    -1,    -1,    52,    -1,    -1,    -1,    -1,    -1,    58,
      59,    -1,    61,    -1,    63,    64,    65,     3,    -1,    68,
       6,    -1,     8,     9,    -1,    11,    -1,    13,    -1,    -1,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    -1,    -1,    32,    33,    -1,    35,
      -1,    37,    38,    39,    -1,    -1,    -1,    -1,    44,    45,
      46,    -1,    -1,    49,    -1,    -1,    52,    -1,    -1,    -1,
      -1,    -1,    58,    59,    -1,    61,    -1,    63,    64,    65,
       3,    -1,    68,     6,    -1,     8,     9,    -1,    11,    -1,
      13,    -1,    -1,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    -1,    -1,    32,
      33,    -1,    35,    -1,    37,    -1,    39,    -1,    -1,    -1,
      -1,    44,    45,    46,    -1,    -1,    49,    -1,    -1,    52,
      -1,    -1,    -1,    -1,    -1,    58,    59,    -1,    61,     8,
      63,    64,    65,    -1,    -1,    68,    -1,    16,    17,    -1,
      -1,    20,    -1,    22,    23,    -1,    25,    -1,    27,    -1,
      -1,    -1,    -1,    32,    33,    -1,    35,    -1,    37,    -1,
      39,    -1,    -1,    -1,    -1,    44,    45,    46,    -1,    -1,
      49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,
      59,    -1,    61,     8,    63,    64,    65,    -1,    -1,    68,
      69,    16,    17,    -1,    -1,    20,    -1,    22,    23,    -1,
      25,    26,    27,    -1,    -1,    -1,    -1,    32,    33,    -1,
      35,    -1,    37,    -1,    39,    -1,    -1,    -1,    -1,    44,
      45,    46,    -1,    -1,    49,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    58,    59,    -1,    61,     8,    63,    64,
      65,    -1,    -1,    68,    -1,    16,    17,    -1,    -1,    20,
      -1,    22,    23,    -1,    25,    -1,    27,    -1,    -1,    -1,
      -1,    32,    33,    -1,    35,    -1,    37,    -1,    39,    -1,
      -1,    -1,    -1,    44,    45,    46,    -1,    -1,    49,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,    59,    -1,
      61,     8,    63,    64,    65,    66,    -1,    68,    -1,    16,
      17,    -1,    -1,    20,    -1,    22,    23,    -1,    25,    -1,
      27,    -1,    -1,    -1,    -1,    32,    33,    -1,    35,    -1,
      37,    -1,    39,    -1,    -1,    -1,    -1,    44,    45,    46,
      -1,    -1,    49,    16,    17,    -1,    -1,    20,    -1,    22,
      23,    58,    59,    -1,    61,    -1,    63,    64,    65,    -1,
      -1,    68,    35,    -1,    37,    -1,    39,    -1,    -1,    -1,
      -1,    44,    45,    46,    -1,    -1,    49,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    61,    -1,
      -1,    -1,    65,    -1,    -1,    68
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    71,    73,     0,     3,     6,     8,     9,    11,    13,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    32,    33,    34,    35,    37,    39,
      44,    45,    46,    49,    52,    58,    59,    61,    63,    64,
      65,    68,    72,    74,    75,    79,    82,    83,    92,    93,
      94,    95,   100,   101,   102,   103,   104,   105,   111,   112,
     117,   121,   123,   125,   127,   129,   131,   133,   135,   137,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   150,
     151,   155,   159,   160,    39,   158,   158,    39,    49,    74,
     142,   144,    79,     1,    68,   162,   162,   145,   146,   115,
     117,   162,   117,    49,    82,   142,    39,    84,    86,   142,
     162,   162,   142,   142,    53,    38,    39,    44,    46,    79,
      80,   156,   157,   142,   142,   142,   142,    50,    66,   152,
     153,   154,   117,    47,    39,   162,     1,    50,    52,   161,
      31,    54,    30,    55,    56,    57,    41,    14,    15,    43,
      42,    58,    59,    60,    61,    62,    32,    33,    35,    40,
      51,   120,    65,    67,    68,   148,    65,    67,   148,   161,
     161,    38,    39,    44,    46,    28,     1,   116,   117,    26,
     118,   119,   122,   124,   126,   128,   130,   132,   134,   136,
     138,   139,   144,   148,   161,   117,   161,    38,     5,    12,
     113,   114,    51,    88,    89,    50,   161,   116,   117,    79,
      38,    79,    38,    50,    53,    50,    66,    50,    66,   121,
      69,    36,   162,    39,    77,    78,   121,   127,   121,   129,
     131,   133,   135,   137,   139,   139,   139,   140,   141,   141,
     142,   142,   142,   121,   121,   117,    39,    69,   121,   149,
     117,    39,   162,     1,    69,   163,    39,    85,    87,    96,
      50,    31,    54,    30,    55,    56,    57,    41,    15,    43,
      14,    51,   120,   163,   162,    82,   114,   121,    86,   163,
     163,   157,   121,   153,   154,    39,    78,    50,   163,    53,
      66,    50,    69,    66,   116,    79,    51,    90,    91,    50,
      98,    14,     1,    52,   164,   122,   144,   128,   122,   130,
     132,   134,   136,   138,   139,   139,   116,   122,   122,    49,
     106,    39,    79,    79,    53,    66,   121,   162,   163,    39,
      49,   121,   121,   163,    10,   122,    87,   164,   116,   115,
      53,   163,     4,   107,   108,   109,   163,   121,    78,    49,
      73,    76,   161,    79,   115,   163,    97,   122,    79,   117,
       7,    38,   110,   109,    82,   163,    76,    38,    99,    79,
     164,    53,    53,   107,    49,    38,   164,   115,    80,    81,
      81,    38,    76,   115,   163,    38,   163,    79,    79
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    70,    71,    72,    72,    73,    73,    74,    74,    74,
      75,    76,    77,    77,    78,    78,    79,    79,    79,    79,
      79,    79,    79,    79,    79,    79,    79,    79,    79,    79,
      79,    80,    80,    81,    81,    82,    82,    83,    84,    84,
      85,    85,    86,    87,    88,    88,    89,    90,    90,    91,
      92,    93,    94,    94,    95,    95,    96,    97,    95,    98,
      99,    95,    95,    95,   100,   101,   102,   103,   104,   105,
     106,   106,   107,   107,   108,   108,   109,   110,   111,   112,
     112,   112,   113,   114,   115,   115,   116,   116,   117,   117,
     118,   118,   119,   119,   120,   120,   121,   121,   121,   122,
     122,   122,   123,   123,   124,   124,   125,   125,   126,   126,
     127,   127,   128,   128,   129,   129,   130,   130,   131,   131,
     132,   132,   133,   133,   134,   134,   135,   135,   136,   136,
     137,   137,   137,   137,   138,   138,   138,   139,   139,   140,
     140,   140,   141,   141,   141,   141,   142,   142,   142,   142,
     142,   142,   142,   142,   142,   142,   143,   143,   143,   144,
     144,   145,   145,   146,   146,   146,   146,   146,   147,   147,
     147,   147,   148,   148,   149,   149,   150,   150,   150,   150,
     150,   150,   151,   151,   151,   151,   152,   152,   153,   153,
     154,   154,   155,   155,   156,   156,   157,   157,   157,   158,
     158,   159,   159,   159,   159,   159,   159,   160,   160,   160,
     161,   161,   162,   162,   163,   163,   164,   164
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     3,     1,     0,     0,     2,     7,     8,    10,
       1,     1,     1,     3,     0,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     0,     1,     3,     2,     3,     1,     3,
       1,     3,     2,     2,     0,     1,     2,     0,     1,     2,
       1,     2,     7,     5,     7,     5,     0,     0,    11,     0,
       0,    12,     7,     8,     3,     3,     3,     5,     3,     5,
       3,     5,     0,     1,     1,     2,     4,     3,     3,     3,
       3,     4,     5,     2,     0,     1,     1,     1,     1,     3,
       0,     1,     1,     3,     1,     1,     1,     3,     3,     1,
       3,     3,     1,     5,     1,     5,     1,     3,     1,     3,
       1,     3,     1,     3,     1,     3,     1,     3,     1,     3,
       1,     3,     1,     3,     1,     3,     1,     3,     1,     3,
       1,     3,     3,     3,     1,     3,     3,     1,     3,     1,
       3,     3,     1,     3,     3,     3,     1,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     2,     1,
       1,     1,     2,     1,     1,     4,     3,     3,     2,     2,
       4,     3,     2,     3,     1,     3,     1,     1,     1,     1,
       1,     3,     2,     3,     3,     5,     2,     4,     1,     2,
       0,     1,     2,     3,     3,     5,     1,     1,     1,     0,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1
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
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, parser_ctx_t *ctx)
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
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, parser_ctx_t *ctx)
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
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule, parser_ctx_t *ctx)
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
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, parser_ctx_t *ctx)
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
yyparse (parser_ctx_t *ctx)
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
#line 253 "parser.y" /* yacc.c:1646  */
    { program_parsed(ctx, (yyvsp[-2].source_elements)); }
#line 1843 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 3:
#line 256 "parser.y" /* yacc.c:1646  */
    {}
#line 1849 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 4:
#line 257 "parser.y" /* yacc.c:1646  */
    {}
#line 1855 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 5:
#line 261 "parser.y" /* yacc.c:1646  */
    { (yyval.source_elements) = new_source_elements(ctx); }
#line 1861 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 6:
#line 263 "parser.y" /* yacc.c:1646  */
    { (yyval.source_elements) = source_elements_add_statement((yyvsp[-1].source_elements), (yyvsp[0].statement)); }
#line 1867 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 7:
#line 268 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_function_expression(ctx, NULL, (yyvsp[-4].parameter_list), (yyvsp[-1].source_elements), NULL, (yyvsp[-6].srcptr), (yyvsp[0].srcptr)-(yyvsp[-6].srcptr)+1); }
#line 1873 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 8:
#line 270 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_function_expression(ctx, (yyvsp[-6].identifier), (yyvsp[-4].parameter_list), (yyvsp[-1].source_elements), NULL, (yyvsp[-7].srcptr), (yyvsp[0].srcptr)-(yyvsp[-7].srcptr)+1); }
#line 1879 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 9:
#line 272 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_function_expression(ctx, (yyvsp[-6].identifier), (yyvsp[-4].parameter_list), (yyvsp[-1].source_elements), (yyvsp[-8].identifier), (yyvsp[-9].srcptr), (yyvsp[0].srcptr)-(yyvsp[-9].srcptr)+1); }
#line 1885 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 10:
#line 275 "parser.y" /* yacc.c:1646  */
    { (yyval.srcptr) = (yyvsp[0].srcptr); }
#line 1891 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 11:
#line 279 "parser.y" /* yacc.c:1646  */
    { (yyval.source_elements) = (yyvsp[0].source_elements); }
#line 1897 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 12:
#line 283 "parser.y" /* yacc.c:1646  */
    { (yyval.parameter_list) = new_parameter_list(ctx, (yyvsp[0].identifier)); }
#line 1903 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 13:
#line 285 "parser.y" /* yacc.c:1646  */
    { (yyval.parameter_list) = parameter_list_add(ctx, (yyvsp[-2].parameter_list), (yyvsp[0].identifier)); }
#line 1909 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 14:
#line 289 "parser.y" /* yacc.c:1646  */
    { (yyval.parameter_list) = NULL; }
#line 1915 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 15:
#line 290 "parser.y" /* yacc.c:1646  */
    { (yyval.parameter_list) = (yyvsp[0].parameter_list); }
#line 1921 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 16:
#line 294 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1927 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 17:
#line 295 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1933 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 18:
#line 296 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1939 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 19:
#line 297 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_expression_statement(ctx, (yyvsp[0].expr)); }
#line 1945 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 20:
#line 298 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1951 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 21:
#line 299 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1957 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 22:
#line 300 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1963 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 23:
#line 301 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1969 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 24:
#line 302 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1975 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 25:
#line 303 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1981 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 26:
#line 304 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1987 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 27:
#line 305 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1993 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 28:
#line 306 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1999 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 29:
#line 307 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 2005 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 30:
#line 308 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 2011 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 31:
#line 312 "parser.y" /* yacc.c:1646  */
    { (yyval.statement_list) = new_statement_list(ctx, (yyvsp[0].statement)); }
#line 2017 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 32:
#line 314 "parser.y" /* yacc.c:1646  */
    { (yyval.statement_list) = statement_list_add((yyvsp[-1].statement_list), (yyvsp[0].statement)); }
#line 2023 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 33:
#line 318 "parser.y" /* yacc.c:1646  */
    { (yyval.statement_list) = NULL; }
#line 2029 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 34:
#line 319 "parser.y" /* yacc.c:1646  */
    { (yyval.statement_list) = (yyvsp[0].statement_list); }
#line 2035 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 35:
#line 323 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_block_statement(ctx, (yyvsp[-1].statement_list)); }
#line 2041 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 36:
#line 324 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_block_statement(ctx, NULL); }
#line 2047 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 37:
#line 329 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_var_statement(ctx, (yyvsp[-1].variable_list)); }
#line 2053 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 38:
#line 333 "parser.y" /* yacc.c:1646  */
    { (yyval.variable_list) = new_variable_list(ctx, (yyvsp[0].variable_declaration)); }
#line 2059 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 39:
#line 335 "parser.y" /* yacc.c:1646  */
    { (yyval.variable_list) = variable_list_add(ctx, (yyvsp[-2].variable_list), (yyvsp[0].variable_declaration)); }
#line 2065 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 40:
#line 340 "parser.y" /* yacc.c:1646  */
    { (yyval.variable_list) = new_variable_list(ctx, (yyvsp[0].variable_declaration)); }
#line 2071 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 41:
#line 342 "parser.y" /* yacc.c:1646  */
    { (yyval.variable_list) = variable_list_add(ctx, (yyvsp[-2].variable_list), (yyvsp[0].variable_declaration)); }
#line 2077 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 42:
#line 347 "parser.y" /* yacc.c:1646  */
    { (yyval.variable_declaration) = new_variable_declaration(ctx, (yyvsp[-1].identifier), (yyvsp[0].expr)); }
#line 2083 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 43:
#line 352 "parser.y" /* yacc.c:1646  */
    { (yyval.variable_declaration) = new_variable_declaration(ctx, (yyvsp[-1].identifier), (yyvsp[0].expr)); }
#line 2089 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 44:
#line 356 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = NULL; }
#line 2095 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 45:
#line 357 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2101 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 46:
#line 362 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2107 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 47:
#line 366 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = NULL; }
#line 2113 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 48:
#line 367 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2119 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 49:
#line 372 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2125 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 50:
#line 376 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_statement(ctx, STAT_EMPTY, 0); }
#line 2131 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 51:
#line 381 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_expression_statement(ctx, (yyvsp[-1].expr)); }
#line 2137 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 52:
#line 386 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_if_statement(ctx, (yyvsp[-4].expr), (yyvsp[-2].statement), (yyvsp[0].statement)); }
#line 2143 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 53:
#line 388 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_if_statement(ctx, (yyvsp[-2].expr), (yyvsp[0].statement), NULL); }
#line 2149 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 54:
#line 393 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_while_statement(ctx, TRUE, (yyvsp[-2].expr), (yyvsp[-5].statement)); }
#line 2155 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 55:
#line 395 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_while_statement(ctx, FALSE, (yyvsp[-2].expr), (yyvsp[0].statement)); }
#line 2161 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 56:
#line 397 "parser.y" /* yacc.c:1646  */
    { if(!explicit_error(ctx, (yyvsp[0].expr), ';')) YYABORT; }
#line 2167 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 57:
#line 399 "parser.y" /* yacc.c:1646  */
    { if(!explicit_error(ctx, (yyvsp[0].expr), ';')) YYABORT; }
#line 2173 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 58:
#line 401 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_for_statement(ctx, NULL, (yyvsp[-8].expr), (yyvsp[-5].expr), (yyvsp[-2].expr), (yyvsp[0].statement)); }
#line 2179 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 59:
#line 403 "parser.y" /* yacc.c:1646  */
    { if(!explicit_error(ctx, (yyvsp[0].variable_list), ';')) YYABORT; }
#line 2185 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 60:
#line 405 "parser.y" /* yacc.c:1646  */
    { if(!explicit_error(ctx, (yyvsp[0].expr), ';')) YYABORT; }
#line 2191 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 61:
#line 407 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_for_statement(ctx, (yyvsp[-8].variable_list), NULL, (yyvsp[-5].expr), (yyvsp[-2].expr), (yyvsp[0].statement)); }
#line 2197 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 62:
#line 409 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_forin_statement(ctx, NULL, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].statement)); }
#line 2203 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 63:
#line 411 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_forin_statement(ctx, (yyvsp[-4].variable_declaration), NULL, (yyvsp[-2].expr), (yyvsp[0].statement)); }
#line 2209 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 64:
#line 416 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_continue_statement(ctx, (yyvsp[-1].identifier)); }
#line 2215 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 65:
#line 421 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_break_statement(ctx, (yyvsp[-1].identifier)); }
#line 2221 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 66:
#line 426 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_return_statement(ctx, (yyvsp[-1].expr)); }
#line 2227 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 67:
#line 431 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_with_statement(ctx, (yyvsp[-2].expr), (yyvsp[0].statement)); }
#line 2233 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 68:
#line 436 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_labelled_statement(ctx, (yyvsp[-2].identifier), (yyvsp[0].statement)); }
#line 2239 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 69:
#line 441 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_switch_statement(ctx, (yyvsp[-2].expr), (yyvsp[0].case_clausule)); }
#line 2245 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 70:
#line 446 "parser.y" /* yacc.c:1646  */
    { (yyval.case_clausule) = new_case_block(ctx, (yyvsp[-1].case_list), NULL, NULL); }
#line 2251 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 71:
#line 448 "parser.y" /* yacc.c:1646  */
    { (yyval.case_clausule) = new_case_block(ctx, (yyvsp[-3].case_list), (yyvsp[-2].case_clausule), (yyvsp[-1].case_list)); }
#line 2257 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 72:
#line 452 "parser.y" /* yacc.c:1646  */
    { (yyval.case_list) = NULL; }
#line 2263 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 73:
#line 453 "parser.y" /* yacc.c:1646  */
    { (yyval.case_list) = (yyvsp[0].case_list); }
#line 2269 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 74:
#line 457 "parser.y" /* yacc.c:1646  */
    { (yyval.case_list) = new_case_list(ctx, (yyvsp[0].case_clausule)); }
#line 2275 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 75:
#line 459 "parser.y" /* yacc.c:1646  */
    { (yyval.case_list) = case_list_add(ctx, (yyvsp[-1].case_list), (yyvsp[0].case_clausule)); }
#line 2281 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 76:
#line 464 "parser.y" /* yacc.c:1646  */
    { (yyval.case_clausule) = new_case_clausule(ctx, (yyvsp[-2].expr), (yyvsp[0].statement_list)); }
#line 2287 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 77:
#line 469 "parser.y" /* yacc.c:1646  */
    { (yyval.case_clausule) = new_case_clausule(ctx, NULL, (yyvsp[0].statement_list)); }
#line 2293 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 78:
#line 474 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_throw_statement(ctx, (yyvsp[-1].expr)); }
#line 2299 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 79:
#line 478 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[-1].statement), (yyvsp[0].catch_block), NULL); }
#line 2305 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 80:
#line 479 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[-1].statement), NULL, (yyvsp[0].statement)); }
#line 2311 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 81:
#line 481 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[-2].statement), (yyvsp[-1].catch_block), (yyvsp[0].statement)); }
#line 2317 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 82:
#line 486 "parser.y" /* yacc.c:1646  */
    { (yyval.catch_block) = new_catch_block(ctx, (yyvsp[-2].identifier), (yyvsp[0].statement)); }
#line 2323 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 83:
#line 490 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 2329 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 84:
#line 494 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = NULL; }
#line 2335 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 85:
#line 495 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2341 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 86:
#line 498 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2347 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 87:
#line 499 "parser.y" /* yacc.c:1646  */
    { set_error(ctx, JS_E_SYNTAX); YYABORT; }
#line 2353 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 88:
#line 503 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2359 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 89:
#line 505 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_COMMA, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2365 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 90:
#line 509 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = NULL; }
#line 2371 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 91:
#line 510 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2377 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 92:
#line 515 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2383 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 93:
#line 517 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_COMMA, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2389 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 94:
#line 520 "parser.y" /* yacc.c:1646  */
    { (yyval.ival) = (yyvsp[0].ival); }
#line 2395 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 95:
#line 521 "parser.y" /* yacc.c:1646  */
    { (yyval.ival) = EXPR_ASSIGNDIV; }
#line 2401 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 96:
#line 525 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2407 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 97:
#line 527 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ASSIGN, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2413 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 98:
#line 529 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2419 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 99:
#line 534 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2425 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 100:
#line 536 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ASSIGN, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2431 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 101:
#line 538 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2437 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 102:
#line 542 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2443 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 103:
#line 544 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_conditional_expression(ctx, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2449 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 104:
#line 549 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2455 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 105:
#line 551 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_conditional_expression(ctx, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2461 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 106:
#line 555 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2467 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 107:
#line 557 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_OR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2473 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 108:
#line 562 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2479 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 109:
#line 564 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_OR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2485 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 110:
#line 568 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2491 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 111:
#line 570 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_AND, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2497 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 112:
#line 575 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2503 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 113:
#line 577 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_AND, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2509 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 114:
#line 581 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2515 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 115:
#line 583 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BOR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2521 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 116:
#line 588 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2527 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 117:
#line 590 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BOR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2533 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 118:
#line 594 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2539 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 119:
#line 596 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BXOR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2545 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 120:
#line 601 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2551 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 121:
#line 603 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BXOR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2557 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 122:
#line 607 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2563 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 123:
#line 609 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BAND, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2569 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 124:
#line 614 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2575 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 125:
#line 616 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BAND, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2581 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 126:
#line 620 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2587 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 127:
#line 622 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2593 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 128:
#line 626 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2599 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 129:
#line 628 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2605 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 130:
#line 632 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2611 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 131:
#line 634 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2617 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 132:
#line 636 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_INSTANCEOF, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2623 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 133:
#line 638 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_IN, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2629 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 134:
#line 642 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2635 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 135:
#line 644 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2641 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 136:
#line 646 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_INSTANCEOF, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2647 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 137:
#line 650 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2653 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 138:
#line 652 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2659 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 139:
#line 657 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2665 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 140:
#line 659 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ADD, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2671 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 141:
#line 661 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_SUB, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2677 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 142:
#line 665 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2683 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 143:
#line 667 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_MUL, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2689 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 144:
#line 669 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_DIV, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2695 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 145:
#line 671 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_MOD, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2701 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 146:
#line 675 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2707 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 147:
#line 677 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_unary_expression(ctx, EXPR_DELETE, (yyvsp[0].expr)); }
#line 2713 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 148:
#line 678 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_unary_expression(ctx, EXPR_VOID, (yyvsp[0].expr)); }
#line 2719 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 149:
#line 680 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_unary_expression(ctx, EXPR_TYPEOF, (yyvsp[0].expr)); }
#line 2725 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 150:
#line 681 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PREINC, (yyvsp[0].expr)); }
#line 2731 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 151:
#line 682 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PREDEC, (yyvsp[0].expr)); }
#line 2737 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 152:
#line 683 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PLUS, (yyvsp[0].expr)); }
#line 2743 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 153:
#line 684 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_unary_expression(ctx, EXPR_MINUS, (yyvsp[0].expr)); }
#line 2749 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 154:
#line 685 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_unary_expression(ctx, EXPR_BITNEG, (yyvsp[0].expr)); }
#line 2755 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 155:
#line 686 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_unary_expression(ctx, EXPR_LOGNEG, (yyvsp[0].expr)); }
#line 2761 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 156:
#line 691 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2767 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 157:
#line 693 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_unary_expression(ctx, EXPR_POSTINC, (yyvsp[-1].expr)); }
#line 2773 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 158:
#line 695 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_unary_expression(ctx, EXPR_POSTDEC, (yyvsp[-1].expr)); }
#line 2779 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 159:
#line 700 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2785 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 160:
#line 701 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2791 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 161:
#line 705 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2797 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 162:
#line 706 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_new_expression(ctx, (yyvsp[0].expr), NULL); }
#line 2803 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 163:
#line 710 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2809 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 164:
#line 711 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2815 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 165:
#line 713 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ARRAY, (yyvsp[-3].expr), (yyvsp[-1].expr)); }
#line 2821 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 166:
#line 715 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_member_expression(ctx, (yyvsp[-2].expr), (yyvsp[0].identifier)); }
#line 2827 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 167:
#line 717 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_new_expression(ctx, (yyvsp[-1].expr), (yyvsp[0].argument_list)); }
#line 2833 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 168:
#line 722 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_call_expression(ctx, (yyvsp[-1].expr), (yyvsp[0].argument_list)); }
#line 2839 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 169:
#line 724 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_call_expression(ctx, (yyvsp[-1].expr), (yyvsp[0].argument_list)); }
#line 2845 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 170:
#line 726 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ARRAY, (yyvsp[-3].expr), (yyvsp[-1].expr)); }
#line 2851 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 171:
#line 728 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_member_expression(ctx, (yyvsp[-2].expr), (yyvsp[0].identifier)); }
#line 2857 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 172:
#line 732 "parser.y" /* yacc.c:1646  */
    { (yyval.argument_list) = NULL; }
#line 2863 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 173:
#line 733 "parser.y" /* yacc.c:1646  */
    { (yyval.argument_list) = (yyvsp[-1].argument_list); }
#line 2869 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 174:
#line 737 "parser.y" /* yacc.c:1646  */
    { (yyval.argument_list) = new_argument_list(ctx, (yyvsp[0].expr)); }
#line 2875 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 175:
#line 739 "parser.y" /* yacc.c:1646  */
    { (yyval.argument_list) = argument_list_add(ctx, (yyvsp[-2].argument_list), (yyvsp[0].expr)); }
#line 2881 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 176:
#line 743 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_expression(ctx, EXPR_THIS, 0); }
#line 2887 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 177:
#line 744 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_identifier_expression(ctx, (yyvsp[0].identifier)); }
#line 2893 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 178:
#line 745 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_literal_expression(ctx, (yyvsp[0].literal)); }
#line 2899 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 179:
#line 746 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2905 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 180:
#line 747 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2911 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 181:
#line 748 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 2917 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 182:
#line 752 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_array_literal_expression(ctx, NULL, 0); }
#line 2923 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 183:
#line 753 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_array_literal_expression(ctx, NULL, (yyvsp[-1].ival)+1); }
#line 2929 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 184:
#line 754 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_array_literal_expression(ctx, (yyvsp[-1].element_list), 0); }
#line 2935 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 185:
#line 756 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_array_literal_expression(ctx, (yyvsp[-3].element_list), (yyvsp[-1].ival)+1); }
#line 2941 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 186:
#line 761 "parser.y" /* yacc.c:1646  */
    { (yyval.element_list) = new_element_list(ctx, (yyvsp[-1].ival), (yyvsp[0].expr)); }
#line 2947 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 187:
#line 763 "parser.y" /* yacc.c:1646  */
    { (yyval.element_list) = element_list_add(ctx, (yyvsp[-3].element_list), (yyvsp[-1].ival), (yyvsp[0].expr)); }
#line 2953 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 188:
#line 767 "parser.y" /* yacc.c:1646  */
    { (yyval.ival) = 1; }
#line 2959 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 189:
#line 768 "parser.y" /* yacc.c:1646  */
    { (yyval.ival) = (yyvsp[-1].ival) + 1; }
#line 2965 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 190:
#line 772 "parser.y" /* yacc.c:1646  */
    { (yyval.ival) = 0; }
#line 2971 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 191:
#line 773 "parser.y" /* yacc.c:1646  */
    { (yyval.ival) = (yyvsp[0].ival); }
#line 2977 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 192:
#line 777 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_prop_and_value_expression(ctx, NULL); }
#line 2983 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 193:
#line 779 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_prop_and_value_expression(ctx, (yyvsp[-1].property_list)); }
#line 2989 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 194:
#line 784 "parser.y" /* yacc.c:1646  */
    { (yyval.property_list) = new_property_list(ctx, (yyvsp[-2].literal), (yyvsp[0].expr)); }
#line 2995 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 195:
#line 786 "parser.y" /* yacc.c:1646  */
    { (yyval.property_list) = property_list_add(ctx, (yyvsp[-4].property_list), (yyvsp[-2].literal), (yyvsp[0].expr)); }
#line 3001 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 196:
#line 790 "parser.y" /* yacc.c:1646  */
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[0].identifier)); }
#line 3007 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 197:
#line 791 "parser.y" /* yacc.c:1646  */
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[0].wstr)); }
#line 3013 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 198:
#line 792 "parser.y" /* yacc.c:1646  */
    { (yyval.literal) = (yyvsp[0].literal); }
#line 3019 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 199:
#line 796 "parser.y" /* yacc.c:1646  */
    { (yyval.identifier) = NULL; }
#line 3025 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 200:
#line 797 "parser.y" /* yacc.c:1646  */
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3031 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 201:
#line 801 "parser.y" /* yacc.c:1646  */
    { (yyval.literal) = new_null_literal(ctx); }
#line 3037 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 202:
#line 802 "parser.y" /* yacc.c:1646  */
    { (yyval.literal) = (yyvsp[0].literal); }
#line 3043 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 203:
#line 803 "parser.y" /* yacc.c:1646  */
    { (yyval.literal) = (yyvsp[0].literal); }
#line 3049 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 204:
#line 804 "parser.y" /* yacc.c:1646  */
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[0].wstr)); }
#line 3055 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 205:
#line 805 "parser.y" /* yacc.c:1646  */
    { (yyval.literal) = parse_regexp(ctx);
                                  if(!(yyval.literal)) YYABORT; }
#line 3062 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 206:
#line 807 "parser.y" /* yacc.c:1646  */
    { (yyval.literal) = parse_regexp(ctx);
                                  if(!(yyval.literal)) YYABORT; }
#line 3069 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 207:
#line 812 "parser.y" /* yacc.c:1646  */
    { (yyval.literal) = new_boolean_literal(ctx, VARIANT_TRUE); }
#line 3075 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 208:
#line 813 "parser.y" /* yacc.c:1646  */
    { (yyval.literal) = new_boolean_literal(ctx, VARIANT_FALSE); }
#line 3081 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 209:
#line 814 "parser.y" /* yacc.c:1646  */
    { (yyval.literal) = (yyvsp[0].literal); }
#line 3087 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 211:
#line 818 "parser.y" /* yacc.c:1646  */
    { if(!allow_auto_semicolon(ctx)) {YYABORT;} }
#line 3093 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 213:
#line 822 "parser.y" /* yacc.c:1646  */
    { set_error(ctx, JS_E_MISSING_LBRACKET); YYABORT; }
#line 3099 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 215:
#line 826 "parser.y" /* yacc.c:1646  */
    { set_error(ctx, JS_E_MISSING_RBRACKET); YYABORT; }
#line 3105 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 217:
#line 830 "parser.y" /* yacc.c:1646  */
    { set_error(ctx, JS_E_MISSING_SEMICOLON); YYABORT; }
#line 3111 "parser.tab.c" /* yacc.c:1646  */
    break;


#line 3115 "parser.tab.c" /* yacc.c:1646  */
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
#line 832 "parser.y" /* yacc.c:1906  */


static BOOL allow_auto_semicolon(parser_ctx_t *ctx)
{
    return ctx->nl || ctx->ptr == ctx->end || *(ctx->ptr-1) == '}';
}

static void *new_statement(parser_ctx_t *ctx, statement_type_t type, size_t size)
{
    statement_t *stat;

    stat = parser_alloc(ctx, size ? size : sizeof(*stat));
    if(!stat)
        return NULL;

    stat->type = type;
    stat->next = NULL;

    return stat;
}

static literal_t *new_string_literal(parser_ctx_t *ctx, const WCHAR *str)
{
    literal_t *ret = parser_alloc(ctx, sizeof(literal_t));

    ret->type = LT_STRING;
    ret->u.wstr = str;

    return ret;
}

static literal_t *new_null_literal(parser_ctx_t *ctx)
{
    literal_t *ret = parser_alloc(ctx, sizeof(literal_t));

    ret->type = LT_NULL;

    return ret;
}

static prop_val_t *new_prop_val(parser_ctx_t *ctx, literal_t *name, expression_t *value)
{
    prop_val_t *ret = parser_alloc(ctx, sizeof(prop_val_t));

    ret->name = name;
    ret->value = value;
    ret->next = NULL;

    return ret;
}

static property_list_t *new_property_list(parser_ctx_t *ctx, literal_t *name, expression_t *value)
{
    property_list_t *ret = parser_alloc_tmp(ctx, sizeof(property_list_t));

    ret->head = ret->tail = new_prop_val(ctx, name, value);

    return ret;
}

static property_list_t *property_list_add(parser_ctx_t *ctx, property_list_t *list, literal_t *name, expression_t *value)
{
    list->tail = list->tail->next = new_prop_val(ctx, name, value);

    return list;
}

static array_element_t *new_array_element(parser_ctx_t *ctx, int elision, expression_t *expr)
{
    array_element_t *ret = parser_alloc(ctx, sizeof(array_element_t));

    ret->elision = elision;
    ret->expr = expr;
    ret->next = NULL;

    return ret;
}

static element_list_t *new_element_list(parser_ctx_t *ctx, int elision, expression_t *expr)
{
    element_list_t *ret = parser_alloc_tmp(ctx, sizeof(element_list_t));

    ret->head = ret->tail = new_array_element(ctx, elision, expr);

    return ret;
}

static element_list_t *element_list_add(parser_ctx_t *ctx, element_list_t *list, int elision, expression_t *expr)
{
    list->tail = list->tail->next = new_array_element(ctx, elision, expr);

    return list;
}

static argument_t *new_argument(parser_ctx_t *ctx, expression_t *expr)
{
    argument_t *ret = parser_alloc(ctx, sizeof(argument_t));

    ret->expr = expr;
    ret->next = NULL;

    return ret;
}

static argument_list_t *new_argument_list(parser_ctx_t *ctx, expression_t *expr)
{
    argument_list_t *ret = parser_alloc_tmp(ctx, sizeof(argument_list_t));

    ret->head = ret->tail = new_argument(ctx, expr);

    return ret;
}

static argument_list_t *argument_list_add(parser_ctx_t *ctx, argument_list_t *list, expression_t *expr)
{
    list->tail = list->tail->next = new_argument(ctx, expr);

    return list;
}

static catch_block_t *new_catch_block(parser_ctx_t *ctx, const WCHAR *identifier, statement_t *statement)
{
    catch_block_t *ret = parser_alloc(ctx, sizeof(catch_block_t));

    ret->identifier = identifier;
    ret->statement = statement;

    return ret;
}

static case_clausule_t *new_case_clausule(parser_ctx_t *ctx, expression_t *expr, statement_list_t *stat_list)
{
    case_clausule_t *ret = parser_alloc(ctx, sizeof(case_clausule_t));

    ret->expr = expr;
    ret->stat = stat_list ? stat_list->head : NULL;
    ret->next = NULL;

    return ret;
}

static case_list_t *new_case_list(parser_ctx_t *ctx, case_clausule_t *case_clausule)
{
    case_list_t *ret = parser_alloc_tmp(ctx, sizeof(case_list_t));

    ret->head = ret->tail = case_clausule;

    return ret;
}

static case_list_t *case_list_add(parser_ctx_t *ctx, case_list_t *list, case_clausule_t *case_clausule)
{
    list->tail = list->tail->next = case_clausule;

    return list;
}

static case_clausule_t *new_case_block(parser_ctx_t *ctx, case_list_t *case_list1,
        case_clausule_t *default_clausule, case_list_t *case_list2)
{
    case_clausule_t *ret = NULL, *iter = NULL, *iter2;
    statement_t *stat = NULL;

    if(case_list1) {
        ret = case_list1->head;
        iter = case_list1->tail;
    }

    if(default_clausule) {
        if(ret)
            iter = iter->next = default_clausule;
        else
            ret = iter = default_clausule;
    }

    if(case_list2) {
        if(ret)
            iter->next = case_list2->head;
        else
            ret = case_list2->head;
    }

    if(!ret)
        return NULL;

    for(iter = ret; iter; iter = iter->next) {
        for(iter2 = iter; iter2 && !iter2->stat; iter2 = iter2->next);
        if(!iter2)
            break;

        while(iter != iter2) {
            iter->stat = iter2->stat;
            iter = iter->next;
        }

        if(stat) {
            while(stat->next)
                stat = stat->next;
            stat->next = iter->stat;
        }else {
            stat = iter->stat;
        }
    }

    return ret;
}

static statement_t *new_block_statement(parser_ctx_t *ctx, statement_list_t *list)
{
    block_statement_t *ret;

    ret = new_statement(ctx, STAT_BLOCK, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->stat_list = list ? list->head : NULL;

    return &ret->stat;
}

static variable_declaration_t *new_variable_declaration(parser_ctx_t *ctx, const WCHAR *identifier, expression_t *expr)
{
    variable_declaration_t *ret = parser_alloc(ctx, sizeof(variable_declaration_t));

    ret->identifier = identifier;
    ret->expr = expr;
    ret->next = NULL;
    ret->global_next = NULL;

    return ret;
}

static variable_list_t *new_variable_list(parser_ctx_t *ctx, variable_declaration_t *decl)
{
    variable_list_t *ret = parser_alloc_tmp(ctx, sizeof(variable_list_t));

    ret->head = ret->tail = decl;

    return ret;
}

static variable_list_t *variable_list_add(parser_ctx_t *ctx, variable_list_t *list, variable_declaration_t *decl)
{
    list->tail = list->tail->next = decl;

    return list;
}

static statement_t *new_var_statement(parser_ctx_t *ctx, variable_list_t *variable_list)
{
    var_statement_t *ret;

    ret = new_statement(ctx, STAT_VAR, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->variable_list = variable_list->head;

    return &ret->stat;
}

static statement_t *new_expression_statement(parser_ctx_t *ctx, expression_t *expr)
{
    expression_statement_t *ret;

    ret = new_statement(ctx, STAT_EXPR, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->expr = expr;

    return &ret->stat;
}

static statement_t *new_if_statement(parser_ctx_t *ctx, expression_t *expr, statement_t *if_stat, statement_t *else_stat)
{
    if_statement_t *ret;

    ret = new_statement(ctx, STAT_IF, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->expr = expr;
    ret->if_stat = if_stat;
    ret->else_stat = else_stat;

    return &ret->stat;
}

static statement_t *new_while_statement(parser_ctx_t *ctx, BOOL dowhile, expression_t *expr, statement_t *stat)
{
    while_statement_t *ret;

    ret = new_statement(ctx, STAT_WHILE, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->do_while = dowhile;
    ret->expr = expr;
    ret->statement = stat;

    return &ret->stat;
}

static statement_t *new_for_statement(parser_ctx_t *ctx, variable_list_t *variable_list, expression_t *begin_expr,
        expression_t *expr, expression_t *end_expr, statement_t *statement)
{
    for_statement_t *ret;

    ret = new_statement(ctx, STAT_FOR, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->variable_list = variable_list ? variable_list->head : NULL;
    ret->begin_expr = begin_expr;
    ret->expr = expr;
    ret->end_expr = end_expr;
    ret->statement = statement;

    return &ret->stat;
}

static statement_t *new_forin_statement(parser_ctx_t *ctx, variable_declaration_t *variable, expression_t *expr,
        expression_t *in_expr, statement_t *statement)
{
    forin_statement_t *ret;

    ret = new_statement(ctx, STAT_FORIN, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->variable = variable;
    ret->expr = expr;
    ret->in_expr = in_expr;
    ret->statement = statement;

    return &ret->stat;
}

static statement_t *new_continue_statement(parser_ctx_t *ctx, const WCHAR *identifier)
{
    branch_statement_t *ret;

    ret = new_statement(ctx, STAT_CONTINUE, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->identifier = identifier;

    return &ret->stat;
}

static statement_t *new_break_statement(parser_ctx_t *ctx, const WCHAR *identifier)
{
    branch_statement_t *ret;

    ret = new_statement(ctx, STAT_BREAK, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->identifier = identifier;

    return &ret->stat;
}

static statement_t *new_return_statement(parser_ctx_t *ctx, expression_t *expr)
{
    expression_statement_t *ret;

    ret = new_statement(ctx, STAT_RETURN, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->expr = expr;

    return &ret->stat;
}

static statement_t *new_with_statement(parser_ctx_t *ctx, expression_t *expr, statement_t *statement)
{
    with_statement_t *ret;

    ret = new_statement(ctx, STAT_WITH, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->expr = expr;
    ret->statement = statement;

    return &ret->stat;
}

static statement_t *new_labelled_statement(parser_ctx_t *ctx, const WCHAR *identifier, statement_t *statement)
{
    labelled_statement_t *ret;

    ret = new_statement(ctx, STAT_LABEL, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->identifier = identifier;
    ret->statement = statement;

    return &ret->stat;
}

static statement_t *new_switch_statement(parser_ctx_t *ctx, expression_t *expr, case_clausule_t *case_list)
{
    switch_statement_t *ret;

    ret = new_statement(ctx, STAT_SWITCH, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->expr = expr;
    ret->case_list = case_list;

    return &ret->stat;
}

static statement_t *new_throw_statement(parser_ctx_t *ctx, expression_t *expr)
{
    expression_statement_t *ret;

    ret = new_statement(ctx, STAT_THROW, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->expr = expr;

    return &ret->stat;
}

static statement_t *new_try_statement(parser_ctx_t *ctx, statement_t *try_statement,
       catch_block_t *catch_block, statement_t *finally_statement)
{
    try_statement_t *ret;

    ret = new_statement(ctx, STAT_TRY, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->try_statement = try_statement;
    ret->catch_block = catch_block;
    ret->finally_statement = finally_statement;

    return &ret->stat;
}

static parameter_t *new_parameter(parser_ctx_t *ctx, const WCHAR *identifier)
{
    parameter_t *ret = parser_alloc(ctx, sizeof(parameter_t));

    ret->identifier = identifier;
    ret->next = NULL;

    return ret;
}

static parameter_list_t *new_parameter_list(parser_ctx_t *ctx, const WCHAR *identifier)
{
    parameter_list_t *ret = parser_alloc_tmp(ctx, sizeof(parameter_list_t));

    ret->head = ret->tail = new_parameter(ctx, identifier);

    return ret;
}

static parameter_list_t *parameter_list_add(parser_ctx_t *ctx, parameter_list_t *list, const WCHAR *identifier)
{
    list->tail = list->tail->next = new_parameter(ctx, identifier);

    return list;
}

static expression_t *new_function_expression(parser_ctx_t *ctx, const WCHAR *identifier, parameter_list_t *parameter_list,
    source_elements_t *source_elements, const WCHAR *event_target, const WCHAR *src_str, DWORD src_len)
{
    function_expression_t *ret = new_expression(ctx, EXPR_FUNC, sizeof(*ret));

    ret->identifier = identifier;
    ret->parameter_list = parameter_list ? parameter_list->head : NULL;
    ret->source_elements = source_elements;
    ret->event_target = event_target;
    ret->src_str = src_str;
    ret->src_len = src_len;
    ret->next = NULL;

    return &ret->expr;
}

static void *new_expression(parser_ctx_t *ctx, expression_type_t type, size_t size)
{
    expression_t *ret = parser_alloc(ctx, size ? size : sizeof(*ret));

    ret->type = type;

    return ret;
}

static expression_t *new_binary_expression(parser_ctx_t *ctx, expression_type_t type,
       expression_t *expression1, expression_t *expression2)
{
    binary_expression_t *ret = new_expression(ctx, type, sizeof(*ret));

    ret->expression1 = expression1;
    ret->expression2 = expression2;

    return &ret->expr;
}

static expression_t *new_unary_expression(parser_ctx_t *ctx, expression_type_t type, expression_t *expression)
{
    unary_expression_t *ret = new_expression(ctx, type, sizeof(*ret));

    ret->expression = expression;

    return &ret->expr;
}

static expression_t *new_conditional_expression(parser_ctx_t *ctx, expression_t *expression,
       expression_t *true_expression, expression_t *false_expression)
{
    conditional_expression_t *ret = new_expression(ctx, EXPR_COND, sizeof(*ret));

    ret->expression = expression;
    ret->true_expression = true_expression;
    ret->false_expression = false_expression;

    return &ret->expr;
}

static expression_t *new_member_expression(parser_ctx_t *ctx, expression_t *expression, const WCHAR *identifier)
{
    member_expression_t *ret = new_expression(ctx, EXPR_MEMBER, sizeof(*ret));

    ret->expression = expression;
    ret->identifier = identifier;

    return &ret->expr;
}

static expression_t *new_new_expression(parser_ctx_t *ctx, expression_t *expression, argument_list_t *argument_list)
{
    call_expression_t *ret = new_expression(ctx, EXPR_NEW, sizeof(*ret));

    ret->expression = expression;
    ret->argument_list = argument_list ? argument_list->head : NULL;

    return &ret->expr;
}

static expression_t *new_call_expression(parser_ctx_t *ctx, expression_t *expression, argument_list_t *argument_list)
{
    call_expression_t *ret = new_expression(ctx, EXPR_CALL, sizeof(*ret));

    ret->expression = expression;
    ret->argument_list = argument_list ? argument_list->head : NULL;

    return &ret->expr;
}

static int parser_error(parser_ctx_t *ctx, const char *str)
{
    return 0;
}

static void set_error(parser_ctx_t *ctx, UINT error)
{
    ctx->hres = error;
}

static BOOL explicit_error(parser_ctx_t *ctx, void *obj, WCHAR next)
{
    if(obj || *(ctx->ptr-1)==next) return TRUE;

    set_error(ctx, JS_E_SYNTAX);
    return FALSE;
}


static expression_t *new_identifier_expression(parser_ctx_t *ctx, const WCHAR *identifier)
{
    identifier_expression_t *ret = new_expression(ctx, EXPR_IDENT, sizeof(*ret));

    ret->identifier = identifier;

    return &ret->expr;
}

static expression_t *new_array_literal_expression(parser_ctx_t *ctx, element_list_t *element_list, int length)
{
    array_literal_expression_t *ret = new_expression(ctx, EXPR_ARRAYLIT, sizeof(*ret));

    ret->element_list = element_list ? element_list->head : NULL;
    ret->length = length;

    return &ret->expr;
}

static expression_t *new_prop_and_value_expression(parser_ctx_t *ctx, property_list_t *property_list)
{
    property_value_expression_t *ret = new_expression(ctx, EXPR_PROPVAL, sizeof(*ret));

    ret->property_list = property_list ? property_list->head : NULL;

    return &ret->expr;
}

static expression_t *new_literal_expression(parser_ctx_t *ctx, literal_t *literal)
{
    literal_expression_t *ret = new_expression(ctx, EXPR_LITERAL, sizeof(*ret));

    ret->literal = literal;

    return &ret->expr;
}

static source_elements_t *new_source_elements(parser_ctx_t *ctx)
{
    source_elements_t *ret = parser_alloc(ctx, sizeof(source_elements_t));

    memset(ret, 0, sizeof(*ret));

    return ret;
}

static source_elements_t *source_elements_add_statement(source_elements_t *source_elements, statement_t *statement)
{
    if(source_elements->statement_tail)
        source_elements->statement_tail = source_elements->statement_tail->next = statement;
    else
        source_elements->statement = source_elements->statement_tail = statement;

    return source_elements;
}

static statement_list_t *new_statement_list(parser_ctx_t *ctx, statement_t *statement)
{
    statement_list_t *ret =  parser_alloc_tmp(ctx, sizeof(statement_list_t));

    ret->head = ret->tail = statement;

    return ret;
}

static statement_list_t *statement_list_add(statement_list_t *list, statement_t *statement)
{
    list->tail = list->tail->next = statement;

    return list;
}

static void program_parsed(parser_ctx_t *ctx, source_elements_t *source)
{
    ctx->source = source;
    if(!ctx->lexer_error)
        ctx->hres = S_OK;
}

void parser_release(parser_ctx_t *ctx)
{
    script_release(ctx->script);
    heap_pool_free(&ctx->heap);
    heap_free(ctx);
}

HRESULT script_parse(script_ctx_t *ctx, const WCHAR *code, const WCHAR *delimiter, BOOL from_eval,
        parser_ctx_t **ret)
{
    parser_ctx_t *parser_ctx;
    heap_pool_t *mark;
    HRESULT hres;

    const WCHAR html_tagW[] = {'<','/','s','c','r','i','p','t','>',0};

    parser_ctx = heap_alloc_zero(sizeof(parser_ctx_t));
    if(!parser_ctx)
        return E_OUTOFMEMORY;

    parser_ctx->hres = JS_E_SYNTAX;
    parser_ctx->is_html = delimiter && !strcmpiW(delimiter, html_tagW);

    parser_ctx->begin = parser_ctx->ptr = code;
    parser_ctx->end = parser_ctx->begin + strlenW(parser_ctx->begin);

    script_addref(ctx);
    parser_ctx->script = ctx;

    mark = heap_pool_mark(&ctx->tmp_heap);
    heap_pool_init(&parser_ctx->heap);

    parser_parse(parser_ctx);
    heap_pool_clear(mark);
    hres = parser_ctx->hres;
    if(FAILED(hres)) {
        WARN("parser failed around %s\n",
            debugstr_w(parser_ctx->begin+20 > parser_ctx->ptr ? parser_ctx->begin : parser_ctx->ptr-20));
        parser_release(parser_ctx);
        return hres;
    }

    *ret = parser_ctx;
    return S_OK;
}
