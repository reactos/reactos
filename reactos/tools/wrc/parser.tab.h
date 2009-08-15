
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



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 238 "parser.y"

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



/* Line 1676 of yacc.c  */
#line 183 "parser.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE parser_lval;


