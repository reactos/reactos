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
     tNL = 258,
     tNUMBER = 259,
     tLNUMBER = 260,
     tSTRING = 261,
     tIDENT = 262,
     tFILENAME = 263,
     tRAWDATA = 264,
     tACCELERATORS = 265,
     tBITMAP = 266,
     tCURSOR = 267,
     tDIALOG = 268,
     tDIALOGEX = 269,
     tMENU = 270,
     tMENUEX = 271,
     tMESSAGETABLE = 272,
     tRCDATA = 273,
     tVERSIONINFO = 274,
     tSTRINGTABLE = 275,
     tFONT = 276,
     tFONTDIR = 277,
     tICON = 278,
     tHTML = 279,
     tAUTO3STATE = 280,
     tAUTOCHECKBOX = 281,
     tAUTORADIOBUTTON = 282,
     tCHECKBOX = 283,
     tDEFPUSHBUTTON = 284,
     tPUSHBUTTON = 285,
     tRADIOBUTTON = 286,
     tSTATE3 = 287,
     tGROUPBOX = 288,
     tCOMBOBOX = 289,
     tLISTBOX = 290,
     tSCROLLBAR = 291,
     tCONTROL = 292,
     tEDITTEXT = 293,
     tRTEXT = 294,
     tCTEXT = 295,
     tLTEXT = 296,
     tBLOCK = 297,
     tVALUE = 298,
     tSHIFT = 299,
     tALT = 300,
     tASCII = 301,
     tVIRTKEY = 302,
     tGRAYED = 303,
     tCHECKED = 304,
     tINACTIVE = 305,
     tNOINVERT = 306,
     tPURE = 307,
     tIMPURE = 308,
     tDISCARDABLE = 309,
     tLOADONCALL = 310,
     tPRELOAD = 311,
     tFIXED = 312,
     tMOVEABLE = 313,
     tCLASS = 314,
     tCAPTION = 315,
     tCHARACTERISTICS = 316,
     tEXSTYLE = 317,
     tSTYLE = 318,
     tVERSION = 319,
     tLANGUAGE = 320,
     tFILEVERSION = 321,
     tPRODUCTVERSION = 322,
     tFILEFLAGSMASK = 323,
     tFILEOS = 324,
     tFILETYPE = 325,
     tFILEFLAGS = 326,
     tFILESUBTYPE = 327,
     tMENUBARBREAK = 328,
     tMENUBREAK = 329,
     tMENUITEM = 330,
     tPOPUP = 331,
     tSEPARATOR = 332,
     tHELP = 333,
     tTOOLBAR = 334,
     tBUTTON = 335,
     tBEGIN = 336,
     tEND = 337,
     tDLGINIT = 338,
     tNOT = 339,
     pUPM = 340
   };
#endif
/* Tokens.  */
#define tNL 258
#define tNUMBER 259
#define tLNUMBER 260
#define tSTRING 261
#define tIDENT 262
#define tFILENAME 263
#define tRAWDATA 264
#define tACCELERATORS 265
#define tBITMAP 266
#define tCURSOR 267
#define tDIALOG 268
#define tDIALOGEX 269
#define tMENU 270
#define tMENUEX 271
#define tMESSAGETABLE 272
#define tRCDATA 273
#define tVERSIONINFO 274
#define tSTRINGTABLE 275
#define tFONT 276
#define tFONTDIR 277
#define tICON 278
#define tHTML 279
#define tAUTO3STATE 280
#define tAUTOCHECKBOX 281
#define tAUTORADIOBUTTON 282
#define tCHECKBOX 283
#define tDEFPUSHBUTTON 284
#define tPUSHBUTTON 285
#define tRADIOBUTTON 286
#define tSTATE3 287
#define tGROUPBOX 288
#define tCOMBOBOX 289
#define tLISTBOX 290
#define tSCROLLBAR 291
#define tCONTROL 292
#define tEDITTEXT 293
#define tRTEXT 294
#define tCTEXT 295
#define tLTEXT 296
#define tBLOCK 297
#define tVALUE 298
#define tSHIFT 299
#define tALT 300
#define tASCII 301
#define tVIRTKEY 302
#define tGRAYED 303
#define tCHECKED 304
#define tINACTIVE 305
#define tNOINVERT 306
#define tPURE 307
#define tIMPURE 308
#define tDISCARDABLE 309
#define tLOADONCALL 310
#define tPRELOAD 311
#define tFIXED 312
#define tMOVEABLE 313
#define tCLASS 314
#define tCAPTION 315
#define tCHARACTERISTICS 316
#define tEXSTYLE 317
#define tSTYLE 318
#define tVERSION 319
#define tLANGUAGE 320
#define tFILEVERSION 321
#define tPRODUCTVERSION 322
#define tFILEFLAGSMASK 323
#define tFILEOS 324
#define tFILETYPE 325
#define tFILEFLAGS 326
#define tFILESUBTYPE 327
#define tMENUBARBREAK 328
#define tMENUBREAK 329
#define tMENUITEM 330
#define tPOPUP 331
#define tSEPARATOR 332
#define tHELP 333
#define tTOOLBAR 334
#define tBUTTON 335
#define tBEGIN 336
#define tEND 337
#define tDLGINIT 338
#define tNOT 339
#define pUPM 340




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 241 "parser.y"
typedef union YYSTYPE {
	string_t	*str;
	int		num;
	int		*iptr;
	char		*cptr;
	resource_t	*res;
	accelerator_t	*acc;
	bitmap_t	*bmp;
	dialog_t	*dlg;
	dialogex_t	*dlgex;
	font_t		*fnt;
	fontdir_t	*fnd;
	menu_t		*men;
	menuex_t	*menex;
	html_t		*html;
	rcdata_t	*rdt;
	stringtable_t	*stt;
	stt_entry_t	*stte;
	user_t		*usr;
	messagetable_t	*msg;
	versioninfo_t	*veri;
	control_t	*ctl;
	name_id_t	*nid;
	font_id_t	*fntid;
	language_t	*lan;
	version_t	*ver;
	characts_t	*chars;
	event_t		*event;
	menu_item_t	*menitm;
	menuex_item_t	*menexitm;
	itemex_opt_t	*exopt;
	raw_data_t	*raw;
	lvc_t		*lvc;
	ver_value_t	*val;
	ver_block_t	*blk;
	ver_words_t	*verw;
	toolbar_t	*tlbar;
	toolbar_item_t	*tlbarItems;
	dlginit_t       *dginit;
	style_pair_t	*styles;
	style_t		*style;
	ani_any_t	*ani;
} YYSTYPE;
/* Line 1447 of yacc.c.  */
#line 252 "parser.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE parser_lval;



