/* A Bison parser, made by GNU Bison 2.4.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
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


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     ATTRIBUTE = 258,
     CONST_TOK = 259,
     BOOL_TOK = 260,
     FLOAT_TOK = 261,
     INT_TOK = 262,
     UINT_TOK = 263,
     BREAK = 264,
     CONTINUE = 265,
     DO = 266,
     ELSE = 267,
     FOR = 268,
     IF = 269,
     DISCARD = 270,
     RETURN = 271,
     SWITCH = 272,
     CASE = 273,
     DEFAULT = 274,
     BVEC2 = 275,
     BVEC3 = 276,
     BVEC4 = 277,
     IVEC2 = 278,
     IVEC3 = 279,
     IVEC4 = 280,
     UVEC2 = 281,
     UVEC3 = 282,
     UVEC4 = 283,
     VEC2 = 284,
     VEC3 = 285,
     VEC4 = 286,
     CENTROID = 287,
     IN_TOK = 288,
     OUT_TOK = 289,
     INOUT_TOK = 290,
     UNIFORM = 291,
     VARYING = 292,
     NOPERSPECTIVE = 293,
     FLAT = 294,
     SMOOTH = 295,
     MAT2X2 = 296,
     MAT2X3 = 297,
     MAT2X4 = 298,
     MAT3X2 = 299,
     MAT3X3 = 300,
     MAT3X4 = 301,
     MAT4X2 = 302,
     MAT4X3 = 303,
     MAT4X4 = 304,
     SAMPLER1D = 305,
     SAMPLER2D = 306,
     SAMPLER3D = 307,
     SAMPLERCUBE = 308,
     SAMPLER1DSHADOW = 309,
     SAMPLER2DSHADOW = 310,
     SAMPLERCUBESHADOW = 311,
     SAMPLER1DARRAY = 312,
     SAMPLER2DARRAY = 313,
     SAMPLER1DARRAYSHADOW = 314,
     SAMPLER2DARRAYSHADOW = 315,
     ISAMPLER1D = 316,
     ISAMPLER2D = 317,
     ISAMPLER3D = 318,
     ISAMPLERCUBE = 319,
     ISAMPLER1DARRAY = 320,
     ISAMPLER2DARRAY = 321,
     USAMPLER1D = 322,
     USAMPLER2D = 323,
     USAMPLER3D = 324,
     USAMPLERCUBE = 325,
     USAMPLER1DARRAY = 326,
     USAMPLER2DARRAY = 327,
     SAMPLEREXTERNALOES = 328,
     STRUCT = 329,
     VOID_TOK = 330,
     WHILE = 331,
     IDENTIFIER = 332,
     TYPE_IDENTIFIER = 333,
     NEW_IDENTIFIER = 334,
     FLOATCONSTANT = 335,
     INTCONSTANT = 336,
     UINTCONSTANT = 337,
     BOOLCONSTANT = 338,
     FIELD_SELECTION = 339,
     LEFT_OP = 340,
     RIGHT_OP = 341,
     INC_OP = 342,
     DEC_OP = 343,
     LE_OP = 344,
     GE_OP = 345,
     EQ_OP = 346,
     NE_OP = 347,
     AND_OP = 348,
     OR_OP = 349,
     XOR_OP = 350,
     MUL_ASSIGN = 351,
     DIV_ASSIGN = 352,
     ADD_ASSIGN = 353,
     MOD_ASSIGN = 354,
     LEFT_ASSIGN = 355,
     RIGHT_ASSIGN = 356,
     AND_ASSIGN = 357,
     XOR_ASSIGN = 358,
     OR_ASSIGN = 359,
     SUB_ASSIGN = 360,
     INVARIANT = 361,
     LOWP = 362,
     MEDIUMP = 363,
     HIGHP = 364,
     SUPERP = 365,
     PRECISION = 366,
     VERSION_TOK = 367,
     EXTENSION = 368,
     LINE = 369,
     COLON = 370,
     EOL = 371,
     INTERFACE = 372,
     OUTPUT = 373,
     PRAGMA_DEBUG_ON = 374,
     PRAGMA_DEBUG_OFF = 375,
     PRAGMA_OPTIMIZE_ON = 376,
     PRAGMA_OPTIMIZE_OFF = 377,
     PRAGMA_INVARIANT_ALL = 378,
     LAYOUT_TOK = 379,
     ASM = 380,
     CLASS = 381,
     UNION = 382,
     ENUM = 383,
     TYPEDEF = 384,
     TEMPLATE = 385,
     THIS = 386,
     PACKED_TOK = 387,
     GOTO = 388,
     INLINE_TOK = 389,
     NOINLINE = 390,
     VOLATILE = 391,
     PUBLIC_TOK = 392,
     STATIC = 393,
     EXTERN = 394,
     EXTERNAL = 395,
     LONG_TOK = 396,
     SHORT_TOK = 397,
     DOUBLE_TOK = 398,
     HALF = 399,
     FIXED_TOK = 400,
     UNSIGNED = 401,
     INPUT_TOK = 402,
     OUPTUT = 403,
     HVEC2 = 404,
     HVEC3 = 405,
     HVEC4 = 406,
     DVEC2 = 407,
     DVEC3 = 408,
     DVEC4 = 409,
     FVEC2 = 410,
     FVEC3 = 411,
     FVEC4 = 412,
     SAMPLER2DRECT = 413,
     SAMPLER3DRECT = 414,
     SAMPLER2DRECTSHADOW = 415,
     SIZEOF = 416,
     CAST = 417,
     NAMESPACE = 418,
     USING = 419,
     ERROR_TOK = 420,
     COMMON = 421,
     PARTITION = 422,
     ACTIVE = 423,
     SAMPLERBUFFER = 424,
     FILTER = 425,
     IMAGE1D = 426,
     IMAGE2D = 427,
     IMAGE3D = 428,
     IMAGECUBE = 429,
     IMAGE1DARRAY = 430,
     IMAGE2DARRAY = 431,
     IIMAGE1D = 432,
     IIMAGE2D = 433,
     IIMAGE3D = 434,
     IIMAGECUBE = 435,
     IIMAGE1DARRAY = 436,
     IIMAGE2DARRAY = 437,
     UIMAGE1D = 438,
     UIMAGE2D = 439,
     UIMAGE3D = 440,
     UIMAGECUBE = 441,
     UIMAGE1DARRAY = 442,
     UIMAGE2DARRAY = 443,
     IMAGE1DSHADOW = 444,
     IMAGE2DSHADOW = 445,
     IMAGEBUFFER = 446,
     IIMAGEBUFFER = 447,
     UIMAGEBUFFER = 448,
     IMAGE1DARRAYSHADOW = 449,
     IMAGE2DARRAYSHADOW = 450,
     ROW_MAJOR = 451
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1685 of yacc.c  */
#line 58 "glsl_parser.yy"

   int n;
   float real;
   char *identifier;

   struct ast_type_qualifier type_qualifier;

   ast_node *node;
   ast_type_specifier *type_specifier;
   ast_fully_specified_type *fully_specified_type;
   ast_function *function;
   ast_parameter_declarator *parameter_declarator;
   ast_function_definition *function_definition;
   ast_compound_statement *compound_statement;
   ast_expression *expression;
   ast_declarator_list *declarator_list;
   ast_struct_specifier *struct_specifier;
   ast_declaration *declaration;
   ast_switch_body *switch_body;
   ast_case_label *case_label;
   ast_case_label_list *case_label_list;
   ast_case_statement *case_statement;
   ast_case_statement_list *case_statement_list;

   struct {
      ast_node *cond;
      ast_expression *rest;
   } for_rest_statement;

   struct {
      ast_node *then_statement;
      ast_node *else_statement;
   } selection_rest_statement;



/* Line 1685 of yacc.c  */
#line 284 "glsl_parser.h"
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



