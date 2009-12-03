
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
     aIDENTIFIER = 258,
     aKNOWNTYPE = 259,
     aNUM = 260,
     aHEXNUM = 261,
     aDOUBLE = 262,
     aSTRING = 263,
     aWSTRING = 264,
     aUUID = 265,
     aEOF = 266,
     SHL = 267,
     SHR = 268,
     MEMBERPTR = 269,
     EQUALITY = 270,
     INEQUALITY = 271,
     GREATEREQUAL = 272,
     LESSEQUAL = 273,
     LOGICALOR = 274,
     LOGICALAND = 275,
     ELLIPSIS = 276,
     tAGGREGATABLE = 277,
     tALLOCATE = 278,
     tANNOTATION = 279,
     tAPPOBJECT = 280,
     tASYNC = 281,
     tASYNCUUID = 282,
     tAUTOHANDLE = 283,
     tBINDABLE = 284,
     tBOOLEAN = 285,
     tBROADCAST = 286,
     tBYTE = 287,
     tBYTECOUNT = 288,
     tCALLAS = 289,
     tCALLBACK = 290,
     tCASE = 291,
     tCDECL = 292,
     tCHAR = 293,
     tCOCLASS = 294,
     tCODE = 295,
     tCOMMSTATUS = 296,
     tCONST = 297,
     tCONTEXTHANDLE = 298,
     tCONTEXTHANDLENOSERIALIZE = 299,
     tCONTEXTHANDLESERIALIZE = 300,
     tCONTROL = 301,
     tCPPQUOTE = 302,
     tDEFAULT = 303,
     tDEFAULTCOLLELEM = 304,
     tDEFAULTVALUE = 305,
     tDEFAULTVTABLE = 306,
     tDISPLAYBIND = 307,
     tDISPINTERFACE = 308,
     tDLLNAME = 309,
     tDOUBLE = 310,
     tDUAL = 311,
     tENDPOINT = 312,
     tENTRY = 313,
     tENUM = 314,
     tERRORSTATUST = 315,
     tEXPLICITHANDLE = 316,
     tEXTERN = 317,
     tFALSE = 318,
     tFASTCALL = 319,
     tFLOAT = 320,
     tHANDLE = 321,
     tHANDLET = 322,
     tHELPCONTEXT = 323,
     tHELPFILE = 324,
     tHELPSTRING = 325,
     tHELPSTRINGCONTEXT = 326,
     tHELPSTRINGDLL = 327,
     tHIDDEN = 328,
     tHYPER = 329,
     tID = 330,
     tIDEMPOTENT = 331,
     tIIDIS = 332,
     tIMMEDIATEBIND = 333,
     tIMPLICITHANDLE = 334,
     tIMPORT = 335,
     tIMPORTLIB = 336,
     tIN = 337,
     tIN_LINE = 338,
     tINLINE = 339,
     tINPUTSYNC = 340,
     tINT = 341,
     tINT3264 = 342,
     tINT64 = 343,
     tINTERFACE = 344,
     tLCID = 345,
     tLENGTHIS = 346,
     tLIBRARY = 347,
     tLOCAL = 348,
     tLONG = 349,
     tMETHODS = 350,
     tMODULE = 351,
     tNONBROWSABLE = 352,
     tNONCREATABLE = 353,
     tNONEXTENSIBLE = 354,
     tNULL = 355,
     tOBJECT = 356,
     tODL = 357,
     tOLEAUTOMATION = 358,
     tOPTIONAL = 359,
     tOUT = 360,
     tPASCAL = 361,
     tPOINTERDEFAULT = 362,
     tPROPERTIES = 363,
     tPROPGET = 364,
     tPROPPUT = 365,
     tPROPPUTREF = 366,
     tPTR = 367,
     tPUBLIC = 368,
     tRANGE = 369,
     tREADONLY = 370,
     tREF = 371,
     tREGISTER = 372,
     tREQUESTEDIT = 373,
     tRESTRICTED = 374,
     tRETVAL = 375,
     tSAFEARRAY = 376,
     tSHORT = 377,
     tSIGNED = 378,
     tSIZEIS = 379,
     tSIZEOF = 380,
     tSMALL = 381,
     tSOURCE = 382,
     tSTATIC = 383,
     tSTDCALL = 384,
     tSTRICTCONTEXTHANDLE = 385,
     tSTRING = 386,
     tSTRUCT = 387,
     tSWITCH = 388,
     tSWITCHIS = 389,
     tSWITCHTYPE = 390,
     tTRANSMITAS = 391,
     tTRUE = 392,
     tTYPEDEF = 393,
     tUNION = 394,
     tUNIQUE = 395,
     tUNSIGNED = 396,
     tUUID = 397,
     tV1ENUM = 398,
     tVARARG = 399,
     tVERSION = 400,
     tVOID = 401,
     tWCHAR = 402,
     tWIREMARSHAL = 403,
     ADDRESSOF = 404,
     NEG = 405,
     POS = 406,
     PPTR = 407,
     CAST = 408
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 156 "parser.y"

	attr_t *attr;
	attr_list_t *attr_list;
	str_list_t *str_list;
	expr_t *expr;
	expr_list_t *expr_list;
	array_dims_t *array_dims;
	type_t *type;
	var_t *var;
	var_list_t *var_list;
	declarator_t *declarator;
	declarator_list_t *declarator_list;
	func_t *func;
	func_list_t *func_list;
	statement_t *statement;
	statement_list_t *stmt_list;
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



/* Line 1676 of yacc.c  */
#line 236 "parser.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE parser_lval;


