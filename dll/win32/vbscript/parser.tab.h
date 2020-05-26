/* A Bison parser, made by GNU Bison 3.4.1.  */

/* Bison interface for Yacc-like parsers in C

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

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

#ifndef YY_PARSER_E_REACTOSSYNC_GCC_DLL_WIN32_VBSCRIPT_PARSER_TAB_H_INCLUDED
# define YY_PARSER_E_REACTOSSYNC_GCC_DLL_WIN32_VBSCRIPT_PARSER_TAB_H_INCLUDED
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
    tEXPRESSION = 258,
    tEOF = 259,
    tNL = 260,
    tEMPTYBRACKETS = 261,
    tLTEQ = 262,
    tGTEQ = 263,
    tNEQ = 264,
    tSTOP = 265,
    tME = 266,
    tREM = 267,
    tTRUE = 268,
    tFALSE = 269,
    tNOT = 270,
    tAND = 271,
    tOR = 272,
    tXOR = 273,
    tEQV = 274,
    tIMP = 275,
    tIS = 276,
    tMOD = 277,
    tCALL = 278,
    tDIM = 279,
    tSUB = 280,
    tFUNCTION = 281,
    tGET = 282,
    tLET = 283,
    tCONST = 284,
    tIF = 285,
    tELSE = 286,
    tELSEIF = 287,
    tEND = 288,
    tTHEN = 289,
    tEXIT = 290,
    tWHILE = 291,
    tWEND = 292,
    tDO = 293,
    tLOOP = 294,
    tUNTIL = 295,
    tFOR = 296,
    tTO = 297,
    tEACH = 298,
    tIN = 299,
    tSELECT = 300,
    tCASE = 301,
    tBYREF = 302,
    tBYVAL = 303,
    tOPTION = 304,
    tNOTHING = 305,
    tEMPTY = 306,
    tNULL = 307,
    tCLASS = 308,
    tSET = 309,
    tNEW = 310,
    tPUBLIC = 311,
    tPRIVATE = 312,
    tNEXT = 313,
    tON = 314,
    tRESUME = 315,
    tGOTO = 316,
    tIdentifier = 317,
    tString = 318,
    tDEFAULT = 319,
    tERROR = 320,
    tEXPLICIT = 321,
    tPROPERTY = 322,
    tSTEP = 323,
    tInt = 324,
    tDouble = 325
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 87 "parser.y"

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
    LONG integer;
    BOOL boolean;
    double dbl;

#line 147 "parser.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int parser_parse (parser_ctx_t *ctx);

#endif /* !YY_PARSER_E_REACTOSSYNC_GCC_DLL_WIN32_VBSCRIPT_PARSER_TAB_H_INCLUDED  */
