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
     aUUID = 264,
     aEOF = 265,
     SHL = 266,
     SHR = 267,
     tAGGREGATABLE = 268,
     tALLOCATE = 269,
     tAPPOBJECT = 270,
     tASYNC = 271,
     tASYNCUUID = 272,
     tAUTOHANDLE = 273,
     tBINDABLE = 274,
     tBOOLEAN = 275,
     tBROADCAST = 276,
     tBYTE = 277,
     tBYTECOUNT = 278,
     tCALLAS = 279,
     tCALLBACK = 280,
     tCASE = 281,
     tCDECL = 282,
     tCHAR = 283,
     tCOCLASS = 284,
     tCODE = 285,
     tCOMMSTATUS = 286,
     tCONST = 287,
     tCONTEXTHANDLE = 288,
     tCONTEXTHANDLENOSERIALIZE = 289,
     tCONTEXTHANDLESERIALIZE = 290,
     tCONTROL = 291,
     tCPPQUOTE = 292,
     tDEFAULT = 293,
     tDEFAULTCOLLELEM = 294,
     tDEFAULTVALUE = 295,
     tDEFAULTVTABLE = 296,
     tDISPLAYBIND = 297,
     tDISPINTERFACE = 298,
     tDLLNAME = 299,
     tDOUBLE = 300,
     tDUAL = 301,
     tENDPOINT = 302,
     tENTRY = 303,
     tENUM = 304,
     tERRORSTATUST = 305,
     tEXPLICITHANDLE = 306,
     tEXTERN = 307,
     tFALSE = 308,
     tFASTCALL = 309,
     tFLOAT = 310,
     tHANDLE = 311,
     tHANDLET = 312,
     tHELPCONTEXT = 313,
     tHELPFILE = 314,
     tHELPSTRING = 315,
     tHELPSTRINGCONTEXT = 316,
     tHELPSTRINGDLL = 317,
     tHIDDEN = 318,
     tHYPER = 319,
     tID = 320,
     tIDEMPOTENT = 321,
     tIIDIS = 322,
     tIMMEDIATEBIND = 323,
     tIMPLICITHANDLE = 324,
     tIMPORT = 325,
     tIMPORTLIB = 326,
     tIN = 327,
     tINLINE = 328,
     tINPUTSYNC = 329,
     tINT = 330,
     tINT64 = 331,
     tINTERFACE = 332,
     tLCID = 333,
     tLENGTHIS = 334,
     tLIBRARY = 335,
     tLOCAL = 336,
     tLONG = 337,
     tMETHODS = 338,
     tMODULE = 339,
     tNONBROWSABLE = 340,
     tNONCREATABLE = 341,
     tNONEXTENSIBLE = 342,
     tOBJECT = 343,
     tODL = 344,
     tOLEAUTOMATION = 345,
     tOPTIONAL = 346,
     tOUT = 347,
     tPASCAL = 348,
     tPOINTERDEFAULT = 349,
     tPROPERTIES = 350,
     tPROPGET = 351,
     tPROPPUT = 352,
     tPROPPUTREF = 353,
     tPTR = 354,
     tPUBLIC = 355,
     tRANGE = 356,
     tREADONLY = 357,
     tREF = 358,
     tREQUESTEDIT = 359,
     tRESTRICTED = 360,
     tRETVAL = 361,
     tSAFEARRAY = 362,
     tSHORT = 363,
     tSIGNED = 364,
     tSINGLE = 365,
     tSIZEIS = 366,
     tSIZEOF = 367,
     tSMALL = 368,
     tSOURCE = 369,
     tSTDCALL = 370,
     tSTRICTCONTEXTHANDLE = 371,
     tSTRING = 372,
     tSTRUCT = 373,
     tSWITCH = 374,
     tSWITCHIS = 375,
     tSWITCHTYPE = 376,
     tTRANSMITAS = 377,
     tTRUE = 378,
     tTYPEDEF = 379,
     tUNION = 380,
     tUNIQUE = 381,
     tUNSIGNED = 382,
     tUUID = 383,
     tV1ENUM = 384,
     tVARARG = 385,
     tVERSION = 386,
     tVOID = 387,
     tWCHAR = 388,
     tWIREMARSHAL = 389,
     CAST = 390,
     PPTR = 391,
     NEG = 392,
     ADDRESSOF = 393
   };
#endif
/* Tokens.  */
#define aIDENTIFIER 258
#define aKNOWNTYPE 259
#define aNUM 260
#define aHEXNUM 261
#define aDOUBLE 262
#define aSTRING 263
#define aUUID 264
#define aEOF 265
#define SHL 266
#define SHR 267
#define tAGGREGATABLE 268
#define tALLOCATE 269
#define tAPPOBJECT 270
#define tASYNC 271
#define tASYNCUUID 272
#define tAUTOHANDLE 273
#define tBINDABLE 274
#define tBOOLEAN 275
#define tBROADCAST 276
#define tBYTE 277
#define tBYTECOUNT 278
#define tCALLAS 279
#define tCALLBACK 280
#define tCASE 281
#define tCDECL 282
#define tCHAR 283
#define tCOCLASS 284
#define tCODE 285
#define tCOMMSTATUS 286
#define tCONST 287
#define tCONTEXTHANDLE 288
#define tCONTEXTHANDLENOSERIALIZE 289
#define tCONTEXTHANDLESERIALIZE 290
#define tCONTROL 291
#define tCPPQUOTE 292
#define tDEFAULT 293
#define tDEFAULTCOLLELEM 294
#define tDEFAULTVALUE 295
#define tDEFAULTVTABLE 296
#define tDISPLAYBIND 297
#define tDISPINTERFACE 298
#define tDLLNAME 299
#define tDOUBLE 300
#define tDUAL 301
#define tENDPOINT 302
#define tENTRY 303
#define tENUM 304
#define tERRORSTATUST 305
#define tEXPLICITHANDLE 306
#define tEXTERN 307
#define tFALSE 308
#define tFASTCALL 309
#define tFLOAT 310
#define tHANDLE 311
#define tHANDLET 312
#define tHELPCONTEXT 313
#define tHELPFILE 314
#define tHELPSTRING 315
#define tHELPSTRINGCONTEXT 316
#define tHELPSTRINGDLL 317
#define tHIDDEN 318
#define tHYPER 319
#define tID 320
#define tIDEMPOTENT 321
#define tIIDIS 322
#define tIMMEDIATEBIND 323
#define tIMPLICITHANDLE 324
#define tIMPORT 325
#define tIMPORTLIB 326
#define tIN 327
#define tINLINE 328
#define tINPUTSYNC 329
#define tINT 330
#define tINT64 331
#define tINTERFACE 332
#define tLCID 333
#define tLENGTHIS 334
#define tLIBRARY 335
#define tLOCAL 336
#define tLONG 337
#define tMETHODS 338
#define tMODULE 339
#define tNONBROWSABLE 340
#define tNONCREATABLE 341
#define tNONEXTENSIBLE 342
#define tOBJECT 343
#define tODL 344
#define tOLEAUTOMATION 345
#define tOPTIONAL 346
#define tOUT 347
#define tPASCAL 348
#define tPOINTERDEFAULT 349
#define tPROPERTIES 350
#define tPROPGET 351
#define tPROPPUT 352
#define tPROPPUTREF 353
#define tPTR 354
#define tPUBLIC 355
#define tRANGE 356
#define tREADONLY 357
#define tREF 358
#define tREQUESTEDIT 359
#define tRESTRICTED 360
#define tRETVAL 361
#define tSAFEARRAY 362
#define tSHORT 363
#define tSIGNED 364
#define tSINGLE 365
#define tSIZEIS 366
#define tSIZEOF 367
#define tSMALL 368
#define tSOURCE 369
#define tSTDCALL 370
#define tSTRICTCONTEXTHANDLE 371
#define tSTRING 372
#define tSTRUCT 373
#define tSWITCH 374
#define tSWITCHIS 375
#define tSWITCHTYPE 376
#define tTRANSMITAS 377
#define tTRUE 378
#define tTYPEDEF 379
#define tUNION 380
#define tUNIQUE 381
#define tUNSIGNED 382
#define tUUID 383
#define tV1ENUM 384
#define tVARARG 385
#define tVERSION 386
#define tVOID 387
#define tWCHAR 388
#define tWIREMARSHAL 389
#define CAST 390
#define PPTR 391
#define NEG 392
#define ADDRESSOF 393




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 139 "parser.y"
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
	pident_t *pident;
	pident_list_t *pident_list;
	func_t *func;
	func_list_t *func_list;
	ifref_t *ifref;
	ifref_list_t *ifref_list;
	char *str;
	UUID *uuid;
	unsigned int num;
	double dbl;
	interface_info_t ifinfo;
} YYSTYPE;
/* Line 1447 of yacc.c.  */
#line 337 "parser.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE parser_lval;



