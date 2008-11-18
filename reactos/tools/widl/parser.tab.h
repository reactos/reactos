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
     tAGGREGATABLE = 276,
     tALLOCATE = 277,
     tAPPOBJECT = 278,
     tASYNC = 279,
     tASYNCUUID = 280,
     tAUTOHANDLE = 281,
     tBINDABLE = 282,
     tBOOLEAN = 283,
     tBROADCAST = 284,
     tBYTE = 285,
     tBYTECOUNT = 286,
     tCALLAS = 287,
     tCALLBACK = 288,
     tCASE = 289,
     tCDECL = 290,
     tCHAR = 291,
     tCOCLASS = 292,
     tCODE = 293,
     tCOMMSTATUS = 294,
     tCONST = 295,
     tCONTEXTHANDLE = 296,
     tCONTEXTHANDLENOSERIALIZE = 297,
     tCONTEXTHANDLESERIALIZE = 298,
     tCONTROL = 299,
     tCPPQUOTE = 300,
     tDEFAULT = 301,
     tDEFAULTCOLLELEM = 302,
     tDEFAULTVALUE = 303,
     tDEFAULTVTABLE = 304,
     tDISPLAYBIND = 305,
     tDISPINTERFACE = 306,
     tDLLNAME = 307,
     tDOUBLE = 308,
     tDUAL = 309,
     tENDPOINT = 310,
     tENTRY = 311,
     tENUM = 312,
     tERRORSTATUST = 313,
     tEXPLICITHANDLE = 314,
     tEXTERN = 315,
     tFALSE = 316,
     tFASTCALL = 317,
     tFLOAT = 318,
     tHANDLE = 319,
     tHANDLET = 320,
     tHELPCONTEXT = 321,
     tHELPFILE = 322,
     tHELPSTRING = 323,
     tHELPSTRINGCONTEXT = 324,
     tHELPSTRINGDLL = 325,
     tHIDDEN = 326,
     tHYPER = 327,
     tID = 328,
     tIDEMPOTENT = 329,
     tIIDIS = 330,
     tIMMEDIATEBIND = 331,
     tIMPLICITHANDLE = 332,
     tIMPORT = 333,
     tIMPORTLIB = 334,
     tIN = 335,
     tIN_LINE = 336,
     tINLINE = 337,
     tINPUTSYNC = 338,
     tINT = 339,
     tINT64 = 340,
     tINTERFACE = 341,
     tLCID = 342,
     tLENGTHIS = 343,
     tLIBRARY = 344,
     tLOCAL = 345,
     tLONG = 346,
     tMETHODS = 347,
     tMODULE = 348,
     tNONBROWSABLE = 349,
     tNONCREATABLE = 350,
     tNONEXTENSIBLE = 351,
     tNULL = 352,
     tOBJECT = 353,
     tODL = 354,
     tOLEAUTOMATION = 355,
     tOPTIONAL = 356,
     tOUT = 357,
     tPASCAL = 358,
     tPOINTERDEFAULT = 359,
     tPROPERTIES = 360,
     tPROPGET = 361,
     tPROPPUT = 362,
     tPROPPUTREF = 363,
     tPTR = 364,
     tPUBLIC = 365,
     tRANGE = 366,
     tREADONLY = 367,
     tREF = 368,
     tREGISTER = 369,
     tREQUESTEDIT = 370,
     tRESTRICTED = 371,
     tRETVAL = 372,
     tSAFEARRAY = 373,
     tSHORT = 374,
     tSIGNED = 375,
     tSINGLE = 376,
     tSIZEIS = 377,
     tSIZEOF = 378,
     tSMALL = 379,
     tSOURCE = 380,
     tSTATIC = 381,
     tSTDCALL = 382,
     tSTRICTCONTEXTHANDLE = 383,
     tSTRING = 384,
     tSTRUCT = 385,
     tSWITCH = 386,
     tSWITCHIS = 387,
     tSWITCHTYPE = 388,
     tTRANSMITAS = 389,
     tTRUE = 390,
     tTYPEDEF = 391,
     tUNION = 392,
     tUNIQUE = 393,
     tUNSIGNED = 394,
     tUUID = 395,
     tV1ENUM = 396,
     tVARARG = 397,
     tVERSION = 398,
     tVOID = 399,
     tWCHAR = 400,
     tWIREMARSHAL = 401,
     ADDRESSOF = 402,
     NEG = 403,
     POS = 404,
     PPTR = 405,
     CAST = 406
   };
#endif
/* Tokens.  */
#define aIDENTIFIER 258
#define aKNOWNTYPE 259
#define aNUM 260
#define aHEXNUM 261
#define aDOUBLE 262
#define aSTRING 263
#define aWSTRING 264
#define aUUID 265
#define aEOF 266
#define SHL 267
#define SHR 268
#define MEMBERPTR 269
#define EQUALITY 270
#define INEQUALITY 271
#define GREATEREQUAL 272
#define LESSEQUAL 273
#define LOGICALOR 274
#define LOGICALAND 275
#define tAGGREGATABLE 276
#define tALLOCATE 277
#define tAPPOBJECT 278
#define tASYNC 279
#define tASYNCUUID 280
#define tAUTOHANDLE 281
#define tBINDABLE 282
#define tBOOLEAN 283
#define tBROADCAST 284
#define tBYTE 285
#define tBYTECOUNT 286
#define tCALLAS 287
#define tCALLBACK 288
#define tCASE 289
#define tCDECL 290
#define tCHAR 291
#define tCOCLASS 292
#define tCODE 293
#define tCOMMSTATUS 294
#define tCONST 295
#define tCONTEXTHANDLE 296
#define tCONTEXTHANDLENOSERIALIZE 297
#define tCONTEXTHANDLESERIALIZE 298
#define tCONTROL 299
#define tCPPQUOTE 300
#define tDEFAULT 301
#define tDEFAULTCOLLELEM 302
#define tDEFAULTVALUE 303
#define tDEFAULTVTABLE 304
#define tDISPLAYBIND 305
#define tDISPINTERFACE 306
#define tDLLNAME 307
#define tDOUBLE 308
#define tDUAL 309
#define tENDPOINT 310
#define tENTRY 311
#define tENUM 312
#define tERRORSTATUST 313
#define tEXPLICITHANDLE 314
#define tEXTERN 315
#define tFALSE 316
#define tFASTCALL 317
#define tFLOAT 318
#define tHANDLE 319
#define tHANDLET 320
#define tHELPCONTEXT 321
#define tHELPFILE 322
#define tHELPSTRING 323
#define tHELPSTRINGCONTEXT 324
#define tHELPSTRINGDLL 325
#define tHIDDEN 326
#define tHYPER 327
#define tID 328
#define tIDEMPOTENT 329
#define tIIDIS 330
#define tIMMEDIATEBIND 331
#define tIMPLICITHANDLE 332
#define tIMPORT 333
#define tIMPORTLIB 334
#define tIN 335
#define tIN_LINE 336
#define tINLINE 337
#define tINPUTSYNC 338
#define tINT 339
#define tINT64 340
#define tINTERFACE 341
#define tLCID 342
#define tLENGTHIS 343
#define tLIBRARY 344
#define tLOCAL 345
#define tLONG 346
#define tMETHODS 347
#define tMODULE 348
#define tNONBROWSABLE 349
#define tNONCREATABLE 350
#define tNONEXTENSIBLE 351
#define tNULL 352
#define tOBJECT 353
#define tODL 354
#define tOLEAUTOMATION 355
#define tOPTIONAL 356
#define tOUT 357
#define tPASCAL 358
#define tPOINTERDEFAULT 359
#define tPROPERTIES 360
#define tPROPGET 361
#define tPROPPUT 362
#define tPROPPUTREF 363
#define tPTR 364
#define tPUBLIC 365
#define tRANGE 366
#define tREADONLY 367
#define tREF 368
#define tREGISTER 369
#define tREQUESTEDIT 370
#define tRESTRICTED 371
#define tRETVAL 372
#define tSAFEARRAY 373
#define tSHORT 374
#define tSIGNED 375
#define tSINGLE 376
#define tSIZEIS 377
#define tSIZEOF 378
#define tSMALL 379
#define tSOURCE 380
#define tSTATIC 381
#define tSTDCALL 382
#define tSTRICTCONTEXTHANDLE 383
#define tSTRING 384
#define tSTRUCT 385
#define tSWITCH 386
#define tSWITCHIS 387
#define tSWITCHTYPE 388
#define tTRANSMITAS 389
#define tTRUE 390
#define tTYPEDEF 391
#define tUNION 392
#define tUNIQUE 393
#define tUNSIGNED 394
#define tUUID 395
#define tV1ENUM 396
#define tVARARG 397
#define tVERSION 398
#define tVOID 399
#define tWCHAR 400
#define tWIREMARSHAL 401
#define ADDRESSOF 402
#define NEG 403
#define POS 404
#define PPTR 405
#define CAST 406




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 177 "parser.y"
typedef union YYSTYPE {
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
} YYSTYPE;
/* Line 1447 of yacc.c.  */
#line 369 "parser.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE parser_lval;



