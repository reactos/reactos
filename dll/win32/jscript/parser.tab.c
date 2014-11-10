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
        source_elements_t*,const WCHAR*,DWORD);
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


#line 192 "parser.tab.c" /* yacc.c:339  */

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
    kFUNCTION = 291,
    tIdentifier = 292,
    tAssignOper = 293,
    tEqOper = 294,
    tShiftOper = 295,
    tRelOper = 296,
    tNumericLiteral = 297,
    tBooleanLiteral = 298,
    tStringLiteral = 299,
    tEOF = 300,
    LOWER_THAN_ELSE = 301
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 144 "parser.y" /* yacc.c:355  */

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

#line 297 "parser.tab.c" /* yacc.c:355  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int parser_parse (parser_ctx_t *ctx);



/* Copy the second part of user declarations.  */

#line 311 "parser.tab.c" /* yacc.c:358  */

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
#define YYLAST   1053

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  69
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  95
/* YYNRULES -- Number of rules.  */
#define YYNRULES  215
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  375

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   301

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    63,     2,     2,     2,    61,    56,     2,
      67,    68,    59,    57,    49,    58,    66,    60,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    52,    51,
       2,    50,     2,    53,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    64,     2,    65,    55,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    48,    54,    37,    62,     2,     2,     2,
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
      35,    36,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   251,   251,   255,   256,   260,   261,   266,   270,   274,
     278,   279,   284,   285,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   307,
     308,   313,   314,   318,   319,   323,   328,   329,   334,   336,
     341,   346,   351,   352,   356,   361,   362,   366,   371,   375,
     380,   382,   387,   389,   392,   394,   391,   398,   400,   397,
     403,   405,   410,   415,   420,   425,   430,   435,   440,   442,
     447,   448,   452,   453,   458,   463,   468,   473,   474,   475,
     480,   485,   489,   490,   493,   494,   498,   499,   504,   505,
     509,   511,   515,   516,   520,   521,   523,   528,   530,   532,
     537,   538,   543,   545,   550,   551,   556,   558,   563,   564,
     569,   571,   576,   577,   582,   584,   589,   590,   595,   597,
     602,   603,   608,   610,   615,   616,   621,   622,   627,   628,
     630,   632,   637,   638,   640,   645,   646,   651,   653,   655,
     660,   661,   663,   665,   670,   671,   673,   674,   676,   677,
     678,   679,   680,   681,   685,   687,   689,   695,   696,   700,
     701,   705,   706,   707,   709,   711,   716,   718,   720,   722,
     727,   728,   732,   733,   738,   739,   740,   741,   742,   743,
     747,   748,   749,   750,   755,   757,   762,   763,   767,   768,
     772,   773,   778,   780,   785,   786,   787,   791,   792,   796,
     797,   798,   799,   800,   802,   807,   808,   809,   812,   813,
     816,   817,   820,   821,   824,   825
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
  "tANDAND", "tOROR", "tINC", "tDEC", "tHTMLCOMMENT", "kDIVEQ",
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
     285,   286,   287,   288,   289,   290,   291,   125,   292,   293,
     294,   295,   296,   297,   298,   299,   300,   301,   123,    44,
      61,    59,    58,    63,   124,    94,    38,    43,    45,    42,
      47,    37,   126,    33,    91,    93,    46,    40,    41
};
# endif

#define YYPACT_NINF -293

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-293)))

#define YYTABLE_NINF -197

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -293,    53,   496,  -293,    26,    26,   956,   744,     8,     8,
     986,  -293,   956,     8,  -293,   956,  -293,  -293,    39,   956,
      71,   956,     8,     8,   956,   956,  -293,  -293,  -293,    67,
    -293,  -293,  -293,   558,  -293,   956,   956,  -293,   956,   956,
      41,   956,    76,   410,    26,  -293,  -293,  -293,  -293,  -293,
    -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,
      19,  -293,  -293,    -7,   102,   100,   108,   111,   135,    36,
     142,   -43,    97,  -293,  -293,   129,  -293,   105,   118,  -293,
    -293,  -293,  -293,  -293,  -293,     7,     7,  -293,   176,  -293,
    -293,    49,   158,  -293,  -293,   372,   850,  -293,   105,     7,
     140,   956,    19,   620,   125,  -293,   153,    47,  -293,  -293,
     372,   956,  -293,  -293,   744,   442,    67,   155,   163,  -293,
     682,    80,   165,  -293,  -293,  -293,  -293,  -293,  -293,    42,
      48,   956,    31,  -293,     8,  -293,   956,  -293,  -293,   956,
     956,   956,   956,   956,   956,   956,   956,   956,   956,   956,
     956,   956,   956,   956,   956,  -293,  -293,  -293,  -293,   956,
     956,   956,   180,   797,  -293,   956,   182,  -293,  -293,  -293,
    -293,  -293,  -293,  -293,     8,  -293,     3,   140,   184,  -293,
     181,  -293,  -293,    35,   195,   172,   178,   175,   192,    23,
     142,   120,  -293,  -293,    11,  -293,  -293,     8,    39,   222,
    -293,   956,  -293,  -293,    71,  -293,     3,    11,  -293,  -293,
    -293,  -293,    90,   956,   186,  -293,  -293,  -293,  -293,  -293,
     198,  -293,   102,   185,   100,   108,   111,   135,    36,   142,
     142,   142,   -43,    97,    97,  -293,  -293,  -293,  -293,  -293,
      56,  -293,  -293,  -293,    34,    66,  -293,   372,  -293,  -293,
     744,   188,   190,   226,    10,   956,   956,   956,   956,   956,
     956,   956,   956,   956,   956,   372,   956,   956,   193,   204,
    -293,  -293,  -293,  -293,   744,   744,   194,  -293,   196,   903,
    -293,   202,     3,   956,  -293,   956,  -293,  -293,     3,   234,
     956,  -293,  -293,   184,    10,   372,  -293,  -293,   956,  -293,
     177,   195,   214,   172,   178,   175,   192,    23,   142,   142,
       3,  -293,  -293,   263,  -293,     3,  -293,  -293,   956,  -293,
    -293,   230,   221,  -293,  -293,     7,   744,  -293,  -293,   956,
       3,  -293,   956,   744,   956,     6,   263,  -293,    39,  -293,
    -293,  -293,  -293,  -293,  -293,   744,    10,  -293,  -293,    59,
     219,  -293,   263,  -293,  -293,   744,   239,    10,  -293,   956,
     744,   744,   240,  -293,   956,     3,   744,  -293,  -293,  -293,
       3,   744,   744,  -293,  -293
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       5,     0,     4,     1,   197,   197,     0,     0,     0,     0,
       0,   199,    82,     0,   174,     0,   205,   206,     0,     0,
       0,     0,     0,     0,     0,     0,     3,   204,     8,   175,
     201,   207,   202,     0,    48,     0,     0,   203,     0,     0,
     188,     0,     0,    17,   197,     6,    14,    15,    16,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
       0,    86,    94,   100,   104,   108,   112,   116,   120,   124,
     128,   135,   137,   140,   144,   154,   157,   159,   158,   161,
     177,   178,   176,   200,   198,     0,     0,   175,     0,   162,
     145,   154,     0,   211,   210,     0,    88,   160,   159,     0,
      83,     0,     0,     0,     0,   147,    42,     0,    36,   146,
       0,     0,   148,   149,     0,    34,   175,   201,   202,    29,
       0,     0,     0,   150,   151,   152,   153,   186,   180,     0,
     189,     0,     0,     2,     0,   209,     0,   208,    49,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   155,   156,    93,    92,     0,
       0,     0,     0,     0,   166,     0,     0,   167,    63,    62,
     190,   194,   196,   195,     0,    85,     0,    84,     0,    54,
      89,    90,    97,   102,   106,   110,   114,   118,   122,   126,
     132,   154,   165,    64,     0,    76,    34,     0,     0,    77,
      78,     0,    40,    43,     0,    35,     0,     0,    66,    33,
      30,   191,     0,     0,   188,   182,   187,   181,   184,   179,
      12,    87,   105,     0,   109,   113,   117,   121,   125,   131,
     130,   129,   136,   138,   139,   141,   142,   143,    95,    96,
       0,   164,   170,   172,     0,     0,   169,     0,   213,   212,
       0,    45,    57,    38,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      81,    79,    44,    37,     0,     0,     0,   192,   189,     0,
      10,    13,     0,     0,   163,     0,   171,   168,     0,    51,
       0,    41,    46,     0,     0,     0,   215,   214,    82,    91,
     154,   107,     0,   111,   115,   119,   123,   127,   134,   133,
       0,    98,    99,    70,    67,     0,    53,    65,     0,   183,
     185,     0,     0,   101,   173,     0,     0,    47,    39,    82,
       0,    55,     0,     0,     0,     0,    71,    72,     0,   193,
      11,     5,    52,    50,    58,     0,     0,   103,    60,     0,
       0,    68,    70,    73,    80,     9,     0,     0,    61,    82,
      31,    31,     0,     7,    82,     0,    32,    74,    75,    69,
       0,     0,     0,    56,    59
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -293,  -293,  -293,   -63,    -2,  -293,  -293,  -293,  -293,     0,
    -187,   -82,   -17,  -293,  -293,  -293,    77,   -10,  -293,  -293,
    -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,  -293,
    -293,  -293,  -293,  -293,  -293,  -293,  -293,   -68,  -293,   -51,
    -293,  -293,  -293,  -293,    87,  -292,   -87,    15,  -293,  -293,
     213,   -36,  -238,  -293,  -293,  -293,  -293,   150,    40,   149,
      33,   151,    43,   152,    44,   148,    45,   156,    46,   -62,
     145,    37,    38,  -293,    -3,   287,   288,  -293,    -9,  -293,
    -293,  -293,  -293,    86,    89,  -293,  -293,    95,     5,  -293,
    -293,   -60,    32,    17,  -254
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    42,     2,    89,    44,   356,   281,   282,   119,
     120,   367,    46,    47,   107,   252,   108,   253,   202,   203,
     291,   292,    48,    49,    50,    51,   254,   346,   294,   357,
      52,    53,    54,    55,    56,    57,   314,   335,   336,   337,
     352,    58,    59,   199,   200,    99,   176,    60,   179,   180,
     267,    61,   181,    62,   182,    63,   183,    64,   184,    65,
     185,    66,   186,    67,   187,    68,   188,    69,   189,    70,
      71,    72,    73,    74,    75,    76,    77,    78,   164,   244,
      79,    80,   129,   130,   131,    81,   121,   122,    85,    82,
      83,   138,    95,   250,   298
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      43,   104,    45,    91,   248,    43,   331,    92,   135,    93,
      86,   296,   248,   350,   150,   151,    91,   299,    91,   302,
     135,    91,    91,   206,   139,   168,   169,   100,   311,   312,
     102,    43,    91,    91,   190,    91,    91,   344,   263,   193,
     329,    96,   195,   351,    90,   101,   140,   205,   135,   134,
     146,   147,   327,     3,   110,   111,   132,   105,   137,   109,
     136,   297,   112,   113,    84,   264,   256,   365,   136,   167,
     137,   249,   370,   123,   124,    94,   125,   126,   148,   249,
     136,   155,   156,   285,   229,   230,   231,   103,   257,   192,
     127,   214,   359,   191,   347,   218,   204,   216,   137,   219,
     221,    43,   286,   364,   223,   136,   128,   215,   136,   106,
     177,   360,    43,   217,   208,   136,   194,   211,    43,   114,
     210,   284,   133,   238,   239,   177,   207,   243,   171,   212,
     197,   287,   141,   172,   265,   173,    91,   198,    91,    91,
      91,    91,    91,    91,    91,    91,    91,    91,    91,    91,
      91,    91,   155,   156,   142,   157,   152,   153,   154,   158,
     288,   155,   156,   143,   157,   272,   220,   144,   158,   161,
     266,   162,   163,   366,   366,   145,   240,   277,   310,   159,
     245,   270,   165,   149,   166,   163,   174,   233,   234,   136,
     235,   236,   237,   190,   190,   190,   190,   190,   190,   190,
     190,   308,   309,   201,   190,   190,   247,  -196,   330,   155,
     156,   268,   157,   170,   171,  -195,   158,   213,   241,   172,
     246,   173,   251,   274,   275,   258,   259,   266,   190,   269,
     255,   261,   262,   260,   198,   127,   280,   283,   290,   293,
     295,   313,   315,   320,   326,   216,   318,   323,    43,   324,
     289,   321,   300,    91,   300,    91,    91,    91,    91,    91,
      91,    91,   177,   300,   300,   342,   332,   334,   340,   341,
     190,   361,    43,    43,   316,   317,   363,   369,   355,   368,
     177,   273,   339,   328,   362,   353,   271,   300,   160,   222,
     224,   303,   227,   225,   232,   226,   301,    97,    98,   322,
     278,   228,   304,   279,   305,   325,   306,   276,   307,     0,
     177,     0,     0,   100,     0,     0,     0,     0,     0,     0,
       0,   354,     0,     0,    43,     0,   343,   333,     0,   300,
       0,    43,   338,   348,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    43,   100,   358,     0,   345,     0,   349,
       0,     0,     0,    43,     0,    45,     0,     0,    43,    43,
       0,     0,     0,     0,    43,     0,   210,     0,     0,    43,
      43,   373,   374,   175,   100,     0,     0,     0,     0,   100,
       6,     0,   371,     0,     0,     0,     0,   372,    10,    11,
       0,     0,    14,     0,    16,    17,     0,    19,     0,    21,
       0,     0,     0,     0,    24,    25,     0,    27,    28,     0,
      87,  -162,     0,     0,     0,    30,    31,    32,     0,     0,
      88,     0,     0,     0,  -162,  -162,     0,     0,     0,    35,
      36,     0,    37,     0,    38,    39,    40,     0,     0,    41,
    -162,  -162,     0,  -190,     0,     0,     0,     0,     0,  -162,
    -162,  -162,  -162,     0,     0,     0,  -190,  -190,     0,  -162,
    -162,     0,     0,  -162,  -162,  -162,  -162,     0,     0,  -162,
       0,  -162,  -190,  -190,     0,     0,  -162,     0,     0,     0,
       0,  -190,  -190,  -190,  -190,     0,     0,     0,     0,     0,
       0,  -190,  -190,     0,     0,  -190,  -190,  -190,  -190,     4,
       0,  -190,     5,  -190,     6,     7,     0,     8,  -190,     9,
       0,     0,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,     0,     0,    24,    25,
      26,    27,    28,     0,    29,     0,     0,     0,     0,    30,
      31,    32,     0,     0,    33,     0,     0,    34,     0,     0,
       0,     0,     0,    35,    36,     0,    37,     0,    38,    39,
      40,     4,     0,    41,     5,     0,     6,     7,     0,     8,
       0,     9,     0,     0,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,     0,     0,
      24,    25,     0,    27,    28,   115,   116,     0,     0,     0,
       0,   117,    31,   118,     0,     0,    33,     0,     0,    34,
       0,     0,     0,     0,     0,    35,    36,     0,    37,     0,
      38,    39,    40,     4,     0,    41,     5,     0,     6,     7,
       0,     8,     0,     9,     0,     0,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
       0,     0,    24,    25,     0,    27,    28,   196,    29,     0,
       0,     0,     0,    30,    31,    32,     0,     0,    33,     0,
       0,    34,     0,     0,     0,     0,     0,    35,    36,     0,
      37,     0,    38,    39,    40,     4,     0,    41,     5,     0,
       6,     7,     0,     8,     0,     9,     0,     0,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,     0,     0,    24,    25,     0,    27,    28,   209,
      29,     0,     0,     0,     0,    30,    31,    32,     0,     0,
      33,     0,     0,    34,     0,     0,     0,     0,     0,    35,
      36,     0,    37,     0,    38,    39,    40,     4,     0,    41,
       5,     0,     6,     7,     0,     8,     0,     9,     0,     0,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,     0,     0,    24,    25,     0,    27,
      28,     0,    29,     0,     0,     0,     0,    30,    31,    32,
       0,     0,    33,     0,     0,    34,     0,     0,     0,     0,
       0,    35,    36,     0,    37,     6,    38,    39,    40,     0,
       0,    41,     0,    10,    11,     0,     0,    14,     0,    16,
      17,     0,    19,     0,    21,     0,     0,     0,     0,    24,
      25,     0,    27,    28,     0,    87,     0,     0,     0,     0,
      30,    31,    32,     0,     0,    88,     0,     0,     0,     0,
       0,     0,     0,     0,    35,    36,     0,    37,     6,    38,
      39,    40,     0,     0,    41,   242,    10,    11,     0,     0,
      14,     0,    16,    17,     0,    19,   178,    21,     0,     0,
       0,     0,    24,    25,     0,    27,    28,     0,    87,     0,
       0,     0,     0,    30,    31,    32,     0,     0,    88,     0,
       0,     0,     0,     0,     0,     0,     0,    35,    36,     0,
      37,     6,    38,    39,    40,     0,     0,    41,     0,    10,
      11,     0,     0,    14,     0,    16,    17,     0,    19,     0,
      21,     0,     0,     0,     0,    24,    25,     0,    27,    28,
       0,    87,     0,     0,     0,     0,    30,    31,    32,     0,
       0,    88,     0,     0,     0,     0,     0,     0,     0,     0,
      35,    36,     0,    37,     6,    38,    39,    40,   319,     0,
      41,     0,    10,    11,     0,     0,    14,     0,    16,    17,
       0,    19,     0,    21,     0,     0,     0,     0,    24,    25,
       0,    27,    28,     0,    87,     0,     0,     0,     0,    30,
      31,    32,    10,    11,    88,     0,    14,     0,    16,    17,
       0,     0,     0,    35,    36,     0,    37,     0,    38,    39,
      40,    27,    28,    41,    87,     0,     0,     0,     0,    30,
      31,    32,     0,     0,    88,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    37,     0,     0,     0,
      40,     0,     0,    41
};

static const yytype_int16 yycheck[] =
{
       2,    18,     2,     6,     1,     7,   298,     7,     1,     1,
       5,     1,     1,     7,    57,    58,    19,   255,    21,   257,
       1,    24,    25,   110,    31,    85,    86,    12,   266,   267,
      15,    33,    35,    36,    96,    38,    39,   329,    15,    99,
     294,     9,   102,    37,     6,    13,    53,   107,     1,    44,
      14,    15,   290,     0,    22,    23,    41,    19,    51,    21,
      49,    51,    24,    25,    38,    42,    31,   359,    49,    78,
      51,    68,   364,    35,    36,    67,    38,    39,    42,    68,
      49,    32,    33,    49,   146,   147,   148,    48,    53,    98,
      49,    49,   346,    96,   332,   131,    49,    49,    51,    68,
     136,   103,    68,   357,   140,    49,    65,    65,    49,    38,
      95,    52,   114,    65,   114,    49,   101,    37,   120,    52,
     120,    65,    46,   159,   160,   110,   111,   163,    38,    49,
       5,    65,    30,    43,    14,    45,   139,    12,   141,   142,
     143,   144,   145,   146,   147,   148,   149,   150,   151,   152,
     153,   154,    32,    33,    54,    35,    59,    60,    61,    39,
     247,    32,    33,    55,    35,   201,   134,    56,    39,    64,
      50,    66,    67,   360,   361,    40,   161,   213,   265,    50,
     165,   198,    64,    41,    66,    67,    28,   150,   151,    49,
     152,   153,   154,   255,   256,   257,   258,   259,   260,   261,
     262,   263,   264,    50,   266,   267,   174,    52,   295,    32,
      33,   194,    35,    37,    38,    52,    39,    52,    38,    43,
      38,    45,    38,   206,   207,    30,    54,    50,   290,   197,
      49,    56,    40,    55,    12,    49,    38,    52,    50,    49,
      14,    48,    38,   279,    10,    49,    52,   283,   250,   285,
     250,    49,   255,   256,   257,   258,   259,   260,   261,   262,
     263,   264,   247,   266,   267,   325,    52,     4,    38,    48,
     332,    52,   274,   275,   274,   275,    37,    37,   341,   361,
     265,   204,   318,   293,   352,   336,   199,   290,    75,   139,
     141,   258,   144,   142,   149,   143,   256,    10,    10,   282,
     214,   145,   259,   214,   260,   288,   261,   212,   262,    -1,
     295,    -1,    -1,   298,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   338,    -1,    -1,   326,    -1,   326,   310,    -1,   332,
      -1,   333,   315,   333,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   345,   329,   345,    -1,   330,    -1,   334,
      -1,    -1,    -1,   355,    -1,   355,    -1,    -1,   360,   361,
      -1,    -1,    -1,    -1,   366,    -1,   366,    -1,    -1,   371,
     372,   371,   372,     1,   359,    -1,    -1,    -1,    -1,   364,
       8,    -1,   365,    -1,    -1,    -1,    -1,   370,    16,    17,
      -1,    -1,    20,    -1,    22,    23,    -1,    25,    -1,    27,
      -1,    -1,    -1,    -1,    32,    33,    -1,    35,    36,    -1,
      38,     1,    -1,    -1,    -1,    43,    44,    45,    -1,    -1,
      48,    -1,    -1,    -1,    14,    15,    -1,    -1,    -1,    57,
      58,    -1,    60,    -1,    62,    63,    64,    -1,    -1,    67,
      30,    31,    -1,     1,    -1,    -1,    -1,    -1,    -1,    39,
      40,    41,    42,    -1,    -1,    -1,    14,    15,    -1,    49,
      50,    -1,    -1,    53,    54,    55,    56,    -1,    -1,    59,
      -1,    61,    30,    31,    -1,    -1,    66,    -1,    -1,    -1,
      -1,    39,    40,    41,    42,    -1,    -1,    -1,    -1,    -1,
      -1,    49,    50,    -1,    -1,    53,    54,    55,    56,     3,
      -1,    59,     6,    61,     8,     9,    -1,    11,    66,    13,
      -1,    -1,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    -1,    -1,    32,    33,
      34,    35,    36,    -1,    38,    -1,    -1,    -1,    -1,    43,
      44,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    -1,    -1,    57,    58,    -1,    60,    -1,    62,    63,
      64,     3,    -1,    67,     6,    -1,     8,     9,    -1,    11,
      -1,    13,    -1,    -1,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    -1,    -1,
      32,    33,    -1,    35,    36,    37,    38,    -1,    -1,    -1,
      -1,    43,    44,    45,    -1,    -1,    48,    -1,    -1,    51,
      -1,    -1,    -1,    -1,    -1,    57,    58,    -1,    60,    -1,
      62,    63,    64,     3,    -1,    67,     6,    -1,     8,     9,
      -1,    11,    -1,    13,    -1,    -1,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      -1,    -1,    32,    33,    -1,    35,    36,    37,    38,    -1,
      -1,    -1,    -1,    43,    44,    45,    -1,    -1,    48,    -1,
      -1,    51,    -1,    -1,    -1,    -1,    -1,    57,    58,    -1,
      60,    -1,    62,    63,    64,     3,    -1,    67,     6,    -1,
       8,     9,    -1,    11,    -1,    13,    -1,    -1,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    -1,    -1,    32,    33,    -1,    35,    36,    37,
      38,    -1,    -1,    -1,    -1,    43,    44,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    -1,    -1,    57,
      58,    -1,    60,    -1,    62,    63,    64,     3,    -1,    67,
       6,    -1,     8,     9,    -1,    11,    -1,    13,    -1,    -1,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    -1,    -1,    32,    33,    -1,    35,
      36,    -1,    38,    -1,    -1,    -1,    -1,    43,    44,    45,
      -1,    -1,    48,    -1,    -1,    51,    -1,    -1,    -1,    -1,
      -1,    57,    58,    -1,    60,     8,    62,    63,    64,    -1,
      -1,    67,    -1,    16,    17,    -1,    -1,    20,    -1,    22,
      23,    -1,    25,    -1,    27,    -1,    -1,    -1,    -1,    32,
      33,    -1,    35,    36,    -1,    38,    -1,    -1,    -1,    -1,
      43,    44,    45,    -1,    -1,    48,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    57,    58,    -1,    60,     8,    62,
      63,    64,    -1,    -1,    67,    68,    16,    17,    -1,    -1,
      20,    -1,    22,    23,    -1,    25,    26,    27,    -1,    -1,
      -1,    -1,    32,    33,    -1,    35,    36,    -1,    38,    -1,
      -1,    -1,    -1,    43,    44,    45,    -1,    -1,    48,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    57,    58,    -1,
      60,     8,    62,    63,    64,    -1,    -1,    67,    -1,    16,
      17,    -1,    -1,    20,    -1,    22,    23,    -1,    25,    -1,
      27,    -1,    -1,    -1,    -1,    32,    33,    -1,    35,    36,
      -1,    38,    -1,    -1,    -1,    -1,    43,    44,    45,    -1,
      -1,    48,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      57,    58,    -1,    60,     8,    62,    63,    64,    65,    -1,
      67,    -1,    16,    17,    -1,    -1,    20,    -1,    22,    23,
      -1,    25,    -1,    27,    -1,    -1,    -1,    -1,    32,    33,
      -1,    35,    36,    -1,    38,    -1,    -1,    -1,    -1,    43,
      44,    45,    16,    17,    48,    -1,    20,    -1,    22,    23,
      -1,    -1,    -1,    57,    58,    -1,    60,    -1,    62,    63,
      64,    35,    36,    67,    38,    -1,    -1,    -1,    -1,    43,
      44,    45,    -1,    -1,    48,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    60,    -1,    -1,    -1,
      64,    -1,    -1,    67
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    70,    72,     0,     3,     6,     8,     9,    11,    13,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    32,    33,    34,    35,    36,    38,
      43,    44,    45,    48,    51,    57,    58,    60,    62,    63,
      64,    67,    71,    73,    74,    78,    81,    82,    91,    92,
      93,    94,    99,   100,   101,   102,   103,   104,   110,   111,
     116,   120,   122,   124,   126,   128,   130,   132,   134,   136,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   149,
     150,   154,   158,   159,    38,   157,   157,    38,    48,    73,
     141,   143,    78,     1,    67,   161,   161,   144,   145,   114,
     116,   161,   116,    48,    81,   141,    38,    83,    85,   141,
     161,   161,   141,   141,    52,    37,    38,    43,    45,    78,
      79,   155,   156,   141,   141,   141,   141,    49,    65,   151,
     152,   153,   116,    46,   157,     1,    49,    51,   160,    31,
      53,    30,    54,    55,    56,    40,    14,    15,    42,    41,
      57,    58,    59,    60,    61,    32,    33,    35,    39,    50,
     119,    64,    66,    67,   147,    64,    66,   147,   160,   160,
      37,    38,    43,    45,    28,     1,   115,   116,    26,   117,
     118,   121,   123,   125,   127,   129,   131,   133,   135,   137,
     138,   143,   147,   160,   116,   160,    37,     5,    12,   112,
     113,    50,    87,    88,    49,   160,   115,   116,    78,    37,
      78,    37,    49,    52,    49,    65,    49,    65,   120,    68,
     161,   120,   126,   120,   128,   130,   132,   134,   136,   138,
     138,   138,   139,   140,   140,   141,   141,   141,   120,   120,
     116,    38,    68,   120,   148,   116,    38,   161,     1,    68,
     162,    38,    84,    86,    95,    49,    31,    53,    30,    54,
      55,    56,    40,    15,    42,    14,    50,   119,   162,   161,
      81,   113,   120,    85,   162,   162,   156,   120,   152,   153,
      38,    76,    77,    52,    65,    49,    68,    65,   115,    78,
      50,    89,    90,    49,    97,    14,     1,    51,   163,   121,
     143,   127,   121,   129,   131,   133,   135,   137,   138,   138,
     115,   121,   121,    48,   105,    38,    78,    78,    52,    65,
     120,    49,   162,   120,   120,   162,    10,   121,    86,   163,
     115,   114,    52,   162,     4,   106,   107,   108,   162,   120,
      38,    48,   160,    78,   114,   162,    96,   121,    78,   116,
       7,    37,   109,   108,    81,    72,    75,    98,    78,   163,
      52,    52,   106,    37,   163,   114,    79,    80,    80,    37,
     114,   162,   162,    78,    78
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    69,    70,    71,    71,    72,    72,    73,    74,    75,
      76,    76,    77,    77,    78,    78,    78,    78,    78,    78,
      78,    78,    78,    78,    78,    78,    78,    78,    78,    79,
      79,    80,    80,    81,    81,    82,    83,    83,    84,    84,
      85,    86,    87,    87,    88,    89,    89,    90,    91,    92,
      93,    93,    94,    94,    95,    96,    94,    97,    98,    94,
      94,    94,    99,   100,   101,   102,   103,   104,   105,   105,
     106,   106,   107,   107,   108,   109,   110,   111,   111,   111,
     112,   113,   114,   114,   115,   115,   116,   116,   117,   117,
     118,   118,   119,   119,   120,   120,   120,   121,   121,   121,
     122,   122,   123,   123,   124,   124,   125,   125,   126,   126,
     127,   127,   128,   128,   129,   129,   130,   130,   131,   131,
     132,   132,   133,   133,   134,   134,   135,   135,   136,   136,
     136,   136,   137,   137,   137,   138,   138,   139,   139,   139,
     140,   140,   140,   140,   141,   141,   141,   141,   141,   141,
     141,   141,   141,   141,   142,   142,   142,   143,   143,   144,
     144,   145,   145,   145,   145,   145,   146,   146,   146,   146,
     147,   147,   148,   148,   149,   149,   149,   149,   149,   149,
     150,   150,   150,   150,   151,   151,   152,   152,   153,   153,
     154,   154,   155,   155,   156,   156,   156,   157,   157,   158,
     158,   158,   158,   158,   158,   159,   159,   159,   160,   160,
     161,   161,   162,   162,   163,   163
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     3,     1,     0,     0,     2,     8,     1,     1,
       1,     3,     0,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       2,     0,     1,     3,     2,     3,     1,     3,     1,     3,
       2,     2,     0,     1,     2,     0,     1,     2,     1,     2,
       7,     5,     7,     5,     0,     0,    11,     0,     0,    12,
       7,     8,     3,     3,     3,     5,     3,     5,     3,     5,
       0,     1,     1,     2,     4,     3,     3,     3,     3,     4,
       5,     2,     0,     1,     1,     1,     1,     3,     0,     1,
       1,     3,     1,     1,     1,     3,     3,     1,     3,     3,
       1,     5,     1,     5,     1,     3,     1,     3,     1,     3,
       1,     3,     1,     3,     1,     3,     1,     3,     1,     3,
       1,     3,     1,     3,     1,     3,     1,     3,     1,     3,
       3,     3,     1,     3,     3,     1,     3,     1,     3,     3,
       1,     3,     3,     3,     1,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     1,     2,     2,     1,     1,     1,
       2,     1,     1,     4,     3,     3,     2,     2,     4,     3,
       2,     3,     1,     3,     1,     1,     1,     1,     1,     3,
       2,     3,     3,     5,     2,     4,     1,     2,     0,     1,
       2,     3,     3,     5,     1,     1,     1,     0,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1
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
#line 252 "parser.y" /* yacc.c:1646  */
    { program_parsed(ctx, (yyvsp[-2].source_elements)); }
#line 1832 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 3:
#line 255 "parser.y" /* yacc.c:1646  */
    {}
#line 1838 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 4:
#line 256 "parser.y" /* yacc.c:1646  */
    {}
#line 1844 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 5:
#line 260 "parser.y" /* yacc.c:1646  */
    { (yyval.source_elements) = new_source_elements(ctx); }
#line 1850 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 6:
#line 262 "parser.y" /* yacc.c:1646  */
    { (yyval.source_elements) = source_elements_add_statement((yyvsp[-1].source_elements), (yyvsp[0].statement)); }
#line 1856 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 7:
#line 267 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_function_expression(ctx, (yyvsp[-6].identifier), (yyvsp[-4].parameter_list), (yyvsp[-1].source_elements), (yyvsp[-7].srcptr), (yyvsp[0].srcptr)-(yyvsp[-7].srcptr)+1); }
#line 1862 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 8:
#line 270 "parser.y" /* yacc.c:1646  */
    { (yyval.srcptr) = (yyvsp[0].srcptr); }
#line 1868 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 9:
#line 274 "parser.y" /* yacc.c:1646  */
    { (yyval.source_elements) = (yyvsp[0].source_elements); }
#line 1874 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 10:
#line 278 "parser.y" /* yacc.c:1646  */
    { (yyval.parameter_list) = new_parameter_list(ctx, (yyvsp[0].identifier)); }
#line 1880 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 11:
#line 280 "parser.y" /* yacc.c:1646  */
    { (yyval.parameter_list) = parameter_list_add(ctx, (yyvsp[-2].parameter_list), (yyvsp[0].identifier)); }
#line 1886 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 12:
#line 284 "parser.y" /* yacc.c:1646  */
    { (yyval.parameter_list) = NULL; }
#line 1892 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 13:
#line 285 "parser.y" /* yacc.c:1646  */
    { (yyval.parameter_list) = (yyvsp[0].parameter_list); }
#line 1898 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 14:
#line 289 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1904 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 15:
#line 290 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1910 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 16:
#line 291 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1916 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 17:
#line 292 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_expression_statement(ctx, (yyvsp[0].expr)); }
#line 1922 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 18:
#line 293 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1928 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 19:
#line 294 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1934 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 20:
#line 295 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1940 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 21:
#line 296 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1946 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 22:
#line 297 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1952 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 23:
#line 298 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1958 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 24:
#line 299 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1964 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 25:
#line 300 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1970 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 26:
#line 301 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1976 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 27:
#line 302 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1982 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 28:
#line 303 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1988 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 29:
#line 307 "parser.y" /* yacc.c:1646  */
    { (yyval.statement_list) = new_statement_list(ctx, (yyvsp[0].statement)); }
#line 1994 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 30:
#line 309 "parser.y" /* yacc.c:1646  */
    { (yyval.statement_list) = statement_list_add((yyvsp[-1].statement_list), (yyvsp[0].statement)); }
#line 2000 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 31:
#line 313 "parser.y" /* yacc.c:1646  */
    { (yyval.statement_list) = NULL; }
#line 2006 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 32:
#line 314 "parser.y" /* yacc.c:1646  */
    { (yyval.statement_list) = (yyvsp[0].statement_list); }
#line 2012 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 33:
#line 318 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_block_statement(ctx, (yyvsp[-1].statement_list)); }
#line 2018 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 34:
#line 319 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_block_statement(ctx, NULL); }
#line 2024 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 35:
#line 324 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_var_statement(ctx, (yyvsp[-1].variable_list)); }
#line 2030 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 36:
#line 328 "parser.y" /* yacc.c:1646  */
    { (yyval.variable_list) = new_variable_list(ctx, (yyvsp[0].variable_declaration)); }
#line 2036 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 37:
#line 330 "parser.y" /* yacc.c:1646  */
    { (yyval.variable_list) = variable_list_add(ctx, (yyvsp[-2].variable_list), (yyvsp[0].variable_declaration)); }
#line 2042 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 38:
#line 335 "parser.y" /* yacc.c:1646  */
    { (yyval.variable_list) = new_variable_list(ctx, (yyvsp[0].variable_declaration)); }
#line 2048 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 39:
#line 337 "parser.y" /* yacc.c:1646  */
    { (yyval.variable_list) = variable_list_add(ctx, (yyvsp[-2].variable_list), (yyvsp[0].variable_declaration)); }
#line 2054 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 40:
#line 342 "parser.y" /* yacc.c:1646  */
    { (yyval.variable_declaration) = new_variable_declaration(ctx, (yyvsp[-1].identifier), (yyvsp[0].expr)); }
#line 2060 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 41:
#line 347 "parser.y" /* yacc.c:1646  */
    { (yyval.variable_declaration) = new_variable_declaration(ctx, (yyvsp[-1].identifier), (yyvsp[0].expr)); }
#line 2066 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 42:
#line 351 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = NULL; }
#line 2072 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 43:
#line 352 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2078 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 44:
#line 357 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2084 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 45:
#line 361 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = NULL; }
#line 2090 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 46:
#line 362 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2096 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 47:
#line 367 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2102 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 48:
#line 371 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_statement(ctx, STAT_EMPTY, 0); }
#line 2108 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 49:
#line 376 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_expression_statement(ctx, (yyvsp[-1].expr)); }
#line 2114 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 50:
#line 381 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_if_statement(ctx, (yyvsp[-4].expr), (yyvsp[-2].statement), (yyvsp[0].statement)); }
#line 2120 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 51:
#line 383 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_if_statement(ctx, (yyvsp[-2].expr), (yyvsp[0].statement), NULL); }
#line 2126 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 52:
#line 388 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_while_statement(ctx, TRUE, (yyvsp[-2].expr), (yyvsp[-5].statement)); }
#line 2132 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 53:
#line 390 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_while_statement(ctx, FALSE, (yyvsp[-2].expr), (yyvsp[0].statement)); }
#line 2138 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 54:
#line 392 "parser.y" /* yacc.c:1646  */
    { if(!explicit_error(ctx, (yyvsp[0].expr), ';')) YYABORT; }
#line 2144 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 55:
#line 394 "parser.y" /* yacc.c:1646  */
    { if(!explicit_error(ctx, (yyvsp[0].expr), ';')) YYABORT; }
#line 2150 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 56:
#line 396 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_for_statement(ctx, NULL, (yyvsp[-8].expr), (yyvsp[-5].expr), (yyvsp[-2].expr), (yyvsp[0].statement)); }
#line 2156 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 57:
#line 398 "parser.y" /* yacc.c:1646  */
    { if(!explicit_error(ctx, (yyvsp[0].variable_list), ';')) YYABORT; }
#line 2162 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 58:
#line 400 "parser.y" /* yacc.c:1646  */
    { if(!explicit_error(ctx, (yyvsp[0].expr), ';')) YYABORT; }
#line 2168 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 59:
#line 402 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_for_statement(ctx, (yyvsp[-8].variable_list), NULL, (yyvsp[-5].expr), (yyvsp[-2].expr), (yyvsp[0].statement)); }
#line 2174 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 60:
#line 404 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_forin_statement(ctx, NULL, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].statement)); }
#line 2180 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 61:
#line 406 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_forin_statement(ctx, (yyvsp[-4].variable_declaration), NULL, (yyvsp[-2].expr), (yyvsp[0].statement)); }
#line 2186 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 62:
#line 411 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_continue_statement(ctx, (yyvsp[-1].identifier)); }
#line 2192 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 63:
#line 416 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_break_statement(ctx, (yyvsp[-1].identifier)); }
#line 2198 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 64:
#line 421 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_return_statement(ctx, (yyvsp[-1].expr)); }
#line 2204 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 65:
#line 426 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_with_statement(ctx, (yyvsp[-2].expr), (yyvsp[0].statement)); }
#line 2210 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 66:
#line 431 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_labelled_statement(ctx, (yyvsp[-2].identifier), (yyvsp[0].statement)); }
#line 2216 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 67:
#line 436 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_switch_statement(ctx, (yyvsp[-2].expr), (yyvsp[0].case_clausule)); }
#line 2222 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 68:
#line 441 "parser.y" /* yacc.c:1646  */
    { (yyval.case_clausule) = new_case_block(ctx, (yyvsp[-1].case_list), NULL, NULL); }
#line 2228 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 69:
#line 443 "parser.y" /* yacc.c:1646  */
    { (yyval.case_clausule) = new_case_block(ctx, (yyvsp[-3].case_list), (yyvsp[-2].case_clausule), (yyvsp[-1].case_list)); }
#line 2234 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 70:
#line 447 "parser.y" /* yacc.c:1646  */
    { (yyval.case_list) = NULL; }
#line 2240 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 71:
#line 448 "parser.y" /* yacc.c:1646  */
    { (yyval.case_list) = (yyvsp[0].case_list); }
#line 2246 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 72:
#line 452 "parser.y" /* yacc.c:1646  */
    { (yyval.case_list) = new_case_list(ctx, (yyvsp[0].case_clausule)); }
#line 2252 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 73:
#line 454 "parser.y" /* yacc.c:1646  */
    { (yyval.case_list) = case_list_add(ctx, (yyvsp[-1].case_list), (yyvsp[0].case_clausule)); }
#line 2258 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 74:
#line 459 "parser.y" /* yacc.c:1646  */
    { (yyval.case_clausule) = new_case_clausule(ctx, (yyvsp[-2].expr), (yyvsp[0].statement_list)); }
#line 2264 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 75:
#line 464 "parser.y" /* yacc.c:1646  */
    { (yyval.case_clausule) = new_case_clausule(ctx, NULL, (yyvsp[0].statement_list)); }
#line 2270 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 76:
#line 469 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_throw_statement(ctx, (yyvsp[-1].expr)); }
#line 2276 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 77:
#line 473 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[-1].statement), (yyvsp[0].catch_block), NULL); }
#line 2282 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 78:
#line 474 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[-1].statement), NULL, (yyvsp[0].statement)); }
#line 2288 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 79:
#line 476 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[-2].statement), (yyvsp[-1].catch_block), (yyvsp[0].statement)); }
#line 2294 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 80:
#line 481 "parser.y" /* yacc.c:1646  */
    { (yyval.catch_block) = new_catch_block(ctx, (yyvsp[-2].identifier), (yyvsp[0].statement)); }
#line 2300 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 81:
#line 485 "parser.y" /* yacc.c:1646  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 2306 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 82:
#line 489 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = NULL; }
#line 2312 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 83:
#line 490 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2318 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 84:
#line 493 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2324 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 85:
#line 494 "parser.y" /* yacc.c:1646  */
    { set_error(ctx, JS_E_SYNTAX); YYABORT; }
#line 2330 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 86:
#line 498 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2336 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 87:
#line 500 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_COMMA, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2342 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 88:
#line 504 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = NULL; }
#line 2348 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 89:
#line 505 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2354 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 90:
#line 510 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2360 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 91:
#line 512 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_COMMA, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2366 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 92:
#line 515 "parser.y" /* yacc.c:1646  */
    { (yyval.ival) = (yyvsp[0].ival); }
#line 2372 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 93:
#line 516 "parser.y" /* yacc.c:1646  */
    { (yyval.ival) = EXPR_ASSIGNDIV; }
#line 2378 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 94:
#line 520 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2384 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 95:
#line 522 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ASSIGN, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2390 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 96:
#line 524 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2396 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 97:
#line 529 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2402 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 98:
#line 531 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ASSIGN, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2408 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 99:
#line 533 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2414 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 100:
#line 537 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2420 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 101:
#line 539 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_conditional_expression(ctx, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2426 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 102:
#line 544 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2432 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 103:
#line 546 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_conditional_expression(ctx, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2438 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 104:
#line 550 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2444 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 105:
#line 552 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_OR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2450 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 106:
#line 557 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2456 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 107:
#line 559 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_OR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2462 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 108:
#line 563 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2468 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 109:
#line 565 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_AND, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2474 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 110:
#line 570 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2480 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 111:
#line 572 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_AND, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2486 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 112:
#line 576 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2492 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 113:
#line 578 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BOR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2498 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 114:
#line 583 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2504 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 115:
#line 585 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BOR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2510 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 116:
#line 589 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2516 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 117:
#line 591 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BXOR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2522 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 118:
#line 596 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2528 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 119:
#line 598 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BXOR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2534 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 120:
#line 602 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2540 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 121:
#line 604 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BAND, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2546 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 122:
#line 609 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2552 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 123:
#line 611 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BAND, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2558 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 124:
#line 615 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2564 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 125:
#line 617 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2570 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 126:
#line 621 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2576 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 127:
#line 623 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2582 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 128:
#line 627 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2588 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 129:
#line 629 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2594 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 130:
#line 631 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_INSTANCEOF, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2600 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 131:
#line 633 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_IN, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2606 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 132:
#line 637 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2612 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 133:
#line 639 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2618 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 134:
#line 641 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_INSTANCEOF, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2624 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 135:
#line 645 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2630 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 136:
#line 647 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2636 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 137:
#line 652 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2642 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 138:
#line 654 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ADD, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2648 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 139:
#line 656 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_SUB, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2654 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 140:
#line 660 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2660 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 141:
#line 662 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_MUL, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2666 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 142:
#line 664 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_DIV, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2672 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 143:
#line 666 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_MOD, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2678 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 144:
#line 670 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2684 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 145:
#line 672 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_unary_expression(ctx, EXPR_DELETE, (yyvsp[0].expr)); }
#line 2690 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 146:
#line 673 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_unary_expression(ctx, EXPR_VOID, (yyvsp[0].expr)); }
#line 2696 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 147:
#line 675 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_unary_expression(ctx, EXPR_TYPEOF, (yyvsp[0].expr)); }
#line 2702 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 148:
#line 676 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PREINC, (yyvsp[0].expr)); }
#line 2708 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 149:
#line 677 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PREDEC, (yyvsp[0].expr)); }
#line 2714 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 150:
#line 678 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PLUS, (yyvsp[0].expr)); }
#line 2720 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 151:
#line 679 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_unary_expression(ctx, EXPR_MINUS, (yyvsp[0].expr)); }
#line 2726 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 152:
#line 680 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_unary_expression(ctx, EXPR_BITNEG, (yyvsp[0].expr)); }
#line 2732 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 153:
#line 681 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_unary_expression(ctx, EXPR_LOGNEG, (yyvsp[0].expr)); }
#line 2738 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 154:
#line 686 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2744 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 155:
#line 688 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_unary_expression(ctx, EXPR_POSTINC, (yyvsp[-1].expr)); }
#line 2750 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 156:
#line 690 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_unary_expression(ctx, EXPR_POSTDEC, (yyvsp[-1].expr)); }
#line 2756 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 157:
#line 695 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2762 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 158:
#line 696 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2768 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 159:
#line 700 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2774 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 160:
#line 701 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_new_expression(ctx, (yyvsp[0].expr), NULL); }
#line 2780 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 161:
#line 705 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2786 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 162:
#line 706 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2792 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 163:
#line 708 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ARRAY, (yyvsp[-3].expr), (yyvsp[-1].expr)); }
#line 2798 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 164:
#line 710 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_member_expression(ctx, (yyvsp[-2].expr), (yyvsp[0].identifier)); }
#line 2804 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 165:
#line 712 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_new_expression(ctx, (yyvsp[-1].expr), (yyvsp[0].argument_list)); }
#line 2810 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 166:
#line 717 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_call_expression(ctx, (yyvsp[-1].expr), (yyvsp[0].argument_list)); }
#line 2816 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 167:
#line 719 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_call_expression(ctx, (yyvsp[-1].expr), (yyvsp[0].argument_list)); }
#line 2822 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 168:
#line 721 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ARRAY, (yyvsp[-3].expr), (yyvsp[-1].expr)); }
#line 2828 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 169:
#line 723 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_member_expression(ctx, (yyvsp[-2].expr), (yyvsp[0].identifier)); }
#line 2834 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 170:
#line 727 "parser.y" /* yacc.c:1646  */
    { (yyval.argument_list) = NULL; }
#line 2840 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 171:
#line 728 "parser.y" /* yacc.c:1646  */
    { (yyval.argument_list) = (yyvsp[-1].argument_list); }
#line 2846 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 172:
#line 732 "parser.y" /* yacc.c:1646  */
    { (yyval.argument_list) = new_argument_list(ctx, (yyvsp[0].expr)); }
#line 2852 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 173:
#line 734 "parser.y" /* yacc.c:1646  */
    { (yyval.argument_list) = argument_list_add(ctx, (yyvsp[-2].argument_list), (yyvsp[0].expr)); }
#line 2858 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 174:
#line 738 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_expression(ctx, EXPR_THIS, 0); }
#line 2864 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 175:
#line 739 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_identifier_expression(ctx, (yyvsp[0].identifier)); }
#line 2870 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 176:
#line 740 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_literal_expression(ctx, (yyvsp[0].literal)); }
#line 2876 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 177:
#line 741 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2882 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 178:
#line 742 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2888 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 179:
#line 743 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 2894 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 180:
#line 747 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_array_literal_expression(ctx, NULL, 0); }
#line 2900 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 181:
#line 748 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_array_literal_expression(ctx, NULL, (yyvsp[-1].ival)+1); }
#line 2906 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 182:
#line 749 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_array_literal_expression(ctx, (yyvsp[-1].element_list), 0); }
#line 2912 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 183:
#line 751 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_array_literal_expression(ctx, (yyvsp[-3].element_list), (yyvsp[-1].ival)+1); }
#line 2918 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 184:
#line 756 "parser.y" /* yacc.c:1646  */
    { (yyval.element_list) = new_element_list(ctx, (yyvsp[-1].ival), (yyvsp[0].expr)); }
#line 2924 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 185:
#line 758 "parser.y" /* yacc.c:1646  */
    { (yyval.element_list) = element_list_add(ctx, (yyvsp[-3].element_list), (yyvsp[-1].ival), (yyvsp[0].expr)); }
#line 2930 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 186:
#line 762 "parser.y" /* yacc.c:1646  */
    { (yyval.ival) = 1; }
#line 2936 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 187:
#line 763 "parser.y" /* yacc.c:1646  */
    { (yyval.ival) = (yyvsp[-1].ival) + 1; }
#line 2942 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 188:
#line 767 "parser.y" /* yacc.c:1646  */
    { (yyval.ival) = 0; }
#line 2948 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 189:
#line 768 "parser.y" /* yacc.c:1646  */
    { (yyval.ival) = (yyvsp[0].ival); }
#line 2954 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 190:
#line 772 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_prop_and_value_expression(ctx, NULL); }
#line 2960 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 191:
#line 774 "parser.y" /* yacc.c:1646  */
    { (yyval.expr) = new_prop_and_value_expression(ctx, (yyvsp[-1].property_list)); }
#line 2966 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 192:
#line 779 "parser.y" /* yacc.c:1646  */
    { (yyval.property_list) = new_property_list(ctx, (yyvsp[-2].literal), (yyvsp[0].expr)); }
#line 2972 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 193:
#line 781 "parser.y" /* yacc.c:1646  */
    { (yyval.property_list) = property_list_add(ctx, (yyvsp[-4].property_list), (yyvsp[-2].literal), (yyvsp[0].expr)); }
#line 2978 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 194:
#line 785 "parser.y" /* yacc.c:1646  */
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[0].identifier)); }
#line 2984 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 195:
#line 786 "parser.y" /* yacc.c:1646  */
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[0].wstr)); }
#line 2990 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 196:
#line 787 "parser.y" /* yacc.c:1646  */
    { (yyval.literal) = (yyvsp[0].literal); }
#line 2996 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 197:
#line 791 "parser.y" /* yacc.c:1646  */
    { (yyval.identifier) = NULL; }
#line 3002 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 198:
#line 792 "parser.y" /* yacc.c:1646  */
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3008 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 199:
#line 796 "parser.y" /* yacc.c:1646  */
    { (yyval.literal) = new_null_literal(ctx); }
#line 3014 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 200:
#line 797 "parser.y" /* yacc.c:1646  */
    { (yyval.literal) = (yyvsp[0].literal); }
#line 3020 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 201:
#line 798 "parser.y" /* yacc.c:1646  */
    { (yyval.literal) = (yyvsp[0].literal); }
#line 3026 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 202:
#line 799 "parser.y" /* yacc.c:1646  */
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[0].wstr)); }
#line 3032 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 203:
#line 800 "parser.y" /* yacc.c:1646  */
    { (yyval.literal) = parse_regexp(ctx);
                                  if(!(yyval.literal)) YYABORT; }
#line 3039 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 204:
#line 802 "parser.y" /* yacc.c:1646  */
    { (yyval.literal) = parse_regexp(ctx);
                                  if(!(yyval.literal)) YYABORT; }
#line 3046 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 205:
#line 807 "parser.y" /* yacc.c:1646  */
    { (yyval.literal) = new_boolean_literal(ctx, VARIANT_TRUE); }
#line 3052 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 206:
#line 808 "parser.y" /* yacc.c:1646  */
    { (yyval.literal) = new_boolean_literal(ctx, VARIANT_FALSE); }
#line 3058 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 207:
#line 809 "parser.y" /* yacc.c:1646  */
    { (yyval.literal) = (yyvsp[0].literal); }
#line 3064 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 209:
#line 813 "parser.y" /* yacc.c:1646  */
    { if(!allow_auto_semicolon(ctx)) {YYABORT;} }
#line 3070 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 211:
#line 817 "parser.y" /* yacc.c:1646  */
    { set_error(ctx, JS_E_MISSING_LBRACKET); YYABORT; }
#line 3076 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 213:
#line 821 "parser.y" /* yacc.c:1646  */
    { set_error(ctx, JS_E_MISSING_RBRACKET); YYABORT; }
#line 3082 "parser.tab.c" /* yacc.c:1646  */
    break;

  case 215:
#line 825 "parser.y" /* yacc.c:1646  */
    { set_error(ctx, JS_E_MISSING_SEMICOLON); YYABORT; }
#line 3088 "parser.tab.c" /* yacc.c:1646  */
    break;


#line 3092 "parser.tab.c" /* yacc.c:1646  */
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
#line 827 "parser.y" /* yacc.c:1906  */


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

static expression_t *new_function_expression(parser_ctx_t *ctx, const WCHAR *identifier,
       parameter_list_t *parameter_list, source_elements_t *source_elements, const WCHAR *src_str, DWORD src_len)
{
    function_expression_t *ret = new_expression(ctx, EXPR_FUNC, sizeof(*ret));

    ret->identifier = identifier;
    ret->parameter_list = parameter_list ? parameter_list->head : NULL;
    ret->source_elements = source_elements;
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
