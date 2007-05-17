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
/* Tokens.  */
#define tRCINCLUDE 258
#define tIF 259
#define tIFDEF 260
#define tIFNDEF 261
#define tELSE 262
#define tELIF 263
#define tENDIF 264
#define tDEFINED 265
#define tNL 266
#define tINCLUDE 267
#define tLINE 268
#define tGCCLINE 269
#define tERROR 270
#define tWARNING 271
#define tPRAGMA 272
#define tPPIDENT 273
#define tUNDEF 274
#define tMACROEND 275
#define tCONCAT 276
#define tELIPSIS 277
#define tSTRINGIZE 278
#define tIDENT 279
#define tLITERAL 280
#define tMACRO 281
#define tDEFINE 282
#define tDQSTRING 283
#define tSQSTRING 284
#define tIQSTRING 285
#define tUINT 286
#define tSINT 287
#define tULONG 288
#define tSLONG 289
#define tULONGLONG 290
#define tSLONGLONG 291
#define tRCINCLUDEPATH 292
#define tLOGOR 293
#define tLOGAND 294
#define tNE 295
#define tEQ 296
#define tGTE 297
#define tLTE 298
#define tRSHIFT 299
#define tLSHIFT 300




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 126 "tools\\wpp_new\\ppy.y"
typedef union YYSTYPE {
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
} YYSTYPE;
/* Line 1447 of yacc.c.  */
#line 142 "tools\\wpp_new\\ppy.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE ppy_lval;



