/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
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
     kFUNCTION = 269,
     kIN = 270,
     kINSTANCEOF = 271,
     kNEW = 272,
     kNULL = 273,
     kUNDEFINED = 274,
     kRETURN = 275,
     kSWITCH = 276,
     kTHIS = 277,
     kTHROW = 278,
     kTRUE = 279,
     kFALSE = 280,
     kTRY = 281,
     kTYPEOF = 282,
     kVAR = 283,
     kVOID = 284,
     kWHILE = 285,
     kWITH = 286,
     tANDAND = 287,
     tOROR = 288,
     tINC = 289,
     tDEC = 290,
     tIdentifier = 291,
     tAssignOper = 292,
     tEqOper = 293,
     tShiftOper = 294,
     tRelOper = 295,
     tNumericLiteral = 296,
     tStringLiteral = 297
   };
#endif
/* Tokens.  */
#define kBREAK 258
#define kCASE 259
#define kCATCH 260
#define kCONTINUE 261
#define kDEFAULT 262
#define kDELETE 263
#define kDO 264
#define kELSE 265
#define kIF 266
#define kFINALLY 267
#define kFOR 268
#define kFUNCTION 269
#define kIN 270
#define kINSTANCEOF 271
#define kNEW 272
#define kNULL 273
#define kUNDEFINED 274
#define kRETURN 275
#define kSWITCH 276
#define kTHIS 277
#define kTHROW 278
#define kTRUE 279
#define kFALSE 280
#define kTRY 281
#define kTYPEOF 282
#define kVAR 283
#define kVOID 284
#define kWHILE 285
#define kWITH 286
#define tANDAND 287
#define tOROR 288
#define tINC 289
#define tDEC 290
#define tIdentifier 291
#define tAssignOper 292
#define tEqOper 293
#define tShiftOper 294
#define tRelOper 295
#define tNumericLiteral 296
#define tStringLiteral 297




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 147 "parser.y"
typedef union YYSTYPE {
    int                     ival;
    LPCWSTR                 wstr;
    literal_t               *literal;
    struct _argument_list_t *argument_list;
    case_clausule_t         *case_clausule;
    struct _case_list_t     *case_list;
    catch_block_t           *catch_block;
    struct _element_list_t  *element_list;
    expression_t            *expr;
    const WCHAR            *identifier;
    function_declaration_t  *function_declaration;
    struct _parameter_list_t *parameter_list;
    struct _property_list_t *property_list;
    source_elements_t       *source_elements;
    statement_t             *statement;
    struct _statement_list_t *statement_list;
    struct _variable_list_t *variable_list;
    variable_declaration_t  *variable_declaration;
} YYSTYPE;
/* Line 1447 of yacc.c.  */
#line 143 "parser.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif





