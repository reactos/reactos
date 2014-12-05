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

#ifndef YY_PARSER_PARSER_TAB_H_INCLUDED
# define YY_PARSER_PARSER_TAB_H_INCLUDED
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
    tEOF = 258,
    tNL = 259,
    tREM = 260,
    tEMPTYBRACKETS = 261,
    tTRUE = 262,
    tFALSE = 263,
    tNOT = 264,
    tAND = 265,
    tOR = 266,
    tXOR = 267,
    tEQV = 268,
    tIMP = 269,
    tNEQ = 270,
    tIS = 271,
    tLTEQ = 272,
    tGTEQ = 273,
    tMOD = 274,
    tCALL = 275,
    tDIM = 276,
    tSUB = 277,
    tFUNCTION = 278,
    tPROPERTY = 279,
    tGET = 280,
    tLET = 281,
    tCONST = 282,
    tIF = 283,
    tELSE = 284,
    tELSEIF = 285,
    tEND = 286,
    tTHEN = 287,
    tEXIT = 288,
    tWHILE = 289,
    tWEND = 290,
    tDO = 291,
    tLOOP = 292,
    tUNTIL = 293,
    tFOR = 294,
    tTO = 295,
    tSTEP = 296,
    tEACH = 297,
    tIN = 298,
    tSELECT = 299,
    tCASE = 300,
    tBYREF = 301,
    tBYVAL = 302,
    tOPTION = 303,
    tEXPLICIT = 304,
    tSTOP = 305,
    tNOTHING = 306,
    tEMPTY = 307,
    tNULL = 308,
    tCLASS = 309,
    tSET = 310,
    tNEW = 311,
    tPUBLIC = 312,
    tPRIVATE = 313,
    tDEFAULT = 314,
    tME = 315,
    tERROR = 316,
    tNEXT = 317,
    tON = 318,
    tRESUME = 319,
    tGOTO = 320,
    tIdentifier = 321,
    tString = 322,
    tLong = 323,
    tShort = 324,
    tDouble = 325
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 88 "parser.y" /* yacc.c:1909  */

    const WCHAR *string;
    statement_t *statement;
    expression_t *expression;
    member_expression_t *member;
    elseif_decl_t *elseif;
    dim_decl_t *dim_decl;
    dim_list_t *dim_list;
    function_decl_t *func_decl;
    arg_decl_t *arg_decl;
    class_decl_t *class_decl;
    const_decl_t *const_decl;
    case_clausule_t *case_clausule;
    unsigned uint;
    LONG lng;
    BOOL boolean;
    double dbl;

#line 144 "parser.tab.h" /* yacc.c:1909  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int parser_parse (parser_ctx_t *ctx);

#endif /* !YY_PARSER_PARSER_TAB_H_INCLUDED  */
