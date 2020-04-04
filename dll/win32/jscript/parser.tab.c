/* A Bison parser, made by GNU Bison 3.4.1.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2019 Free Software Foundation,
   Inc.

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

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.4.1"

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


/* First part of user prologue.  */
#line 19 "parser.y"


#include "jscript.h"
#include "engine.h"
#include "parser.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

static int parser_error(parser_ctx_t*,const char*);
static void set_error(parser_ctx_t*,UINT);
static BOOL explicit_error(parser_ctx_t*,void*,WCHAR);
static BOOL allow_auto_semicolon(parser_ctx_t*);
static void program_parsed(parser_ctx_t*,source_elements_t*);

typedef struct _statement_list_t {
    statement_t *head;
    statement_t *tail;
} statement_list_t;

static literal_t *new_string_literal(parser_ctx_t*,jsstr_t*);
static literal_t *new_null_literal(parser_ctx_t*);

typedef struct _property_list_t {
    property_definition_t *head;
    property_definition_t *tail;
} property_list_t;

static property_definition_t *new_property_definition(parser_ctx_t *ctx, property_definition_type_t,
                                                      literal_t *name, expression_t *value);
static property_list_t *new_property_list(parser_ctx_t*,property_definition_t*);
static property_list_t *property_list_add(parser_ctx_t*,property_list_t*,property_definition_t*);

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


#line 199 "parser.tab.c"

# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
#ifndef YY_PARSER_E_REACTOSSYNC_GCC_DLL_WIN32_JSCRIPT_PARSER_TAB_H_INCLUDED
# define YY_PARSER_E_REACTOSSYNC_GCC_DLL_WIN32_JSCRIPT_PARSER_TAB_H_INCLUDED
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
    kFUNCTION = 266,
    kIF = 267,
    kFINALLY = 268,
    kFOR = 269,
    kGET = 270,
    kIN = 271,
    kSET = 272,
    kINSTANCEOF = 273,
    kNEW = 274,
    kNULL = 275,
    kRETURN = 276,
    kSWITCH = 277,
    kTHIS = 278,
    kTHROW = 279,
    kTRUE = 280,
    kFALSE = 281,
    kTRY = 282,
    kTYPEOF = 283,
    kVAR = 284,
    kVOID = 285,
    kWHILE = 286,
    kWITH = 287,
    tANDAND = 288,
    tOROR = 289,
    tINC = 290,
    tDEC = 291,
    tHTMLCOMMENT = 292,
    kDIVEQ = 293,
    kDCOL = 294,
    tIdentifier = 295,
    tAssignOper = 296,
    tEqOper = 297,
    tShiftOper = 298,
    tRelOper = 299,
    tNumericLiteral = 300,
    tBooleanLiteral = 301,
    tStringLiteral = 302,
    tEOF = 303,
    LOWER_THAN_ELSE = 304
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 147 "parser.y"

    int                     ival;
    const WCHAR             *srcptr;
    jsstr_t                 *str;
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
    property_definition_t   *property_definition;
    source_elements_t       *source_elements;
    statement_t             *statement;
    struct _statement_list_t *statement_list;
    struct _variable_list_t *variable_list;
    variable_declaration_t  *variable_declaration;

#line 314 "parser.tab.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int parser_parse (parser_ctx_t *ctx);

#endif /* !YY_PARSER_E_REACTOSSYNC_GCC_DLL_WIN32_JSCRIPT_PARSER_TAB_H_INCLUDED  */



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


#define YY_ASSERT(E) ((void) (0 && (E)))

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
#define YYLAST   1393

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  72
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  99
/* YYNRULES -- Number of rules.  */
#define YYNRULES  254
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  454

#define YYUNDEFTOK  2
#define YYMAXUTOK   304

/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                \
  ((unsigned) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    66,     2,     2,     2,    64,    59,     2,
      70,    71,    62,    60,    52,    61,    69,    63,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    55,    54,
       2,    53,     2,    56,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    67,     2,    68,    58,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    51,    57,    40,    65,     2,     2,     2,
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
      35,    36,    37,    38,    39,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   258,   258,   262,   263,   267,   268,   273,   275,   277,
     281,   285,   289,   290,   295,   296,   300,   301,   302,   303,
     304,   305,   306,   307,   308,   309,   310,   311,   312,   313,
     314,   318,   319,   324,   325,   329,   330,   334,   339,   340,
     345,   347,   352,   357,   362,   363,   367,   372,   373,   377,
     382,   386,   391,   393,   398,   400,   403,   405,   402,   409,
     411,   408,   414,   416,   421,   426,   431,   436,   441,   446,
     451,   453,   458,   459,   463,   464,   469,   474,   479,   484,
     485,   486,   491,   496,   500,   501,   504,   505,   509,   510,
     515,   516,   520,   522,   526,   527,   531,   532,   534,   539,
     541,   543,   548,   549,   554,   556,   561,   562,   567,   569,
     574,   575,   580,   582,   587,   588,   593,   595,   600,   601,
     606,   608,   613,   614,   619,   621,   626,   627,   632,   633,
     638,   639,   641,   643,   648,   649,   651,   656,   657,   662,
     664,   666,   671,   672,   674,   676,   681,   682,   684,   685,
     687,   688,   689,   690,   691,   692,   696,   698,   700,   706,
     707,   711,   712,   716,   717,   718,   720,   722,   727,   729,
     731,   733,   738,   739,   743,   744,   749,   750,   751,   752,
     753,   754,   758,   759,   760,   761,   766,   768,   773,   774,
     778,   779,   783,   784,   786,   797,   798,   803,   805,   807,
     811,   816,   817,   818,   822,   823,   827,   828,   839,   840,
     841,   842,   843,   844,   845,   846,   847,   848,   849,   850,
     851,   852,   853,   854,   855,   856,   857,   858,   859,   860,
     861,   862,   863,   864,   865,   866,   867,   868,   872,   873,
     874,   875,   876,   878,   883,   884,   885,   888,   889,   892,
     893,   896,   897,   900,   901
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "kBREAK", "kCASE", "kCATCH", "kCONTINUE",
  "kDEFAULT", "kDELETE", "kDO", "kELSE", "kFUNCTION", "kIF", "kFINALLY",
  "kFOR", "kGET", "kIN", "kSET", "kINSTANCEOF", "kNEW", "kNULL", "kRETURN",
  "kSWITCH", "kTHIS", "kTHROW", "kTRUE", "kFALSE", "kTRY", "kTYPEOF",
  "kVAR", "kVOID", "kWHILE", "kWITH", "tANDAND", "tOROR", "tINC", "tDEC",
  "tHTMLCOMMENT", "kDIVEQ", "kDCOL", "'}'", "tIdentifier", "tAssignOper",
  "tEqOper", "tShiftOper", "tRelOper", "tNumericLiteral",
  "tBooleanLiteral", "tStringLiteral", "tEOF", "LOWER_THAN_ELSE", "'{'",
  "','", "'='", "';'", "':'", "'?'", "'|'", "'^'", "'&'", "'+'", "'-'",
  "'*'", "'/'", "'%'", "'~'", "'!'", "'['", "']'", "'.'", "'('", "')'",
  "$accept", "Program", "HtmlComment", "SourceElements",
  "FunctionExpression", "KFunction", "FunctionBody", "FormalParameterList",
  "FormalParameterList_opt", "Statement", "StatementList",
  "StatementList_opt", "Block", "VariableStatement",
  "VariableDeclarationList", "VariableDeclarationListNoIn",
  "VariableDeclaration", "VariableDeclarationNoIn", "Initialiser_opt",
  "Initialiser", "InitialiserNoIn_opt", "InitialiserNoIn",
  "EmptyStatement", "ExpressionStatement", "IfStatement",
  "IterationStatement", "$@1", "$@2", "$@3", "$@4", "ContinueStatement",
  "BreakStatement", "ReturnStatement", "WithStatement",
  "LabelledStatement", "SwitchStatement", "CaseBlock", "CaseClausules_opt",
  "CaseClausules", "CaseClausule", "DefaultClausule", "ThrowStatement",
  "TryStatement", "Catch", "Finally", "Expression_opt", "Expression_err",
  "Expression", "ExpressionNoIn_opt", "ExpressionNoIn", "AssignOper",
  "AssignmentExpression", "AssignmentExpressionNoIn",
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
  "PropertyNameAndValueList", "PropertyDefinition", "GetterSetterMethod",
  "PropertyName", "Identifier_opt", "IdentifierName",
  "ReservedAsIdentifier", "Literal", "BooleanLiteral", "semicolon_opt",
  "left_bracket", "right_bracket", "semicolon", YY_NULLPTR
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
     125,   295,   296,   297,   298,   299,   300,   301,   302,   303,
     304,   123,    44,    61,    59,    58,    63,   124,    94,    38,
      43,    45,    42,    47,    37,   126,    33,    91,    93,    46,
      40,    41
};
# endif

#define YYPACT_NINF -361

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-361)))

#define YYTABLE_NINF -238

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -361,    82,   649,  -361,   -15,   -15,  1146,   844,  -361,    60,
      60,   236,  -361,  1146,    60,  -361,  1146,  -361,  -361,    14,
    1146,    73,  1146,    60,    60,  1146,  1146,  -361,  -361,    72,
    -361,  -361,  -361,   584,  -361,  1146,  1146,  -361,  1146,  1146,
      94,  1146,    88,   472,    36,  -361,  -361,  -361,  -361,  -361,
    -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,
      51,  -361,  -361,    86,   123,   124,   130,   145,   163,    22,
     183,    -5,   182,  -361,  -361,   155,  -361,   219,   224,  -361,
    -361,  -361,  -361,  -361,  -361,    31,    31,  -361,  1214,  -361,
    -361,   230,   217,  -361,  -361,   434,   965,  -361,   219,    31,
     205,  1146,    51,   714,   198,  -361,   197,    59,  -361,  -361,
     434,  1146,  -361,  -361,   844,     9,  -361,  -361,   114,  -361,
    1146,   844,  -361,   212,    38,  -361,    79,  1306,  -361,  1306,
    -361,   236,   221,  1025,    83,   223,  1146,   226,   246,    14,
    1146,    73,  1146,    95,   103,   517,    72,   249,   250,  -361,
     779,   149,  -361,   252,  -361,  -361,  -361,  -361,  -361,  -361,
    -361,  -361,   102,   107,  1146,    17,  -361,    93,   244,  -361,
    1146,  -361,  -361,  1146,  1146,  1146,  1146,  1146,  1146,  1146,
    1146,  1146,  1146,  1146,  1146,  1146,  1146,  1146,  1146,  -361,
    -361,  -361,  -361,  1146,  1146,  1146,  1352,   904,  -361,  1146,
    1352,  -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,
    -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,
    -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,    60,
    -361,    24,   205,   251,  -361,   256,  -361,  -361,   111,   276,
     253,   255,   258,   272,    98,   183,   237,  -361,  -361,    16,
    -361,  -361,    60,    14,   305,  -361,  1146,  -361,  -361,    73,
    -361,    24,    16,  -361,  -361,  -361,    60,    60,  -361,  -361,
    -361,  1260,  1146,   267,  -361,  -361,  -361,  -361,  -361,   279,
     244,  -361,   269,    24,  -361,   123,   270,   124,   130,   145,
     163,    22,   183,   183,   183,    -5,   182,   182,  -361,  -361,
    -361,  -361,  -361,   108,  -361,  -361,  -361,    26,   116,  -361,
     434,  -361,  -361,   844,   271,   274,   306,    48,  1146,  1146,
    1146,  1146,  1146,  1146,  1146,  1146,  1146,  1146,   434,  1146,
    1146,   277,   286,  -361,  -361,  -361,  -361,   844,   844,  -361,
     244,  -361,  -361,  -361,  -361,   278,  1085,    60,    24,   288,
     280,  1146,  -361,  1146,  -361,  -361,    24,   322,  1146,  -361,
    -361,   251,    48,   434,  -361,  -361,  1146,  -361,   216,   276,
     282,   253,   255,   258,   272,    98,   183,   183,    24,  -361,
    -361,   329,  -361,    24,  -361,  -361,    24,  -361,  -361,   244,
     283,  -361,  -361,  -361,  -361,    31,   844,  -361,  -361,  1146,
      24,  -361,  1146,   844,  1146,    46,   329,  -361,    14,   289,
      24,  -361,   844,   301,  -361,  -361,  -361,   844,    48,  -361,
    -361,   186,   287,  -361,   329,  -361,  -361,  -361,   292,   309,
    -361,    48,  -361,  1146,   844,   844,   310,   312,  -361,  -361,
    1146,    24,   844,  -361,  -361,  -361,  -361,   313,    24,   844,
    -361,   844,  -361,  -361
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       5,     0,     4,     1,   204,   204,     0,     0,    10,     0,
       0,     0,   238,    84,     0,   176,     0,   244,   245,     0,
       0,     0,     0,     0,     0,     0,     0,     3,   243,   177,
     240,   246,   241,     0,    50,     0,     0,   242,     0,     0,
     190,     0,     0,    19,     0,     6,    16,    17,    18,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
       0,    88,    96,   102,   106,   110,   114,   118,   122,   126,
     130,   137,   139,   142,   146,   156,   159,   161,   160,   163,
     179,   180,   178,   239,   205,     0,     0,   177,     0,   164,
     147,   156,     0,   250,   249,     0,    90,   162,   161,     0,
      85,     0,     0,     0,     0,   149,    44,     0,    38,   148,
       0,     0,   150,   151,     0,   204,   209,   210,   204,   212,
     213,   214,   215,    10,     0,   217,     0,   220,   222,   227,
     223,   224,   238,    84,     0,   176,   230,   244,   245,   232,
     233,   234,   235,     0,     0,    36,   177,   240,   241,    31,
       0,     0,   195,     0,   201,   207,   152,   153,   154,   155,
     188,   182,     0,   191,     0,     0,     2,     0,    14,   248,
       0,   247,    51,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   157,
     158,    95,    94,     0,     0,     0,     0,     0,   168,     0,
       0,   169,    65,    64,   208,   211,   213,   214,   219,   221,
     218,   224,   225,   226,   228,   229,   230,   231,   216,   232,
     233,   234,   235,   236,   237,   192,   206,   203,   202,     0,
      87,     0,    86,     0,    56,    91,    92,    99,   104,   108,
     112,   116,   120,   124,   128,   134,   156,   167,    66,     0,
      78,    36,     0,     0,    79,    80,     0,    42,    45,     0,
      37,     0,     0,    68,   220,   227,     0,     0,    35,    32,
     193,     0,     0,   190,   184,   189,   183,   186,   181,     0,
      14,    12,    15,     0,    89,   107,     0,   111,   115,   119,
     123,   127,   133,   132,   131,   138,   140,   141,   143,   144,
     145,    97,    98,     0,   166,   172,   174,     0,     0,   171,
       0,   252,   251,     0,    47,    59,    40,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    83,    81,    46,    39,     0,     0,   198,
      14,   199,   194,   196,   197,   191,     0,     0,     0,     0,
       0,     0,   165,     0,   173,   170,     0,    53,     0,    43,
      48,     0,     0,     0,   254,   253,    84,    93,   156,   109,
       0,   113,   117,   121,   125,   129,   136,   135,     0,   100,
     101,    72,    69,     0,    55,    67,     0,   185,   187,    14,
       0,    13,     5,   103,   175,     0,     0,    49,    41,    84,
       0,    57,     0,     0,     0,     0,    73,    74,     0,     0,
       0,     5,    11,     0,    54,    52,    60,     0,     0,   105,
      62,     0,     0,    70,    72,    75,    82,     5,     0,     0,
       7,     0,    63,    84,    33,    33,     0,     0,     5,     8,
      84,     0,    34,    76,    77,    71,   200,     0,     0,     0,
       9,     0,    58,    61
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -361,  -361,  -361,   345,    -2,  -361,  -357,  -361,  -261,     1,
    -164,   -70,   -10,  -361,  -361,  -361,   109,     8,  -361,  -361,
    -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,
    -361,  -361,  -361,  -361,  -361,  -361,  -361,   -54,  -361,   -35,
    -361,  -361,  -361,  -361,   118,  -333,   -68,   -12,  -361,  -361,
     298,  -146,  -284,  -361,  -361,  -361,  -361,   201,    56,   202,
      55,   203,    58,   204,    61,   200,    62,   206,    57,   -90,
     207,   112,   160,  -361,    37,     4,     5,  -361,    74,  -361,
    -361,  -361,  -361,   110,   115,  -361,  -361,   120,   122,    65,
       7,   -22,  -361,  -361,  -361,   -72,    -3,  -239,  -360
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    42,   412,    89,    44,   413,   282,   283,   149,
     150,   443,    46,    47,   107,   315,   108,   316,   257,   258,
     359,   360,    48,    49,    50,    51,   317,   418,   362,   431,
      52,    53,    54,    55,    56,    57,   382,   405,   406,   407,
     424,    58,    59,   254,   255,    99,   231,    60,   234,   235,
     330,    61,   236,    62,   237,    63,   238,    64,   239,    65,
     240,    66,   241,    67,   242,    68,   243,    69,   244,    70,
      71,    72,    73,    74,    75,    76,    77,    78,   198,   307,
      79,    80,   162,   163,   164,    81,   151,   152,   339,   153,
      85,   154,   155,    82,    83,   172,    95,   313,   366
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      43,   100,   399,    45,   102,    43,   245,    96,    92,   104,
     331,   101,    86,   202,   203,    97,    98,   311,   277,   348,
     110,   111,   337,   338,   284,   311,    84,   248,   286,   165,
     250,    43,   169,   401,   367,   260,   370,    93,   180,    93,
     181,   168,   261,    91,   350,   379,   380,   301,   302,   364,
      84,   306,   169,   422,   429,   184,   185,    91,   433,    91,
     169,    93,    91,    91,  -208,   103,   416,   182,   170,   170,
     437,   440,    91,    91,   397,    91,    91,   167,   353,   386,
      93,   447,     3,   232,    93,   171,   423,   312,   278,   249,
     292,   293,   294,  -221,    93,   312,    93,   354,   232,   262,
     441,    43,   365,   170,    93,   171,    94,   448,    94,   390,
     335,   259,    43,   171,   106,   263,   326,   395,   419,    43,
     173,   100,    92,    96,   102,    86,   344,   114,   410,   104,
      94,   101,   279,   246,  -218,    97,    98,   166,  -228,   403,
     110,   111,   174,   327,   408,   319,   160,   409,    43,    94,
    -236,   269,   201,    94,   273,    84,   175,    91,  -237,   275,
     170,   417,   161,    94,   280,    94,    90,   320,   170,  -211,
     274,   428,   247,    94,   304,   276,   352,    91,   309,    91,
     105,   176,   109,   303,   355,   112,   113,   308,   177,   270,
     189,   190,   266,   191,   267,   156,   157,   192,   158,   159,
     388,   271,   449,   252,   178,   393,   179,   394,   193,   451,
      91,   253,    91,    91,    91,    91,    91,    91,    91,    91,
      91,    91,    91,    91,    91,    91,   310,   183,   245,   245,
     245,   245,   245,   245,   245,   245,   376,   377,   170,   245,
     245,   434,   356,   333,   186,   187,   188,     8,   229,   332,
     256,   189,   190,   328,   191,    11,    12,   170,   192,    15,
     378,    17,    18,   340,   340,   189,   190,  -219,   245,   329,
     442,   442,   189,   190,    28,   191,  -225,    87,  -229,   192,
      90,  -231,    30,    31,    32,   281,   195,    88,   196,   197,
     329,   199,   314,   200,   197,   400,   296,   297,   232,    37,
     105,  -216,   109,    40,  -203,  -202,    41,   272,   318,   321,
     322,    43,   245,   323,   357,   325,   232,   324,   253,   160,
     347,   349,   363,   414,   358,   351,   361,   383,   381,   391,
     275,   392,   396,   404,   411,    43,    43,   402,   384,   385,
     427,   430,   435,   438,   389,     2,   298,   299,   300,   439,
     445,   232,   446,   450,   100,   368,    91,   368,    91,    91,
      91,    91,    91,    91,    91,   444,   368,   368,   336,   398,
     436,   425,   334,   194,   285,   369,   371,   287,   290,   288,
     372,   289,   375,   345,   373,   291,   374,   100,   346,   341,
     295,   343,   421,     0,    43,   368,     0,   415,   426,     0,
       0,    43,     0,     0,   420,     0,     0,     0,     0,     0,
      43,     0,     0,    45,     0,    43,     0,     0,   432,     0,
       0,   100,     0,     0,     0,     0,     0,     0,   100,     0,
       0,     0,    43,    43,     0,   230,     0,     0,     0,   368,
      43,     0,     6,   269,     0,     8,     0,    43,     0,    43,
     452,     0,   453,    11,    12,     0,     0,    15,     0,    17,
      18,     0,    20,     0,    22,     0,     0,     0,     0,    25,
      26,     0,    28,  -164,     0,    87,     0,     0,     0,     0,
      30,    31,    32,     0,     0,    88,     0,     0,  -164,     0,
    -164,     0,     0,     0,    35,    36,     0,    37,     0,    38,
      39,    40,     0,     0,    41,  -164,  -164,     0,     0,     0,
       0,     0,     0,     0,  -164,  -164,  -164,  -164,  -192,     0,
       0,     0,     0,     0,  -164,  -164,     0,     0,  -164,  -164,
    -164,  -164,     0,  -192,  -164,  -192,  -164,     0,     0,     0,
       0,  -164,     0,     0,     0,     0,     0,     0,     0,     0,
    -192,  -192,     0,     0,     0,     0,     0,     0,     0,  -192,
    -192,  -192,  -192,     0,     0,     0,     0,     0,     0,  -192,
    -192,     0,     0,  -192,  -192,  -192,  -192,     0,     0,  -192,
       0,  -192,     0,     0,     0,     0,  -192,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,     0,     0,    25,
      26,     0,    28,     0,   145,   146,     0,     0,     0,     0,
     147,    31,   148,     0,     0,    33,     0,     0,    34,     0,
       0,     0,     0,     0,    35,    36,     0,    37,     0,    38,
      39,    40,     4,     0,    41,     5,     0,     6,     7,     0,
       8,     9,     0,    10,     0,     0,     0,     0,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,     0,     0,    25,    26,    27,    28,     0,     0,
      29,     0,     0,     0,     0,    30,    31,    32,     0,     0,
      33,     0,     0,    34,     0,     0,     0,     0,     0,    35,
      36,     0,    37,     0,    38,    39,    40,     4,     0,    41,
       5,     0,     6,     7,     0,     8,     9,     0,    10,     0,
       0,     0,     0,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,     0,     0,    25,
      26,     0,    28,     0,   251,    29,     0,     0,     0,     0,
      30,    31,    32,     0,     0,    33,     0,     0,    34,     0,
       0,     0,     0,     0,    35,    36,     0,    37,     0,    38,
      39,    40,     4,     0,    41,     5,     0,     6,     7,     0,
       8,     9,     0,    10,     0,     0,     0,     0,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,     0,     0,    25,    26,     0,    28,     0,   268,
      29,     0,     0,     0,     0,    30,    31,    32,     0,     0,
      33,     0,     0,    34,     0,     0,     0,     0,     0,    35,
      36,     0,    37,     0,    38,    39,    40,     4,     0,    41,
       5,     0,     6,     7,     0,     8,     9,     0,    10,     0,
       0,     0,     0,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,     0,     0,    25,
      26,     0,    28,     0,     0,    29,     0,     0,     0,     0,
      30,    31,    32,     0,     0,    33,     0,     0,    34,     0,
       0,     0,     0,     0,    35,    36,     0,    37,     0,    38,
      39,    40,     6,     0,    41,     8,     0,     0,     0,     0,
       0,     0,     0,    11,    12,     0,     0,    15,     0,    17,
      18,     0,    20,     0,    22,     0,     0,     0,     0,    25,
      26,     0,    28,     0,     0,    87,     0,     0,     0,     0,
      30,    31,    32,     0,     0,    88,     0,     0,     0,     0,
       0,     0,     0,     0,    35,    36,     0,    37,     0,    38,
      39,    40,     0,     6,    41,   305,     8,     0,     0,     0,
       0,     0,     0,     0,    11,    12,     0,     0,    15,     0,
      17,    18,     0,    20,   233,    22,     0,     0,     0,     0,
      25,    26,     0,    28,     0,     0,    87,     0,     0,     0,
       0,    30,    31,    32,     0,     0,    88,     0,     0,     0,
       0,     0,     0,     0,     0,    35,    36,     0,    37,     0,
      38,    39,    40,     6,     0,    41,     8,     0,     0,     0,
       0,     0,     0,     0,    11,    12,     0,     0,    15,     0,
      17,    18,     0,    20,     0,    22,     0,     0,     0,     0,
      25,    26,     0,    28,     0,     0,    87,     0,     0,     0,
       0,    30,    31,    32,     0,     0,    88,     0,     0,     0,
    -226,     0,     0,     0,     0,    35,    36,     0,    37,     0,
      38,    39,    40,     6,     0,    41,     8,     0,     0,     0,
       0,     0,     0,     0,    11,    12,     0,     0,    15,     0,
      17,    18,     0,    20,     0,    22,     0,     0,     0,     0,
      25,    26,     0,    28,     0,     0,    87,     0,     0,     0,
       0,    30,    31,    32,     0,     0,    88,     0,     0,     0,
       0,     0,     0,     0,     0,    35,    36,     0,    37,     0,
      38,    39,    40,   387,     6,    41,     0,     8,     0,     0,
       0,     0,     0,     0,     0,    11,    12,     0,     0,    15,
       0,    17,    18,     0,    20,     0,    22,     0,     0,     0,
       0,    25,    26,     0,    28,     0,     0,    87,     0,     0,
       0,     0,    30,    31,    32,     0,     0,    88,     0,     0,
       0,     0,     0,     0,     0,     0,    35,    36,     0,    37,
       0,    38,    39,    40,     0,     0,    41,   204,   116,   117,
     205,   119,   206,   207,   122,   208,   209,   125,   210,   127,
     128,   129,   130,   211,   212,   213,   214,   215,   216,   217,
     218,   219,   220,   221,   222,   223,   224,     0,     0,     0,
       0,     0,     0,     0,   225,   226,     0,     0,     0,     0,
     227,     0,   228,   204,   116,   117,   205,   119,   206,   207,
     122,   208,   209,   125,   210,   127,   128,   129,   130,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,     0,     0,     0,     0,     0,     0,     0,
     342,   226,     0,     0,     0,     0,   227,     0,   228,   204,
     116,   117,   205,   119,   206,   207,   122,   208,   209,   125,
     210,   264,   128,   265,   130,   211,   212,   213,   214,   215,
     216,   217,   218,   219,   220,   221,   222,   223,   224,     0,
       0,     0,     0,     0,     0,     0,     0,   226,     0,     0,
       0,     0,   227,     0,   228,   204,   116,   117,   205,   119,
     206,   207,   122,   208,   209,   125,   210,   264,   128,   265,
     130,   211,   212,   213,   214,   215,   216,   217,   218,   219,
     220,   221,   222,   223,   224,     0,     0,     0,     0,     0,
       0,     0,     0,   226
};

static const yytype_int16 yycheck[] =
{
       2,    13,   362,     2,    16,     7,    96,    10,     7,    19,
     249,    14,     5,    85,    86,    11,    11,     1,   164,   280,
      23,    24,   261,   262,   170,     1,    41,    99,   174,    41,
     102,    33,     1,   366,   318,   107,   320,     1,    16,     1,
      18,    44,   110,     6,   283,   329,   330,   193,   194,     1,
      41,   197,     1,     7,   411,    60,    61,    20,   418,    22,
       1,     1,    25,    26,    55,    51,   399,    45,    52,    52,
     427,   431,    35,    36,   358,    38,    39,    41,    52,   340,
       1,   438,     0,    95,     1,    54,    40,    71,    71,   101,
     180,   181,   182,    55,     1,    71,     1,    71,   110,   111,
     433,   103,    54,    52,     1,    54,    70,   440,    70,   348,
     256,    52,   114,    54,    41,   114,    18,   356,   402,   121,
      34,   133,   121,   126,   136,   118,   272,    55,   389,   139,
      70,   134,    39,    96,    55,   131,   131,    49,    55,   378,
     143,   144,    56,    45,   383,    34,    52,   386,   150,    70,
      55,   150,    78,    70,    52,    41,    33,   120,    55,    52,
      52,   400,    68,    70,   167,    70,     6,    56,    52,    55,
      68,   410,    98,    70,   196,    68,    68,   140,   200,   142,
      20,    57,    22,   195,    68,    25,    26,   199,    58,    40,
      35,    36,   127,    38,   129,    35,    36,    42,    38,    39,
     346,    52,   441,     5,    59,   351,    43,   353,    53,   448,
     173,    13,   175,   176,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,   187,   188,   229,    44,   318,   319,
     320,   321,   322,   323,   324,   325,   326,   327,    52,   329,
     330,    55,   310,   253,    62,    63,    64,    11,    31,   252,
      53,    35,    36,    16,    38,    19,    20,    52,    42,    23,
     328,    25,    26,   266,   267,    35,    36,    55,   358,    53,
     434,   435,    35,    36,    38,    38,    55,    41,    55,    42,
     120,    55,    46,    47,    48,    41,    67,    51,    69,    70,
      53,    67,    41,    69,    70,   363,   184,   185,   310,    63,
     140,    55,   142,    67,    55,    55,    70,    55,    52,    33,
      57,   313,   402,    58,   313,    43,   328,    59,    13,    52,
      41,    52,    16,   395,    53,    55,    52,    41,    51,    41,
      52,    51,    10,     4,    51,   337,   338,    55,   337,   338,
      51,    40,    55,    51,   347,     0,   186,   187,   188,    40,
      40,   363,    40,    40,   366,   318,   319,   320,   321,   322,
     323,   324,   325,   326,   327,   435,   329,   330,   259,   361,
     424,   406,   254,    75,   173,   319,   321,   175,   178,   176,
     322,   177,   325,   273,   323,   179,   324,   399,   273,   267,
     183,   271,   404,    -1,   396,   358,    -1,   396,   408,    -1,
      -1,   403,    -1,    -1,   403,    -1,    -1,    -1,    -1,    -1,
     412,    -1,    -1,   412,    -1,   417,    -1,    -1,   417,    -1,
      -1,   433,    -1,    -1,    -1,    -1,    -1,    -1,   440,    -1,
      -1,    -1,   434,   435,    -1,     1,    -1,    -1,    -1,   402,
     442,    -1,     8,   442,    -1,    11,    -1,   449,    -1,   451,
     449,    -1,   451,    19,    20,    -1,    -1,    23,    -1,    25,
      26,    -1,    28,    -1,    30,    -1,    -1,    -1,    -1,    35,
      36,    -1,    38,     1,    -1,    41,    -1,    -1,    -1,    -1,
      46,    47,    48,    -1,    -1,    51,    -1,    -1,    16,    -1,
      18,    -1,    -1,    -1,    60,    61,    -1,    63,    -1,    65,
      66,    67,    -1,    -1,    70,    33,    34,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    42,    43,    44,    45,     1,    -1,
      -1,    -1,    -1,    -1,    52,    53,    -1,    -1,    56,    57,
      58,    59,    -1,    16,    62,    18,    64,    -1,    -1,    -1,
      -1,    69,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      33,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    42,
      43,    44,    45,    -1,    -1,    -1,    -1,    -1,    -1,    52,
      53,    -1,    -1,    56,    57,    58,    59,    -1,    -1,    62,
      -1,    64,    -1,    -1,    -1,    -1,    69,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    -1,    -1,    35,
      36,    -1,    38,    -1,    40,    41,    -1,    -1,    -1,    -1,
      46,    47,    48,    -1,    -1,    51,    -1,    -1,    54,    -1,
      -1,    -1,    -1,    -1,    60,    61,    -1,    63,    -1,    65,
      66,    67,     3,    -1,    70,     6,    -1,     8,     9,    -1,
      11,    12,    -1,    14,    -1,    -1,    -1,    -1,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    -1,    -1,    35,    36,    37,    38,    -1,    -1,
      41,    -1,    -1,    -1,    -1,    46,    47,    48,    -1,    -1,
      51,    -1,    -1,    54,    -1,    -1,    -1,    -1,    -1,    60,
      61,    -1,    63,    -1,    65,    66,    67,     3,    -1,    70,
       6,    -1,     8,     9,    -1,    11,    12,    -1,    14,    -1,
      -1,    -1,    -1,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    -1,    -1,    35,
      36,    -1,    38,    -1,    40,    41,    -1,    -1,    -1,    -1,
      46,    47,    48,    -1,    -1,    51,    -1,    -1,    54,    -1,
      -1,    -1,    -1,    -1,    60,    61,    -1,    63,    -1,    65,
      66,    67,     3,    -1,    70,     6,    -1,     8,     9,    -1,
      11,    12,    -1,    14,    -1,    -1,    -1,    -1,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    -1,    -1,    35,    36,    -1,    38,    -1,    40,
      41,    -1,    -1,    -1,    -1,    46,    47,    48,    -1,    -1,
      51,    -1,    -1,    54,    -1,    -1,    -1,    -1,    -1,    60,
      61,    -1,    63,    -1,    65,    66,    67,     3,    -1,    70,
       6,    -1,     8,     9,    -1,    11,    12,    -1,    14,    -1,
      -1,    -1,    -1,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    -1,    -1,    35,
      36,    -1,    38,    -1,    -1,    41,    -1,    -1,    -1,    -1,
      46,    47,    48,    -1,    -1,    51,    -1,    -1,    54,    -1,
      -1,    -1,    -1,    -1,    60,    61,    -1,    63,    -1,    65,
      66,    67,     8,    -1,    70,    11,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    19,    20,    -1,    -1,    23,    -1,    25,
      26,    -1,    28,    -1,    30,    -1,    -1,    -1,    -1,    35,
      36,    -1,    38,    -1,    -1,    41,    -1,    -1,    -1,    -1,
      46,    47,    48,    -1,    -1,    51,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    60,    61,    -1,    63,    -1,    65,
      66,    67,    -1,     8,    70,    71,    11,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    19,    20,    -1,    -1,    23,    -1,
      25,    26,    -1,    28,    29,    30,    -1,    -1,    -1,    -1,
      35,    36,    -1,    38,    -1,    -1,    41,    -1,    -1,    -1,
      -1,    46,    47,    48,    -1,    -1,    51,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    60,    61,    -1,    63,    -1,
      65,    66,    67,     8,    -1,    70,    11,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    19,    20,    -1,    -1,    23,    -1,
      25,    26,    -1,    28,    -1,    30,    -1,    -1,    -1,    -1,
      35,    36,    -1,    38,    -1,    -1,    41,    -1,    -1,    -1,
      -1,    46,    47,    48,    -1,    -1,    51,    -1,    -1,    -1,
      55,    -1,    -1,    -1,    -1,    60,    61,    -1,    63,    -1,
      65,    66,    67,     8,    -1,    70,    11,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    19,    20,    -1,    -1,    23,    -1,
      25,    26,    -1,    28,    -1,    30,    -1,    -1,    -1,    -1,
      35,    36,    -1,    38,    -1,    -1,    41,    -1,    -1,    -1,
      -1,    46,    47,    48,    -1,    -1,    51,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    60,    61,    -1,    63,    -1,
      65,    66,    67,    68,     8,    70,    -1,    11,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    19,    20,    -1,    -1,    23,
      -1,    25,    26,    -1,    28,    -1,    30,    -1,    -1,    -1,
      -1,    35,    36,    -1,    38,    -1,    -1,    41,    -1,    -1,
      -1,    -1,    46,    47,    48,    -1,    -1,    51,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    60,    61,    -1,    63,
      -1,    65,    66,    67,    -1,    -1,    70,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    40,    41,    -1,    -1,    -1,    -1,
      46,    -1,    48,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      40,    41,    -1,    -1,    -1,    -1,    46,    -1,    48,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,    -1,    -1,
      -1,    -1,    46,    -1,    48,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    41
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    73,    75,     0,     3,     6,     8,     9,    11,    12,
      14,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    35,    36,    37,    38,    41,
      46,    47,    48,    51,    54,    60,    61,    63,    65,    66,
      67,    70,    74,    76,    77,    81,    84,    85,    94,    95,
      96,    97,   102,   103,   104,   105,   106,   107,   113,   114,
     119,   123,   125,   127,   129,   131,   133,   135,   137,   139,
     141,   142,   143,   144,   145,   146,   147,   148,   149,   152,
     153,   157,   165,   166,    41,   162,   162,    41,    51,    76,
     144,   146,    81,     1,    70,   168,   168,   147,   148,   117,
     119,   168,   119,    51,    84,   144,    41,    86,    88,   144,
     168,   168,   144,   144,    55,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    40,    41,    46,    48,    81,
      82,   158,   159,   161,   163,   164,   144,   144,   144,   144,
      52,    68,   154,   155,   156,   119,    49,    41,   168,     1,
      52,    54,   167,    34,    56,    33,    57,    58,    59,    43,
      16,    18,    45,    44,    60,    61,    62,    63,    64,    35,
      36,    38,    42,    53,   122,    67,    69,    70,   150,    67,
      69,   150,   167,   167,     3,     6,     8,     9,    11,    12,
      14,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    40,    41,    46,    48,    31,
       1,   118,   119,    29,   120,   121,   124,   126,   128,   130,
     132,   134,   136,   138,   140,   141,   146,   150,   167,   119,
     167,    40,     5,    13,   115,   116,    53,    90,    91,    52,
     167,   118,   119,    81,    15,    17,   161,   161,    40,    81,
      40,    52,    55,    52,    68,    52,    68,   123,    71,    39,
     168,    41,    79,    80,   123,   129,   123,   131,   133,   135,
     137,   139,   141,   141,   141,   142,   143,   143,   144,   144,
     144,   123,   123,   119,   163,    71,   123,   151,   119,   163,
     168,     1,    71,   169,    41,    87,    89,    98,    52,    34,
      56,    33,    57,    58,    59,    43,    18,    45,    16,    53,
     122,   169,   168,    84,   116,   123,    88,   169,   169,   160,
     168,   160,    40,   159,   123,   155,   156,    41,    80,    52,
     169,    55,    68,    52,    71,    68,   118,    81,    53,    92,
      93,    52,   100,    16,     1,    54,   170,   124,   146,   130,
     124,   132,   134,   136,   138,   140,   141,   141,   118,   124,
     124,    51,   108,    41,    81,    81,    80,    68,   123,   168,
     169,    41,    51,   123,   123,   169,    10,   124,    89,   170,
     118,   117,    55,   169,     4,   109,   110,   111,   169,   169,
      80,    51,    75,    78,   167,    81,   117,   169,    99,   124,
      81,   119,     7,    40,   112,   111,    84,    51,   169,    78,
      40,   101,    81,   170,    55,    55,   109,    78,    51,    40,
     170,   117,    82,    83,    83,    40,    40,    78,   117,   169,
      40,   169,    81,    81
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    72,    73,    74,    74,    75,    75,    76,    76,    76,
      77,    78,    79,    79,    80,    80,    81,    81,    81,    81,
      81,    81,    81,    81,    81,    81,    81,    81,    81,    81,
      81,    82,    82,    83,    83,    84,    84,    85,    86,    86,
      87,    87,    88,    89,    90,    90,    91,    92,    92,    93,
      94,    95,    96,    96,    97,    97,    98,    99,    97,   100,
     101,    97,    97,    97,   102,   103,   104,   105,   106,   107,
     108,   108,   109,   109,   110,   110,   111,   112,   113,   114,
     114,   114,   115,   116,   117,   117,   118,   118,   119,   119,
     120,   120,   121,   121,   122,   122,   123,   123,   123,   124,
     124,   124,   125,   125,   126,   126,   127,   127,   128,   128,
     129,   129,   130,   130,   131,   131,   132,   132,   133,   133,
     134,   134,   135,   135,   136,   136,   137,   137,   138,   138,
     139,   139,   139,   139,   140,   140,   140,   141,   141,   142,
     142,   142,   143,   143,   143,   143,   144,   144,   144,   144,
     144,   144,   144,   144,   144,   144,   145,   145,   145,   146,
     146,   147,   147,   148,   148,   148,   148,   148,   149,   149,
     149,   149,   150,   150,   151,   151,   152,   152,   152,   152,
     152,   152,   153,   153,   153,   153,   154,   154,   155,   155,
     156,   156,   157,   157,   157,   158,   158,   159,   159,   159,
     160,   161,   161,   161,   162,   162,   163,   163,   164,   164,
     164,   164,   164,   164,   164,   164,   164,   164,   164,   164,
     164,   164,   164,   164,   164,   164,   164,   164,   164,   164,
     164,   164,   164,   164,   164,   164,   164,   164,   165,   165,
     165,   165,   165,   165,   166,   166,   166,   167,   167,   168,
     168,   169,   169,   170,   170
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
       0,     1,     2,     3,     4,     1,     3,     3,     3,     3,
       6,     1,     1,     1,     0,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
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


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep, parser_ctx_t *ctx)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  YYUSE (ctx);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep, parser_ctx_t *ctx)
{
  YYFPRINTF (yyo, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyo, yytype, yyvaluep, ctx);
  YYFPRINTF (yyo, ")");
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
                       &yyvsp[(yyi + 1) - (yynrhs)]
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
            else
              goto append;

          append:
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

  return (YYSIZE_T) (yystpcpy (yyres, yystr) - yyres);
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
                  if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
                    yysize = yysize1;
                  else
                    return 2;
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
    if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
      yysize = yysize1;
    else
      return 2;
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
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yynewstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  *yyssp = (yytype_int16) yystate;

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = (YYSIZE_T) (yyssp - yyss + 1);

# if defined yyoverflow
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
# else /* defined YYSTACK_RELOCATE */
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
# undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

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
| yyreduce -- do a reduction.  |
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
#line 259 "parser.y"
    { program_parsed(ctx, (yyvsp[-2].source_elements)); }
#line 1957 "parser.tab.c"
    break;

  case 3:
#line 262 "parser.y"
    {}
#line 1963 "parser.tab.c"
    break;

  case 4:
#line 263 "parser.y"
    {}
#line 1969 "parser.tab.c"
    break;

  case 5:
#line 267 "parser.y"
    { (yyval.source_elements) = new_source_elements(ctx); }
#line 1975 "parser.tab.c"
    break;

  case 6:
#line 269 "parser.y"
    { (yyval.source_elements) = source_elements_add_statement((yyvsp[-1].source_elements), (yyvsp[0].statement)); }
#line 1981 "parser.tab.c"
    break;

  case 7:
#line 274 "parser.y"
    { (yyval.expr) = new_function_expression(ctx, NULL, (yyvsp[-4].parameter_list), (yyvsp[-1].source_elements), NULL, (yyvsp[-6].srcptr), (yyvsp[0].srcptr)-(yyvsp[-6].srcptr)+1); }
#line 1987 "parser.tab.c"
    break;

  case 8:
#line 276 "parser.y"
    { (yyval.expr) = new_function_expression(ctx, (yyvsp[-6].identifier), (yyvsp[-4].parameter_list), (yyvsp[-1].source_elements), NULL, (yyvsp[-7].srcptr), (yyvsp[0].srcptr)-(yyvsp[-7].srcptr)+1); }
#line 1993 "parser.tab.c"
    break;

  case 9:
#line 278 "parser.y"
    { (yyval.expr) = new_function_expression(ctx, (yyvsp[-6].identifier), (yyvsp[-4].parameter_list), (yyvsp[-1].source_elements), (yyvsp[-8].identifier), (yyvsp[-9].srcptr), (yyvsp[0].srcptr)-(yyvsp[-9].srcptr)+1); }
#line 1999 "parser.tab.c"
    break;

  case 10:
#line 281 "parser.y"
    { (yyval.srcptr) = ctx->ptr - 8; }
#line 2005 "parser.tab.c"
    break;

  case 11:
#line 285 "parser.y"
    { (yyval.source_elements) = (yyvsp[0].source_elements); }
#line 2011 "parser.tab.c"
    break;

  case 12:
#line 289 "parser.y"
    { (yyval.parameter_list) = new_parameter_list(ctx, (yyvsp[0].identifier)); }
#line 2017 "parser.tab.c"
    break;

  case 13:
#line 291 "parser.y"
    { (yyval.parameter_list) = parameter_list_add(ctx, (yyvsp[-2].parameter_list), (yyvsp[0].identifier)); }
#line 2023 "parser.tab.c"
    break;

  case 14:
#line 295 "parser.y"
    { (yyval.parameter_list) = NULL; }
#line 2029 "parser.tab.c"
    break;

  case 15:
#line 296 "parser.y"
    { (yyval.parameter_list) = (yyvsp[0].parameter_list); }
#line 2035 "parser.tab.c"
    break;

  case 16:
#line 300 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); }
#line 2041 "parser.tab.c"
    break;

  case 17:
#line 301 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); }
#line 2047 "parser.tab.c"
    break;

  case 18:
#line 302 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); }
#line 2053 "parser.tab.c"
    break;

  case 19:
#line 303 "parser.y"
    { (yyval.statement) = new_expression_statement(ctx, (yyvsp[0].expr)); }
#line 2059 "parser.tab.c"
    break;

  case 20:
#line 304 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); }
#line 2065 "parser.tab.c"
    break;

  case 21:
#line 305 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); }
#line 2071 "parser.tab.c"
    break;

  case 22:
#line 306 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); }
#line 2077 "parser.tab.c"
    break;

  case 23:
#line 307 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); }
#line 2083 "parser.tab.c"
    break;

  case 24:
#line 308 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); }
#line 2089 "parser.tab.c"
    break;

  case 25:
#line 309 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); }
#line 2095 "parser.tab.c"
    break;

  case 26:
#line 310 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); }
#line 2101 "parser.tab.c"
    break;

  case 27:
#line 311 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); }
#line 2107 "parser.tab.c"
    break;

  case 28:
#line 312 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); }
#line 2113 "parser.tab.c"
    break;

  case 29:
#line 313 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); }
#line 2119 "parser.tab.c"
    break;

  case 30:
#line 314 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); }
#line 2125 "parser.tab.c"
    break;

  case 31:
#line 318 "parser.y"
    { (yyval.statement_list) = new_statement_list(ctx, (yyvsp[0].statement)); }
#line 2131 "parser.tab.c"
    break;

  case 32:
#line 320 "parser.y"
    { (yyval.statement_list) = statement_list_add((yyvsp[-1].statement_list), (yyvsp[0].statement)); }
#line 2137 "parser.tab.c"
    break;

  case 33:
#line 324 "parser.y"
    { (yyval.statement_list) = NULL; }
#line 2143 "parser.tab.c"
    break;

  case 34:
#line 325 "parser.y"
    { (yyval.statement_list) = (yyvsp[0].statement_list); }
#line 2149 "parser.tab.c"
    break;

  case 35:
#line 329 "parser.y"
    { (yyval.statement) = new_block_statement(ctx, (yyvsp[-1].statement_list)); }
#line 2155 "parser.tab.c"
    break;

  case 36:
#line 330 "parser.y"
    { (yyval.statement) = new_block_statement(ctx, NULL); }
#line 2161 "parser.tab.c"
    break;

  case 37:
#line 335 "parser.y"
    { (yyval.statement) = new_var_statement(ctx, (yyvsp[-1].variable_list)); }
#line 2167 "parser.tab.c"
    break;

  case 38:
#line 339 "parser.y"
    { (yyval.variable_list) = new_variable_list(ctx, (yyvsp[0].variable_declaration)); }
#line 2173 "parser.tab.c"
    break;

  case 39:
#line 341 "parser.y"
    { (yyval.variable_list) = variable_list_add(ctx, (yyvsp[-2].variable_list), (yyvsp[0].variable_declaration)); }
#line 2179 "parser.tab.c"
    break;

  case 40:
#line 346 "parser.y"
    { (yyval.variable_list) = new_variable_list(ctx, (yyvsp[0].variable_declaration)); }
#line 2185 "parser.tab.c"
    break;

  case 41:
#line 348 "parser.y"
    { (yyval.variable_list) = variable_list_add(ctx, (yyvsp[-2].variable_list), (yyvsp[0].variable_declaration)); }
#line 2191 "parser.tab.c"
    break;

  case 42:
#line 353 "parser.y"
    { (yyval.variable_declaration) = new_variable_declaration(ctx, (yyvsp[-1].identifier), (yyvsp[0].expr)); }
#line 2197 "parser.tab.c"
    break;

  case 43:
#line 358 "parser.y"
    { (yyval.variable_declaration) = new_variable_declaration(ctx, (yyvsp[-1].identifier), (yyvsp[0].expr)); }
#line 2203 "parser.tab.c"
    break;

  case 44:
#line 362 "parser.y"
    { (yyval.expr) = NULL; }
#line 2209 "parser.tab.c"
    break;

  case 45:
#line 363 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2215 "parser.tab.c"
    break;

  case 46:
#line 368 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2221 "parser.tab.c"
    break;

  case 47:
#line 372 "parser.y"
    { (yyval.expr) = NULL; }
#line 2227 "parser.tab.c"
    break;

  case 48:
#line 373 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2233 "parser.tab.c"
    break;

  case 49:
#line 378 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2239 "parser.tab.c"
    break;

  case 50:
#line 382 "parser.y"
    { (yyval.statement) = new_statement(ctx, STAT_EMPTY, 0); }
#line 2245 "parser.tab.c"
    break;

  case 51:
#line 387 "parser.y"
    { (yyval.statement) = new_expression_statement(ctx, (yyvsp[-1].expr)); }
#line 2251 "parser.tab.c"
    break;

  case 52:
#line 392 "parser.y"
    { (yyval.statement) = new_if_statement(ctx, (yyvsp[-4].expr), (yyvsp[-2].statement), (yyvsp[0].statement)); }
#line 2257 "parser.tab.c"
    break;

  case 53:
#line 394 "parser.y"
    { (yyval.statement) = new_if_statement(ctx, (yyvsp[-2].expr), (yyvsp[0].statement), NULL); }
#line 2263 "parser.tab.c"
    break;

  case 54:
#line 399 "parser.y"
    { (yyval.statement) = new_while_statement(ctx, TRUE, (yyvsp[-2].expr), (yyvsp[-5].statement)); }
#line 2269 "parser.tab.c"
    break;

  case 55:
#line 401 "parser.y"
    { (yyval.statement) = new_while_statement(ctx, FALSE, (yyvsp[-2].expr), (yyvsp[0].statement)); }
#line 2275 "parser.tab.c"
    break;

  case 56:
#line 403 "parser.y"
    { if(!explicit_error(ctx, (yyvsp[0].expr), ';')) YYABORT; }
#line 2281 "parser.tab.c"
    break;

  case 57:
#line 405 "parser.y"
    { if(!explicit_error(ctx, (yyvsp[0].expr), ';')) YYABORT; }
#line 2287 "parser.tab.c"
    break;

  case 58:
#line 407 "parser.y"
    { (yyval.statement) = new_for_statement(ctx, NULL, (yyvsp[-8].expr), (yyvsp[-5].expr), (yyvsp[-2].expr), (yyvsp[0].statement)); }
#line 2293 "parser.tab.c"
    break;

  case 59:
#line 409 "parser.y"
    { if(!explicit_error(ctx, (yyvsp[0].variable_list), ';')) YYABORT; }
#line 2299 "parser.tab.c"
    break;

  case 60:
#line 411 "parser.y"
    { if(!explicit_error(ctx, (yyvsp[0].expr), ';')) YYABORT; }
#line 2305 "parser.tab.c"
    break;

  case 61:
#line 413 "parser.y"
    { (yyval.statement) = new_for_statement(ctx, (yyvsp[-8].variable_list), NULL, (yyvsp[-5].expr), (yyvsp[-2].expr), (yyvsp[0].statement)); }
#line 2311 "parser.tab.c"
    break;

  case 62:
#line 415 "parser.y"
    { (yyval.statement) = new_forin_statement(ctx, NULL, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].statement)); }
#line 2317 "parser.tab.c"
    break;

  case 63:
#line 417 "parser.y"
    { (yyval.statement) = new_forin_statement(ctx, (yyvsp[-4].variable_declaration), NULL, (yyvsp[-2].expr), (yyvsp[0].statement)); }
#line 2323 "parser.tab.c"
    break;

  case 64:
#line 422 "parser.y"
    { (yyval.statement) = new_continue_statement(ctx, (yyvsp[-1].identifier)); }
#line 2329 "parser.tab.c"
    break;

  case 65:
#line 427 "parser.y"
    { (yyval.statement) = new_break_statement(ctx, (yyvsp[-1].identifier)); }
#line 2335 "parser.tab.c"
    break;

  case 66:
#line 432 "parser.y"
    { (yyval.statement) = new_return_statement(ctx, (yyvsp[-1].expr)); }
#line 2341 "parser.tab.c"
    break;

  case 67:
#line 437 "parser.y"
    { (yyval.statement) = new_with_statement(ctx, (yyvsp[-2].expr), (yyvsp[0].statement)); }
#line 2347 "parser.tab.c"
    break;

  case 68:
#line 442 "parser.y"
    { (yyval.statement) = new_labelled_statement(ctx, (yyvsp[-2].identifier), (yyvsp[0].statement)); }
#line 2353 "parser.tab.c"
    break;

  case 69:
#line 447 "parser.y"
    { (yyval.statement) = new_switch_statement(ctx, (yyvsp[-2].expr), (yyvsp[0].case_clausule)); }
#line 2359 "parser.tab.c"
    break;

  case 70:
#line 452 "parser.y"
    { (yyval.case_clausule) = new_case_block(ctx, (yyvsp[-1].case_list), NULL, NULL); }
#line 2365 "parser.tab.c"
    break;

  case 71:
#line 454 "parser.y"
    { (yyval.case_clausule) = new_case_block(ctx, (yyvsp[-3].case_list), (yyvsp[-2].case_clausule), (yyvsp[-1].case_list)); }
#line 2371 "parser.tab.c"
    break;

  case 72:
#line 458 "parser.y"
    { (yyval.case_list) = NULL; }
#line 2377 "parser.tab.c"
    break;

  case 73:
#line 459 "parser.y"
    { (yyval.case_list) = (yyvsp[0].case_list); }
#line 2383 "parser.tab.c"
    break;

  case 74:
#line 463 "parser.y"
    { (yyval.case_list) = new_case_list(ctx, (yyvsp[0].case_clausule)); }
#line 2389 "parser.tab.c"
    break;

  case 75:
#line 465 "parser.y"
    { (yyval.case_list) = case_list_add(ctx, (yyvsp[-1].case_list), (yyvsp[0].case_clausule)); }
#line 2395 "parser.tab.c"
    break;

  case 76:
#line 470 "parser.y"
    { (yyval.case_clausule) = new_case_clausule(ctx, (yyvsp[-2].expr), (yyvsp[0].statement_list)); }
#line 2401 "parser.tab.c"
    break;

  case 77:
#line 475 "parser.y"
    { (yyval.case_clausule) = new_case_clausule(ctx, NULL, (yyvsp[0].statement_list)); }
#line 2407 "parser.tab.c"
    break;

  case 78:
#line 480 "parser.y"
    { (yyval.statement) = new_throw_statement(ctx, (yyvsp[-1].expr)); }
#line 2413 "parser.tab.c"
    break;

  case 79:
#line 484 "parser.y"
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[-1].statement), (yyvsp[0].catch_block), NULL); }
#line 2419 "parser.tab.c"
    break;

  case 80:
#line 485 "parser.y"
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[-1].statement), NULL, (yyvsp[0].statement)); }
#line 2425 "parser.tab.c"
    break;

  case 81:
#line 487 "parser.y"
    { (yyval.statement) = new_try_statement(ctx, (yyvsp[-2].statement), (yyvsp[-1].catch_block), (yyvsp[0].statement)); }
#line 2431 "parser.tab.c"
    break;

  case 82:
#line 492 "parser.y"
    { (yyval.catch_block) = new_catch_block(ctx, (yyvsp[-2].identifier), (yyvsp[0].statement)); }
#line 2437 "parser.tab.c"
    break;

  case 83:
#line 496 "parser.y"
    { (yyval.statement) = (yyvsp[0].statement); }
#line 2443 "parser.tab.c"
    break;

  case 84:
#line 500 "parser.y"
    { (yyval.expr) = NULL; }
#line 2449 "parser.tab.c"
    break;

  case 85:
#line 501 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2455 "parser.tab.c"
    break;

  case 86:
#line 504 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2461 "parser.tab.c"
    break;

  case 87:
#line 505 "parser.y"
    { set_error(ctx, JS_E_SYNTAX); YYABORT; }
#line 2467 "parser.tab.c"
    break;

  case 88:
#line 509 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2473 "parser.tab.c"
    break;

  case 89:
#line 511 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_COMMA, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2479 "parser.tab.c"
    break;

  case 90:
#line 515 "parser.y"
    { (yyval.expr) = NULL; }
#line 2485 "parser.tab.c"
    break;

  case 91:
#line 516 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2491 "parser.tab.c"
    break;

  case 92:
#line 521 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2497 "parser.tab.c"
    break;

  case 93:
#line 523 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_COMMA, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2503 "parser.tab.c"
    break;

  case 94:
#line 526 "parser.y"
    { (yyval.ival) = (yyvsp[0].ival); }
#line 2509 "parser.tab.c"
    break;

  case 95:
#line 527 "parser.y"
    { (yyval.ival) = EXPR_ASSIGNDIV; }
#line 2515 "parser.tab.c"
    break;

  case 96:
#line 531 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2521 "parser.tab.c"
    break;

  case 97:
#line 533 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ASSIGN, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2527 "parser.tab.c"
    break;

  case 98:
#line 535 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2533 "parser.tab.c"
    break;

  case 99:
#line 540 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2539 "parser.tab.c"
    break;

  case 100:
#line 542 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ASSIGN, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2545 "parser.tab.c"
    break;

  case 101:
#line 544 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2551 "parser.tab.c"
    break;

  case 102:
#line 548 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2557 "parser.tab.c"
    break;

  case 103:
#line 550 "parser.y"
    { (yyval.expr) = new_conditional_expression(ctx, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2563 "parser.tab.c"
    break;

  case 104:
#line 555 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2569 "parser.tab.c"
    break;

  case 105:
#line 557 "parser.y"
    { (yyval.expr) = new_conditional_expression(ctx, (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2575 "parser.tab.c"
    break;

  case 106:
#line 561 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2581 "parser.tab.c"
    break;

  case 107:
#line 563 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_OR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2587 "parser.tab.c"
    break;

  case 108:
#line 568 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2593 "parser.tab.c"
    break;

  case 109:
#line 570 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_OR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2599 "parser.tab.c"
    break;

  case 110:
#line 574 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2605 "parser.tab.c"
    break;

  case 111:
#line 576 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_AND, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2611 "parser.tab.c"
    break;

  case 112:
#line 581 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2617 "parser.tab.c"
    break;

  case 113:
#line 583 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_AND, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2623 "parser.tab.c"
    break;

  case 114:
#line 587 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2629 "parser.tab.c"
    break;

  case 115:
#line 589 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BOR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2635 "parser.tab.c"
    break;

  case 116:
#line 594 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2641 "parser.tab.c"
    break;

  case 117:
#line 596 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BOR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2647 "parser.tab.c"
    break;

  case 118:
#line 600 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2653 "parser.tab.c"
    break;

  case 119:
#line 602 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BXOR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2659 "parser.tab.c"
    break;

  case 120:
#line 607 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2665 "parser.tab.c"
    break;

  case 121:
#line 609 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BXOR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2671 "parser.tab.c"
    break;

  case 122:
#line 613 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2677 "parser.tab.c"
    break;

  case 123:
#line 615 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BAND, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2683 "parser.tab.c"
    break;

  case 124:
#line 620 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2689 "parser.tab.c"
    break;

  case 125:
#line 622 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_BAND, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2695 "parser.tab.c"
    break;

  case 126:
#line 626 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2701 "parser.tab.c"
    break;

  case 127:
#line 628 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2707 "parser.tab.c"
    break;

  case 128:
#line 632 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2713 "parser.tab.c"
    break;

  case 129:
#line 634 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2719 "parser.tab.c"
    break;

  case 130:
#line 638 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2725 "parser.tab.c"
    break;

  case 131:
#line 640 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2731 "parser.tab.c"
    break;

  case 132:
#line 642 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_INSTANCEOF, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2737 "parser.tab.c"
    break;

  case 133:
#line 644 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_IN, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2743 "parser.tab.c"
    break;

  case 134:
#line 648 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2749 "parser.tab.c"
    break;

  case 135:
#line 650 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2755 "parser.tab.c"
    break;

  case 136:
#line 652 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_INSTANCEOF, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2761 "parser.tab.c"
    break;

  case 137:
#line 656 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2767 "parser.tab.c"
    break;

  case 138:
#line 658 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, (yyvsp[-1].ival), (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2773 "parser.tab.c"
    break;

  case 139:
#line 663 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2779 "parser.tab.c"
    break;

  case 140:
#line 665 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ADD, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2785 "parser.tab.c"
    break;

  case 141:
#line 667 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_SUB, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2791 "parser.tab.c"
    break;

  case 142:
#line 671 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2797 "parser.tab.c"
    break;

  case 143:
#line 673 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_MUL, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2803 "parser.tab.c"
    break;

  case 144:
#line 675 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_DIV, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2809 "parser.tab.c"
    break;

  case 145:
#line 677 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_MOD, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2815 "parser.tab.c"
    break;

  case 146:
#line 681 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2821 "parser.tab.c"
    break;

  case 147:
#line 683 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_DELETE, (yyvsp[0].expr)); }
#line 2827 "parser.tab.c"
    break;

  case 148:
#line 684 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_VOID, (yyvsp[0].expr)); }
#line 2833 "parser.tab.c"
    break;

  case 149:
#line 686 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_TYPEOF, (yyvsp[0].expr)); }
#line 2839 "parser.tab.c"
    break;

  case 150:
#line 687 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PREINC, (yyvsp[0].expr)); }
#line 2845 "parser.tab.c"
    break;

  case 151:
#line 688 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PREDEC, (yyvsp[0].expr)); }
#line 2851 "parser.tab.c"
    break;

  case 152:
#line 689 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_PLUS, (yyvsp[0].expr)); }
#line 2857 "parser.tab.c"
    break;

  case 153:
#line 690 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_MINUS, (yyvsp[0].expr)); }
#line 2863 "parser.tab.c"
    break;

  case 154:
#line 691 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_BITNEG, (yyvsp[0].expr)); }
#line 2869 "parser.tab.c"
    break;

  case 155:
#line 692 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_LOGNEG, (yyvsp[0].expr)); }
#line 2875 "parser.tab.c"
    break;

  case 156:
#line 697 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2881 "parser.tab.c"
    break;

  case 157:
#line 699 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_POSTINC, (yyvsp[-1].expr)); }
#line 2887 "parser.tab.c"
    break;

  case 158:
#line 701 "parser.y"
    { (yyval.expr) = new_unary_expression(ctx, EXPR_POSTDEC, (yyvsp[-1].expr)); }
#line 2893 "parser.tab.c"
    break;

  case 159:
#line 706 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2899 "parser.tab.c"
    break;

  case 160:
#line 707 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2905 "parser.tab.c"
    break;

  case 161:
#line 711 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2911 "parser.tab.c"
    break;

  case 162:
#line 712 "parser.y"
    { (yyval.expr) = new_new_expression(ctx, (yyvsp[0].expr), NULL); }
#line 2917 "parser.tab.c"
    break;

  case 163:
#line 716 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2923 "parser.tab.c"
    break;

  case 164:
#line 717 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 2929 "parser.tab.c"
    break;

  case 165:
#line 719 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ARRAY, (yyvsp[-3].expr), (yyvsp[-1].expr)); }
#line 2935 "parser.tab.c"
    break;

  case 166:
#line 721 "parser.y"
    { (yyval.expr) = new_member_expression(ctx, (yyvsp[-2].expr), (yyvsp[0].identifier)); }
#line 2941 "parser.tab.c"
    break;

  case 167:
#line 723 "parser.y"
    { (yyval.expr) = new_new_expression(ctx, (yyvsp[-1].expr), (yyvsp[0].argument_list)); }
#line 2947 "parser.tab.c"
    break;

  case 168:
#line 728 "parser.y"
    { (yyval.expr) = new_call_expression(ctx, (yyvsp[-1].expr), (yyvsp[0].argument_list)); }
#line 2953 "parser.tab.c"
    break;

  case 169:
#line 730 "parser.y"
    { (yyval.expr) = new_call_expression(ctx, (yyvsp[-1].expr), (yyvsp[0].argument_list)); }
#line 2959 "parser.tab.c"
    break;

  case 170:
#line 732 "parser.y"
    { (yyval.expr) = new_binary_expression(ctx, EXPR_ARRAY, (yyvsp[-3].expr), (yyvsp[-1].expr)); }
#line 2965 "parser.tab.c"
    break;

  case 171:
#line 734 "parser.y"
    { (yyval.expr) = new_member_expression(ctx, (yyvsp[-2].expr), (yyvsp[0].identifier)); }
#line 2971 "parser.tab.c"
    break;

  case 172:
#line 738 "parser.y"
    { (yyval.argument_list) = NULL; }
#line 2977 "parser.tab.c"
    break;

  case 173:
#line 739 "parser.y"
    { (yyval.argument_list) = (yyvsp[-1].argument_list); }
#line 2983 "parser.tab.c"
    break;

  case 174:
#line 743 "parser.y"
    { (yyval.argument_list) = new_argument_list(ctx, (yyvsp[0].expr)); }
#line 2989 "parser.tab.c"
    break;

  case 175:
#line 745 "parser.y"
    { (yyval.argument_list) = argument_list_add(ctx, (yyvsp[-2].argument_list), (yyvsp[0].expr)); }
#line 2995 "parser.tab.c"
    break;

  case 176:
#line 749 "parser.y"
    { (yyval.expr) = new_expression(ctx, EXPR_THIS, 0); }
#line 3001 "parser.tab.c"
    break;

  case 177:
#line 750 "parser.y"
    { (yyval.expr) = new_identifier_expression(ctx, (yyvsp[0].identifier)); }
#line 3007 "parser.tab.c"
    break;

  case 178:
#line 751 "parser.y"
    { (yyval.expr) = new_literal_expression(ctx, (yyvsp[0].literal)); }
#line 3013 "parser.tab.c"
    break;

  case 179:
#line 752 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 3019 "parser.tab.c"
    break;

  case 180:
#line 753 "parser.y"
    { (yyval.expr) = (yyvsp[0].expr); }
#line 3025 "parser.tab.c"
    break;

  case 181:
#line 754 "parser.y"
    { (yyval.expr) = (yyvsp[-1].expr); }
#line 3031 "parser.tab.c"
    break;

  case 182:
#line 758 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, NULL, 0); }
#line 3037 "parser.tab.c"
    break;

  case 183:
#line 759 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, NULL, (yyvsp[-1].ival)+1); }
#line 3043 "parser.tab.c"
    break;

  case 184:
#line 760 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, (yyvsp[-1].element_list), 0); }
#line 3049 "parser.tab.c"
    break;

  case 185:
#line 762 "parser.y"
    { (yyval.expr) = new_array_literal_expression(ctx, (yyvsp[-3].element_list), (yyvsp[-1].ival)+1); }
#line 3055 "parser.tab.c"
    break;

  case 186:
#line 767 "parser.y"
    { (yyval.element_list) = new_element_list(ctx, (yyvsp[-1].ival), (yyvsp[0].expr)); }
#line 3061 "parser.tab.c"
    break;

  case 187:
#line 769 "parser.y"
    { (yyval.element_list) = element_list_add(ctx, (yyvsp[-3].element_list), (yyvsp[-1].ival), (yyvsp[0].expr)); }
#line 3067 "parser.tab.c"
    break;

  case 188:
#line 773 "parser.y"
    { (yyval.ival) = 1; }
#line 3073 "parser.tab.c"
    break;

  case 189:
#line 774 "parser.y"
    { (yyval.ival) = (yyvsp[-1].ival) + 1; }
#line 3079 "parser.tab.c"
    break;

  case 190:
#line 778 "parser.y"
    { (yyval.ival) = 0; }
#line 3085 "parser.tab.c"
    break;

  case 191:
#line 779 "parser.y"
    { (yyval.ival) = (yyvsp[0].ival); }
#line 3091 "parser.tab.c"
    break;

  case 192:
#line 783 "parser.y"
    { (yyval.expr) = new_prop_and_value_expression(ctx, NULL); }
#line 3097 "parser.tab.c"
    break;

  case 193:
#line 785 "parser.y"
    { (yyval.expr) = new_prop_and_value_expression(ctx, (yyvsp[-1].property_list)); }
#line 3103 "parser.tab.c"
    break;

  case 194:
#line 787 "parser.y"
    {
            if(ctx->script->version < 2) {
                WARN("Trailing comma in object literal is illegal in legacy mode.\n");
                YYABORT;
            }
            (yyval.expr) = new_prop_and_value_expression(ctx, (yyvsp[-2].property_list));
        }
#line 3115 "parser.tab.c"
    break;

  case 195:
#line 797 "parser.y"
    { (yyval.property_list) = new_property_list(ctx, (yyvsp[0].property_definition)); }
#line 3121 "parser.tab.c"
    break;

  case 196:
#line 799 "parser.y"
    { (yyval.property_list) = property_list_add(ctx, (yyvsp[-2].property_list), (yyvsp[0].property_definition)); }
#line 3127 "parser.tab.c"
    break;

  case 197:
#line 804 "parser.y"
    { (yyval.property_definition) = new_property_definition(ctx, PROPERTY_DEFINITION_VALUE, (yyvsp[-2].literal), (yyvsp[0].expr)); }
#line 3133 "parser.tab.c"
    break;

  case 198:
#line 806 "parser.y"
    { (yyval.property_definition) = new_property_definition(ctx, PROPERTY_DEFINITION_GETTER, (yyvsp[-1].literal), (yyvsp[0].expr)); }
#line 3139 "parser.tab.c"
    break;

  case 199:
#line 808 "parser.y"
    { (yyval.property_definition) = new_property_definition(ctx, PROPERTY_DEFINITION_SETTER, (yyvsp[-1].literal), (yyvsp[0].expr)); }
#line 3145 "parser.tab.c"
    break;

  case 200:
#line 812 "parser.y"
    { (yyval.expr) = new_function_expression(ctx, NULL, (yyvsp[-4].parameter_list), (yyvsp[-1].source_elements), NULL, (yyvsp[-5].srcptr), (yyvsp[0].srcptr)-(yyvsp[-5].srcptr)); }
#line 3151 "parser.tab.c"
    break;

  case 201:
#line 816 "parser.y"
    { (yyval.literal) = new_string_literal(ctx, compiler_alloc_string_len(ctx->compiler, (yyvsp[0].identifier), lstrlenW((yyvsp[0].identifier)))); }
#line 3157 "parser.tab.c"
    break;

  case 202:
#line 817 "parser.y"
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[0].str)); }
#line 3163 "parser.tab.c"
    break;

  case 203:
#line 818 "parser.y"
    { (yyval.literal) = (yyvsp[0].literal); }
#line 3169 "parser.tab.c"
    break;

  case 204:
#line 822 "parser.y"
    { (yyval.identifier) = NULL; }
#line 3175 "parser.tab.c"
    break;

  case 205:
#line 823 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3181 "parser.tab.c"
    break;

  case 206:
#line 827 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3187 "parser.tab.c"
    break;

  case 207:
#line 829 "parser.y"
    {
            if(ctx->script->version < SCRIPTLANGUAGEVERSION_ES5) {
                WARN("%s keyword used as an identifier in legacy mode.\n",
                     debugstr_w((yyvsp[0].identifier)));
                YYABORT;
            }
            (yyval.identifier) = (yyvsp[0].identifier);
        }
#line 3200 "parser.tab.c"
    break;

  case 208:
#line 839 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3206 "parser.tab.c"
    break;

  case 209:
#line 840 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3212 "parser.tab.c"
    break;

  case 210:
#line 841 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3218 "parser.tab.c"
    break;

  case 211:
#line 842 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3224 "parser.tab.c"
    break;

  case 212:
#line 843 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3230 "parser.tab.c"
    break;

  case 213:
#line 844 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3236 "parser.tab.c"
    break;

  case 214:
#line 845 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3242 "parser.tab.c"
    break;

  case 215:
#line 846 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3248 "parser.tab.c"
    break;

  case 216:
#line 847 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3254 "parser.tab.c"
    break;

  case 217:
#line 848 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3260 "parser.tab.c"
    break;

  case 218:
#line 849 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3266 "parser.tab.c"
    break;

  case 219:
#line 850 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3272 "parser.tab.c"
    break;

  case 220:
#line 851 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3278 "parser.tab.c"
    break;

  case 221:
#line 852 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3284 "parser.tab.c"
    break;

  case 222:
#line 853 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3290 "parser.tab.c"
    break;

  case 223:
#line 854 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3296 "parser.tab.c"
    break;

  case 224:
#line 855 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3302 "parser.tab.c"
    break;

  case 225:
#line 856 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3308 "parser.tab.c"
    break;

  case 226:
#line 857 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3314 "parser.tab.c"
    break;

  case 227:
#line 858 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3320 "parser.tab.c"
    break;

  case 228:
#line 859 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3326 "parser.tab.c"
    break;

  case 229:
#line 860 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3332 "parser.tab.c"
    break;

  case 230:
#line 861 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3338 "parser.tab.c"
    break;

  case 231:
#line 862 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3344 "parser.tab.c"
    break;

  case 232:
#line 863 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3350 "parser.tab.c"
    break;

  case 233:
#line 864 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3356 "parser.tab.c"
    break;

  case 234:
#line 865 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3362 "parser.tab.c"
    break;

  case 235:
#line 866 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3368 "parser.tab.c"
    break;

  case 236:
#line 867 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3374 "parser.tab.c"
    break;

  case 237:
#line 868 "parser.y"
    { (yyval.identifier) = (yyvsp[0].identifier); }
#line 3380 "parser.tab.c"
    break;

  case 238:
#line 872 "parser.y"
    { (yyval.literal) = new_null_literal(ctx); }
#line 3386 "parser.tab.c"
    break;

  case 239:
#line 873 "parser.y"
    { (yyval.literal) = (yyvsp[0].literal); }
#line 3392 "parser.tab.c"
    break;

  case 240:
#line 874 "parser.y"
    { (yyval.literal) = (yyvsp[0].literal); }
#line 3398 "parser.tab.c"
    break;

  case 241:
#line 875 "parser.y"
    { (yyval.literal) = new_string_literal(ctx, (yyvsp[0].str)); }
#line 3404 "parser.tab.c"
    break;

  case 242:
#line 876 "parser.y"
    { (yyval.literal) = parse_regexp(ctx);
                                  if(!(yyval.literal)) YYABORT; }
#line 3411 "parser.tab.c"
    break;

  case 243:
#line 878 "parser.y"
    { (yyval.literal) = parse_regexp(ctx);
                                  if(!(yyval.literal)) YYABORT; }
#line 3418 "parser.tab.c"
    break;

  case 244:
#line 883 "parser.y"
    { (yyval.literal) = new_boolean_literal(ctx, VARIANT_TRUE); }
#line 3424 "parser.tab.c"
    break;

  case 245:
#line 884 "parser.y"
    { (yyval.literal) = new_boolean_literal(ctx, VARIANT_FALSE); }
#line 3430 "parser.tab.c"
    break;

  case 246:
#line 885 "parser.y"
    { (yyval.literal) = (yyvsp[0].literal); }
#line 3436 "parser.tab.c"
    break;

  case 248:
#line 889 "parser.y"
    { if(!allow_auto_semicolon(ctx)) {YYABORT;} }
#line 3442 "parser.tab.c"
    break;

  case 249:
#line 892 "parser.y"
    { (yyval.srcptr) = ctx->ptr; }
#line 3448 "parser.tab.c"
    break;

  case 250:
#line 893 "parser.y"
    { set_error(ctx, JS_E_MISSING_LBRACKET); YYABORT; }
#line 3454 "parser.tab.c"
    break;

  case 252:
#line 897 "parser.y"
    { set_error(ctx, JS_E_MISSING_RBRACKET); YYABORT; }
#line 3460 "parser.tab.c"
    break;

  case 254:
#line 901 "parser.y"
    { set_error(ctx, JS_E_MISSING_SEMICOLON); YYABORT; }
#line 3466 "parser.tab.c"
    break;


#line 3470 "parser.tab.c"

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
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

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
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

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


/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
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
#line 903 "parser.y"


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

static literal_t *new_string_literal(parser_ctx_t *ctx, jsstr_t *str)
{
    literal_t *ret = parser_alloc(ctx, sizeof(literal_t));

    ret->type = LT_STRING;
    ret->u.str = str;

    return ret;
}

static literal_t *new_null_literal(parser_ctx_t *ctx)
{
    literal_t *ret = parser_alloc(ctx, sizeof(literal_t));

    ret->type = LT_NULL;

    return ret;
}

static property_definition_t *new_property_definition(parser_ctx_t *ctx, property_definition_type_t type,
                                                      literal_t *name, expression_t *value)
{
    property_definition_t *ret = parser_alloc(ctx, sizeof(property_definition_t));

    ret->type = type;
    ret->name = name;
    ret->value = value;
    ret->next = NULL;

    return ret;
}

static property_list_t *new_property_list(parser_ctx_t *ctx, property_definition_t *prop)
{
    property_list_t *ret = parser_alloc_tmp(ctx, sizeof(property_list_t));
    ret->head = ret->tail = prop;
    return ret;
}

static property_list_t *property_list_add(parser_ctx_t *ctx, property_list_t *list, property_definition_t *prop)
{
    list->tail = list->tail->next = prop;
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

HRESULT script_parse(script_ctx_t *ctx, struct _compiler_ctx_t *compiler, const WCHAR *code, const WCHAR *delimiter, BOOL from_eval,
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
    parser_ctx->is_html = delimiter && !wcsicmp(delimiter, html_tagW);

    parser_ctx->begin = parser_ctx->ptr = code;
    parser_ctx->end = parser_ctx->begin + lstrlenW(parser_ctx->begin);

    script_addref(ctx);
    parser_ctx->script = ctx;

    mark = heap_pool_mark(&ctx->tmp_heap);
    heap_pool_init(&parser_ctx->heap);

    parser_ctx->compiler = compiler;
    parser_parse(parser_ctx);
    parser_ctx->compiler = NULL;

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
