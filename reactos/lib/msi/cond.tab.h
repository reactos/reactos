/* A Bison parser, made by GNU Bison 1.875c.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

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
     COND_LT = 263,
     COND_GT = 264,
     COND_EQ = 265,
     COND_LPAR = 266,
     COND_RPAR = 267,
     COND_TILDA = 268,
     COND_PERCENT = 269,
     COND_DOLLARS = 270,
     COND_QUESTION = 271,
     COND_AMPER = 272,
     COND_EXCLAM = 273,
     COND_IDENT = 274,
     COND_NUMBER = 275,
     COND_LITER = 276,
     COND_ERROR = 277
   };
#endif
#define COND_SPACE 258
#define COND_EOF 259
#define COND_OR 260
#define COND_AND 261
#define COND_NOT 262
#define COND_LT 263
#define COND_GT 264
#define COND_EQ 265
#define COND_LPAR 266
#define COND_RPAR 267
#define COND_TILDA 268
#define COND_PERCENT 269
#define COND_DOLLARS 270
#define COND_QUESTION 271
#define COND_AMPER 272
#define COND_EXCLAM 273
#define COND_IDENT 274
#define COND_NUMBER 275
#define COND_LITER 276
#define COND_ERROR 277




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 106 "./cond.y"
typedef union YYSTYPE {
    struct cond_str str;
    LPWSTR    string;
    INT       value;
    comp_int  fn_comp_int;
    comp_str  fn_comp_str;
    comp_m1   fn_comp_m1;
    comp_m2   fn_comp_m2;
} YYSTYPE;
/* Line 1275 of yacc.c.  */
#line 91 "cond.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif





