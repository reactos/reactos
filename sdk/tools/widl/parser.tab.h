/* A Bison parser, made by GNU Bison 3.0.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

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

#ifndef YY_PARSER_E_REACTOSSYNC_MSVC_HOST_TOOLS_SDK_TOOLS_WIDL_PARSER_TAB_H_INCLUDED
# define YY_PARSER_E_REACTOSSYNC_MSVC_HOST_TOOLS_SDK_TOOLS_WIDL_PARSER_TAB_H_INCLUDED
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
    tALLOCATE = 281,
    tANNOTATION = 282,
    tAPPOBJECT = 283,
    tASYNC = 284,
    tASYNCUUID = 285,
    tAUTOHANDLE = 286,
    tBINDABLE = 287,
    tBOOLEAN = 288,
    tBROADCAST = 289,
    tBYTE = 290,
    tBYTECOUNT = 291,
    tCALLAS = 292,
    tCALLBACK = 293,
    tCASE = 294,
    tCDECL = 295,
    tCHAR = 296,
    tCOCLASS = 297,
    tCODE = 298,
    tCOMMSTATUS = 299,
    tCONST = 300,
    tCONTEXTHANDLE = 301,
    tCONTEXTHANDLENOSERIALIZE = 302,
    tCONTEXTHANDLESERIALIZE = 303,
    tCONTROL = 304,
    tCPPQUOTE = 305,
    tDECODE = 306,
    tDEFAULT = 307,
    tDEFAULTBIND = 308,
    tDEFAULTCOLLELEM = 309,
    tDEFAULTVALUE = 310,
    tDEFAULTVTABLE = 311,
    tDISABLECONSISTENCYCHECK = 312,
    tDISPLAYBIND = 313,
    tDISPINTERFACE = 314,
    tDLLNAME = 315,
    tDOUBLE = 316,
    tDUAL = 317,
    tENABLEALLOCATE = 318,
    tENCODE = 319,
    tENDPOINT = 320,
    tENTRY = 321,
    tENUM = 322,
    tERRORSTATUST = 323,
    tEXPLICITHANDLE = 324,
    tEXTERN = 325,
    tFALSE = 326,
    tFASTCALL = 327,
    tFAULTSTATUS = 328,
    tFLOAT = 329,
    tFORCEALLOCATE = 330,
    tHANDLE = 331,
    tHANDLET = 332,
    tHELPCONTEXT = 333,
    tHELPFILE = 334,
    tHELPSTRING = 335,
    tHELPSTRINGCONTEXT = 336,
    tHELPSTRINGDLL = 337,
    tHIDDEN = 338,
    tHYPER = 339,
    tID = 340,
    tIDEMPOTENT = 341,
    tIGNORE = 342,
    tIIDIS = 343,
    tIMMEDIATEBIND = 344,
    tIMPLICITHANDLE = 345,
    tIMPORT = 346,
    tIMPORTLIB = 347,
    tIN = 348,
    tIN_LINE = 349,
    tINLINE = 350,
    tINPUTSYNC = 351,
    tINT = 352,
    tINT32 = 353,
    tINT3264 = 354,
    tINT64 = 355,
    tINTERFACE = 356,
    tLCID = 357,
    tLENGTHIS = 358,
    tLIBRARY = 359,
    tLICENSED = 360,
    tLOCAL = 361,
    tLONG = 362,
    tMAYBE = 363,
    tMESSAGE = 364,
    tMETHODS = 365,
    tMODULE = 366,
    tNAMESPACE = 367,
    tNOCODE = 368,
    tNONBROWSABLE = 369,
    tNONCREATABLE = 370,
    tNONEXTENSIBLE = 371,
    tNOTIFY = 372,
    tNOTIFYFLAG = 373,
    tNULL = 374,
    tOBJECT = 375,
    tODL = 376,
    tOLEAUTOMATION = 377,
    tOPTIMIZE = 378,
    tOPTIONAL = 379,
    tOUT = 380,
    tPARTIALIGNORE = 381,
    tPASCAL = 382,
    tPOINTERDEFAULT = 383,
    tPRAGMA_WARNING = 384,
    tPROGID = 385,
    tPROPERTIES = 386,
    tPROPGET = 387,
    tPROPPUT = 388,
    tPROPPUTREF = 389,
    tPROXY = 390,
    tPTR = 391,
    tPUBLIC = 392,
    tRANGE = 393,
    tREADONLY = 394,
    tREF = 395,
    tREGISTER = 396,
    tREPRESENTAS = 397,
    tREQUESTEDIT = 398,
    tRESTRICTED = 399,
    tRETVAL = 400,
    tSAFEARRAY = 401,
    tSHORT = 402,
    tSIGNED = 403,
    tSIZEIS = 404,
    tSIZEOF = 405,
    tSMALL = 406,
    tSOURCE = 407,
    tSTATIC = 408,
    tSTDCALL = 409,
    tSTRICTCONTEXTHANDLE = 410,
    tSTRING = 411,
    tSTRUCT = 412,
    tSWITCH = 413,
    tSWITCHIS = 414,
    tSWITCHTYPE = 415,
    tTHREADING = 416,
    tTRANSMITAS = 417,
    tTRUE = 418,
    tTYPEDEF = 419,
    tUIDEFAULT = 420,
    tUNION = 421,
    tUNIQUE = 422,
    tUNSIGNED = 423,
    tUSESGETLASTERROR = 424,
    tUSERMARSHAL = 425,
    tUUID = 426,
    tV1ENUM = 427,
    tVARARG = 428,
    tVERSION = 429,
    tVIPROGID = 430,
    tVOID = 431,
    tWCHAR = 432,
    tWIREMARSHAL = 433,
    tAPARTMENT = 434,
    tNEUTRAL = 435,
    tSINGLE = 436,
    tFREE = 437,
    tBOTH = 438,
    CAST = 439,
    PPTR = 440,
    POS = 441,
    NEG = 442,
    ADDRESSOF = 443
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 144 "parser.y" /* yacc.c:1909  */

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

#line 271 "parser.tab.h" /* yacc.c:1909  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE parser_lval;

int parser_parse (void);

#endif /* !YY_PARSER_E_REACTOSSYNC_MSVC_HOST_TOOLS_SDK_TOOLS_WIDL_PARSER_TAB_H_INCLUDED  */
