/* A Bison parser, made by GNU Bison 3.5.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

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

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

#ifndef YY_PARSER_PARSER_TAB_H_INCLUDED
# define YY_PARSER_PARSER_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int parser_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    aIDENTIFIER = 258,
    aPRAGMA = 259,
    aKNOWNTYPE = 260,
    aNUM = 261,
    aHEXNUM = 262,
    aDOUBLE = 263,
    aSTRING = 264,
    aWSTRING = 265,
    aSQSTRING = 266,
    aUUID = 267,
    aEOF = 268,
    aACF = 269,
    SHL = 270,
    SHR = 271,
    MEMBERPTR = 272,
    EQUALITY = 273,
    INEQUALITY = 274,
    GREATEREQUAL = 275,
    LESSEQUAL = 276,
    LOGICALOR = 277,
    LOGICALAND = 278,
    ELLIPSIS = 279,
    tAGGREGATABLE = 280,
    tALLNODES = 281,
    tALLOCATE = 282,
    tANNOTATION = 283,
    tAPPOBJECT = 284,
    tASYNC = 285,
    tASYNCUUID = 286,
    tAUTOHANDLE = 287,
    tBINDABLE = 288,
    tBOOLEAN = 289,
    tBROADCAST = 290,
    tBYTE = 291,
    tBYTECOUNT = 292,
    tCALLAS = 293,
    tCALLBACK = 294,
    tCASE = 295,
    tCDECL = 296,
    tCHAR = 297,
    tCOCLASS = 298,
    tCODE = 299,
    tCOMMSTATUS = 300,
    tCONST = 301,
    tCONTEXTHANDLE = 302,
    tCONTEXTHANDLENOSERIALIZE = 303,
    tCONTEXTHANDLESERIALIZE = 304,
    tCONTROL = 305,
    tCPPQUOTE = 306,
    tDECODE = 307,
    tDEFAULT = 308,
    tDEFAULTBIND = 309,
    tDEFAULTCOLLELEM = 310,
    tDEFAULTVALUE = 311,
    tDEFAULTVTABLE = 312,
    tDISABLECONSISTENCYCHECK = 313,
    tDISPLAYBIND = 314,
    tDISPINTERFACE = 315,
    tDLLNAME = 316,
    tDONTFREE = 317,
    tDOUBLE = 318,
    tDUAL = 319,
    tENABLEALLOCATE = 320,
    tENCODE = 321,
    tENDPOINT = 322,
    tENTRY = 323,
    tENUM = 324,
    tERRORSTATUST = 325,
    tEXPLICITHANDLE = 326,
    tEXTERN = 327,
    tFALSE = 328,
    tFASTCALL = 329,
    tFAULTSTATUS = 330,
    tFLOAT = 331,
    tFORCEALLOCATE = 332,
    tHANDLE = 333,
    tHANDLET = 334,
    tHELPCONTEXT = 335,
    tHELPFILE = 336,
    tHELPSTRING = 337,
    tHELPSTRINGCONTEXT = 338,
    tHELPSTRINGDLL = 339,
    tHIDDEN = 340,
    tHYPER = 341,
    tID = 342,
    tIDEMPOTENT = 343,
    tIGNORE = 344,
    tIIDIS = 345,
    tIMMEDIATEBIND = 346,
    tIMPLICITHANDLE = 347,
    tIMPORT = 348,
    tIMPORTLIB = 349,
    tIN = 350,
    tIN_LINE = 351,
    tINLINE = 352,
    tINPUTSYNC = 353,
    tINT = 354,
    tINT32 = 355,
    tINT3264 = 356,
    tINT64 = 357,
    tINTERFACE = 358,
    tLCID = 359,
    tLENGTHIS = 360,
    tLIBRARY = 361,
    tLICENSED = 362,
    tLOCAL = 363,
    tLONG = 364,
    tMAYBE = 365,
    tMESSAGE = 366,
    tMETHODS = 367,
    tMODULE = 368,
    tNAMESPACE = 369,
    tNOCODE = 370,
    tNONBROWSABLE = 371,
    tNONCREATABLE = 372,
    tNONEXTENSIBLE = 373,
    tNOTIFY = 374,
    tNOTIFYFLAG = 375,
    tNULL = 376,
    tOBJECT = 377,
    tODL = 378,
    tOLEAUTOMATION = 379,
    tOPTIMIZE = 380,
    tOPTIONAL = 381,
    tOUT = 382,
    tPARTIALIGNORE = 383,
    tPASCAL = 384,
    tPOINTERDEFAULT = 385,
    tPRAGMA_WARNING = 386,
    tPROGID = 387,
    tPROPERTIES = 388,
    tPROPGET = 389,
    tPROPPUT = 390,
    tPROPPUTREF = 391,
    tPROXY = 392,
    tPTR = 393,
    tPUBLIC = 394,
    tRANGE = 395,
    tREADONLY = 396,
    tREF = 397,
    tREGISTER = 398,
    tREPRESENTAS = 399,
    tREQUESTEDIT = 400,
    tRESTRICTED = 401,
    tRETVAL = 402,
    tSAFEARRAY = 403,
    tSHORT = 404,
    tSIGNED = 405,
    tSINGLENODE = 406,
    tSIZEIS = 407,
    tSIZEOF = 408,
    tSMALL = 409,
    tSOURCE = 410,
    tSTATIC = 411,
    tSTDCALL = 412,
    tSTRICTCONTEXTHANDLE = 413,
    tSTRING = 414,
    tSTRUCT = 415,
    tSWITCH = 416,
    tSWITCHIS = 417,
    tSWITCHTYPE = 418,
    tTHREADING = 419,
    tTRANSMITAS = 420,
    tTRUE = 421,
    tTYPEDEF = 422,
    tUIDEFAULT = 423,
    tUNION = 424,
    tUNIQUE = 425,
    tUNSIGNED = 426,
    tUSESGETLASTERROR = 427,
    tUSERMARSHAL = 428,
    tUUID = 429,
    tV1ENUM = 430,
    tVARARG = 431,
    tVERSION = 432,
    tVIPROGID = 433,
    tVOID = 434,
    tWCHAR = 435,
    tWIREMARSHAL = 436,
    tAPARTMENT = 437,
    tNEUTRAL = 438,
    tSINGLE = 439,
    tFREE = 440,
    tBOTH = 441,
    CAST = 442,
    PPTR = 443,
    POS = 444,
    NEG = 445,
    ADDRESSOF = 446
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 144 "parser.y"

	attr_t *attr;
	attr_list_t *attr_list;
	str_list_t *str_list;
	expr_t *expr;
	expr_list_t *expr_list;
	type_t *type;
	var_t *var;
	var_list_t *var_list;
	declarator_t *declarator;
	declarator_list_t *declarator_list;
	statement_t *statement;
	statement_list_t *stmt_list;
	warning_t *warning;
	warning_list_t *warning_list;
	ifref_t *ifref;
	ifref_list_t *ifref_list;
	char *str;
	UUID *uuid;
	unsigned int num;
	double dbl;
	interface_info_t ifinfo;
	typelib_t *typelib;
	struct _import_t *import;
	struct _decl_spec_t *declspec;
	enum storage_class stgclass;

#line 277 "parser.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE parser_lval;

int parser_parse (void);

#endif /* !YY_PARSER_PARSER_TAB_H_INCLUDED  */
