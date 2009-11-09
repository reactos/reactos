
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
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



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 111 "cond.y"

    struct cond_str str;
    LPWSTR    string;
    INT       value;



/* Line 1676 of yacc.c  */
#line 100 "cond.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif




