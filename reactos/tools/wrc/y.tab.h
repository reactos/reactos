/* A Bison parser, made by GNU Bison 1.875.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

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
     tAUTO3STATE = 279,
     tAUTOCHECKBOX = 280,
     tAUTORADIOBUTTON = 281,
     tCHECKBOX = 282,
     tDEFPUSHBUTTON = 283,
     tPUSHBUTTON = 284,
     tRADIOBUTTON = 285,
     tSTATE3 = 286,
     tGROUPBOX = 287,
     tCOMBOBOX = 288,
     tLISTBOX = 289,
     tSCROLLBAR = 290,
     tCONTROL = 291,
     tEDITTEXT = 292,
     tRTEXT = 293,
     tCTEXT = 294,
     tLTEXT = 295,
     tBLOCK = 296,
     tVALUE = 297,
     tSHIFT = 298,
     tALT = 299,
     tASCII = 300,
     tVIRTKEY = 301,
     tGRAYED = 302,
     tCHECKED = 303,
     tINACTIVE = 304,
     tNOINVERT = 305,
     tPURE = 306,
     tIMPURE = 307,
     tDISCARDABLE = 308,
     tLOADONCALL = 309,
     tPRELOAD = 310,
     tFIXED = 311,
     tMOVEABLE = 312,
     tCLASS = 313,
     tCAPTION = 314,
     tCHARACTERISTICS = 315,
     tEXSTYLE = 316,
     tSTYLE = 317,
     tVERSION = 318,
     tLANGUAGE = 319,
     tFILEVERSION = 320,
     tPRODUCTVERSION = 321,
     tFILEFLAGSMASK = 322,
     tFILEOS = 323,
     tFILETYPE = 324,
     tFILEFLAGS = 325,
     tFILESUBTYPE = 326,
     tMENUBARBREAK = 327,
     tMENUBREAK = 328,
     tMENUITEM = 329,
     tPOPUP = 330,
     tSEPARATOR = 331,
     tHELP = 332,
     tTOOLBAR = 333,
     tBUTTON = 334,
     tBEGIN = 335,
     tEND = 336,
     tDLGINIT = 337,
     tNOT = 338,
     pUPM = 339
   };
#endif
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
#define tAUTO3STATE 279
#define tAUTOCHECKBOX 280
#define tAUTORADIOBUTTON 281
#define tCHECKBOX 282
#define tDEFPUSHBUTTON 283
#define tPUSHBUTTON 284
#define tRADIOBUTTON 285
#define tSTATE3 286
#define tGROUPBOX 287
#define tCOMBOBOX 288
#define tLISTBOX 289
#define tSCROLLBAR 290
#define tCONTROL 291
#define tEDITTEXT 292
#define tRTEXT 293
#define tCTEXT 294
#define tLTEXT 295
#define tBLOCK 296
#define tVALUE 297
#define tSHIFT 298
#define tALT 299
#define tASCII 300
#define tVIRTKEY 301
#define tGRAYED 302
#define tCHECKED 303
#define tINACTIVE 304
#define tNOINVERT 305
#define tPURE 306
#define tIMPURE 307
#define tDISCARDABLE 308
#define tLOADONCALL 309
#define tPRELOAD 310
#define tFIXED 311
#define tMOVEABLE 312
#define tCLASS 313
#define tCAPTION 314
#define tCHARACTERISTICS 315
#define tEXSTYLE 316
#define tSTYLE 317
#define tVERSION 318
#define tLANGUAGE 319
#define tFILEVERSION 320
#define tPRODUCTVERSION 321
#define tFILEFLAGSMASK 322
#define tFILEOS 323
#define tFILETYPE 324
#define tFILEFLAGS 325
#define tFILESUBTYPE 326
#define tMENUBARBREAK 327
#define tMENUBREAK 328
#define tMENUITEM 329
#define tPOPUP 330
#define tSEPARATOR 331
#define tHELP 332
#define tTOOLBAR 333
#define tBUTTON 334
#define tBEGIN 335
#define tEND 336
#define tDLGINIT 337
#define tNOT 338
#define pUPM 339




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 240 "parser.y"
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
/* Line 1248 of yacc.c.  */
#line 247 "y.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



