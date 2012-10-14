/* A Bison parser, made by GNU Bison 2.4.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006,
   2009, 2010 Free Software Foundation, Inc.
   
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
#define YYBISON_VERSION "2.4.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 1



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 1 "program/program_parse.y"

/*
 * Copyright © 2009 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main/mtypes.h"
#include "main/imports.h"
#include "program/program.h"
#include "program/prog_parameter.h"
#include "program/prog_parameter_layout.h"
#include "program/prog_statevars.h"
#include "program/prog_instruction.h"

#include "program/symbol_table.h"
#include "program/program_parser.h"

extern void *yy_scan_string(char *);
extern void yy_delete_buffer(void *);

static struct asm_symbol *declare_variable(struct asm_parser_state *state,
    char *name, enum asm_type t, struct YYLTYPE *locp);

static int add_state_reference(struct gl_program_parameter_list *param_list,
    const gl_state_index tokens[STATE_LENGTH]);

static int initialize_symbol_from_state(struct gl_program *prog,
    struct asm_symbol *param_var, const gl_state_index tokens[STATE_LENGTH]);

static int initialize_symbol_from_param(struct gl_program *prog,
    struct asm_symbol *param_var, const gl_state_index tokens[STATE_LENGTH]);

static int initialize_symbol_from_const(struct gl_program *prog,
    struct asm_symbol *param_var, const struct asm_vector *vec,
    GLboolean allowSwizzle);

static int yyparse(struct asm_parser_state *state);

static char *make_error_string(const char *fmt, ...);

static void yyerror(struct YYLTYPE *locp, struct asm_parser_state *state,
    const char *s);

static int validate_inputs(struct YYLTYPE *locp,
    struct asm_parser_state *state);

static void init_dst_reg(struct prog_dst_register *r);

static void set_dst_reg(struct prog_dst_register *r,
                        gl_register_file file, GLint index);

static void init_src_reg(struct asm_src_register *r);

static void set_src_reg(struct asm_src_register *r,
                        gl_register_file file, GLint index);

static void set_src_reg_swz(struct asm_src_register *r,
                            gl_register_file file, GLint index, GLuint swizzle);

static void asm_instruction_set_operands(struct asm_instruction *inst,
    const struct prog_dst_register *dst, const struct asm_src_register *src0,
    const struct asm_src_register *src1, const struct asm_src_register *src2);

static struct asm_instruction *asm_instruction_ctor(gl_inst_opcode op,
    const struct prog_dst_register *dst, const struct asm_src_register *src0,
    const struct asm_src_register *src1, const struct asm_src_register *src2);

static struct asm_instruction *asm_instruction_copy_ctor(
    const struct prog_instruction *base, const struct prog_dst_register *dst,
    const struct asm_src_register *src0, const struct asm_src_register *src1,
    const struct asm_src_register *src2);

#ifndef FALSE
#define FALSE 0
#define TRUE (!FALSE)
#endif

#define YYLLOC_DEFAULT(Current, Rhs, N)					\
   do {									\
      if (YYID(N)) {							\
	 (Current).first_line = YYRHSLOC(Rhs, 1).first_line;		\
	 (Current).first_column = YYRHSLOC(Rhs, 1).first_column;	\
	 (Current).position = YYRHSLOC(Rhs, 1).position;		\
	 (Current).last_line = YYRHSLOC(Rhs, N).last_line;		\
	 (Current).last_column = YYRHSLOC(Rhs, N).last_column;		\
      } else {								\
	 (Current).first_line = YYRHSLOC(Rhs, 0).last_line;		\
	 (Current).last_line = (Current).first_line;			\
	 (Current).first_column = YYRHSLOC(Rhs, 0).last_column;		\
	 (Current).last_column = (Current).first_column;		\
	 (Current).position = YYRHSLOC(Rhs, 0).position			\
	    + (Current).first_column;					\
      }									\
   } while(YYID(0))

#define YYLEX_PARAM state->scanner


/* Line 189 of yacc.c  */
#line 192 "program/program_parse.tab.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
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
     ARBvp_10 = 258,
     ARBfp_10 = 259,
     ADDRESS = 260,
     ALIAS = 261,
     ATTRIB = 262,
     OPTION = 263,
     OUTPUT = 264,
     PARAM = 265,
     TEMP = 266,
     END = 267,
     BIN_OP = 268,
     BINSC_OP = 269,
     SAMPLE_OP = 270,
     SCALAR_OP = 271,
     TRI_OP = 272,
     VECTOR_OP = 273,
     ARL = 274,
     KIL = 275,
     SWZ = 276,
     TXD_OP = 277,
     INTEGER = 278,
     REAL = 279,
     AMBIENT = 280,
     ATTENUATION = 281,
     BACK = 282,
     CLIP = 283,
     COLOR = 284,
     DEPTH = 285,
     DIFFUSE = 286,
     DIRECTION = 287,
     EMISSION = 288,
     ENV = 289,
     EYE = 290,
     FOG = 291,
     FOGCOORD = 292,
     FRAGMENT = 293,
     FRONT = 294,
     HALF = 295,
     INVERSE = 296,
     INVTRANS = 297,
     LIGHT = 298,
     LIGHTMODEL = 299,
     LIGHTPROD = 300,
     LOCAL = 301,
     MATERIAL = 302,
     MAT_PROGRAM = 303,
     MATRIX = 304,
     MATRIXINDEX = 305,
     MODELVIEW = 306,
     MVP = 307,
     NORMAL = 308,
     OBJECT = 309,
     PALETTE = 310,
     PARAMS = 311,
     PLANE = 312,
     POINT_TOK = 313,
     POINTSIZE = 314,
     POSITION = 315,
     PRIMARY = 316,
     PROGRAM = 317,
     PROJECTION = 318,
     RANGE = 319,
     RESULT = 320,
     ROW = 321,
     SCENECOLOR = 322,
     SECONDARY = 323,
     SHININESS = 324,
     SIZE_TOK = 325,
     SPECULAR = 326,
     SPOT = 327,
     STATE = 328,
     TEXCOORD = 329,
     TEXENV = 330,
     TEXGEN = 331,
     TEXGEN_Q = 332,
     TEXGEN_R = 333,
     TEXGEN_S = 334,
     TEXGEN_T = 335,
     TEXTURE = 336,
     TRANSPOSE = 337,
     TEXTURE_UNIT = 338,
     TEX_1D = 339,
     TEX_2D = 340,
     TEX_3D = 341,
     TEX_CUBE = 342,
     TEX_RECT = 343,
     TEX_SHADOW1D = 344,
     TEX_SHADOW2D = 345,
     TEX_SHADOWRECT = 346,
     TEX_ARRAY1D = 347,
     TEX_ARRAY2D = 348,
     TEX_ARRAYSHADOW1D = 349,
     TEX_ARRAYSHADOW2D = 350,
     VERTEX = 351,
     VTXATTRIB = 352,
     WEIGHT = 353,
     IDENTIFIER = 354,
     USED_IDENTIFIER = 355,
     MASK4 = 356,
     MASK3 = 357,
     MASK2 = 358,
     MASK1 = 359,
     SWIZZLE = 360,
     DOT_DOT = 361,
     DOT = 362
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 126 "program/program_parse.y"

   struct asm_instruction *inst;
   struct asm_symbol *sym;
   struct asm_symbol temp_sym;
   struct asm_swizzle_mask swiz_mask;
   struct asm_src_register src_reg;
   struct prog_dst_register dst_reg;
   struct prog_instruction temp_inst;
   char *string;
   unsigned result;
   unsigned attrib;
   int integer;
   float real;
   gl_state_index state[STATE_LENGTH];
   int negate;
   struct asm_vector vector;
   gl_inst_opcode opcode;

   struct {
      unsigned swz;
      unsigned rgba_valid:1;
      unsigned xyzw_valid:1;
      unsigned negate:1;
   } ext_swizzle;



/* Line 214 of yacc.c  */
#line 363 "program/program_parse.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


/* Copy the second part of user declarations.  */

/* Line 264 of yacc.c  */
#line 271 "program/program_parse.y"

extern int yylex(YYSTYPE *yylval_param, YYLTYPE *yylloc_param,
    void *yyscanner);


/* Line 264 of yacc.c  */
#line 394 "program/program_parse.tab.c"

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
	 || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	     && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

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
#define YYFINAL  5
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   402

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  120
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  143
/* YYNRULES -- Number of rules.  */
#define YYNRULES  283
/* YYNRULES -- Number of states.  */
#define YYNSTATES  478

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   362

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     115,   116,     2,   113,   109,   114,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,   108,
       2,   117,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   111,     2,   112,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   118,   110,   119,     2,     2,     2,     2,
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
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     8,    10,    12,    15,    16,    20,    23,
      24,    27,    30,    32,    34,    36,    38,    40,    42,    44,
      46,    48,    50,    52,    54,    59,    64,    69,    76,    83,
      92,   101,   104,   107,   120,   123,   125,   127,   129,   131,
     133,   135,   137,   139,   141,   143,   145,   147,   154,   157,
     162,   165,   167,   171,   177,   181,   184,   192,   195,   197,
     199,   201,   203,   208,   210,   212,   214,   216,   218,   220,
     222,   226,   227,   230,   233,   235,   237,   239,   241,   243,
     245,   247,   249,   251,   252,   254,   256,   258,   260,   261,
     265,   269,   270,   273,   276,   278,   280,   282,   284,   286,
     288,   290,   292,   297,   300,   303,   305,   308,   310,   313,
     315,   318,   323,   328,   330,   331,   335,   337,   339,   342,
     344,   347,   349,   351,   355,   362,   363,   365,   368,   373,
     375,   379,   381,   383,   385,   387,   389,   391,   393,   395,
     397,   399,   402,   405,   408,   411,   414,   417,   420,   423,
     426,   429,   432,   435,   439,   441,   443,   445,   451,   453,
     455,   457,   460,   462,   464,   467,   469,   472,   479,   481,
     485,   487,   489,   491,   493,   495,   500,   502,   504,   506,
     508,   510,   512,   515,   517,   519,   525,   527,   530,   532,
     534,   540,   543,   544,   551,   555,   556,   558,   560,   562,
     564,   566,   569,   571,   573,   576,   581,   586,   587,   591,
     593,   595,   597,   600,   602,   604,   606,   608,   614,   616,
     620,   626,   632,   634,   638,   644,   646,   648,   650,   652,
     654,   656,   658,   660,   662,   666,   672,   680,   690,   693,
     696,   698,   700,   701,   702,   707,   709,   710,   711,   715,
     719,   721,   727,   730,   733,   736,   739,   743,   746,   750,
     751,   755,   757,   759,   760,   762,   764,   765,   767,   769,
     770,   772,   774,   775,   779,   780,   784,   785,   789,   791,
     793,   795,   800,   802
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     121,     0,    -1,   122,   123,   125,    12,    -1,     3,    -1,
       4,    -1,   123,   124,    -1,    -1,     8,   262,   108,    -1,
     125,   126,    -1,    -1,   127,   108,    -1,   170,   108,    -1,
     128,    -1,   129,    -1,   130,    -1,   131,    -1,   132,    -1,
     133,    -1,   134,    -1,   135,    -1,   141,    -1,   136,    -1,
     137,    -1,   138,    -1,    19,   146,   109,   142,    -1,    18,
     145,   109,   144,    -1,    16,   145,   109,   142,    -1,    14,
     145,   109,   142,   109,   142,    -1,    13,   145,   109,   144,
     109,   144,    -1,    17,   145,   109,   144,   109,   144,   109,
     144,    -1,    15,   145,   109,   144,   109,   139,   109,   140,
      -1,    20,   144,    -1,    20,   166,    -1,    22,   145,   109,
     144,   109,   144,   109,   144,   109,   139,   109,   140,    -1,
      83,   256,    -1,    84,    -1,    85,    -1,    86,    -1,    87,
      -1,    88,    -1,    89,    -1,    90,    -1,    91,    -1,    92,
      -1,    93,    -1,    94,    -1,    95,    -1,    21,   145,   109,
     150,   109,   147,    -1,   241,   143,    -1,   241,   110,   143,
     110,    -1,   150,   162,    -1,   238,    -1,   241,   150,   163,
      -1,   241,   110,   150,   163,   110,    -1,   151,   164,   165,
      -1,   159,   161,    -1,   148,   109,   148,   109,   148,   109,
     148,    -1,   241,   149,    -1,    23,    -1,   262,    -1,   100,
      -1,   172,    -1,   152,   111,   153,   112,    -1,   186,    -1,
     249,    -1,   100,    -1,   100,    -1,   154,    -1,   155,    -1,
      23,    -1,   159,   160,   156,    -1,    -1,   113,   157,    -1,
     114,   158,    -1,    23,    -1,    23,    -1,   100,    -1,   104,
      -1,   104,    -1,   104,    -1,   104,    -1,   101,    -1,   105,
      -1,    -1,   101,    -1,   102,    -1,   103,    -1,   104,    -1,
      -1,   115,   166,   116,    -1,   115,   167,   116,    -1,    -1,
     168,   163,    -1,   169,   163,    -1,    99,    -1,   100,    -1,
     171,    -1,   178,    -1,   242,    -1,   245,    -1,   248,    -1,
     261,    -1,     7,    99,   117,   172,    -1,    96,   173,    -1,
      38,   177,    -1,    60,    -1,    98,   175,    -1,    53,    -1,
      29,   254,    -1,    37,    -1,    74,   255,    -1,    50,   111,
     176,   112,    -1,    97,   111,   174,   112,    -1,    23,    -1,
      -1,   111,   176,   112,    -1,    23,    -1,    60,    -1,    29,
     254,    -1,    37,    -1,    74,   255,    -1,   179,    -1,   180,
      -1,    10,    99,   182,    -1,    10,    99,   111,   181,   112,
     183,    -1,    -1,    23,    -1,   117,   185,    -1,   117,   118,
     184,   119,    -1,   187,    -1,   184,   109,   187,    -1,   189,
      -1,   225,    -1,   235,    -1,   189,    -1,   225,    -1,   236,
      -1,   188,    -1,   226,    -1,   235,    -1,   189,    -1,    73,
     213,    -1,    73,   190,    -1,    73,   192,    -1,    73,   195,
      -1,    73,   197,    -1,    73,   203,    -1,    73,   199,    -1,
      73,   206,    -1,    73,   208,    -1,    73,   210,    -1,    73,
     212,    -1,    73,   224,    -1,    47,   253,   191,    -1,   201,
      -1,    33,    -1,    69,    -1,    43,   111,   202,   112,   193,
      -1,   201,    -1,    60,    -1,    26,    -1,    72,   194,    -1,
      40,    -1,    32,    -1,    44,   196,    -1,    25,    -1,   253,
      67,    -1,    45,   111,   202,   112,   253,   198,    -1,   201,
      -1,    75,   257,   200,    -1,    29,    -1,    25,    -1,    31,
      -1,    71,    -1,    23,    -1,    76,   255,   204,   205,    -1,
      35,    -1,    54,    -1,    79,    -1,    80,    -1,    78,    -1,
      77,    -1,    36,   207,    -1,    29,    -1,    56,    -1,    28,
     111,   209,   112,    57,    -1,    23,    -1,    58,   211,    -1,
      70,    -1,    26,    -1,   215,    66,   111,   218,   112,    -1,
     215,   214,    -1,    -1,    66,   111,   218,   106,   218,   112,
      -1,    49,   219,   216,    -1,    -1,   217,    -1,    41,    -1,
      82,    -1,    42,    -1,    23,    -1,    51,   220,    -1,    63,
      -1,    52,    -1,    81,   255,    -1,    55,   111,   222,   112,
      -1,    48,   111,   223,   112,    -1,    -1,   111,   221,   112,
      -1,    23,    -1,    23,    -1,    23,    -1,    30,    64,    -1,
     229,    -1,   232,    -1,   227,    -1,   230,    -1,    62,    34,
     111,   228,   112,    -1,   233,    -1,   233,   106,   233,    -1,
      62,    34,   111,   233,   112,    -1,    62,    46,   111,   231,
     112,    -1,   234,    -1,   234,   106,   234,    -1,    62,    46,
     111,   234,   112,    -1,    23,    -1,    23,    -1,   237,    -1,
     239,    -1,   238,    -1,   239,    -1,   240,    -1,    24,    -1,
      23,    -1,   118,   240,   119,    -1,   118,   240,   109,   240,
     119,    -1,   118,   240,   109,   240,   109,   240,   119,    -1,
     118,   240,   109,   240,   109,   240,   109,   240,   119,    -1,
     241,    24,    -1,   241,    23,    -1,   113,    -1,   114,    -1,
      -1,    -1,   244,    11,   243,   247,    -1,   262,    -1,    -1,
      -1,     5,   246,   247,    -1,   247,   109,    99,    -1,    99,
      -1,   244,     9,    99,   117,   249,    -1,    65,    60,    -1,
      65,    37,    -1,    65,   250,    -1,    65,    59,    -1,    65,
      74,   255,    -1,    65,    30,    -1,    29,   251,   252,    -1,
      -1,   111,    23,   112,    -1,    39,    -1,    27,    -1,    -1,
      61,    -1,    68,    -1,    -1,    39,    -1,    27,    -1,    -1,
      61,    -1,    68,    -1,    -1,   111,   258,   112,    -1,    -1,
     111,   259,   112,    -1,    -1,   111,   260,   112,    -1,    23,
      -1,    23,    -1,    23,    -1,     6,    99,   117,   100,    -1,
      99,    -1,   100,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   278,   278,   281,   289,   301,   302,   305,   329,   330,
     333,   348,   351,   356,   363,   364,   365,   366,   367,   368,
     369,   372,   373,   374,   377,   383,   389,   395,   402,   408,
     415,   459,   464,   474,   518,   524,   525,   526,   527,   528,
     529,   530,   531,   532,   533,   534,   535,   538,   550,   558,
     575,   582,   601,   612,   632,   657,   664,   697,   704,   719,
     774,   817,   826,   848,   858,   862,   891,   910,   910,   912,
     919,   931,   932,   933,   936,   950,   964,   984,   995,  1007,
    1009,  1010,  1011,  1012,  1015,  1015,  1015,  1015,  1016,  1019,
    1023,  1028,  1035,  1042,  1049,  1072,  1095,  1096,  1097,  1098,
    1099,  1100,  1103,  1122,  1126,  1132,  1136,  1140,  1144,  1153,
    1162,  1166,  1171,  1177,  1188,  1188,  1189,  1191,  1195,  1199,
    1203,  1209,  1209,  1211,  1229,  1255,  1258,  1273,  1279,  1285,
    1286,  1293,  1299,  1305,  1313,  1319,  1325,  1333,  1339,  1345,
    1353,  1354,  1357,  1358,  1359,  1360,  1361,  1362,  1363,  1364,
    1365,  1366,  1367,  1370,  1379,  1383,  1387,  1393,  1402,  1406,
    1410,  1419,  1423,  1429,  1435,  1442,  1447,  1455,  1465,  1467,
    1475,  1481,  1485,  1489,  1495,  1506,  1515,  1519,  1524,  1528,
    1532,  1536,  1542,  1549,  1553,  1559,  1567,  1578,  1585,  1589,
    1595,  1605,  1616,  1620,  1638,  1647,  1650,  1656,  1660,  1664,
    1670,  1681,  1686,  1691,  1696,  1701,  1706,  1714,  1717,  1722,
    1735,  1743,  1754,  1762,  1762,  1764,  1764,  1766,  1776,  1781,
    1788,  1798,  1807,  1812,  1819,  1829,  1839,  1851,  1851,  1852,
    1852,  1854,  1864,  1872,  1882,  1890,  1898,  1907,  1918,  1922,
    1928,  1929,  1930,  1933,  1933,  1936,  1971,  1975,  1975,  1978,
    1985,  1994,  2008,  2017,  2026,  2030,  2039,  2048,  2059,  2066,
    2076,  2104,  2113,  2125,  2128,  2137,  2148,  2149,  2150,  2153,
    2154,  2155,  2158,  2159,  2162,  2163,  2166,  2167,  2170,  2181,
    2192,  2203,  2229,  2230
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ARBvp_10", "ARBfp_10", "ADDRESS",
  "ALIAS", "ATTRIB", "OPTION", "OUTPUT", "PARAM", "TEMP", "END", "BIN_OP",
  "BINSC_OP", "SAMPLE_OP", "SCALAR_OP", "TRI_OP", "VECTOR_OP", "ARL",
  "KIL", "SWZ", "TXD_OP", "INTEGER", "REAL", "AMBIENT", "ATTENUATION",
  "BACK", "CLIP", "COLOR", "DEPTH", "DIFFUSE", "DIRECTION", "EMISSION",
  "ENV", "EYE", "FOG", "FOGCOORD", "FRAGMENT", "FRONT", "HALF", "INVERSE",
  "INVTRANS", "LIGHT", "LIGHTMODEL", "LIGHTPROD", "LOCAL", "MATERIAL",
  "MAT_PROGRAM", "MATRIX", "MATRIXINDEX", "MODELVIEW", "MVP", "NORMAL",
  "OBJECT", "PALETTE", "PARAMS", "PLANE", "POINT_TOK", "POINTSIZE",
  "POSITION", "PRIMARY", "PROGRAM", "PROJECTION", "RANGE", "RESULT", "ROW",
  "SCENECOLOR", "SECONDARY", "SHININESS", "SIZE_TOK", "SPECULAR", "SPOT",
  "STATE", "TEXCOORD", "TEXENV", "TEXGEN", "TEXGEN_Q", "TEXGEN_R",
  "TEXGEN_S", "TEXGEN_T", "TEXTURE", "TRANSPOSE", "TEXTURE_UNIT", "TEX_1D",
  "TEX_2D", "TEX_3D", "TEX_CUBE", "TEX_RECT", "TEX_SHADOW1D",
  "TEX_SHADOW2D", "TEX_SHADOWRECT", "TEX_ARRAY1D", "TEX_ARRAY2D",
  "TEX_ARRAYSHADOW1D", "TEX_ARRAYSHADOW2D", "VERTEX", "VTXATTRIB",
  "WEIGHT", "IDENTIFIER", "USED_IDENTIFIER", "MASK4", "MASK3", "MASK2",
  "MASK1", "SWIZZLE", "DOT_DOT", "DOT", "';'", "','", "'|'", "'['", "']'",
  "'+'", "'-'", "'('", "')'", "'='", "'{'", "'}'", "$accept", "program",
  "language", "optionSequence", "option", "statementSequence", "statement",
  "instruction", "ALU_instruction", "TexInstruction", "ARL_instruction",
  "VECTORop_instruction", "SCALARop_instruction", "BINSCop_instruction",
  "BINop_instruction", "TRIop_instruction", "SAMPLE_instruction",
  "KIL_instruction", "TXD_instruction", "texImageUnit", "texTarget",
  "SWZ_instruction", "scalarSrcReg", "scalarUse", "swizzleSrcReg",
  "maskedDstReg", "maskedAddrReg", "extendedSwizzle", "extSwizComp",
  "extSwizSel", "srcReg", "dstReg", "progParamArray", "progParamArrayMem",
  "progParamArrayAbs", "progParamArrayRel", "addrRegRelOffset",
  "addrRegPosOffset", "addrRegNegOffset", "addrReg", "addrComponent",
  "addrWriteMask", "scalarSuffix", "swizzleSuffix", "optionalMask",
  "optionalCcMask", "ccTest", "ccTest2", "ccMaskRule", "ccMaskRule2",
  "namingStatement", "ATTRIB_statement", "attribBinding", "vtxAttribItem",
  "vtxAttribNum", "vtxOptWeightNum", "vtxWeightNum", "fragAttribItem",
  "PARAM_statement", "PARAM_singleStmt", "PARAM_multipleStmt",
  "optArraySize", "paramSingleInit", "paramMultipleInit",
  "paramMultInitList", "paramSingleItemDecl", "paramSingleItemUse",
  "paramMultipleItem", "stateMultipleItem", "stateSingleItem",
  "stateMaterialItem", "stateMatProperty", "stateLightItem",
  "stateLightProperty", "stateSpotProperty", "stateLightModelItem",
  "stateLModProperty", "stateLightProdItem", "stateLProdProperty",
  "stateTexEnvItem", "stateTexEnvProperty", "ambDiffSpecProperty",
  "stateLightNumber", "stateTexGenItem", "stateTexGenType",
  "stateTexGenCoord", "stateFogItem", "stateFogProperty",
  "stateClipPlaneItem", "stateClipPlaneNum", "statePointItem",
  "statePointProperty", "stateMatrixRow", "stateMatrixRows",
  "optMatrixRows", "stateMatrixItem", "stateOptMatModifier",
  "stateMatModifier", "stateMatrixRowNum", "stateMatrixName",
  "stateOptModMatNum", "stateModMatNum", "statePaletteMatNum",
  "stateProgramMatNum", "stateDepthItem", "programSingleItem",
  "programMultipleItem", "progEnvParams", "progEnvParamNums",
  "progEnvParam", "progLocalParams", "progLocalParamNums",
  "progLocalParam", "progEnvParamNum", "progLocalParamNum",
  "paramConstDecl", "paramConstUse", "paramConstScalarDecl",
  "paramConstScalarUse", "paramConstVector", "signedFloatConstant",
  "optionalSign", "TEMP_statement", "@1", "optVarSize",
  "ADDRESS_statement", "@2", "varNameList", "OUTPUT_statement",
  "resultBinding", "resultColBinding", "optResultFaceType",
  "optResultColorType", "optFaceType", "optColorType",
  "optTexCoordUnitNum", "optTexImageUnitNum", "optLegacyTexUnitNum",
  "texCoordUnitNum", "texImageUnitNum", "legacyTexUnitNum",
  "ALIAS_statement", "string", 0
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
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,    59,    44,
     124,    91,    93,    43,    45,    40,    41,    61,   123,   125
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   120,   121,   122,   122,   123,   123,   124,   125,   125,
     126,   126,   127,   127,   128,   128,   128,   128,   128,   128,
     128,   129,   129,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   137,   138,   139,   140,   140,   140,   140,   140,
     140,   140,   140,   140,   140,   140,   140,   141,   142,   142,
     143,   143,   144,   144,   145,   146,   147,   148,   149,   149,
     150,   150,   150,   150,   151,   151,   152,   153,   153,   154,
     155,   156,   156,   156,   157,   158,   159,   160,   161,   162,
     163,   163,   163,   163,   164,   164,   164,   164,   164,   165,
     165,   165,   166,   167,   168,   169,   170,   170,   170,   170,
     170,   170,   171,   172,   172,   173,   173,   173,   173,   173,
     173,   173,   173,   174,   175,   175,   176,   177,   177,   177,
     177,   178,   178,   179,   180,   181,   181,   182,   183,   184,
     184,   185,   185,   185,   186,   186,   186,   187,   187,   187,
     188,   188,   189,   189,   189,   189,   189,   189,   189,   189,
     189,   189,   189,   190,   191,   191,   191,   192,   193,   193,
     193,   193,   193,   194,   195,   196,   196,   197,   198,   199,
     200,   201,   201,   201,   202,   203,   204,   204,   205,   205,
     205,   205,   206,   207,   207,   208,   209,   210,   211,   211,
     212,   213,   214,   214,   215,   216,   216,   217,   217,   217,
     218,   219,   219,   219,   219,   219,   219,   220,   220,   221,
     222,   223,   224,   225,   225,   226,   226,   227,   228,   228,
     229,   230,   231,   231,   232,   233,   234,   235,   235,   236,
     236,   237,   238,   238,   239,   239,   239,   239,   240,   240,
     241,   241,   241,   243,   242,   244,   244,   246,   245,   247,
     247,   248,   249,   249,   249,   249,   249,   249,   250,   251,
     251,   251,   251,   252,   252,   252,   253,   253,   253,   254,
     254,   254,   255,   255,   256,   256,   257,   257,   258,   259,
     260,   261,   262,   262
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     4,     1,     1,     2,     0,     3,     2,     0,
       2,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     4,     4,     4,     6,     6,     8,
       8,     2,     2,    12,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     6,     2,     4,
       2,     1,     3,     5,     3,     2,     7,     2,     1,     1,
       1,     1,     4,     1,     1,     1,     1,     1,     1,     1,
       3,     0,     2,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     0,     1,     1,     1,     1,     0,     3,
       3,     0,     2,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     4,     2,     2,     1,     2,     1,     2,     1,
       2,     4,     4,     1,     0,     3,     1,     1,     2,     1,
       2,     1,     1,     3,     6,     0,     1,     2,     4,     1,
       3,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     3,     1,     1,     1,     5,     1,     1,
       1,     2,     1,     1,     2,     1,     2,     6,     1,     3,
       1,     1,     1,     1,     1,     4,     1,     1,     1,     1,
       1,     1,     2,     1,     1,     5,     1,     2,     1,     1,
       5,     2,     0,     6,     3,     0,     1,     1,     1,     1,
       1,     2,     1,     1,     2,     4,     4,     0,     3,     1,
       1,     1,     2,     1,     1,     1,     1,     5,     1,     3,
       5,     5,     1,     3,     5,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     5,     7,     9,     2,     2,
       1,     1,     0,     0,     4,     1,     0,     0,     3,     3,
       1,     5,     2,     2,     2,     2,     3,     2,     3,     0,
       3,     1,     1,     0,     1,     1,     0,     1,     1,     0,
       1,     1,     0,     3,     0,     3,     0,     3,     1,     1,
       1,     4,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,     3,     4,     0,     6,     1,     9,     0,     5,   246,
     282,   283,     0,   247,     0,     0,     0,     2,     0,     0,
       0,     0,     0,     0,     0,   242,     0,     0,     8,     0,
      12,    13,    14,    15,    16,    17,    18,    19,    21,    22,
      23,    20,     0,    96,    97,   121,   122,    98,     0,    99,
     100,   101,   245,     7,     0,     0,     0,     0,     0,    65,
       0,    88,    64,     0,     0,     0,     0,     0,    76,     0,
       0,    94,   240,   241,    31,    32,    83,     0,     0,     0,
      10,    11,     0,   243,   250,   248,     0,     0,   125,   242,
     123,   259,   257,   253,   255,   252,   272,   254,   242,    84,
      85,    86,    87,    91,   242,   242,   242,   242,   242,   242,
      78,    55,    81,    80,    82,    92,   233,   232,     0,     0,
       0,     0,    60,     0,   242,    83,     0,    61,    63,   134,
     135,   213,   214,   136,   229,   230,     0,   242,     0,     0,
       0,   281,   102,   126,     0,   127,   131,   132,   133,   227,
     228,   231,     0,   262,   261,     0,   263,     0,   256,     0,
       0,    54,     0,     0,     0,    26,     0,    25,    24,   269,
     119,   117,   272,   104,     0,     0,     0,     0,     0,     0,
     266,     0,   266,     0,     0,   276,   272,   142,   143,   144,
     145,   147,   146,   148,   149,   150,   151,     0,   152,   269,
     109,     0,   107,   105,   272,     0,   114,   103,    83,     0,
      52,     0,     0,     0,     0,   244,   249,     0,   239,   238,
       0,   264,   265,   258,   278,     0,   242,    95,     0,     0,
      83,   242,     0,    48,     0,    51,     0,   242,   270,   271,
     118,   120,     0,     0,     0,   212,   183,   184,   182,     0,
     165,   268,   267,   164,     0,     0,     0,     0,   207,   203,
       0,   202,   272,   195,   189,   188,   187,     0,     0,     0,
       0,   108,     0,   110,     0,     0,   106,     0,   242,   234,
      69,     0,    67,    68,     0,   242,   242,   251,     0,   124,
     260,   273,    28,    89,    90,    93,    27,     0,    79,    50,
     274,     0,     0,   225,     0,   226,     0,   186,     0,   174,
       0,   166,     0,   171,   172,   155,   156,   173,   153,   154,
       0,     0,   201,     0,   204,   197,   199,   198,   194,   196,
     280,     0,   170,   169,   176,   177,     0,     0,   116,     0,
     113,     0,     0,    53,     0,    62,    77,    71,    47,     0,
       0,     0,   242,    49,     0,    34,     0,   242,   220,   224,
       0,     0,   266,   211,     0,   209,     0,   210,     0,   277,
     181,   180,   178,   179,   175,   200,     0,   111,   112,   115,
     242,   235,     0,     0,    70,   242,    58,    57,    59,   242,
       0,     0,     0,   129,   137,   140,   138,   215,   216,   139,
     279,     0,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    30,    29,   185,   160,   162,   159,
       0,   157,   158,     0,   206,   208,   205,   190,     0,    74,
      72,    75,    73,     0,     0,     0,     0,   141,   192,   242,
     128,   275,   163,   161,   167,   168,   242,   236,   242,     0,
       0,     0,     0,   191,   130,     0,     0,     0,     0,   218,
       0,   222,     0,   237,   242,     0,   217,     0,   221,     0,
       0,    56,    33,   219,   223,     0,     0,   193
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     3,     4,     6,     8,     9,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,   301,
     414,    41,   162,   233,    74,    60,    69,   348,   349,   387,
     234,    61,   126,   281,   282,   283,   384,   430,   432,    70,
     347,   111,   299,   115,   103,   161,    75,   229,    76,   230,
      42,    43,   127,   207,   341,   276,   339,   173,    44,    45,
      46,   144,    90,   289,   392,   145,   128,   393,   394,   129,
     187,   318,   188,   421,   443,   189,   253,   190,   444,   191,
     333,   319,   310,   192,   336,   374,   193,   248,   194,   308,
     195,   266,   196,   437,   453,   197,   328,   329,   376,   263,
     322,   366,   368,   364,   198,   130,   396,   397,   458,   131,
     398,   460,   132,   304,   306,   399,   133,   149,   134,   135,
     151,    77,    47,   139,    48,    49,    54,    85,    50,    62,
      97,   156,   223,   254,   240,   158,   355,   268,   225,   401,
     331,    51,    12
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -398
static const yytype_int16 yypact[] =
{
      52,  -398,  -398,    14,  -398,  -398,    67,   152,  -398,    24,
    -398,  -398,     5,  -398,    47,    81,    99,  -398,    -1,    -1,
      -1,    -1,    -1,    -1,    43,    56,    -1,    -1,  -398,    97,
    -398,  -398,  -398,  -398,  -398,  -398,  -398,  -398,  -398,  -398,
    -398,  -398,   112,  -398,  -398,  -398,  -398,  -398,   156,  -398,
    -398,  -398,  -398,  -398,   111,    98,   141,    95,   127,  -398,
      84,   142,  -398,   146,   150,   153,   157,   158,  -398,   159,
     165,  -398,  -398,  -398,  -398,  -398,   113,   -13,   161,   163,
    -398,  -398,   162,  -398,  -398,   164,   174,    10,   252,    -3,
    -398,   -11,  -398,  -398,  -398,  -398,   166,  -398,   -20,  -398,
    -398,  -398,  -398,   167,   -20,   -20,   -20,   -20,   -20,   -20,
    -398,  -398,  -398,  -398,  -398,  -398,  -398,  -398,   137,    70,
     132,    85,   168,    34,   -20,   113,   169,  -398,  -398,  -398,
    -398,  -398,  -398,  -398,  -398,  -398,    34,   -20,   171,   111,
     179,  -398,  -398,  -398,   172,  -398,  -398,  -398,  -398,  -398,
    -398,  -398,   216,  -398,  -398,   253,    76,   258,  -398,   176,
     154,  -398,   178,    29,   180,  -398,   181,  -398,  -398,   110,
    -398,  -398,   166,  -398,   175,   182,   183,   219,    32,   184,
     177,   186,    94,   140,     7,   187,   166,  -398,  -398,  -398,
    -398,  -398,  -398,  -398,  -398,  -398,  -398,   226,  -398,   110,
    -398,   188,  -398,  -398,   166,   189,   190,  -398,   113,     9,
    -398,     1,   193,   195,   240,   164,  -398,   191,  -398,  -398,
     194,  -398,  -398,  -398,  -398,   197,   -20,  -398,   196,   198,
     113,   -20,    34,  -398,   203,   206,   228,   -20,  -398,  -398,
    -398,  -398,   290,   292,   293,  -398,  -398,  -398,  -398,   294,
    -398,  -398,  -398,  -398,   251,   294,    48,   208,   209,  -398,
     210,  -398,   166,    21,  -398,  -398,  -398,   299,   295,    12,
     212,  -398,   302,  -398,   304,   302,  -398,   218,   -20,  -398,
    -398,   217,  -398,  -398,   227,   -20,   -20,  -398,   214,  -398,
    -398,  -398,  -398,  -398,  -398,  -398,  -398,   220,  -398,  -398,
     222,   225,   229,  -398,   223,  -398,   224,  -398,   230,  -398,
     231,  -398,   233,  -398,  -398,  -398,  -398,  -398,  -398,  -398,
     314,   316,  -398,   317,  -398,  -398,  -398,  -398,  -398,  -398,
    -398,   234,  -398,  -398,  -398,  -398,   170,   318,  -398,   235,
    -398,   236,   237,  -398,    44,  -398,  -398,   143,  -398,   244,
     -15,   245,    36,  -398,   332,  -398,   138,   -20,  -398,  -398,
     301,   101,    94,  -398,   248,  -398,   249,  -398,   250,  -398,
    -398,  -398,  -398,  -398,  -398,  -398,   254,  -398,  -398,  -398,
     -20,  -398,   333,   340,  -398,   -20,  -398,  -398,  -398,   -20,
     102,   132,    75,  -398,  -398,  -398,  -398,  -398,  -398,  -398,
    -398,   255,  -398,  -398,  -398,  -398,  -398,  -398,  -398,  -398,
    -398,  -398,  -398,  -398,  -398,  -398,  -398,  -398,  -398,  -398,
     336,  -398,  -398,    49,  -398,  -398,  -398,  -398,    90,  -398,
    -398,  -398,  -398,   256,   260,   259,   261,  -398,   298,    36,
    -398,  -398,  -398,  -398,  -398,  -398,   -20,  -398,   -20,   228,
     290,   292,   262,  -398,  -398,   257,   265,   268,   266,   273,
     269,   274,   318,  -398,   -20,   138,  -398,   290,  -398,   292,
     107,  -398,  -398,  -398,  -398,   318,   270,  -398
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -398,  -398,  -398,  -398,  -398,  -398,  -398,  -398,  -398,  -398,
    -398,  -398,  -398,  -398,  -398,  -398,  -398,  -398,  -398,   -78,
     -82,  -398,  -100,   155,   -86,   215,  -398,  -398,  -372,  -398,
     -54,  -398,  -398,  -398,  -398,  -398,  -398,  -398,  -398,   173,
    -398,  -398,  -398,  -118,  -398,  -398,   232,  -398,  -398,  -398,
    -398,  -398,   303,  -398,  -398,  -398,   114,  -398,  -398,  -398,
    -398,  -398,  -398,  -398,  -398,  -398,  -398,   -53,  -398,   -88,
    -398,  -398,  -398,  -398,  -398,  -398,  -398,  -398,  -398,  -398,
    -398,  -334,   130,  -398,  -398,  -398,  -398,  -398,  -398,  -398,
    -398,  -398,  -398,  -398,  -398,     0,  -398,  -398,  -397,  -398,
    -398,  -398,  -398,  -398,  -398,   305,  -398,  -398,  -398,  -398,
    -398,  -398,  -398,  -396,  -383,   306,  -398,  -398,  -137,   -87,
    -120,   -89,  -398,  -398,  -398,  -398,  -398,   263,  -398,   185,
    -398,  -398,  -398,  -177,   199,  -154,  -398,  -398,  -398,  -398,
    -398,  -398,    -6
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -230
static const yytype_int16 yytable[] =
{
     152,   146,   150,    52,   209,   256,   165,   210,   386,   168,
     116,   117,   159,   433,     5,   163,   153,   163,   241,   164,
     163,   166,   167,   125,   280,   118,   235,   422,   154,    13,
      14,    15,   269,   264,    16,   152,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,   334,   118,   119,
     273,   213,   116,   117,   459,     1,     2,   116,   117,   119,
     120,   246,   325,   326,    58,   470,   335,   118,   461,   208,
     120,   473,   118,   313,   313,     7,   456,   265,   476,   314,
     314,   315,   212,   121,    10,    11,   474,   122,   247,   445,
     277,   119,   471,    72,    73,   235,   119,   123,   390,    59,
     155,    68,   120,   327,   174,   124,   121,   120,   324,   391,
      72,    73,   295,    53,   199,   124,   175,   316,   278,   317,
     317,   251,   200,    10,    11,   121,   313,   417,   279,   122,
     121,   296,   314,   252,   122,   201,   435,   221,   202,   232,
     292,   418,   163,    68,   222,   203,    55,   124,   436,    72,
      73,   302,   124,   380,   124,    71,    91,    92,   344,   204,
     176,   419,   177,   381,    93,    82,   169,    83,   178,    72,
      73,   238,   317,   420,   170,   179,   180,   181,   239,   182,
      56,   183,   205,   206,   439,   423,    94,    95,   257,   152,
     184,   258,   259,    98,   440,   260,   350,   171,    57,   446,
     351,    96,   250,   261,   251,    80,    88,   185,   186,   447,
      84,   172,    89,   475,   112,    86,   252,   113,   114,   427,
      81,   262,   402,   403,   404,   405,   406,   407,   408,   409,
     410,   411,   412,   413,    63,    64,    65,    66,    67,   218,
     219,    78,    79,    99,   100,   101,   102,   370,   371,   372,
     373,    10,    11,    71,   227,   104,   382,   383,    87,   105,
     428,   138,   106,   152,   395,   150,   107,   108,   109,   110,
     136,   415,   137,   140,   141,   143,   220,   157,   216,   -66,
     211,   224,   160,   245,   217,   226,   242,   231,   214,   236,
     237,   152,   270,   243,   244,   249,   350,   255,   267,   272,
     274,   275,   285,   434,   286,    58,   290,   298,   288,   291,
    -229,   300,   293,   303,   294,   305,   307,   309,   311,   320,
     321,   323,   330,   337,   332,   338,   455,   340,   343,   345,
     353,   346,   352,   354,   356,   358,   359,   363,   357,   365,
     367,   375,   360,   361,   388,   362,   369,   377,   378,   379,
     152,   395,   150,   385,   389,   400,   429,   152,   416,   350,
     424,   425,   426,   431,   452,   448,   427,   441,   442,   449,
     450,   457,   451,   462,   464,   350,   463,   465,   466,   467,
     469,   468,   477,   472,   284,   312,   454,   297,     0,   342,
     142,   438,   228,     0,   147,   148,     0,     0,   271,   287,
       0,     0,   215
};

static const yytype_int16 yycheck[] =
{
      89,    89,    89,     9,   124,   182,   106,   125,    23,   109,
      23,    24,    98,   385,     0,   104,    27,   106,   172,   105,
     109,   107,   108,    77,    23,    38,   163,   361,    39,     5,
       6,     7,   186,    26,    10,   124,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    35,    38,    62,
     204,   137,    23,    24,   450,     3,     4,    23,    24,    62,
      73,    29,    41,    42,    65,   462,    54,    38,   451,   123,
      73,   467,    38,    25,    25,     8,   448,    70,   475,    31,
      31,    33,   136,    96,    99,   100,   469,   100,    56,   423,
     208,    62,   464,   113,   114,   232,    62,   110,    62,   100,
     111,   100,    73,    82,    34,   118,    96,    73,   262,    73,
     113,   114,   230,   108,    29,   118,    46,    69,   109,    71,
      71,    27,    37,    99,   100,    96,    25,    26,   119,   100,
      96,   231,    31,    39,   100,    50,    34,    61,    53,   110,
     226,    40,   231,   100,    68,    60,    99,   118,    46,   113,
     114,   237,   118,   109,   118,    99,    29,    30,   278,    74,
      28,    60,    30,   119,    37,     9,    29,    11,    36,   113,
     114,    61,    71,    72,    37,    43,    44,    45,    68,    47,
      99,    49,    97,    98,   109,   362,    59,    60,    48,   278,
      58,    51,    52,   109,   119,    55,   285,    60,    99,   109,
     286,    74,    25,    63,    27,   108,   111,    75,    76,   119,
      99,    74,   117,   106,   101,   117,    39,   104,   105,   112,
     108,    81,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    19,    20,    21,    22,    23,    23,
      24,    26,    27,   101,   102,   103,   104,    77,    78,    79,
      80,    99,   100,    99,   100,   109,   113,   114,   117,   109,
     380,    99,   109,   352,   352,   352,   109,   109,   109,   104,
     109,   357,   109,   109,   100,    23,    23,   111,    99,   111,
     111,    23,   115,    64,   112,   109,   111,   109,   117,   109,
     109,   380,    66,   111,   111,   111,   385,   111,   111,   111,
     111,   111,   109,   389,   109,    65,   112,   104,   117,   112,
     104,    83,   116,    23,   116,    23,    23,    23,    67,   111,
     111,   111,    23,   111,    29,    23,   446,    23,   110,   112,
     110,   104,   118,   111,   109,   112,   112,    23,   109,    23,
      23,    23,   112,   112,   350,   112,   112,   112,   112,   112,
     439,   439,   439,   109,   109,    23,    23,   446,    57,   448,
     112,   112,   112,    23,    66,   109,   112,   112,    32,   109,
     111,   449,   111,   111,   109,   464,   119,   109,   112,   106,
     106,   112,   112,   465,   211,   255,   439,   232,    -1,   275,
      87,   391,   160,    -1,    89,    89,    -1,    -1,   199,   214,
      -1,    -1,   139
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,     3,     4,   121,   122,     0,   123,     8,   124,   125,
      99,   100,   262,     5,     6,     7,    10,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   141,   170,   171,   178,   179,   180,   242,   244,   245,
     248,   261,   262,   108,   246,    99,    99,    99,    65,   100,
     145,   151,   249,   145,   145,   145,   145,   145,   100,   146,
     159,    99,   113,   114,   144,   166,   168,   241,   145,   145,
     108,   108,     9,    11,    99,   247,   117,   117,   111,   117,
     182,    29,    30,    37,    59,    60,    74,   250,   109,   101,
     102,   103,   104,   164,   109,   109,   109,   109,   109,   109,
     104,   161,   101,   104,   105,   163,    23,    24,    38,    62,
      73,    96,   100,   110,   118,   150,   152,   172,   186,   189,
     225,   229,   232,   236,   238,   239,   109,   109,    99,   243,
     109,   100,   172,    23,   181,   185,   189,   225,   235,   237,
     239,   240,   241,    27,    39,   111,   251,   111,   255,   144,
     115,   165,   142,   241,   144,   142,   144,   144,   142,    29,
      37,    60,    74,   177,    34,    46,    28,    30,    36,    43,
      44,    45,    47,    49,    58,    75,    76,   190,   192,   195,
     197,   199,   203,   206,   208,   210,   212,   215,   224,    29,
      37,    50,    53,    60,    74,    97,    98,   173,   150,   240,
     163,   111,   150,   144,   117,   247,    99,   112,    23,    24,
      23,    61,    68,   252,    23,   258,   109,   100,   166,   167,
     169,   109,   110,   143,   150,   238,   109,   109,    61,    68,
     254,   255,   111,   111,   111,    64,    29,    56,   207,   111,
      25,    27,    39,   196,   253,   111,   253,    48,    51,    52,
      55,    63,    81,   219,    26,    70,   211,   111,   257,   255,
      66,   254,   111,   255,   111,   111,   175,   163,   109,   119,
      23,   153,   154,   155,   159,   109,   109,   249,   117,   183,
     112,   112,   144,   116,   116,   163,   142,   143,   104,   162,
      83,   139,   144,    23,   233,    23,   234,    23,   209,    23,
     202,    67,   202,    25,    31,    33,    69,    71,   191,   201,
     111,   111,   220,   111,   255,    41,    42,    82,   216,   217,
      23,   260,    29,   200,    35,    54,   204,   111,    23,   176,
      23,   174,   176,   110,   240,   112,   104,   160,   147,   148,
     241,   144,   118,   110,   111,   256,   109,   109,   112,   112,
     112,   112,   112,    23,   223,    23,   221,    23,   222,   112,
      77,    78,    79,    80,   205,    23,   218,   112,   112,   112,
     109,   119,   113,   114,   156,   109,    23,   149,   262,   109,
      62,    73,   184,   187,   188,   189,   226,   227,   230,   235,
      23,   259,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,   140,   144,    57,    26,    40,    60,
      72,   193,   201,   253,   112,   112,   112,   112,   240,    23,
     157,    23,   158,   148,   144,    34,    46,   213,   215,   109,
     119,   112,    32,   194,   198,   201,   109,   119,   109,   109,
     111,   111,    66,   214,   187,   240,   148,   139,   228,   233,
     231,   234,   111,   119,   109,   109,   112,   106,   112,   106,
     218,   148,   140,   233,   234,   106,   218,   112
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
      yyerror (&yylloc, state, YY_("syntax error: cannot back up")); \
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
# define YYLEX yylex (&yylval, &yylloc, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, &yylloc, scanner)
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
		  Type, Value, Location, state); \
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
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, struct asm_parser_state *state)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, state)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    struct asm_parser_state *state;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
  YYUSE (state);
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
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, struct asm_parser_state *state)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp, state)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    struct asm_parser_state *state;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, state);
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
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule, struct asm_parser_state *state)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule, state)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
    struct asm_parser_state *state;
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
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       , state);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule, state); \
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
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, struct asm_parser_state *state)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp, state)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
    struct asm_parser_state *state;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (state);

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
int yyparse (struct asm_parser_state *state);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */





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
yyparse (struct asm_parser_state *state)
#else
int
yyparse (state)
    struct asm_parser_state *state;
#endif
#endif
{
/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Location data for the lookahead symbol.  */
YYLTYPE yylloc;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.
       `yyls': related to locations.

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

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[3];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yyls = yylsa;
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
  yylsp = yyls;

#if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 1;
#endif

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
	YYLTYPE *yyls1 = yyls;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);

	yyls = yyls1;
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
	YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

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
  *++yylsp = yylloc;
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

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 3:

/* Line 1464 of yacc.c  */
#line 282 "program/program_parse.y"
    {
	   if (state->prog->Target != GL_VERTEX_PROGRAM_ARB) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid fragment program header");

	   }
	   state->mode = ARB_vertex;
	;}
    break;

  case 4:

/* Line 1464 of yacc.c  */
#line 290 "program/program_parse.y"
    {
	   if (state->prog->Target != GL_FRAGMENT_PROGRAM_ARB) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid vertex program header");
	   }
	   state->mode = ARB_fragment;

	   state->option.TexRect =
	      (state->ctx->Extensions.NV_texture_rectangle != GL_FALSE);
	;}
    break;

  case 7:

/* Line 1464 of yacc.c  */
#line 306 "program/program_parse.y"
    {
	   int valid = 0;

	   if (state->mode == ARB_vertex) {
	      valid = _mesa_ARBvp_parse_option(state, (yyvsp[(2) - (3)].string));
	   } else if (state->mode == ARB_fragment) {
	      valid = _mesa_ARBfp_parse_option(state, (yyvsp[(2) - (3)].string));
	   }


	   free((yyvsp[(2) - (3)].string));

	   if (!valid) {
	      const char *const err_str = (state->mode == ARB_vertex)
		 ? "invalid ARB vertex program option"
		 : "invalid ARB fragment program option";

	      yyerror(& (yylsp[(2) - (3)]), state, err_str);
	      YYERROR;
	   }
	;}
    break;

  case 10:

/* Line 1464 of yacc.c  */
#line 334 "program/program_parse.y"
    {
	   if ((yyvsp[(1) - (2)].inst) != NULL) {
	      if (state->inst_tail == NULL) {
		 state->inst_head = (yyvsp[(1) - (2)].inst);
	      } else {
		 state->inst_tail->next = (yyvsp[(1) - (2)].inst);
	      }

	      state->inst_tail = (yyvsp[(1) - (2)].inst);
	      (yyvsp[(1) - (2)].inst)->next = NULL;

	      state->prog->NumInstructions++;
	   }
	;}
    break;

  case 12:

/* Line 1464 of yacc.c  */
#line 352 "program/program_parse.y"
    {
	   (yyval.inst) = (yyvsp[(1) - (1)].inst);
	   state->prog->NumAluInstructions++;
	;}
    break;

  case 13:

/* Line 1464 of yacc.c  */
#line 357 "program/program_parse.y"
    {
	   (yyval.inst) = (yyvsp[(1) - (1)].inst);
	   state->prog->NumTexInstructions++;
	;}
    break;

  case 24:

/* Line 1464 of yacc.c  */
#line 378 "program/program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor(OPCODE_ARL, & (yyvsp[(2) - (4)].dst_reg), & (yyvsp[(4) - (4)].src_reg), NULL, NULL);
	;}
    break;

  case 25:

/* Line 1464 of yacc.c  */
#line 384 "program/program_parse.y"
    {
	   (yyval.inst) = asm_instruction_copy_ctor(& (yyvsp[(1) - (4)].temp_inst), & (yyvsp[(2) - (4)].dst_reg), & (yyvsp[(4) - (4)].src_reg), NULL, NULL);
	;}
    break;

  case 26:

/* Line 1464 of yacc.c  */
#line 390 "program/program_parse.y"
    {
	   (yyval.inst) = asm_instruction_copy_ctor(& (yyvsp[(1) - (4)].temp_inst), & (yyvsp[(2) - (4)].dst_reg), & (yyvsp[(4) - (4)].src_reg), NULL, NULL);
	;}
    break;

  case 27:

/* Line 1464 of yacc.c  */
#line 396 "program/program_parse.y"
    {
	   (yyval.inst) = asm_instruction_copy_ctor(& (yyvsp[(1) - (6)].temp_inst), & (yyvsp[(2) - (6)].dst_reg), & (yyvsp[(4) - (6)].src_reg), & (yyvsp[(6) - (6)].src_reg), NULL);
	;}
    break;

  case 28:

/* Line 1464 of yacc.c  */
#line 403 "program/program_parse.y"
    {
	   (yyval.inst) = asm_instruction_copy_ctor(& (yyvsp[(1) - (6)].temp_inst), & (yyvsp[(2) - (6)].dst_reg), & (yyvsp[(4) - (6)].src_reg), & (yyvsp[(6) - (6)].src_reg), NULL);
	;}
    break;

  case 29:

/* Line 1464 of yacc.c  */
#line 410 "program/program_parse.y"
    {
	   (yyval.inst) = asm_instruction_copy_ctor(& (yyvsp[(1) - (8)].temp_inst), & (yyvsp[(2) - (8)].dst_reg), & (yyvsp[(4) - (8)].src_reg), & (yyvsp[(6) - (8)].src_reg), & (yyvsp[(8) - (8)].src_reg));
	;}
    break;

  case 30:

/* Line 1464 of yacc.c  */
#line 416 "program/program_parse.y"
    {
	   (yyval.inst) = asm_instruction_copy_ctor(& (yyvsp[(1) - (8)].temp_inst), & (yyvsp[(2) - (8)].dst_reg), & (yyvsp[(4) - (8)].src_reg), NULL, NULL);
	   if ((yyval.inst) != NULL) {
	      const GLbitfield tex_mask = (1U << (yyvsp[(6) - (8)].integer));
	      GLbitfield shadow_tex = 0;
	      GLbitfield target_mask = 0;


	      (yyval.inst)->Base.TexSrcUnit = (yyvsp[(6) - (8)].integer);

	      if ((yyvsp[(8) - (8)].integer) < 0) {
		 shadow_tex = tex_mask;

		 (yyval.inst)->Base.TexSrcTarget = -(yyvsp[(8) - (8)].integer);
		 (yyval.inst)->Base.TexShadow = 1;
	      } else {
		 (yyval.inst)->Base.TexSrcTarget = (yyvsp[(8) - (8)].integer);
	      }

	      target_mask = (1U << (yyval.inst)->Base.TexSrcTarget);

	      /* If this texture unit was previously accessed and that access
	       * had a different texture target, generate an error.
	       *
	       * If this texture unit was previously accessed and that access
	       * had a different shadow mode, generate an error.
	       */
	      if ((state->prog->TexturesUsed[(yyvsp[(6) - (8)].integer)] != 0)
		  && ((state->prog->TexturesUsed[(yyvsp[(6) - (8)].integer)] != target_mask)
		      || ((state->prog->ShadowSamplers & tex_mask)
			  != shadow_tex))) {
		 yyerror(& (yylsp[(8) - (8)]), state,
			 "multiple targets used on one texture image unit");
		 YYERROR;
	      }


	      state->prog->TexturesUsed[(yyvsp[(6) - (8)].integer)] |= target_mask;
	      state->prog->ShadowSamplers |= shadow_tex;
	   }
	;}
    break;

  case 31:

/* Line 1464 of yacc.c  */
#line 460 "program/program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor(OPCODE_KIL, NULL, & (yyvsp[(2) - (2)].src_reg), NULL, NULL);
	   state->fragment.UsesKill = 1;
	;}
    break;

  case 32:

/* Line 1464 of yacc.c  */
#line 465 "program/program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor(OPCODE_KIL_NV, NULL, NULL, NULL, NULL);
	   (yyval.inst)->Base.DstReg.CondMask = (yyvsp[(2) - (2)].dst_reg).CondMask;
	   (yyval.inst)->Base.DstReg.CondSwizzle = (yyvsp[(2) - (2)].dst_reg).CondSwizzle;
	   (yyval.inst)->Base.DstReg.CondSrc = (yyvsp[(2) - (2)].dst_reg).CondSrc;
	   state->fragment.UsesKill = 1;
	;}
    break;

  case 33:

/* Line 1464 of yacc.c  */
#line 475 "program/program_parse.y"
    {
	   (yyval.inst) = asm_instruction_copy_ctor(& (yyvsp[(1) - (12)].temp_inst), & (yyvsp[(2) - (12)].dst_reg), & (yyvsp[(4) - (12)].src_reg), & (yyvsp[(6) - (12)].src_reg), & (yyvsp[(8) - (12)].src_reg));
	   if ((yyval.inst) != NULL) {
	      const GLbitfield tex_mask = (1U << (yyvsp[(10) - (12)].integer));
	      GLbitfield shadow_tex = 0;
	      GLbitfield target_mask = 0;


	      (yyval.inst)->Base.TexSrcUnit = (yyvsp[(10) - (12)].integer);

	      if ((yyvsp[(12) - (12)].integer) < 0) {
		 shadow_tex = tex_mask;

		 (yyval.inst)->Base.TexSrcTarget = -(yyvsp[(12) - (12)].integer);
		 (yyval.inst)->Base.TexShadow = 1;
	      } else {
		 (yyval.inst)->Base.TexSrcTarget = (yyvsp[(12) - (12)].integer);
	      }

	      target_mask = (1U << (yyval.inst)->Base.TexSrcTarget);

	      /* If this texture unit was previously accessed and that access
	       * had a different texture target, generate an error.
	       *
	       * If this texture unit was previously accessed and that access
	       * had a different shadow mode, generate an error.
	       */
	      if ((state->prog->TexturesUsed[(yyvsp[(10) - (12)].integer)] != 0)
		  && ((state->prog->TexturesUsed[(yyvsp[(10) - (12)].integer)] != target_mask)
		      || ((state->prog->ShadowSamplers & tex_mask)
			  != shadow_tex))) {
		 yyerror(& (yylsp[(12) - (12)]), state,
			 "multiple targets used on one texture image unit");
		 YYERROR;
	      }


	      state->prog->TexturesUsed[(yyvsp[(10) - (12)].integer)] |= target_mask;
	      state->prog->ShadowSamplers |= shadow_tex;
	   }
	;}
    break;

  case 34:

/* Line 1464 of yacc.c  */
#line 519 "program/program_parse.y"
    {
	   (yyval.integer) = (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 35:

/* Line 1464 of yacc.c  */
#line 524 "program/program_parse.y"
    { (yyval.integer) = TEXTURE_1D_INDEX; ;}
    break;

  case 36:

/* Line 1464 of yacc.c  */
#line 525 "program/program_parse.y"
    { (yyval.integer) = TEXTURE_2D_INDEX; ;}
    break;

  case 37:

/* Line 1464 of yacc.c  */
#line 526 "program/program_parse.y"
    { (yyval.integer) = TEXTURE_3D_INDEX; ;}
    break;

  case 38:

/* Line 1464 of yacc.c  */
#line 527 "program/program_parse.y"
    { (yyval.integer) = TEXTURE_CUBE_INDEX; ;}
    break;

  case 39:

/* Line 1464 of yacc.c  */
#line 528 "program/program_parse.y"
    { (yyval.integer) = TEXTURE_RECT_INDEX; ;}
    break;

  case 40:

/* Line 1464 of yacc.c  */
#line 529 "program/program_parse.y"
    { (yyval.integer) = -TEXTURE_1D_INDEX; ;}
    break;

  case 41:

/* Line 1464 of yacc.c  */
#line 530 "program/program_parse.y"
    { (yyval.integer) = -TEXTURE_2D_INDEX; ;}
    break;

  case 42:

/* Line 1464 of yacc.c  */
#line 531 "program/program_parse.y"
    { (yyval.integer) = -TEXTURE_RECT_INDEX; ;}
    break;

  case 43:

/* Line 1464 of yacc.c  */
#line 532 "program/program_parse.y"
    { (yyval.integer) = TEXTURE_1D_ARRAY_INDEX; ;}
    break;

  case 44:

/* Line 1464 of yacc.c  */
#line 533 "program/program_parse.y"
    { (yyval.integer) = TEXTURE_2D_ARRAY_INDEX; ;}
    break;

  case 45:

/* Line 1464 of yacc.c  */
#line 534 "program/program_parse.y"
    { (yyval.integer) = -TEXTURE_1D_ARRAY_INDEX; ;}
    break;

  case 46:

/* Line 1464 of yacc.c  */
#line 535 "program/program_parse.y"
    { (yyval.integer) = -TEXTURE_2D_ARRAY_INDEX; ;}
    break;

  case 47:

/* Line 1464 of yacc.c  */
#line 539 "program/program_parse.y"
    {
	   /* FIXME: Is this correct?  Should the extenedSwizzle be applied
	    * FIXME: to the existing swizzle?
	    */
	   (yyvsp[(4) - (6)].src_reg).Base.Swizzle = (yyvsp[(6) - (6)].swiz_mask).swizzle;
	   (yyvsp[(4) - (6)].src_reg).Base.Negate = (yyvsp[(6) - (6)].swiz_mask).mask;

	   (yyval.inst) = asm_instruction_copy_ctor(& (yyvsp[(1) - (6)].temp_inst), & (yyvsp[(2) - (6)].dst_reg), & (yyvsp[(4) - (6)].src_reg), NULL, NULL);
	;}
    break;

  case 48:

/* Line 1464 of yacc.c  */
#line 551 "program/program_parse.y"
    {
	   (yyval.src_reg) = (yyvsp[(2) - (2)].src_reg);

	   if ((yyvsp[(1) - (2)].negate)) {
	      (yyval.src_reg).Base.Negate = ~(yyval.src_reg).Base.Negate;
	   }
	;}
    break;

  case 49:

/* Line 1464 of yacc.c  */
#line 559 "program/program_parse.y"
    {
	   (yyval.src_reg) = (yyvsp[(3) - (4)].src_reg);

	   if (!state->option.NV_fragment) {
	      yyerror(& (yylsp[(2) - (4)]), state, "unexpected character '|'");
	      YYERROR;
	   }

	   if ((yyvsp[(1) - (4)].negate)) {
	      (yyval.src_reg).Base.Negate = ~(yyval.src_reg).Base.Negate;
	   }

	   (yyval.src_reg).Base.Abs = 1;
	;}
    break;

  case 50:

/* Line 1464 of yacc.c  */
#line 576 "program/program_parse.y"
    {
	   (yyval.src_reg) = (yyvsp[(1) - (2)].src_reg);

	   (yyval.src_reg).Base.Swizzle = _mesa_combine_swizzles((yyval.src_reg).Base.Swizzle,
						    (yyvsp[(2) - (2)].swiz_mask).swizzle);
	;}
    break;

  case 51:

/* Line 1464 of yacc.c  */
#line 583 "program/program_parse.y"
    {
	   struct asm_symbol temp_sym;

	   if (!state->option.NV_fragment) {
	      yyerror(& (yylsp[(1) - (1)]), state, "expected scalar suffix");
	      YYERROR;
	   }

	   memset(& temp_sym, 0, sizeof(temp_sym));
	   temp_sym.param_binding_begin = ~0;
	   initialize_symbol_from_const(state->prog, & temp_sym, & (yyvsp[(1) - (1)].vector), GL_TRUE);

	   set_src_reg_swz(& (yyval.src_reg), PROGRAM_CONSTANT,
                           temp_sym.param_binding_begin,
                           temp_sym.param_binding_swizzle);
	;}
    break;

  case 52:

/* Line 1464 of yacc.c  */
#line 602 "program/program_parse.y"
    {
	   (yyval.src_reg) = (yyvsp[(2) - (3)].src_reg);

	   if ((yyvsp[(1) - (3)].negate)) {
	      (yyval.src_reg).Base.Negate = ~(yyval.src_reg).Base.Negate;
	   }

	   (yyval.src_reg).Base.Swizzle = _mesa_combine_swizzles((yyval.src_reg).Base.Swizzle,
						    (yyvsp[(3) - (3)].swiz_mask).swizzle);
	;}
    break;

  case 53:

/* Line 1464 of yacc.c  */
#line 613 "program/program_parse.y"
    {
	   (yyval.src_reg) = (yyvsp[(3) - (5)].src_reg);

	   if (!state->option.NV_fragment) {
	      yyerror(& (yylsp[(2) - (5)]), state, "unexpected character '|'");
	      YYERROR;
	   }

	   if ((yyvsp[(1) - (5)].negate)) {
	      (yyval.src_reg).Base.Negate = ~(yyval.src_reg).Base.Negate;
	   }

	   (yyval.src_reg).Base.Abs = 1;
	   (yyval.src_reg).Base.Swizzle = _mesa_combine_swizzles((yyval.src_reg).Base.Swizzle,
						    (yyvsp[(4) - (5)].swiz_mask).swizzle);
	;}
    break;

  case 54:

/* Line 1464 of yacc.c  */
#line 633 "program/program_parse.y"
    {
	   (yyval.dst_reg) = (yyvsp[(1) - (3)].dst_reg);
	   (yyval.dst_reg).WriteMask = (yyvsp[(2) - (3)].swiz_mask).mask;
	   (yyval.dst_reg).CondMask = (yyvsp[(3) - (3)].dst_reg).CondMask;
	   (yyval.dst_reg).CondSwizzle = (yyvsp[(3) - (3)].dst_reg).CondSwizzle;
	   (yyval.dst_reg).CondSrc = (yyvsp[(3) - (3)].dst_reg).CondSrc;

	   if ((yyval.dst_reg).File == PROGRAM_OUTPUT) {
	      /* Technically speaking, this should check that it is in
	       * vertex program mode.  However, PositionInvariant can never be
	       * set in fragment program mode, so it is somewhat irrelevant.
	       */
	      if (state->option.PositionInvariant
	       && ((yyval.dst_reg).Index == VERT_RESULT_HPOS)) {
		 yyerror(& (yylsp[(1) - (3)]), state, "position-invariant programs cannot "
			 "write position");
		 YYERROR;
	      }

	      state->prog->OutputsWritten |= BITFIELD64_BIT((yyval.dst_reg).Index);
	   }
	;}
    break;

  case 55:

/* Line 1464 of yacc.c  */
#line 658 "program/program_parse.y"
    {
	   set_dst_reg(& (yyval.dst_reg), PROGRAM_ADDRESS, 0);
	   (yyval.dst_reg).WriteMask = (yyvsp[(2) - (2)].swiz_mask).mask;
	;}
    break;

  case 56:

/* Line 1464 of yacc.c  */
#line 665 "program/program_parse.y"
    {
	   const unsigned xyzw_valid =
	      ((yyvsp[(1) - (7)].ext_swizzle).xyzw_valid << 0)
	      | ((yyvsp[(3) - (7)].ext_swizzle).xyzw_valid << 1)
	      | ((yyvsp[(5) - (7)].ext_swizzle).xyzw_valid << 2)
	      | ((yyvsp[(7) - (7)].ext_swizzle).xyzw_valid << 3);
	   const unsigned rgba_valid =
	      ((yyvsp[(1) - (7)].ext_swizzle).rgba_valid << 0)
	      | ((yyvsp[(3) - (7)].ext_swizzle).rgba_valid << 1)
	      | ((yyvsp[(5) - (7)].ext_swizzle).rgba_valid << 2)
	      | ((yyvsp[(7) - (7)].ext_swizzle).rgba_valid << 3);

	   /* All of the swizzle components have to be valid in either RGBA
	    * or XYZW.  Note that 0 and 1 are valid in both, so both masks
	    * can have some bits set.
	    *
	    * We somewhat deviate from the spec here.  It would be really hard
	    * to figure out which component is the error, and there probably
	    * isn't a lot of benefit.
	    */
	   if ((rgba_valid != 0x0f) && (xyzw_valid != 0x0f)) {
	      yyerror(& (yylsp[(1) - (7)]), state, "cannot combine RGBA and XYZW swizzle "
		      "components");
	      YYERROR;
	   }

	   (yyval.swiz_mask).swizzle = MAKE_SWIZZLE4((yyvsp[(1) - (7)].ext_swizzle).swz, (yyvsp[(3) - (7)].ext_swizzle).swz, (yyvsp[(5) - (7)].ext_swizzle).swz, (yyvsp[(7) - (7)].ext_swizzle).swz);
	   (yyval.swiz_mask).mask = ((yyvsp[(1) - (7)].ext_swizzle).negate) | ((yyvsp[(3) - (7)].ext_swizzle).negate << 1) | ((yyvsp[(5) - (7)].ext_swizzle).negate << 2)
	      | ((yyvsp[(7) - (7)].ext_swizzle).negate << 3);
	;}
    break;

  case 57:

/* Line 1464 of yacc.c  */
#line 698 "program/program_parse.y"
    {
	   (yyval.ext_swizzle) = (yyvsp[(2) - (2)].ext_swizzle);
	   (yyval.ext_swizzle).negate = ((yyvsp[(1) - (2)].negate)) ? 1 : 0;
	;}
    break;

  case 58:

/* Line 1464 of yacc.c  */
#line 705 "program/program_parse.y"
    {
	   if (((yyvsp[(1) - (1)].integer) != 0) && ((yyvsp[(1) - (1)].integer) != 1)) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid extended swizzle selector");
	      YYERROR;
	   }

	   (yyval.ext_swizzle).swz = ((yyvsp[(1) - (1)].integer) == 0) ? SWIZZLE_ZERO : SWIZZLE_ONE;

	   /* 0 and 1 are valid for both RGBA swizzle names and XYZW
	    * swizzle names.
	    */
	   (yyval.ext_swizzle).xyzw_valid = 1;
	   (yyval.ext_swizzle).rgba_valid = 1;
	;}
    break;

  case 59:

/* Line 1464 of yacc.c  */
#line 720 "program/program_parse.y"
    {
	   char s;

	   if (strlen((yyvsp[(1) - (1)].string)) > 1) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid extended swizzle selector");
	      YYERROR;
	   }

	   s = (yyvsp[(1) - (1)].string)[0];
	   free((yyvsp[(1) - (1)].string));

	   switch (s) {
	   case 'x':
	      (yyval.ext_swizzle).swz = SWIZZLE_X;
	      (yyval.ext_swizzle).xyzw_valid = 1;
	      break;
	   case 'y':
	      (yyval.ext_swizzle).swz = SWIZZLE_Y;
	      (yyval.ext_swizzle).xyzw_valid = 1;
	      break;
	   case 'z':
	      (yyval.ext_swizzle).swz = SWIZZLE_Z;
	      (yyval.ext_swizzle).xyzw_valid = 1;
	      break;
	   case 'w':
	      (yyval.ext_swizzle).swz = SWIZZLE_W;
	      (yyval.ext_swizzle).xyzw_valid = 1;
	      break;

	   case 'r':
	      (yyval.ext_swizzle).swz = SWIZZLE_X;
	      (yyval.ext_swizzle).rgba_valid = 1;
	      break;
	   case 'g':
	      (yyval.ext_swizzle).swz = SWIZZLE_Y;
	      (yyval.ext_swizzle).rgba_valid = 1;
	      break;
	   case 'b':
	      (yyval.ext_swizzle).swz = SWIZZLE_Z;
	      (yyval.ext_swizzle).rgba_valid = 1;
	      break;
	   case 'a':
	      (yyval.ext_swizzle).swz = SWIZZLE_W;
	      (yyval.ext_swizzle).rgba_valid = 1;
	      break;

	   default:
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid extended swizzle selector");
	      YYERROR;
	      break;
	   }
	;}
    break;

  case 60:

/* Line 1464 of yacc.c  */
#line 775 "program/program_parse.y"
    {
	   struct asm_symbol *const s = (struct asm_symbol *)
	      _mesa_symbol_table_find_symbol(state->st, 0, (yyvsp[(1) - (1)].string));

	   free((yyvsp[(1) - (1)].string));

	   if (s == NULL) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid operand variable");
	      YYERROR;
	   } else if ((s->type != at_param) && (s->type != at_temp)
		      && (s->type != at_attrib)) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid operand variable");
	      YYERROR;
	   } else if ((s->type == at_param) && s->param_is_array) {
	      yyerror(& (yylsp[(1) - (1)]), state, "non-array access to array PARAM");
	      YYERROR;
	   }

	   init_src_reg(& (yyval.src_reg));
	   switch (s->type) {
	   case at_temp:
	      set_src_reg(& (yyval.src_reg), PROGRAM_TEMPORARY, s->temp_binding);
	      break;
	   case at_param:
              set_src_reg_swz(& (yyval.src_reg), s->param_binding_type,
                              s->param_binding_begin,
                              s->param_binding_swizzle);
	      break;
	   case at_attrib:
	      set_src_reg(& (yyval.src_reg), PROGRAM_INPUT, s->attrib_binding);
	      state->prog->InputsRead |= BITFIELD64_BIT((yyval.src_reg).Base.Index);

	      if (!validate_inputs(& (yylsp[(1) - (1)]), state)) {
		 YYERROR;
	      }
	      break;

	   default:
	      YYERROR;
	      break;
	   }
	;}
    break;

  case 61:

/* Line 1464 of yacc.c  */
#line 818 "program/program_parse.y"
    {
	   set_src_reg(& (yyval.src_reg), PROGRAM_INPUT, (yyvsp[(1) - (1)].attrib));
	   state->prog->InputsRead |= BITFIELD64_BIT((yyval.src_reg).Base.Index);

	   if (!validate_inputs(& (yylsp[(1) - (1)]), state)) {
	      YYERROR;
	   }
	;}
    break;

  case 62:

/* Line 1464 of yacc.c  */
#line 827 "program/program_parse.y"
    {
	   if (! (yyvsp[(3) - (4)].src_reg).Base.RelAddr
	       && ((unsigned) (yyvsp[(3) - (4)].src_reg).Base.Index >= (yyvsp[(1) - (4)].sym)->param_binding_length)) {
	      yyerror(& (yylsp[(3) - (4)]), state, "out of bounds array access");
	      YYERROR;
	   }

	   init_src_reg(& (yyval.src_reg));
	   (yyval.src_reg).Base.File = (yyvsp[(1) - (4)].sym)->param_binding_type;

	   if ((yyvsp[(3) - (4)].src_reg).Base.RelAddr) {
              state->prog->IndirectRegisterFiles |= (1 << (yyval.src_reg).Base.File);
	      (yyvsp[(1) - (4)].sym)->param_accessed_indirectly = 1;

	      (yyval.src_reg).Base.RelAddr = 1;
	      (yyval.src_reg).Base.Index = (yyvsp[(3) - (4)].src_reg).Base.Index;
	      (yyval.src_reg).Symbol = (yyvsp[(1) - (4)].sym);
	   } else {
	      (yyval.src_reg).Base.Index = (yyvsp[(1) - (4)].sym)->param_binding_begin + (yyvsp[(3) - (4)].src_reg).Base.Index;
	   }
	;}
    break;

  case 63:

/* Line 1464 of yacc.c  */
#line 849 "program/program_parse.y"
    {
           gl_register_file file = ((yyvsp[(1) - (1)].temp_sym).name != NULL) 
	      ? (yyvsp[(1) - (1)].temp_sym).param_binding_type
	      : PROGRAM_CONSTANT;
           set_src_reg_swz(& (yyval.src_reg), file, (yyvsp[(1) - (1)].temp_sym).param_binding_begin,
                           (yyvsp[(1) - (1)].temp_sym).param_binding_swizzle);
	;}
    break;

  case 64:

/* Line 1464 of yacc.c  */
#line 859 "program/program_parse.y"
    {
	   set_dst_reg(& (yyval.dst_reg), PROGRAM_OUTPUT, (yyvsp[(1) - (1)].result));
	;}
    break;

  case 65:

/* Line 1464 of yacc.c  */
#line 863 "program/program_parse.y"
    {
	   struct asm_symbol *const s = (struct asm_symbol *)
	      _mesa_symbol_table_find_symbol(state->st, 0, (yyvsp[(1) - (1)].string));

	   free((yyvsp[(1) - (1)].string));

	   if (s == NULL) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid operand variable");
	      YYERROR;
	   } else if ((s->type != at_output) && (s->type != at_temp)) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid operand variable");
	      YYERROR;
	   }

	   switch (s->type) {
	   case at_temp:
	      set_dst_reg(& (yyval.dst_reg), PROGRAM_TEMPORARY, s->temp_binding);
	      break;
	   case at_output:
	      set_dst_reg(& (yyval.dst_reg), PROGRAM_OUTPUT, s->output_binding);
	      break;
	   default:
	      set_dst_reg(& (yyval.dst_reg), s->param_binding_type, s->param_binding_begin);
	      break;
	   }
	;}
    break;

  case 66:

/* Line 1464 of yacc.c  */
#line 892 "program/program_parse.y"
    {
	   struct asm_symbol *const s = (struct asm_symbol *)
	      _mesa_symbol_table_find_symbol(state->st, 0, (yyvsp[(1) - (1)].string));

	   free((yyvsp[(1) - (1)].string));

	   if (s == NULL) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid operand variable");
	      YYERROR;
	   } else if ((s->type != at_param) || !s->param_is_array) {
	      yyerror(& (yylsp[(1) - (1)]), state, "array access to non-PARAM variable");
	      YYERROR;
	   } else {
	      (yyval.sym) = s;
	   }
	;}
    break;

  case 69:

/* Line 1464 of yacc.c  */
#line 913 "program/program_parse.y"
    {
	   init_src_reg(& (yyval.src_reg));
	   (yyval.src_reg).Base.Index = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 70:

/* Line 1464 of yacc.c  */
#line 920 "program/program_parse.y"
    {
	   /* FINISHME: Add support for multiple address registers.
	    */
	   /* FINISHME: Add support for 4-component address registers.
	    */
	   init_src_reg(& (yyval.src_reg));
	   (yyval.src_reg).Base.RelAddr = 1;
	   (yyval.src_reg).Base.Index = (yyvsp[(3) - (3)].integer);
	;}
    break;

  case 71:

/* Line 1464 of yacc.c  */
#line 931 "program/program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 72:

/* Line 1464 of yacc.c  */
#line 932 "program/program_parse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); ;}
    break;

  case 73:

/* Line 1464 of yacc.c  */
#line 933 "program/program_parse.y"
    { (yyval.integer) = -(yyvsp[(2) - (2)].integer); ;}
    break;

  case 74:

/* Line 1464 of yacc.c  */
#line 937 "program/program_parse.y"
    {
	   if (((yyvsp[(1) - (1)].integer) < 0) || ((yyvsp[(1) - (1)].integer) > (state->limits->MaxAddressOffset - 1))) {
              char s[100];
              _mesa_snprintf(s, sizeof(s),
                             "relative address offset too large (%d)", (yyvsp[(1) - (1)].integer));
	      yyerror(& (yylsp[(1) - (1)]), state, s);
	      YYERROR;
	   } else {
	      (yyval.integer) = (yyvsp[(1) - (1)].integer);
	   }
	;}
    break;

  case 75:

/* Line 1464 of yacc.c  */
#line 951 "program/program_parse.y"
    {
	   if (((yyvsp[(1) - (1)].integer) < 0) || ((yyvsp[(1) - (1)].integer) > state->limits->MaxAddressOffset)) {
              char s[100];
              _mesa_snprintf(s, sizeof(s),
                             "relative address offset too large (%d)", (yyvsp[(1) - (1)].integer));
	      yyerror(& (yylsp[(1) - (1)]), state, s);
	      YYERROR;
	   } else {
	      (yyval.integer) = (yyvsp[(1) - (1)].integer);
	   }
	;}
    break;

  case 76:

/* Line 1464 of yacc.c  */
#line 965 "program/program_parse.y"
    {
	   struct asm_symbol *const s = (struct asm_symbol *)
	      _mesa_symbol_table_find_symbol(state->st, 0, (yyvsp[(1) - (1)].string));

	   free((yyvsp[(1) - (1)].string));

	   if (s == NULL) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid array member");
	      YYERROR;
	   } else if (s->type != at_address) {
	      yyerror(& (yylsp[(1) - (1)]), state,
		      "invalid variable for indexed array access");
	      YYERROR;
	   } else {
	      (yyval.sym) = s;
	   }
	;}
    break;

  case 77:

/* Line 1464 of yacc.c  */
#line 985 "program/program_parse.y"
    {
	   if ((yyvsp[(1) - (1)].swiz_mask).mask != WRITEMASK_X) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid address component selector");
	      YYERROR;
	   } else {
	      (yyval.swiz_mask) = (yyvsp[(1) - (1)].swiz_mask);
	   }
	;}
    break;

  case 78:

/* Line 1464 of yacc.c  */
#line 996 "program/program_parse.y"
    {
	   if ((yyvsp[(1) - (1)].swiz_mask).mask != WRITEMASK_X) {
	      yyerror(& (yylsp[(1) - (1)]), state,
		      "address register write mask must be \".x\"");
	      YYERROR;
	   } else {
	      (yyval.swiz_mask) = (yyvsp[(1) - (1)].swiz_mask);
	   }
	;}
    break;

  case 83:

/* Line 1464 of yacc.c  */
#line 1012 "program/program_parse.y"
    { (yyval.swiz_mask).swizzle = SWIZZLE_NOOP; (yyval.swiz_mask).mask = WRITEMASK_XYZW; ;}
    break;

  case 88:

/* Line 1464 of yacc.c  */
#line 1016 "program/program_parse.y"
    { (yyval.swiz_mask).swizzle = SWIZZLE_NOOP; (yyval.swiz_mask).mask = WRITEMASK_XYZW; ;}
    break;

  case 89:

/* Line 1464 of yacc.c  */
#line 1020 "program/program_parse.y"
    {
	   (yyval.dst_reg) = (yyvsp[(2) - (3)].dst_reg);
	;}
    break;

  case 90:

/* Line 1464 of yacc.c  */
#line 1024 "program/program_parse.y"
    {
	   (yyval.dst_reg) = (yyvsp[(2) - (3)].dst_reg);
	;}
    break;

  case 91:

/* Line 1464 of yacc.c  */
#line 1028 "program/program_parse.y"
    {
	   (yyval.dst_reg).CondMask = COND_TR;
	   (yyval.dst_reg).CondSwizzle = SWIZZLE_NOOP;
	   (yyval.dst_reg).CondSrc = 0;
	;}
    break;

  case 92:

/* Line 1464 of yacc.c  */
#line 1036 "program/program_parse.y"
    {
	   (yyval.dst_reg) = (yyvsp[(1) - (2)].dst_reg);
	   (yyval.dst_reg).CondSwizzle = (yyvsp[(2) - (2)].swiz_mask).swizzle;
	;}
    break;

  case 93:

/* Line 1464 of yacc.c  */
#line 1043 "program/program_parse.y"
    {
	   (yyval.dst_reg) = (yyvsp[(1) - (2)].dst_reg);
	   (yyval.dst_reg).CondSwizzle = (yyvsp[(2) - (2)].swiz_mask).swizzle;
	;}
    break;

  case 94:

/* Line 1464 of yacc.c  */
#line 1050 "program/program_parse.y"
    {
	   const int cond = _mesa_parse_cc((yyvsp[(1) - (1)].string));
	   if ((cond == 0) || ((yyvsp[(1) - (1)].string)[2] != '\0')) {
	      char *const err_str =
		 make_error_string("invalid condition code \"%s\"", (yyvsp[(1) - (1)].string));

	      yyerror(& (yylsp[(1) - (1)]), state, (err_str != NULL)
		      ? err_str : "invalid condition code");

	      if (err_str != NULL) {
		 free(err_str);
	      }

	      YYERROR;
	   }

	   (yyval.dst_reg).CondMask = cond;
	   (yyval.dst_reg).CondSwizzle = SWIZZLE_NOOP;
	   (yyval.dst_reg).CondSrc = 0;
	;}
    break;

  case 95:

/* Line 1464 of yacc.c  */
#line 1073 "program/program_parse.y"
    {
	   const int cond = _mesa_parse_cc((yyvsp[(1) - (1)].string));
	   if ((cond == 0) || ((yyvsp[(1) - (1)].string)[2] != '\0')) {
	      char *const err_str =
		 make_error_string("invalid condition code \"%s\"", (yyvsp[(1) - (1)].string));

	      yyerror(& (yylsp[(1) - (1)]), state, (err_str != NULL)
		      ? err_str : "invalid condition code");

	      if (err_str != NULL) {
		 free(err_str);
	      }

	      YYERROR;
	   }

	   (yyval.dst_reg).CondMask = cond;
	   (yyval.dst_reg).CondSwizzle = SWIZZLE_NOOP;
	   (yyval.dst_reg).CondSrc = 0;
	;}
    break;

  case 102:

/* Line 1464 of yacc.c  */
#line 1104 "program/program_parse.y"
    {
	   struct asm_symbol *const s =
	      declare_variable(state, (yyvsp[(2) - (4)].string), at_attrib, & (yylsp[(2) - (4)]));

	   if (s == NULL) {
	      free((yyvsp[(2) - (4)].string));
	      YYERROR;
	   } else {
	      s->attrib_binding = (yyvsp[(4) - (4)].attrib);
	      state->InputsBound |= BITFIELD64_BIT(s->attrib_binding);

	      if (!validate_inputs(& (yylsp[(4) - (4)]), state)) {
		 YYERROR;
	      }
	   }
	;}
    break;

  case 103:

/* Line 1464 of yacc.c  */
#line 1123 "program/program_parse.y"
    {
	   (yyval.attrib) = (yyvsp[(2) - (2)].attrib);
	;}
    break;

  case 104:

/* Line 1464 of yacc.c  */
#line 1127 "program/program_parse.y"
    {
	   (yyval.attrib) = (yyvsp[(2) - (2)].attrib);
	;}
    break;

  case 105:

/* Line 1464 of yacc.c  */
#line 1133 "program/program_parse.y"
    {
	   (yyval.attrib) = VERT_ATTRIB_POS;
	;}
    break;

  case 106:

/* Line 1464 of yacc.c  */
#line 1137 "program/program_parse.y"
    {
	   (yyval.attrib) = VERT_ATTRIB_WEIGHT;
	;}
    break;

  case 107:

/* Line 1464 of yacc.c  */
#line 1141 "program/program_parse.y"
    {
	   (yyval.attrib) = VERT_ATTRIB_NORMAL;
	;}
    break;

  case 108:

/* Line 1464 of yacc.c  */
#line 1145 "program/program_parse.y"
    {
	   if (!state->ctx->Extensions.EXT_secondary_color) {
	      yyerror(& (yylsp[(2) - (2)]), state, "GL_EXT_secondary_color not supported");
	      YYERROR;
	   }

	   (yyval.attrib) = VERT_ATTRIB_COLOR0 + (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 109:

/* Line 1464 of yacc.c  */
#line 1154 "program/program_parse.y"
    {
	   if (!state->ctx->Extensions.EXT_fog_coord) {
	      yyerror(& (yylsp[(1) - (1)]), state, "GL_EXT_fog_coord not supported");
	      YYERROR;
	   }

	   (yyval.attrib) = VERT_ATTRIB_FOG;
	;}
    break;

  case 110:

/* Line 1464 of yacc.c  */
#line 1163 "program/program_parse.y"
    {
	   (yyval.attrib) = VERT_ATTRIB_TEX0 + (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 111:

/* Line 1464 of yacc.c  */
#line 1167 "program/program_parse.y"
    {
	   yyerror(& (yylsp[(1) - (4)]), state, "GL_ARB_matrix_palette not supported");
	   YYERROR;
	;}
    break;

  case 112:

/* Line 1464 of yacc.c  */
#line 1172 "program/program_parse.y"
    {
	   (yyval.attrib) = VERT_ATTRIB_GENERIC0 + (yyvsp[(3) - (4)].integer);
	;}
    break;

  case 113:

/* Line 1464 of yacc.c  */
#line 1178 "program/program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->limits->MaxAttribs) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid vertex attribute reference");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 117:

/* Line 1464 of yacc.c  */
#line 1192 "program/program_parse.y"
    {
	   (yyval.attrib) = FRAG_ATTRIB_WPOS;
	;}
    break;

  case 118:

/* Line 1464 of yacc.c  */
#line 1196 "program/program_parse.y"
    {
	   (yyval.attrib) = FRAG_ATTRIB_COL0 + (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 119:

/* Line 1464 of yacc.c  */
#line 1200 "program/program_parse.y"
    {
	   (yyval.attrib) = FRAG_ATTRIB_FOGC;
	;}
    break;

  case 120:

/* Line 1464 of yacc.c  */
#line 1204 "program/program_parse.y"
    {
	   (yyval.attrib) = FRAG_ATTRIB_TEX0 + (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 123:

/* Line 1464 of yacc.c  */
#line 1212 "program/program_parse.y"
    {
	   struct asm_symbol *const s =
	      declare_variable(state, (yyvsp[(2) - (3)].string), at_param, & (yylsp[(2) - (3)]));

	   if (s == NULL) {
	      free((yyvsp[(2) - (3)].string));
	      YYERROR;
	   } else {
	      s->param_binding_type = (yyvsp[(3) - (3)].temp_sym).param_binding_type;
	      s->param_binding_begin = (yyvsp[(3) - (3)].temp_sym).param_binding_begin;
	      s->param_binding_length = (yyvsp[(3) - (3)].temp_sym).param_binding_length;
              s->param_binding_swizzle = (yyvsp[(3) - (3)].temp_sym).param_binding_swizzle;
	      s->param_is_array = 0;
	   }
	;}
    break;

  case 124:

/* Line 1464 of yacc.c  */
#line 1230 "program/program_parse.y"
    {
	   if (((yyvsp[(4) - (6)].integer) != 0) && ((unsigned) (yyvsp[(4) - (6)].integer) != (yyvsp[(6) - (6)].temp_sym).param_binding_length)) {
	      free((yyvsp[(2) - (6)].string));
	      yyerror(& (yylsp[(4) - (6)]), state, 
		      "parameter array size and number of bindings must match");
	      YYERROR;
	   } else {
	      struct asm_symbol *const s =
		 declare_variable(state, (yyvsp[(2) - (6)].string), (yyvsp[(6) - (6)].temp_sym).type, & (yylsp[(2) - (6)]));

	      if (s == NULL) {
		 free((yyvsp[(2) - (6)].string));
		 YYERROR;
	      } else {
		 s->param_binding_type = (yyvsp[(6) - (6)].temp_sym).param_binding_type;
		 s->param_binding_begin = (yyvsp[(6) - (6)].temp_sym).param_binding_begin;
		 s->param_binding_length = (yyvsp[(6) - (6)].temp_sym).param_binding_length;
                 s->param_binding_swizzle = SWIZZLE_XYZW;
		 s->param_is_array = 1;
	      }
	   }
	;}
    break;

  case 125:

/* Line 1464 of yacc.c  */
#line 1255 "program/program_parse.y"
    {
	   (yyval.integer) = 0;
	;}
    break;

  case 126:

/* Line 1464 of yacc.c  */
#line 1259 "program/program_parse.y"
    {
	   if (((yyvsp[(1) - (1)].integer) < 1) || ((unsigned) (yyvsp[(1) - (1)].integer) > state->limits->MaxParameters)) {
              char msg[100];
              _mesa_snprintf(msg, sizeof(msg),
                             "invalid parameter array size (size=%d max=%u)",
                             (yyvsp[(1) - (1)].integer), state->limits->MaxParameters);
	      yyerror(& (yylsp[(1) - (1)]), state, msg);
	      YYERROR;
	   } else {
	      (yyval.integer) = (yyvsp[(1) - (1)].integer);
	   }
	;}
    break;

  case 127:

/* Line 1464 of yacc.c  */
#line 1274 "program/program_parse.y"
    {
	   (yyval.temp_sym) = (yyvsp[(2) - (2)].temp_sym);
	;}
    break;

  case 128:

/* Line 1464 of yacc.c  */
#line 1280 "program/program_parse.y"
    {
	   (yyval.temp_sym) = (yyvsp[(3) - (4)].temp_sym);
	;}
    break;

  case 130:

/* Line 1464 of yacc.c  */
#line 1287 "program/program_parse.y"
    {
	   (yyvsp[(1) - (3)].temp_sym).param_binding_length += (yyvsp[(3) - (3)].temp_sym).param_binding_length;
	   (yyval.temp_sym) = (yyvsp[(1) - (3)].temp_sym);
	;}
    break;

  case 131:

/* Line 1464 of yacc.c  */
#line 1294 "program/program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_state(state->prog, & (yyval.temp_sym), (yyvsp[(1) - (1)].state));
	;}
    break;

  case 132:

/* Line 1464 of yacc.c  */
#line 1300 "program/program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_param(state->prog, & (yyval.temp_sym), (yyvsp[(1) - (1)].state));
	;}
    break;

  case 133:

/* Line 1464 of yacc.c  */
#line 1306 "program/program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_const(state->prog, & (yyval.temp_sym), & (yyvsp[(1) - (1)].vector), GL_TRUE);
	;}
    break;

  case 134:

/* Line 1464 of yacc.c  */
#line 1314 "program/program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_state(state->prog, & (yyval.temp_sym), (yyvsp[(1) - (1)].state));
	;}
    break;

  case 135:

/* Line 1464 of yacc.c  */
#line 1320 "program/program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_param(state->prog, & (yyval.temp_sym), (yyvsp[(1) - (1)].state));
	;}
    break;

  case 136:

/* Line 1464 of yacc.c  */
#line 1326 "program/program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_const(state->prog, & (yyval.temp_sym), & (yyvsp[(1) - (1)].vector), GL_TRUE);
	;}
    break;

  case 137:

/* Line 1464 of yacc.c  */
#line 1334 "program/program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_state(state->prog, & (yyval.temp_sym), (yyvsp[(1) - (1)].state));
	;}
    break;

  case 138:

/* Line 1464 of yacc.c  */
#line 1340 "program/program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_param(state->prog, & (yyval.temp_sym), (yyvsp[(1) - (1)].state));
	;}
    break;

  case 139:

/* Line 1464 of yacc.c  */
#line 1346 "program/program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_const(state->prog, & (yyval.temp_sym), & (yyvsp[(1) - (1)].vector), GL_FALSE);
	;}
    break;

  case 140:

/* Line 1464 of yacc.c  */
#line 1353 "program/program_parse.y"
    { memcpy((yyval.state), (yyvsp[(1) - (1)].state), sizeof((yyval.state))); ;}
    break;

  case 141:

/* Line 1464 of yacc.c  */
#line 1354 "program/program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 142:

/* Line 1464 of yacc.c  */
#line 1357 "program/program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 143:

/* Line 1464 of yacc.c  */
#line 1358 "program/program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 144:

/* Line 1464 of yacc.c  */
#line 1359 "program/program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 145:

/* Line 1464 of yacc.c  */
#line 1360 "program/program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 146:

/* Line 1464 of yacc.c  */
#line 1361 "program/program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 147:

/* Line 1464 of yacc.c  */
#line 1362 "program/program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 148:

/* Line 1464 of yacc.c  */
#line 1363 "program/program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 149:

/* Line 1464 of yacc.c  */
#line 1364 "program/program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 150:

/* Line 1464 of yacc.c  */
#line 1365 "program/program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 151:

/* Line 1464 of yacc.c  */
#line 1366 "program/program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 152:

/* Line 1464 of yacc.c  */
#line 1367 "program/program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 153:

/* Line 1464 of yacc.c  */
#line 1371 "program/program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_MATERIAL;
	   (yyval.state)[1] = (yyvsp[(2) - (3)].integer);
	   (yyval.state)[2] = (yyvsp[(3) - (3)].integer);
	;}
    break;

  case 154:

/* Line 1464 of yacc.c  */
#line 1380 "program/program_parse.y"
    {
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 155:

/* Line 1464 of yacc.c  */
#line 1384 "program/program_parse.y"
    {
	   (yyval.integer) = STATE_EMISSION;
	;}
    break;

  case 156:

/* Line 1464 of yacc.c  */
#line 1388 "program/program_parse.y"
    {
	   (yyval.integer) = STATE_SHININESS;
	;}
    break;

  case 157:

/* Line 1464 of yacc.c  */
#line 1394 "program/program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_LIGHT;
	   (yyval.state)[1] = (yyvsp[(3) - (5)].integer);
	   (yyval.state)[2] = (yyvsp[(5) - (5)].integer);
	;}
    break;

  case 158:

/* Line 1464 of yacc.c  */
#line 1403 "program/program_parse.y"
    {
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 159:

/* Line 1464 of yacc.c  */
#line 1407 "program/program_parse.y"
    {
	   (yyval.integer) = STATE_POSITION;
	;}
    break;

  case 160:

/* Line 1464 of yacc.c  */
#line 1411 "program/program_parse.y"
    {
	   if (!state->ctx->Extensions.EXT_point_parameters) {
	      yyerror(& (yylsp[(1) - (1)]), state, "GL_ARB_point_parameters not supported");
	      YYERROR;
	   }

	   (yyval.integer) = STATE_ATTENUATION;
	;}
    break;

  case 161:

/* Line 1464 of yacc.c  */
#line 1420 "program/program_parse.y"
    {
	   (yyval.integer) = (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 162:

/* Line 1464 of yacc.c  */
#line 1424 "program/program_parse.y"
    {
	   (yyval.integer) = STATE_HALF_VECTOR;
	;}
    break;

  case 163:

/* Line 1464 of yacc.c  */
#line 1430 "program/program_parse.y"
    {
	   (yyval.integer) = STATE_SPOT_DIRECTION;
	;}
    break;

  case 164:

/* Line 1464 of yacc.c  */
#line 1436 "program/program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(2) - (2)].state)[0];
	   (yyval.state)[1] = (yyvsp[(2) - (2)].state)[1];
	;}
    break;

  case 165:

/* Line 1464 of yacc.c  */
#line 1443 "program/program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_LIGHTMODEL_AMBIENT;
	;}
    break;

  case 166:

/* Line 1464 of yacc.c  */
#line 1448 "program/program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_LIGHTMODEL_SCENECOLOR;
	   (yyval.state)[1] = (yyvsp[(1) - (2)].integer);
	;}
    break;

  case 167:

/* Line 1464 of yacc.c  */
#line 1456 "program/program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_LIGHTPROD;
	   (yyval.state)[1] = (yyvsp[(3) - (6)].integer);
	   (yyval.state)[2] = (yyvsp[(5) - (6)].integer);
	   (yyval.state)[3] = (yyvsp[(6) - (6)].integer);
	;}
    break;

  case 169:

/* Line 1464 of yacc.c  */
#line 1468 "program/program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = (yyvsp[(3) - (3)].integer);
	   (yyval.state)[1] = (yyvsp[(2) - (3)].integer);
	;}
    break;

  case 170:

/* Line 1464 of yacc.c  */
#line 1476 "program/program_parse.y"
    {
	   (yyval.integer) = STATE_TEXENV_COLOR;
	;}
    break;

  case 171:

/* Line 1464 of yacc.c  */
#line 1482 "program/program_parse.y"
    {
	   (yyval.integer) = STATE_AMBIENT;
	;}
    break;

  case 172:

/* Line 1464 of yacc.c  */
#line 1486 "program/program_parse.y"
    {
	   (yyval.integer) = STATE_DIFFUSE;
	;}
    break;

  case 173:

/* Line 1464 of yacc.c  */
#line 1490 "program/program_parse.y"
    {
	   (yyval.integer) = STATE_SPECULAR;
	;}
    break;

  case 174:

/* Line 1464 of yacc.c  */
#line 1496 "program/program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->MaxLights) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid light selector");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 175:

/* Line 1464 of yacc.c  */
#line 1507 "program/program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_TEXGEN;
	   (yyval.state)[1] = (yyvsp[(2) - (4)].integer);
	   (yyval.state)[2] = (yyvsp[(3) - (4)].integer) + (yyvsp[(4) - (4)].integer);
	;}
    break;

  case 176:

/* Line 1464 of yacc.c  */
#line 1516 "program/program_parse.y"
    {
	   (yyval.integer) = STATE_TEXGEN_EYE_S;
	;}
    break;

  case 177:

/* Line 1464 of yacc.c  */
#line 1520 "program/program_parse.y"
    {
	   (yyval.integer) = STATE_TEXGEN_OBJECT_S;
	;}
    break;

  case 178:

/* Line 1464 of yacc.c  */
#line 1525 "program/program_parse.y"
    {
	   (yyval.integer) = STATE_TEXGEN_EYE_S - STATE_TEXGEN_EYE_S;
	;}
    break;

  case 179:

/* Line 1464 of yacc.c  */
#line 1529 "program/program_parse.y"
    {
	   (yyval.integer) = STATE_TEXGEN_EYE_T - STATE_TEXGEN_EYE_S;
	;}
    break;

  case 180:

/* Line 1464 of yacc.c  */
#line 1533 "program/program_parse.y"
    {
	   (yyval.integer) = STATE_TEXGEN_EYE_R - STATE_TEXGEN_EYE_S;
	;}
    break;

  case 181:

/* Line 1464 of yacc.c  */
#line 1537 "program/program_parse.y"
    {
	   (yyval.integer) = STATE_TEXGEN_EYE_Q - STATE_TEXGEN_EYE_S;
	;}
    break;

  case 182:

/* Line 1464 of yacc.c  */
#line 1543 "program/program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 183:

/* Line 1464 of yacc.c  */
#line 1550 "program/program_parse.y"
    {
	   (yyval.integer) = STATE_FOG_COLOR;
	;}
    break;

  case 184:

/* Line 1464 of yacc.c  */
#line 1554 "program/program_parse.y"
    {
	   (yyval.integer) = STATE_FOG_PARAMS;
	;}
    break;

  case 185:

/* Line 1464 of yacc.c  */
#line 1560 "program/program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_CLIPPLANE;
	   (yyval.state)[1] = (yyvsp[(3) - (5)].integer);
	;}
    break;

  case 186:

/* Line 1464 of yacc.c  */
#line 1568 "program/program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->MaxClipPlanes) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid clip plane selector");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 187:

/* Line 1464 of yacc.c  */
#line 1579 "program/program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 188:

/* Line 1464 of yacc.c  */
#line 1586 "program/program_parse.y"
    {
	   (yyval.integer) = STATE_POINT_SIZE;
	;}
    break;

  case 189:

/* Line 1464 of yacc.c  */
#line 1590 "program/program_parse.y"
    {
	   (yyval.integer) = STATE_POINT_ATTENUATION;
	;}
    break;

  case 190:

/* Line 1464 of yacc.c  */
#line 1596 "program/program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(1) - (5)].state)[0];
	   (yyval.state)[1] = (yyvsp[(1) - (5)].state)[1];
	   (yyval.state)[2] = (yyvsp[(4) - (5)].integer);
	   (yyval.state)[3] = (yyvsp[(4) - (5)].integer);
	   (yyval.state)[4] = (yyvsp[(1) - (5)].state)[2];
	;}
    break;

  case 191:

/* Line 1464 of yacc.c  */
#line 1606 "program/program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(1) - (2)].state)[0];
	   (yyval.state)[1] = (yyvsp[(1) - (2)].state)[1];
	   (yyval.state)[2] = (yyvsp[(2) - (2)].state)[2];
	   (yyval.state)[3] = (yyvsp[(2) - (2)].state)[3];
	   (yyval.state)[4] = (yyvsp[(1) - (2)].state)[2];
	;}
    break;

  case 192:

/* Line 1464 of yacc.c  */
#line 1616 "program/program_parse.y"
    {
	   (yyval.state)[2] = 0;
	   (yyval.state)[3] = 3;
	;}
    break;

  case 193:

/* Line 1464 of yacc.c  */
#line 1621 "program/program_parse.y"
    {
	   /* It seems logical that the matrix row range specifier would have
	    * to specify a range or more than one row (i.e., $5 > $3).
	    * However, the ARB_vertex_program spec says "a program will fail
	    * to load if <a> is greater than <b>."  This means that $3 == $5
	    * is valid.
	    */
	   if ((yyvsp[(3) - (6)].integer) > (yyvsp[(5) - (6)].integer)) {
	      yyerror(& (yylsp[(3) - (6)]), state, "invalid matrix row range");
	      YYERROR;
	   }

	   (yyval.state)[2] = (yyvsp[(3) - (6)].integer);
	   (yyval.state)[3] = (yyvsp[(5) - (6)].integer);
	;}
    break;

  case 194:

/* Line 1464 of yacc.c  */
#line 1639 "program/program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(2) - (3)].state)[0];
	   (yyval.state)[1] = (yyvsp[(2) - (3)].state)[1];
	   (yyval.state)[2] = (yyvsp[(3) - (3)].integer);
	;}
    break;

  case 195:

/* Line 1464 of yacc.c  */
#line 1647 "program/program_parse.y"
    {
	   (yyval.integer) = 0;
	;}
    break;

  case 196:

/* Line 1464 of yacc.c  */
#line 1651 "program/program_parse.y"
    {
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 197:

/* Line 1464 of yacc.c  */
#line 1657 "program/program_parse.y"
    {
	   (yyval.integer) = STATE_MATRIX_INVERSE;
	;}
    break;

  case 198:

/* Line 1464 of yacc.c  */
#line 1661 "program/program_parse.y"
    {
	   (yyval.integer) = STATE_MATRIX_TRANSPOSE;
	;}
    break;

  case 199:

/* Line 1464 of yacc.c  */
#line 1665 "program/program_parse.y"
    {
	   (yyval.integer) = STATE_MATRIX_INVTRANS;
	;}
    break;

  case 200:

/* Line 1464 of yacc.c  */
#line 1671 "program/program_parse.y"
    {
	   if ((yyvsp[(1) - (1)].integer) > 3) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid matrix row reference");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 201:

/* Line 1464 of yacc.c  */
#line 1682 "program/program_parse.y"
    {
	   (yyval.state)[0] = STATE_MODELVIEW_MATRIX;
	   (yyval.state)[1] = (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 202:

/* Line 1464 of yacc.c  */
#line 1687 "program/program_parse.y"
    {
	   (yyval.state)[0] = STATE_PROJECTION_MATRIX;
	   (yyval.state)[1] = 0;
	;}
    break;

  case 203:

/* Line 1464 of yacc.c  */
#line 1692 "program/program_parse.y"
    {
	   (yyval.state)[0] = STATE_MVP_MATRIX;
	   (yyval.state)[1] = 0;
	;}
    break;

  case 204:

/* Line 1464 of yacc.c  */
#line 1697 "program/program_parse.y"
    {
	   (yyval.state)[0] = STATE_TEXTURE_MATRIX;
	   (yyval.state)[1] = (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 205:

/* Line 1464 of yacc.c  */
#line 1702 "program/program_parse.y"
    {
	   yyerror(& (yylsp[(1) - (4)]), state, "GL_ARB_matrix_palette not supported");
	   YYERROR;
	;}
    break;

  case 206:

/* Line 1464 of yacc.c  */
#line 1707 "program/program_parse.y"
    {
	   (yyval.state)[0] = STATE_PROGRAM_MATRIX;
	   (yyval.state)[1] = (yyvsp[(3) - (4)].integer);
	;}
    break;

  case 207:

/* Line 1464 of yacc.c  */
#line 1714 "program/program_parse.y"
    {
	   (yyval.integer) = 0;
	;}
    break;

  case 208:

/* Line 1464 of yacc.c  */
#line 1718 "program/program_parse.y"
    {
	   (yyval.integer) = (yyvsp[(2) - (3)].integer);
	;}
    break;

  case 209:

/* Line 1464 of yacc.c  */
#line 1723 "program/program_parse.y"
    {
	   /* Since GL_ARB_vertex_blend isn't supported, only modelview matrix
	    * zero is valid.
	    */
	   if ((yyvsp[(1) - (1)].integer) != 0) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid modelview matrix index");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 210:

/* Line 1464 of yacc.c  */
#line 1736 "program/program_parse.y"
    {
	   /* Since GL_ARB_matrix_palette isn't supported, just let any value
	    * through here.  The error will be generated later.
	    */
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 211:

/* Line 1464 of yacc.c  */
#line 1744 "program/program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->MaxProgramMatrices) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid program matrix selector");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 212:

/* Line 1464 of yacc.c  */
#line 1755 "program/program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_DEPTH_RANGE;
	;}
    break;

  case 217:

/* Line 1464 of yacc.c  */
#line 1767 "program/program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = state->state_param_enum;
	   (yyval.state)[1] = STATE_ENV;
	   (yyval.state)[2] = (yyvsp[(4) - (5)].state)[0];
	   (yyval.state)[3] = (yyvsp[(4) - (5)].state)[1];
	;}
    break;

  case 218:

/* Line 1464 of yacc.c  */
#line 1777 "program/program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(1) - (1)].integer);
	   (yyval.state)[1] = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 219:

/* Line 1464 of yacc.c  */
#line 1782 "program/program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(1) - (3)].integer);
	   (yyval.state)[1] = (yyvsp[(3) - (3)].integer);
	;}
    break;

  case 220:

/* Line 1464 of yacc.c  */
#line 1789 "program/program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = state->state_param_enum;
	   (yyval.state)[1] = STATE_ENV;
	   (yyval.state)[2] = (yyvsp[(4) - (5)].integer);
	   (yyval.state)[3] = (yyvsp[(4) - (5)].integer);
	;}
    break;

  case 221:

/* Line 1464 of yacc.c  */
#line 1799 "program/program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = state->state_param_enum;
	   (yyval.state)[1] = STATE_LOCAL;
	   (yyval.state)[2] = (yyvsp[(4) - (5)].state)[0];
	   (yyval.state)[3] = (yyvsp[(4) - (5)].state)[1];
	;}
    break;

  case 222:

/* Line 1464 of yacc.c  */
#line 1808 "program/program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(1) - (1)].integer);
	   (yyval.state)[1] = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 223:

/* Line 1464 of yacc.c  */
#line 1813 "program/program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(1) - (3)].integer);
	   (yyval.state)[1] = (yyvsp[(3) - (3)].integer);
	;}
    break;

  case 224:

/* Line 1464 of yacc.c  */
#line 1820 "program/program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = state->state_param_enum;
	   (yyval.state)[1] = STATE_LOCAL;
	   (yyval.state)[2] = (yyvsp[(4) - (5)].integer);
	   (yyval.state)[3] = (yyvsp[(4) - (5)].integer);
	;}
    break;

  case 225:

/* Line 1464 of yacc.c  */
#line 1830 "program/program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->limits->MaxEnvParams) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid environment parameter reference");
	      YYERROR;
	   }
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 226:

/* Line 1464 of yacc.c  */
#line 1840 "program/program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->limits->MaxLocalParams) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid local parameter reference");
	      YYERROR;
	   }
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 231:

/* Line 1464 of yacc.c  */
#line 1855 "program/program_parse.y"
    {
	   (yyval.vector).count = 4;
	   (yyval.vector).data[0].f = (yyvsp[(1) - (1)].real);
	   (yyval.vector).data[1].f = (yyvsp[(1) - (1)].real);
	   (yyval.vector).data[2].f = (yyvsp[(1) - (1)].real);
	   (yyval.vector).data[3].f = (yyvsp[(1) - (1)].real);
	;}
    break;

  case 232:

/* Line 1464 of yacc.c  */
#line 1865 "program/program_parse.y"
    {
	   (yyval.vector).count = 1;
	   (yyval.vector).data[0].f = (yyvsp[(1) - (1)].real);
	   (yyval.vector).data[1].f = (yyvsp[(1) - (1)].real);
	   (yyval.vector).data[2].f = (yyvsp[(1) - (1)].real);
	   (yyval.vector).data[3].f = (yyvsp[(1) - (1)].real);
	;}
    break;

  case 233:

/* Line 1464 of yacc.c  */
#line 1873 "program/program_parse.y"
    {
	   (yyval.vector).count = 1;
	   (yyval.vector).data[0].f = (float) (yyvsp[(1) - (1)].integer);
	   (yyval.vector).data[1].f = (float) (yyvsp[(1) - (1)].integer);
	   (yyval.vector).data[2].f = (float) (yyvsp[(1) - (1)].integer);
	   (yyval.vector).data[3].f = (float) (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 234:

/* Line 1464 of yacc.c  */
#line 1883 "program/program_parse.y"
    {
	   (yyval.vector).count = 4;
	   (yyval.vector).data[0].f = (yyvsp[(2) - (3)].real);
	   (yyval.vector).data[1].f = 0.0f;
	   (yyval.vector).data[2].f = 0.0f;
	   (yyval.vector).data[3].f = 1.0f;
	;}
    break;

  case 235:

/* Line 1464 of yacc.c  */
#line 1891 "program/program_parse.y"
    {
	   (yyval.vector).count = 4;
	   (yyval.vector).data[0].f = (yyvsp[(2) - (5)].real);
	   (yyval.vector).data[1].f = (yyvsp[(4) - (5)].real);
	   (yyval.vector).data[2].f = 0.0f;
	   (yyval.vector).data[3].f = 1.0f;
	;}
    break;

  case 236:

/* Line 1464 of yacc.c  */
#line 1900 "program/program_parse.y"
    {
	   (yyval.vector).count = 4;
	   (yyval.vector).data[0].f = (yyvsp[(2) - (7)].real);
	   (yyval.vector).data[1].f = (yyvsp[(4) - (7)].real);
	   (yyval.vector).data[2].f = (yyvsp[(6) - (7)].real);
	   (yyval.vector).data[3].f = 1.0f;
	;}
    break;

  case 237:

/* Line 1464 of yacc.c  */
#line 1909 "program/program_parse.y"
    {
	   (yyval.vector).count = 4;
	   (yyval.vector).data[0].f = (yyvsp[(2) - (9)].real);
	   (yyval.vector).data[1].f = (yyvsp[(4) - (9)].real);
	   (yyval.vector).data[2].f = (yyvsp[(6) - (9)].real);
	   (yyval.vector).data[3].f = (yyvsp[(8) - (9)].real);
	;}
    break;

  case 238:

/* Line 1464 of yacc.c  */
#line 1919 "program/program_parse.y"
    {
	   (yyval.real) = ((yyvsp[(1) - (2)].negate)) ? -(yyvsp[(2) - (2)].real) : (yyvsp[(2) - (2)].real);
	;}
    break;

  case 239:

/* Line 1464 of yacc.c  */
#line 1923 "program/program_parse.y"
    {
	   (yyval.real) = (float)(((yyvsp[(1) - (2)].negate)) ? -(yyvsp[(2) - (2)].integer) : (yyvsp[(2) - (2)].integer));
	;}
    break;

  case 240:

/* Line 1464 of yacc.c  */
#line 1928 "program/program_parse.y"
    { (yyval.negate) = FALSE; ;}
    break;

  case 241:

/* Line 1464 of yacc.c  */
#line 1929 "program/program_parse.y"
    { (yyval.negate) = TRUE;  ;}
    break;

  case 242:

/* Line 1464 of yacc.c  */
#line 1930 "program/program_parse.y"
    { (yyval.negate) = FALSE; ;}
    break;

  case 243:

/* Line 1464 of yacc.c  */
#line 1933 "program/program_parse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); ;}
    break;

  case 245:

/* Line 1464 of yacc.c  */
#line 1937 "program/program_parse.y"
    {
	   /* NV_fragment_program_option defines the size qualifiers in a
	    * fairly broken way.  "SHORT" or "LONG" can optionally be used
	    * before TEMP or OUTPUT.  However, neither is a reserved word!
	    * This means that we have to parse it as an identifier, then check
	    * to make sure it's one of the valid values.  *sigh*
	    *
	    * In addition, the grammar in the extension spec does *not* allow
	    * the size specifier to be optional, but all known implementations
	    * do.
	    */
	   if (!state->option.NV_fragment) {
	      yyerror(& (yylsp[(1) - (1)]), state, "unexpected IDENTIFIER");
	      YYERROR;
	   }

	   if (strcmp("SHORT", (yyvsp[(1) - (1)].string)) == 0) {
	   } else if (strcmp("LONG", (yyvsp[(1) - (1)].string)) == 0) {
	   } else {
	      char *const err_str =
		 make_error_string("invalid storage size specifier \"%s\"",
				   (yyvsp[(1) - (1)].string));

	      yyerror(& (yylsp[(1) - (1)]), state, (err_str != NULL)
		      ? err_str : "invalid storage size specifier");

	      if (err_str != NULL) {
		 free(err_str);
	      }

	      YYERROR;
	   }
	;}
    break;

  case 246:

/* Line 1464 of yacc.c  */
#line 1971 "program/program_parse.y"
    {
	;}
    break;

  case 247:

/* Line 1464 of yacc.c  */
#line 1975 "program/program_parse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); ;}
    break;

  case 249:

/* Line 1464 of yacc.c  */
#line 1979 "program/program_parse.y"
    {
	   if (!declare_variable(state, (yyvsp[(3) - (3)].string), (yyvsp[(0) - (3)].integer), & (yylsp[(3) - (3)]))) {
	      free((yyvsp[(3) - (3)].string));
	      YYERROR;
	   }
	;}
    break;

  case 250:

/* Line 1464 of yacc.c  */
#line 1986 "program/program_parse.y"
    {
	   if (!declare_variable(state, (yyvsp[(1) - (1)].string), (yyvsp[(0) - (1)].integer), & (yylsp[(1) - (1)]))) {
	      free((yyvsp[(1) - (1)].string));
	      YYERROR;
	   }
	;}
    break;

  case 251:

/* Line 1464 of yacc.c  */
#line 1995 "program/program_parse.y"
    {
	   struct asm_symbol *const s =
	      declare_variable(state, (yyvsp[(3) - (5)].string), at_output, & (yylsp[(3) - (5)]));

	   if (s == NULL) {
	      free((yyvsp[(3) - (5)].string));
	      YYERROR;
	   } else {
	      s->output_binding = (yyvsp[(5) - (5)].result);
	   }
	;}
    break;

  case 252:

/* Line 1464 of yacc.c  */
#line 2009 "program/program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.result) = VERT_RESULT_HPOS;
	   } else {
	      yyerror(& (yylsp[(2) - (2)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 253:

/* Line 1464 of yacc.c  */
#line 2018 "program/program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.result) = VERT_RESULT_FOGC;
	   } else {
	      yyerror(& (yylsp[(2) - (2)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 254:

/* Line 1464 of yacc.c  */
#line 2027 "program/program_parse.y"
    {
	   (yyval.result) = (yyvsp[(2) - (2)].result);
	;}
    break;

  case 255:

/* Line 1464 of yacc.c  */
#line 2031 "program/program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.result) = VERT_RESULT_PSIZ;
	   } else {
	      yyerror(& (yylsp[(2) - (2)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 256:

/* Line 1464 of yacc.c  */
#line 2040 "program/program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.result) = VERT_RESULT_TEX0 + (yyvsp[(3) - (3)].integer);
	   } else {
	      yyerror(& (yylsp[(2) - (3)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 257:

/* Line 1464 of yacc.c  */
#line 2049 "program/program_parse.y"
    {
	   if (state->mode == ARB_fragment) {
	      (yyval.result) = FRAG_RESULT_DEPTH;
	   } else {
	      yyerror(& (yylsp[(2) - (2)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 258:

/* Line 1464 of yacc.c  */
#line 2060 "program/program_parse.y"
    {
	   (yyval.result) = (yyvsp[(2) - (3)].integer) + (yyvsp[(3) - (3)].integer);
	;}
    break;

  case 259:

/* Line 1464 of yacc.c  */
#line 2066 "program/program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.integer) = VERT_RESULT_COL0;
	   } else {
	      if (state->option.DrawBuffers)
		 (yyval.integer) = FRAG_RESULT_DATA0;
	      else
		 (yyval.integer) = FRAG_RESULT_COLOR;
	   }
	;}
    break;

  case 260:

/* Line 1464 of yacc.c  */
#line 2077 "program/program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      yyerror(& (yylsp[(1) - (3)]), state, "invalid program result name");
	      YYERROR;
	   } else {
	      if (!state->option.DrawBuffers) {
		 /* From the ARB_draw_buffers spec (same text exists
		  * for ATI_draw_buffers):
		  *
		  *     If this option is not specified, a fragment
		  *     program that attempts to bind
		  *     "result.color[n]" will fail to load, and only
		  *     "result.color" will be allowed.
		  */
		 yyerror(& (yylsp[(1) - (3)]), state,
			 "result.color[] used without "
			 "`OPTION ARB_draw_buffers' or "
			 "`OPTION ATI_draw_buffers'");
		 YYERROR;
	      } else if ((yyvsp[(2) - (3)].integer) >= state->MaxDrawBuffers) {
		 yyerror(& (yylsp[(1) - (3)]), state,
			 "result.color[] exceeds MAX_DRAW_BUFFERS_ARB");
		 YYERROR;
	      }
	      (yyval.integer) = FRAG_RESULT_DATA0 + (yyvsp[(2) - (3)].integer);
	   }
	;}
    break;

  case 261:

/* Line 1464 of yacc.c  */
#line 2105 "program/program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.integer) = VERT_RESULT_COL0;
	   } else {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 262:

/* Line 1464 of yacc.c  */
#line 2114 "program/program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.integer) = VERT_RESULT_BFC0;
	   } else {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 263:

/* Line 1464 of yacc.c  */
#line 2125 "program/program_parse.y"
    {
	   (yyval.integer) = 0; 
	;}
    break;

  case 264:

/* Line 1464 of yacc.c  */
#line 2129 "program/program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.integer) = 0;
	   } else {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 265:

/* Line 1464 of yacc.c  */
#line 2138 "program/program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.integer) = 1;
	   } else {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 266:

/* Line 1464 of yacc.c  */
#line 2148 "program/program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 267:

/* Line 1464 of yacc.c  */
#line 2149 "program/program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 268:

/* Line 1464 of yacc.c  */
#line 2150 "program/program_parse.y"
    { (yyval.integer) = 1; ;}
    break;

  case 269:

/* Line 1464 of yacc.c  */
#line 2153 "program/program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 270:

/* Line 1464 of yacc.c  */
#line 2154 "program/program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 271:

/* Line 1464 of yacc.c  */
#line 2155 "program/program_parse.y"
    { (yyval.integer) = 1; ;}
    break;

  case 272:

/* Line 1464 of yacc.c  */
#line 2158 "program/program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 273:

/* Line 1464 of yacc.c  */
#line 2159 "program/program_parse.y"
    { (yyval.integer) = (yyvsp[(2) - (3)].integer); ;}
    break;

  case 274:

/* Line 1464 of yacc.c  */
#line 2162 "program/program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 275:

/* Line 1464 of yacc.c  */
#line 2163 "program/program_parse.y"
    { (yyval.integer) = (yyvsp[(2) - (3)].integer); ;}
    break;

  case 276:

/* Line 1464 of yacc.c  */
#line 2166 "program/program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 277:

/* Line 1464 of yacc.c  */
#line 2167 "program/program_parse.y"
    { (yyval.integer) = (yyvsp[(2) - (3)].integer); ;}
    break;

  case 278:

/* Line 1464 of yacc.c  */
#line 2171 "program/program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->MaxTextureCoordUnits) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid texture coordinate unit selector");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 279:

/* Line 1464 of yacc.c  */
#line 2182 "program/program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->MaxTextureImageUnits) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid texture image unit selector");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 280:

/* Line 1464 of yacc.c  */
#line 2193 "program/program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->MaxTextureUnits) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid texture unit selector");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 281:

/* Line 1464 of yacc.c  */
#line 2204 "program/program_parse.y"
    {
	   struct asm_symbol *exist = (struct asm_symbol *)
	      _mesa_symbol_table_find_symbol(state->st, 0, (yyvsp[(2) - (4)].string));
	   struct asm_symbol *target = (struct asm_symbol *)
	      _mesa_symbol_table_find_symbol(state->st, 0, (yyvsp[(4) - (4)].string));

	   free((yyvsp[(4) - (4)].string));

	   if (exist != NULL) {
	      char m[1000];
	      _mesa_snprintf(m, sizeof(m), "redeclared identifier: %s", (yyvsp[(2) - (4)].string));
	      free((yyvsp[(2) - (4)].string));
	      yyerror(& (yylsp[(2) - (4)]), state, m);
	      YYERROR;
	   } else if (target == NULL) {
	      free((yyvsp[(2) - (4)].string));
	      yyerror(& (yylsp[(4) - (4)]), state,
		      "undefined variable binding in ALIAS statement");
	      YYERROR;
	   } else {
	      _mesa_symbol_table_add_symbol(state->st, 0, (yyvsp[(2) - (4)].string), target);
	   }
	;}
    break;



/* Line 1464 of yacc.c  */
#line 4991 "program/program_parse.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

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
      yyerror (&yylloc, state, YY_("syntax error"));
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
	    yyerror (&yylloc, state, yymsg);
	  }
	else
	  {
	    yyerror (&yylloc, state, YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }

  yyerror_range[1] = yylloc;

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
		      yytoken, &yylval, &yylloc, state);
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

  yyerror_range[1] = yylsp[1-yylen];
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

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp, state);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;

  yyerror_range[2] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, yyerror_range, 2);
  *++yylsp = yyloc;

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
  yyerror (&yylloc, state, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc, state);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp, state);
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
#line 2233 "program/program_parse.y"


void
asm_instruction_set_operands(struct asm_instruction *inst,
			     const struct prog_dst_register *dst,
			     const struct asm_src_register *src0,
			     const struct asm_src_register *src1,
			     const struct asm_src_register *src2)
{
   /* In the core ARB extensions only the KIL instruction doesn't have a
    * destination register.
    */
   if (dst == NULL) {
      init_dst_reg(& inst->Base.DstReg);
   } else {
      inst->Base.DstReg = *dst;
   }

   /* The only instruction that doesn't have any source registers is the
    * condition-code based KIL instruction added by NV_fragment_program_option.
    */
   if (src0 != NULL) {
      inst->Base.SrcReg[0] = src0->Base;
      inst->SrcReg[0] = *src0;
   } else {
      init_src_reg(& inst->SrcReg[0]);
   }

   if (src1 != NULL) {
      inst->Base.SrcReg[1] = src1->Base;
      inst->SrcReg[1] = *src1;
   } else {
      init_src_reg(& inst->SrcReg[1]);
   }

   if (src2 != NULL) {
      inst->Base.SrcReg[2] = src2->Base;
      inst->SrcReg[2] = *src2;
   } else {
      init_src_reg(& inst->SrcReg[2]);
   }
}


struct asm_instruction *
asm_instruction_ctor(gl_inst_opcode op,
		     const struct prog_dst_register *dst,
		     const struct asm_src_register *src0,
		     const struct asm_src_register *src1,
		     const struct asm_src_register *src2)
{
   struct asm_instruction *inst = CALLOC_STRUCT(asm_instruction);

   if (inst) {
      _mesa_init_instructions(& inst->Base, 1);
      inst->Base.Opcode = op;

      asm_instruction_set_operands(inst, dst, src0, src1, src2);
   }

   return inst;
}


struct asm_instruction *
asm_instruction_copy_ctor(const struct prog_instruction *base,
			  const struct prog_dst_register *dst,
			  const struct asm_src_register *src0,
			  const struct asm_src_register *src1,
			  const struct asm_src_register *src2)
{
   struct asm_instruction *inst = CALLOC_STRUCT(asm_instruction);

   if (inst) {
      _mesa_init_instructions(& inst->Base, 1);
      inst->Base.Opcode = base->Opcode;
      inst->Base.CondUpdate = base->CondUpdate;
      inst->Base.CondDst = base->CondDst;
      inst->Base.SaturateMode = base->SaturateMode;
      inst->Base.Precision = base->Precision;

      asm_instruction_set_operands(inst, dst, src0, src1, src2);
   }

   return inst;
}


void
init_dst_reg(struct prog_dst_register *r)
{
   memset(r, 0, sizeof(*r));
   r->File = PROGRAM_UNDEFINED;
   r->WriteMask = WRITEMASK_XYZW;
   r->CondMask = COND_TR;
   r->CondSwizzle = SWIZZLE_NOOP;
}


/** Like init_dst_reg() but set the File and Index fields. */
void
set_dst_reg(struct prog_dst_register *r, gl_register_file file, GLint index)
{
   const GLint maxIndex = 1 << INST_INDEX_BITS;
   const GLint minIndex = 0;
   ASSERT(index >= minIndex);
   (void) minIndex;
   ASSERT(index <= maxIndex);
   (void) maxIndex;
   ASSERT(file == PROGRAM_TEMPORARY ||
	  file == PROGRAM_ADDRESS ||
	  file == PROGRAM_OUTPUT);
   memset(r, 0, sizeof(*r));
   r->File = file;
   r->Index = index;
   r->WriteMask = WRITEMASK_XYZW;
   r->CondMask = COND_TR;
   r->CondSwizzle = SWIZZLE_NOOP;
}


void
init_src_reg(struct asm_src_register *r)
{
   memset(r, 0, sizeof(*r));
   r->Base.File = PROGRAM_UNDEFINED;
   r->Base.Swizzle = SWIZZLE_NOOP;
   r->Symbol = NULL;
}


/** Like init_src_reg() but set the File and Index fields.
 * \return GL_TRUE if a valid src register, GL_FALSE otherwise
 */
void
set_src_reg(struct asm_src_register *r, gl_register_file file, GLint index)
{
   set_src_reg_swz(r, file, index, SWIZZLE_XYZW);
}


void
set_src_reg_swz(struct asm_src_register *r, gl_register_file file, GLint index,
                GLuint swizzle)
{
   const GLint maxIndex = (1 << INST_INDEX_BITS) - 1;
   const GLint minIndex = -(1 << INST_INDEX_BITS);
   ASSERT(file < PROGRAM_FILE_MAX);
   ASSERT(index >= minIndex);
   (void) minIndex;
   ASSERT(index <= maxIndex);
   (void) maxIndex;
   memset(r, 0, sizeof(*r));
   r->Base.File = file;
   r->Base.Index = index;
   r->Base.Swizzle = swizzle;
   r->Symbol = NULL;
}


/**
 * Validate the set of inputs used by a program
 *
 * Validates that legal sets of inputs are used by the program.  In this case
 * "used" included both reading the input or binding the input to a name using
 * the \c ATTRIB command.
 *
 * \return
 * \c TRUE if the combination of inputs used is valid, \c FALSE otherwise.
 */
int
validate_inputs(struct YYLTYPE *locp, struct asm_parser_state *state)
{
   const GLbitfield64 inputs = state->prog->InputsRead | state->InputsBound;

   if (((inputs & VERT_BIT_FF_ALL) & (inputs >> VERT_ATTRIB_GENERIC0)) != 0) {
      yyerror(locp, state, "illegal use of generic attribute and name attribute");
      return 0;
   }

   return 1;
}


struct asm_symbol *
declare_variable(struct asm_parser_state *state, char *name, enum asm_type t,
		 struct YYLTYPE *locp)
{
   struct asm_symbol *s = NULL;
   struct asm_symbol *exist = (struct asm_symbol *)
      _mesa_symbol_table_find_symbol(state->st, 0, name);


   if (exist != NULL) {
      yyerror(locp, state, "redeclared identifier");
   } else {
      s = calloc(1, sizeof(struct asm_symbol));
      s->name = name;
      s->type = t;

      switch (t) {
      case at_temp:
	 if (state->prog->NumTemporaries >= state->limits->MaxTemps) {
	    yyerror(locp, state, "too many temporaries declared");
	    free(s);
	    return NULL;
	 }

	 s->temp_binding = state->prog->NumTemporaries;
	 state->prog->NumTemporaries++;
	 break;

      case at_address:
	 if (state->prog->NumAddressRegs >= state->limits->MaxAddressRegs) {
	    yyerror(locp, state, "too many address registers declared");
	    free(s);
	    return NULL;
	 }

	 /* FINISHME: Add support for multiple address registers.
	  */
	 state->prog->NumAddressRegs++;
	 break;

      default:
	 break;
      }

      _mesa_symbol_table_add_symbol(state->st, 0, s->name, s);
      s->next = state->sym;
      state->sym = s;
   }

   return s;
}


int add_state_reference(struct gl_program_parameter_list *param_list,
			const gl_state_index tokens[STATE_LENGTH])
{
   const GLuint size = 4; /* XXX fix */
   char *name;
   GLint index;

   name = _mesa_program_state_string(tokens);
   index = _mesa_add_parameter(param_list, PROGRAM_STATE_VAR, name,
                               size, GL_NONE, NULL, tokens, 0x0);
   param_list->StateFlags |= _mesa_program_state_flags(tokens);

   /* free name string here since we duplicated it in add_parameter() */
   free(name);

   return index;
}


int
initialize_symbol_from_state(struct gl_program *prog,
			     struct asm_symbol *param_var, 
			     const gl_state_index tokens[STATE_LENGTH])
{
   int idx = -1;
   gl_state_index state_tokens[STATE_LENGTH];


   memcpy(state_tokens, tokens, sizeof(state_tokens));

   param_var->type = at_param;
   param_var->param_binding_type = PROGRAM_STATE_VAR;

   /* If we are adding a STATE_MATRIX that has multiple rows, we need to
    * unroll it and call add_state_reference() for each row
    */
   if ((state_tokens[0] == STATE_MODELVIEW_MATRIX ||
	state_tokens[0] == STATE_PROJECTION_MATRIX ||
	state_tokens[0] == STATE_MVP_MATRIX ||
	state_tokens[0] == STATE_TEXTURE_MATRIX ||
	state_tokens[0] == STATE_PROGRAM_MATRIX)
       && (state_tokens[2] != state_tokens[3])) {
      int row;
      const int first_row = state_tokens[2];
      const int last_row = state_tokens[3];

      for (row = first_row; row <= last_row; row++) {
	 state_tokens[2] = state_tokens[3] = row;

	 idx = add_state_reference(prog->Parameters, state_tokens);
	 if (param_var->param_binding_begin == ~0U) {
	    param_var->param_binding_begin = idx;
            param_var->param_binding_swizzle = SWIZZLE_XYZW;
         }

	 param_var->param_binding_length++;
      }
   }
   else {
      idx = add_state_reference(prog->Parameters, state_tokens);
      if (param_var->param_binding_begin == ~0U) {
	 param_var->param_binding_begin = idx;
         param_var->param_binding_swizzle = SWIZZLE_XYZW;
      }
      param_var->param_binding_length++;
   }

   return idx;
}


int
initialize_symbol_from_param(struct gl_program *prog,
			     struct asm_symbol *param_var, 
			     const gl_state_index tokens[STATE_LENGTH])
{
   int idx = -1;
   gl_state_index state_tokens[STATE_LENGTH];


   memcpy(state_tokens, tokens, sizeof(state_tokens));

   assert((state_tokens[0] == STATE_VERTEX_PROGRAM)
	  || (state_tokens[0] == STATE_FRAGMENT_PROGRAM));
   assert((state_tokens[1] == STATE_ENV)
	  || (state_tokens[1] == STATE_LOCAL));

   /*
    * The param type is STATE_VAR.  The program parameter entry will
    * effectively be a pointer into the LOCAL or ENV parameter array.
    */
   param_var->type = at_param;
   param_var->param_binding_type = PROGRAM_STATE_VAR;

   /* If we are adding a STATE_ENV or STATE_LOCAL that has multiple elements,
    * we need to unroll it and call add_state_reference() for each row
    */
   if (state_tokens[2] != state_tokens[3]) {
      int row;
      const int first_row = state_tokens[2];
      const int last_row = state_tokens[3];

      for (row = first_row; row <= last_row; row++) {
	 state_tokens[2] = state_tokens[3] = row;

	 idx = add_state_reference(prog->Parameters, state_tokens);
	 if (param_var->param_binding_begin == ~0U) {
	    param_var->param_binding_begin = idx;
            param_var->param_binding_swizzle = SWIZZLE_XYZW;
         }
	 param_var->param_binding_length++;
      }
   }
   else {
      idx = add_state_reference(prog->Parameters, state_tokens);
      if (param_var->param_binding_begin == ~0U) {
	 param_var->param_binding_begin = idx;
         param_var->param_binding_swizzle = SWIZZLE_XYZW;
      }
      param_var->param_binding_length++;
   }

   return idx;
}


/**
 * Put a float/vector constant/literal into the parameter list.
 * \param param_var  returns info about the parameter/constant's location,
 *                   binding, type, etc.
 * \param vec  the vector/constant to add
 * \param allowSwizzle  if true, try to consolidate constants which only differ
 *                      by a swizzle.  We don't want to do this when building
 *                      arrays of constants that may be indexed indirectly.
 * \return index of the constant in the parameter list.
 */
int
initialize_symbol_from_const(struct gl_program *prog,
			     struct asm_symbol *param_var, 
			     const struct asm_vector *vec,
                             GLboolean allowSwizzle)
{
   unsigned swizzle;
   const int idx = _mesa_add_unnamed_constant(prog->Parameters,
                                              vec->data, vec->count,
                                              allowSwizzle ? &swizzle : NULL);

   param_var->type = at_param;
   param_var->param_binding_type = PROGRAM_CONSTANT;

   if (param_var->param_binding_begin == ~0U) {
      param_var->param_binding_begin = idx;
      param_var->param_binding_swizzle = allowSwizzle ? swizzle : SWIZZLE_XYZW;
   }
   param_var->param_binding_length++;

   return idx;
}


char *
make_error_string(const char *fmt, ...)
{
   int length;
   char *str;
   va_list args;


   /* Call vsnprintf once to determine how large the final string is.  Call it
    * again to do the actual formatting.  from the vsnprintf manual page:
    *
    *    Upon successful return, these functions return the number of
    *    characters printed  (not including the trailing '\0' used to end
    *    output to strings).
    */
   va_start(args, fmt);
   length = 1 + vsnprintf(NULL, 0, fmt, args);
   va_end(args);

   str = malloc(length);
   if (str) {
      va_start(args, fmt);
      vsnprintf(str, length, fmt, args);
      va_end(args);
   }

   return str;
}


void
yyerror(YYLTYPE *locp, struct asm_parser_state *state, const char *s)
{
   char *err_str;


   err_str = make_error_string("glProgramStringARB(%s)\n", s);
   if (err_str) {
      _mesa_error(state->ctx, GL_INVALID_OPERATION, "%s", err_str);
      free(err_str);
   }

   err_str = make_error_string("line %u, char %u: error: %s\n",
			       locp->first_line, locp->first_column, s);
   _mesa_set_program_error(state->ctx, locp->position, err_str);

   if (err_str) {
      free(err_str);
   }
}


GLboolean
_mesa_parse_arb_program(struct gl_context *ctx, GLenum target, const GLubyte *str,
			GLsizei len, struct asm_parser_state *state)
{
   struct asm_instruction *inst;
   unsigned i;
   GLubyte *strz;
   GLboolean result = GL_FALSE;
   void *temp;
   struct asm_symbol *sym;

   state->ctx = ctx;
   state->prog->Target = target;
   state->prog->Parameters = _mesa_new_parameter_list();

   /* Make a copy of the program string and force it to be NUL-terminated.
    */
   strz = (GLubyte *) malloc(len + 1);
   if (strz == NULL) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glProgramStringARB");
      return GL_FALSE;
   }
   memcpy (strz, str, len);
   strz[len] = '\0';

   state->prog->String = strz;

   state->st = _mesa_symbol_table_ctor();

   state->limits = (target == GL_VERTEX_PROGRAM_ARB)
      ? & ctx->Const.VertexProgram
      : & ctx->Const.FragmentProgram;

   state->MaxTextureImageUnits = ctx->Const.MaxTextureImageUnits;
   state->MaxTextureCoordUnits = ctx->Const.MaxTextureCoordUnits;
   state->MaxTextureUnits = ctx->Const.MaxTextureUnits;
   state->MaxClipPlanes = ctx->Const.MaxClipPlanes;
   state->MaxLights = ctx->Const.MaxLights;
   state->MaxProgramMatrices = ctx->Const.MaxProgramMatrices;
   state->MaxDrawBuffers = ctx->Const.MaxDrawBuffers;

   state->state_param_enum = (target == GL_VERTEX_PROGRAM_ARB)
      ? STATE_VERTEX_PROGRAM : STATE_FRAGMENT_PROGRAM;

   _mesa_set_program_error(ctx, -1, NULL);

   _mesa_program_lexer_ctor(& state->scanner, state, (const char *) str, len);
   yyparse(state);
   _mesa_program_lexer_dtor(state->scanner);


   if (ctx->Program.ErrorPos != -1) {
      goto error;
   }

   if (! _mesa_layout_parameters(state)) {
      struct YYLTYPE loc;

      loc.first_line = 0;
      loc.first_column = 0;
      loc.position = len;

      yyerror(& loc, state, "invalid PARAM usage");
      goto error;
   }


   
   /* Add one instruction to store the "END" instruction.
    */
   state->prog->Instructions =
      _mesa_alloc_instructions(state->prog->NumInstructions + 1);
   inst = state->inst_head;
   for (i = 0; i < state->prog->NumInstructions; i++) {
      struct asm_instruction *const temp = inst->next;

      state->prog->Instructions[i] = inst->Base;
      inst = temp;
   }

   /* Finally, tag on an OPCODE_END instruction */
   {
      const GLuint numInst = state->prog->NumInstructions;
      _mesa_init_instructions(state->prog->Instructions + numInst, 1);
      state->prog->Instructions[numInst].Opcode = OPCODE_END;
   }
   state->prog->NumInstructions++;

   state->prog->NumParameters = state->prog->Parameters->NumParameters;
   state->prog->NumAttributes = _mesa_bitcount_64(state->prog->InputsRead);

   /*
    * Initialize native counts to logical counts.  The device driver may
    * change them if program is translated into a hardware program.
    */
   state->prog->NumNativeInstructions = state->prog->NumInstructions;
   state->prog->NumNativeTemporaries = state->prog->NumTemporaries;
   state->prog->NumNativeParameters = state->prog->NumParameters;
   state->prog->NumNativeAttributes = state->prog->NumAttributes;
   state->prog->NumNativeAddressRegs = state->prog->NumAddressRegs;

   result = GL_TRUE;

error:
   for (inst = state->inst_head; inst != NULL; inst = temp) {
      temp = inst->next;
      free(inst);
   }

   state->inst_head = NULL;
   state->inst_tail = NULL;

   for (sym = state->sym; sym != NULL; sym = temp) {
      temp = sym->next;

      free((void *) sym->name);
      free(sym);
   }
   state->sym = NULL;

   _mesa_symbol_table_dtor(state->st);
   state->st = NULL;

   return result;
}

