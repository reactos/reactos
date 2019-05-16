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
    kIN = 270,
    kINSTANCEOF = 271,
    kNEW = 272,
    kNULL = 273,
    kRETURN = 274,
    kSWITCH = 275,
    kTHIS = 276,
    kTHROW = 277,
    kTRUE = 278,
    kFALSE = 279,
    kTRY = 280,
    kTYPEOF = 281,
    kVAR = 282,
    kVOID = 283,
    kWHILE = 284,
    kWITH = 285,
    tANDAND = 286,
    tOROR = 287,
    tINC = 288,
    tDEC = 289,
    tHTMLCOMMENT = 290,
    kDIVEQ = 291,
    kDCOL = 292,
    tIdentifier = 293,
    tAssignOper = 294,
    tEqOper = 295,
    tShiftOper = 296,
    tRelOper = 297,
    tNumericLiteral = 298,
    tBooleanLiteral = 299,
    tStringLiteral = 300,
    tEOF = 301,
    LOWER_THAN_ELSE = 302
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

#line 123 "parser.tab.h" /* yacc.c:1909  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int parser_parse (parser_ctx_t *ctx);

#endif /* !YY_PARSER_E_REACTOSSYNC_GCC_DLL_WIN32_JSCRIPT_PARSER_TAB_H_INCLUDED  */
