/* A Bison parser, made by GNU Bison 3.0.  */

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

#ifndef YY_XSLPATTERN_E_REACTOSSYNC_GCC_DLL_WIN32_MSXML3_XSLPATTERN_TAB_H_INCLUDED
# define YY_XSLPATTERN_E_REACTOSSYNC_GCC_DLL_WIN32_MSXML3_XSLPATTERN_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int xslpattern_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    TOK_Parent = 258,
    TOK_Self = 259,
    TOK_DblFSlash = 260,
    TOK_FSlash = 261,
    TOK_Axis = 262,
    TOK_Colon = 263,
    TOK_OpAnd = 264,
    TOK_OpOr = 265,
    TOK_OpNot = 266,
    TOK_OpEq = 267,
    TOK_OpIEq = 268,
    TOK_OpNEq = 269,
    TOK_OpINEq = 270,
    TOK_OpLt = 271,
    TOK_OpILt = 272,
    TOK_OpGt = 273,
    TOK_OpIGt = 274,
    TOK_OpLEq = 275,
    TOK_OpILEq = 276,
    TOK_OpGEq = 277,
    TOK_OpIGEq = 278,
    TOK_OpAll = 279,
    TOK_OpAny = 280,
    TOK_NCName = 281,
    TOK_Literal = 282,
    TOK_Number = 283
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int xslpattern_parse (parser_param* p, void* scanner);

#endif /* !YY_XSLPATTERN_E_REACTOSSYNC_GCC_DLL_WIN32_MSXML3_XSLPATTERN_TAB_H_INCLUDED  */
