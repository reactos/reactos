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
     tSEVNAMES = 258,
     tFACNAMES = 259,
     tLANNAMES = 260,
     tBASE = 261,
     tCODEPAGE = 262,
     tTYPEDEF = 263,
     tNL = 264,
     tSYMNAME = 265,
     tMSGEND = 266,
     tSEVERITY = 267,
     tFACILITY = 268,
     tLANGUAGE = 269,
     tMSGID = 270,
     tIDENT = 271,
     tLINE = 272,
     tFILE = 273,
     tCOMMENT = 274,
     tNUMBER = 275,
     tTOKEN = 276
   };
#endif
/* Tokens.  */
#define tSEVNAMES 258
#define tFACNAMES 259
#define tLANNAMES 260
#define tBASE 261
#define tCODEPAGE 262
#define tTYPEDEF 263
#define tNL 264
#define tSYMNAME 265
#define tMSGEND 266
#define tSEVERITY 267
#define tFACILITY 268
#define tLANGUAGE 269
#define tMSGID 270
#define tIDENT 271
#define tLINE 272
#define tFILE 273
#define tCOMMENT 274
#define tNUMBER 275
#define tTOKEN 276




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 98 "tools/wmc/mcy.y"
typedef union YYSTYPE {
	WCHAR		*str;
	unsigned	num;
	token_t		*tok;
	lanmsg_t	*lmp;
	msg_t		*msg;
	lan_cp_t	lcp;
} YYSTYPE;
/* Line 1447 of yacc.c.  */
#line 89 "tools/wmc/mcy.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE mcy_lval;



