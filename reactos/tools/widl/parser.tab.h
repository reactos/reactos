
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
     aSQSTRING = 265,
     aUUID = 266,
     aEOF = 267,
     SHL = 268,
     SHR = 269,
     MEMBERPTR = 270,
     EQUALITY = 271,
     INEQUALITY = 272,
     GREATEREQUAL = 273,
     LESSEQUAL = 274,
     LOGICALOR = 275,
     LOGICALAND = 276,
     ELLIPSIS = 277,
     tAGGREGATABLE = 278,
     tALLOCATE = 279,
     tANNOTATION = 280,
     tAPPOBJECT = 281,
     tASYNC = 282,
     tASYNCUUID = 283,
     tAUTOHANDLE = 284,
     tBINDABLE = 285,
     tBOOLEAN = 286,
     tBROADCAST = 287,
     tBYTE = 288,
     tBYTECOUNT = 289,
     tCALLAS = 290,
     tCALLBACK = 291,
     tCASE = 292,
     tCDECL = 293,
     tCHAR = 294,
     tCOCLASS = 295,
     tCODE = 296,
     tCOMMSTATUS = 297,
     tCONST = 298,
     tCONTEXTHANDLE = 299,
     tCONTEXTHANDLENOSERIALIZE = 300,
     tCONTEXTHANDLESERIALIZE = 301,
     tCONTROL = 302,
     tCPPQUOTE = 303,
     tDEFAULT = 304,
     tDEFAULTCOLLELEM = 305,
     tDEFAULTVALUE = 306,
     tDEFAULTVTABLE = 307,
     tDISPLAYBIND = 308,
     tDISPINTERFACE = 309,
     tDLLNAME = 310,
     tDOUBLE = 311,
     tDUAL = 312,
     tENDPOINT = 313,
     tENTRY = 314,
     tENUM = 315,
     tERRORSTATUST = 316,
     tEXPLICITHANDLE = 317,
     tEXTERN = 318,
     tFALSE = 319,
     tFASTCALL = 320,
     tFLOAT = 321,
     tHANDLE = 322,
     tHANDLET = 323,
     tHELPCONTEXT = 324,
     tHELPFILE = 325,
     tHELPSTRING = 326,
     tHELPSTRINGCONTEXT = 327,
     tHELPSTRINGDLL = 328,
     tHIDDEN = 329,
     tHYPER = 330,
     tID = 331,
     tIDEMPOTENT = 332,
     tIIDIS = 333,
     tIMMEDIATEBIND = 334,
     tIMPLICITHANDLE = 335,
     tIMPORT = 336,
     tIMPORTLIB = 337,
     tIN = 338,
     tIN_LINE = 339,
     tINLINE = 340,
     tINPUTSYNC = 341,
     tINT = 342,
     tINT3264 = 343,
     tINT64 = 344,
     tINTERFACE = 345,
     tLCID = 346,
     tLENGTHIS = 347,
     tLIBRARY = 348,
     tLOCAL = 349,
     tLONG = 350,
     tMETHODS = 351,
     tMODULE = 352,
     tNONBROWSABLE = 353,
     tNONCREATABLE = 354,
     tNONEXTENSIBLE = 355,
     tNULL = 356,
     tOBJECT = 357,
     tODL = 358,
     tOLEAUTOMATION = 359,
     tOPTIONAL = 360,
     tOUT = 361,
     tPASCAL = 362,
     tPOINTERDEFAULT = 363,
     tPROPERTIES = 364,
     tPROPGET = 365,
     tPROPPUT = 366,
     tPROPPUTREF = 367,
     tPTR = 368,
     tPUBLIC = 369,
     tRANGE = 370,
     tREADONLY = 371,
     tREF = 372,
     tREGISTER = 373,
     tREQUESTEDIT = 374,
     tRESTRICTED = 375,
     tRETVAL = 376,
     tSAFEARRAY = 377,
     tSHORT = 378,
     tSIGNED = 379,
     tSIZEIS = 380,
     tSIZEOF = 381,
     tSMALL = 382,
     tSOURCE = 383,
     tSTATIC = 384,
     tSTDCALL = 385,
     tSTRICTCONTEXTHANDLE = 386,
     tSTRING = 387,
     tSTRUCT = 388,
     tSWITCH = 389,
     tSWITCHIS = 390,
     tSWITCHTYPE = 391,
     tTRANSMITAS = 392,
     tTRUE = 393,
     tTYPEDEF = 394,
     tUNION = 395,
     tUNIQUE = 396,
     tUNSIGNED = 397,
     tUUID = 398,
     tV1ENUM = 399,
     tVARARG = 400,
     tVERSION = 401,
     tVOID = 402,
     tWCHAR = 403,
     tWIREMARSHAL = 404,
     ADDRESSOF = 405,
     NEG = 406,
     POS = 407,
     PPTR = 408,
     CAST = 409
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
#line 237 "parser.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE parser_lval;


