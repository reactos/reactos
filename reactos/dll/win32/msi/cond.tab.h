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
     COND_SPACE = 258,
     COND_EOF = 259,
     COND_OR = 260,
     COND_AND = 261,
     COND_NOT = 262,
     COND_XOR = 263,
     COND_IMP = 264,
     COND_EQV = 265,
     COND_LT = 266,
     COND_GT = 267,
     COND_EQ = 268,
     COND_NE = 269,
     COND_GE = 270,
     COND_LE = 271,
     COND_ILT = 272,
     COND_IGT = 273,
     COND_IEQ = 274,
     COND_INE = 275,
     COND_IGE = 276,
     COND_ILE = 277,
     COND_LPAR = 278,
     COND_RPAR = 279,
     COND_TILDA = 280,
     COND_SS = 281,
     COND_ISS = 282,
     COND_ILHS = 283,
     COND_IRHS = 284,
     COND_LHS = 285,
     COND_RHS = 286,
     COND_PERCENT = 287,
     COND_DOLLARS = 288,
     COND_QUESTION = 289,
     COND_AMPER = 290,
     COND_EXCLAM = 291,
     COND_IDENT = 292,
     COND_NUMBER = 293,
     COND_LITER = 294,
     COND_ERROR = 295
   };
#endif
/* Tokens.  */
#define COND_SPACE 258
#define COND_EOF 259
#define COND_OR 260
#define COND_AND 261
#define COND_NOT 262
#define COND_XOR 263
#define COND_IMP 264
#define COND_EQV 265
#define COND_LT 266
#define COND_GT 267
#define COND_EQ 268
#define COND_NE 269
#define COND_GE 270
#define COND_LE 271
#define COND_ILT 272
#define COND_IGT 273
#define COND_IEQ 274
#define COND_INE 275
#define COND_IGE 276
#define COND_ILE 277
#define COND_LPAR 278
#define COND_RPAR 279
#define COND_TILDA 280
#define COND_SS 281
#define COND_ISS 282
#define COND_ILHS 283
#define COND_IRHS 284
#define COND_LHS 285
#define COND_RHS 286
#define COND_PERCENT 287
#define COND_DOLLARS 288
#define COND_QUESTION 289
#define COND_AMPER 290
#define COND_EXCLAM 291
#define COND_IDENT 292
#define COND_NUMBER 293
#define COND_LITER 294
#define COND_ERROR 295




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 111 "cond.y"
typedef union YYSTYPE {
    struct cond_str str;
    LPWSTR    string;
    INT       value;
} YYSTYPE;
/* Line 1447 of yacc.c.  */
#line 124 "cond.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif





