/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison interface for Yacc-like parsers in C

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

#ifndef YY_HLSL_HLSL_TAB_H_INCLUDED
# define YY_HLSL_HLSL_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int hlsl_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    KW_BLENDSTATE = 258,
    KW_BREAK = 259,
    KW_BUFFER = 260,
    KW_CBUFFER = 261,
    KW_COLUMN_MAJOR = 262,
    KW_COMPILE = 263,
    KW_CONST = 264,
    KW_CONTINUE = 265,
    KW_DEPTHSTENCILSTATE = 266,
    KW_DEPTHSTENCILVIEW = 267,
    KW_DISCARD = 268,
    KW_DO = 269,
    KW_DOUBLE = 270,
    KW_ELSE = 271,
    KW_EXTERN = 272,
    KW_FALSE = 273,
    KW_FOR = 274,
    KW_GEOMETRYSHADER = 275,
    KW_GROUPSHARED = 276,
    KW_IF = 277,
    KW_IN = 278,
    KW_INLINE = 279,
    KW_INOUT = 280,
    KW_MATRIX = 281,
    KW_NAMESPACE = 282,
    KW_NOINTERPOLATION = 283,
    KW_OUT = 284,
    KW_PASS = 285,
    KW_PIXELSHADER = 286,
    KW_PRECISE = 287,
    KW_RASTERIZERSTATE = 288,
    KW_RENDERTARGETVIEW = 289,
    KW_RETURN = 290,
    KW_REGISTER = 291,
    KW_ROW_MAJOR = 292,
    KW_SAMPLER = 293,
    KW_SAMPLER1D = 294,
    KW_SAMPLER2D = 295,
    KW_SAMPLER3D = 296,
    KW_SAMPLERCUBE = 297,
    KW_SAMPLER_STATE = 298,
    KW_SAMPLERCOMPARISONSTATE = 299,
    KW_SHARED = 300,
    KW_STATEBLOCK = 301,
    KW_STATEBLOCK_STATE = 302,
    KW_STATIC = 303,
    KW_STRING = 304,
    KW_STRUCT = 305,
    KW_SWITCH = 306,
    KW_TBUFFER = 307,
    KW_TECHNIQUE = 308,
    KW_TECHNIQUE10 = 309,
    KW_TEXTURE = 310,
    KW_TEXTURE1D = 311,
    KW_TEXTURE1DARRAY = 312,
    KW_TEXTURE2D = 313,
    KW_TEXTURE2DARRAY = 314,
    KW_TEXTURE2DMS = 315,
    KW_TEXTURE2DMSARRAY = 316,
    KW_TEXTURE3D = 317,
    KW_TEXTURE3DARRAY = 318,
    KW_TEXTURECUBE = 319,
    KW_TRUE = 320,
    KW_TYPEDEF = 321,
    KW_UNIFORM = 322,
    KW_VECTOR = 323,
    KW_VERTEXSHADER = 324,
    KW_VOID = 325,
    KW_VOLATILE = 326,
    KW_WHILE = 327,
    OP_INC = 328,
    OP_DEC = 329,
    OP_AND = 330,
    OP_OR = 331,
    OP_EQ = 332,
    OP_LEFTSHIFT = 333,
    OP_LEFTSHIFTASSIGN = 334,
    OP_RIGHTSHIFT = 335,
    OP_RIGHTSHIFTASSIGN = 336,
    OP_ELLIPSIS = 337,
    OP_LE = 338,
    OP_GE = 339,
    OP_NE = 340,
    OP_ADDASSIGN = 341,
    OP_SUBASSIGN = 342,
    OP_MULASSIGN = 343,
    OP_DIVASSIGN = 344,
    OP_MODASSIGN = 345,
    OP_ANDASSIGN = 346,
    OP_ORASSIGN = 347,
    OP_XORASSIGN = 348,
    OP_UNKNOWN1 = 349,
    OP_UNKNOWN2 = 350,
    OP_UNKNOWN3 = 351,
    OP_UNKNOWN4 = 352,
    PRE_LINE = 353,
    VAR_IDENTIFIER = 354,
    TYPE_IDENTIFIER = 355,
    NEW_IDENTIFIER = 356,
    STRING = 357,
    C_FLOAT = 358,
    C_INTEGER = 359
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 910 "hlsl.y" /* yacc.c:1909  */

    struct hlsl_type *type;
    INT intval;
    FLOAT floatval;
    BOOL boolval;
    char *name;
    DWORD modifiers;
    struct hlsl_ir_var *var;
    struct hlsl_ir_node *instr;
    struct list *list;
    struct parse_function function;
    struct parse_parameter parameter;
    struct parse_variable_def *variable_def;
    struct parse_if_body if_body;
    enum parse_unary_op unary_op;
    enum parse_assign_op assign_op;
    struct reg_reservation *reg_reservation;
    struct parse_colon_attribute colon_attribute;

#line 179 "hlsl.tab.h" /* yacc.c:1909  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


extern YYSTYPE hlsl_lval;
extern YYLTYPE hlsl_lloc;
int hlsl_parse (void);

#endif /* !YY_HLSL_HLSL_TAB_H_INCLUDED  */
