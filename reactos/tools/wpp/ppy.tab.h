/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
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
     tRCINCLUDE = 258,
     tIF = 259,
     tIFDEF = 260,
     tIFNDEF = 261,
     tELSE = 262,
     tELIF = 263,
     tENDIF = 264,
     tDEFINED = 265,
     tNL = 266,
     tINCLUDE = 267,
     tLINE = 268,
     tGCCLINE = 269,
     tERROR = 270,
     tWARNING = 271,
     tPRAGMA = 272,
     tPPIDENT = 273,
     tUNDEF = 274,
     tMACROEND = 275,
     tCONCAT = 276,
     tELIPSIS = 277,
     tSTRINGIZE = 278,
     tIDENT = 279,
     tLITERAL = 280,
     tMACRO = 281,
     tDEFINE = 282,
     tDQSTRING = 283,
     tSQSTRING = 284,
     tIQSTRING = 285,
     tUINT = 286,
     tSINT = 287,
     tULONG = 288,
     tSLONG = 289,
     tULONGLONG = 290,
     tSLONGLONG = 291,
     tRCINCLUDEPATH = 292,
     tLOGOR = 293,
     tLOGAND = 294,
     tNE = 295,
     tEQ = 296,
     tGTE = 297,
     tLTE = 298,
     tRSHIFT = 299,
     tLSHIFT = 300
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 126 "ppy.y"

	int		sint;
	unsigned int	uint;
	long		slong;
	unsigned long	ulong;
	wrc_sll_t	sll;
	wrc_ull_t	ull;
	int		*iptr;
	char		*cptr;
	cval_t		cval;
	marg_t		*marg;
	mtext_t		*mtext;



/* Line 2068 of yacc.c  */
#line 111 "ppy.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE ppy_lval;


