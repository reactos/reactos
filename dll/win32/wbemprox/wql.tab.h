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

#ifndef YY_WQL_E_REACTOSSYNC_GCC_DLL_WIN32_WBEMPROX_WQL_TAB_H_INCLUDED
# define YY_WQL_E_REACTOSSYNC_GCC_DLL_WIN32_WBEMPROX_WQL_TAB_H_INCLUDED
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
#line 194 "wql.y" /* yacc.c:1909  */

    struct string str;
    WCHAR *string;
    struct property *proplist;
    struct view *view;
    struct expr *expr;
    int integer;

#line 95 "wql.tab.h" /* yacc.c:1909  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int wql_parse (struct parser *ctx);

#endif /* !YY_WQL_E_REACTOSSYNC_GCC_DLL_WIN32_WBEMPROX_WQL_TAB_H_INCLUDED  */
