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
#line 145 "parser.y" /* yacc.c:1909  */

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

#line 122 "parser.tab.h" /* yacc.c:1909  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int parser_parse (parser_ctx_t *ctx);

#endif /* !YY_PARSER_PARSER_TAB_H_INCLUDED  */
