/* A Bison parser, made by GNU Bison 1.875b.  */

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
     tINCLUDE_NEXT = 268,
     tLINE = 269,
     tGCCLINE = 270,
     tERROR = 271,
     tWARNING = 272,
     tPRAGMA = 273,
     tPPIDENT = 274,
     tUNDEF = 275,
     tMACROEND = 276,
     tCONCAT = 277,
     tELIPSIS = 278,
     tSTRINGIZE = 279,
     tIDENT = 280,
     tLITERAL = 281,
     tMACRO = 282,
     tDEFINE = 283,
     tDQSTRING = 284,
     tSQSTRING = 285,
     tIQSTRING = 286,
     tUINT = 287,
     tSINT = 288,
     tULONG = 289,
     tSLONG = 290,
     tULONGLONG = 291,
     tSLONGLONG = 292,
     tRCINCLUDEPATH = 293,
     tLOGOR = 294,
     tLOGAND = 295,
     tNE = 296,
     tEQ = 297,
     tGTE = 298,
     tLTE = 299,
     tRSHIFT = 300,
     tLSHIFT = 301
   };
#endif
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
#define tINCLUDE_NEXT 268
#define tLINE 269
#define tGCCLINE 270
#define tERROR 271
#define tWARNING 272
#define tPRAGMA 273
#define tPPIDENT 274
#define tUNDEF 275
#define tMACROEND 276
#define tCONCAT 277
#define tELIPSIS 278
#define tSTRINGIZE 279
#define tIDENT 280
#define tLITERAL 281
#define tMACRO 282
#define tDEFINE 283
#define tDQSTRING 284
#define tSQSTRING 285
#define tIQSTRING 286
#define tUINT 287
#define tSINT 288
#define tULONG 289
#define tSLONG 290
#define tULONGLONG 291
#define tSLONGLONG 292
#define tRCINCLUDEPATH 293
#define tLOGOR 294
#define tLOGAND 295
#define tNE 296
#define tEQ 297
#define tGTE 298
#define tLTE 299
#define tRSHIFT 300
#define tLSHIFT 301




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 126 "wpp/ppy.y"
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
/* Line 1252 of yacc.c.  */
#line 143 "wpp/wpp.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE pplval;



