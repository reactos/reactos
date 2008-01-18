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
     tFLOAT = 309,
     tHANDLE = 310,
     tHANDLET = 311,
     tHELPCONTEXT = 312,
     tHELPFILE = 313,
     tHELPSTRING = 314,
     tHELPSTRINGCONTEXT = 315,
     tHELPSTRINGDLL = 316,
     tHIDDEN = 317,
     tHYPER = 318,
     tID = 319,
     tIDEMPOTENT = 320,
     tIIDIS = 321,
     tIMMEDIATEBIND = 322,
     tIMPLICITHANDLE = 323,
     tIMPORT = 324,
     tIMPORTLIB = 325,
     tIN = 326,
     tINLINE = 327,
     tINPUTSYNC = 328,
     tINT = 329,
     tINT64 = 330,
     tINTERFACE = 331,
     tLCID = 332,
     tLENGTHIS = 333,
     tLIBRARY = 334,
     tLOCAL = 335,
     tLONG = 336,
     tMETHODS = 337,
     tMODULE = 338,
     tNONBROWSABLE = 339,
     tNONCREATABLE = 340,
     tNONEXTENSIBLE = 341,
     tOBJECT = 342,
     tODL = 343,
     tOLEAUTOMATION = 344,
     tOPTIONAL = 345,
     tOUT = 346,
     tPOINTERDEFAULT = 347,
     tPROPERTIES = 348,
     tPROPGET = 349,
     tPROPPUT = 350,
     tPROPPUTREF = 351,
     tPTR = 352,
     tPUBLIC = 353,
     tRANGE = 354,
     tREADONLY = 355,
     tREF = 356,
     tREQUESTEDIT = 357,
     tRESTRICTED = 358,
     tRETVAL = 359,
     tSAFEARRAY = 360,
     tSHORT = 361,
     tSIGNED = 362,
     tSINGLE = 363,
     tSIZEIS = 364,
     tSIZEOF = 365,
     tSMALL = 366,
     tSOURCE = 367,
     tSTDCALL = 368,
     tSTRICTCONTEXTHANDLE = 369,
     tSTRING = 370,
     tSTRUCT = 371,
     tSWITCH = 372,
     tSWITCHIS = 373,
     tSWITCHTYPE = 374,
     tTRANSMITAS = 375,
     tTRUE = 376,
     tTYPEDEF = 377,
     tUNION = 378,
     tUNIQUE = 379,
     tUNSIGNED = 380,
     tUUID = 381,
     tV1ENUM = 382,
     tVARARG = 383,
     tVERSION = 384,
     tVOID = 385,
     tWCHAR = 386,
     tWIREMARSHAL = 387,
     CAST = 388,
     PPTR = 389,
     NEG = 390,
     ADDRESSOF = 391
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
#define tFLOAT 309
#define tHANDLE 310
#define tHANDLET 311
#define tHELPCONTEXT 312
#define tHELPFILE 313
#define tHELPSTRING 314
#define tHELPSTRINGCONTEXT 315
#define tHELPSTRINGDLL 316
#define tHIDDEN 317
#define tHYPER 318
#define tID 319
#define tIDEMPOTENT 320
#define tIIDIS 321
#define tIMMEDIATEBIND 322
#define tIMPLICITHANDLE 323
#define tIMPORT 324
#define tIMPORTLIB 325
#define tIN 326
#define tINLINE 327
#define tINPUTSYNC 328
#define tINT 329
#define tINT64 330
#define tINTERFACE 331
#define tLCID 332
#define tLENGTHIS 333
#define tLIBRARY 334
#define tLOCAL 335
#define tLONG 336
#define tMETHODS 337
#define tMODULE 338
#define tNONBROWSABLE 339
#define tNONCREATABLE 340
#define tNONEXTENSIBLE 341
#define tOBJECT 342
#define tODL 343
#define tOLEAUTOMATION 344
#define tOPTIONAL 345
#define tOUT 346
#define tPOINTERDEFAULT 347
#define tPROPERTIES 348
#define tPROPGET 349
#define tPROPPUT 350
#define tPROPPUTREF 351
#define tPTR 352
#define tPUBLIC 353
#define tRANGE 354
#define tREADONLY 355
#define tREF 356
#define tREQUESTEDIT 357
#define tRESTRICTED 358
#define tRETVAL 359
#define tSAFEARRAY 360
#define tSHORT 361
#define tSIGNED 362
#define tSINGLE 363
#define tSIZEIS 364
#define tSIZEOF 365
#define tSMALL 366
#define tSOURCE 367
#define tSTDCALL 368
#define tSTRICTCONTEXTHANDLE 369
#define tSTRING 370
#define tSTRUCT 371
#define tSWITCH 372
#define tSWITCHIS 373
#define tSWITCHTYPE 374
#define tTRANSMITAS 375
#define tTRUE 376
#define tTYPEDEF 377
#define tUNION 378
#define tUNIQUE 379
#define tUNSIGNED 380
#define tUUID 381
#define tV1ENUM 382
#define tVARARG 383
#define tVERSION 384
#define tVOID 385
#define tWCHAR 386
#define tWIREMARSHAL 387
#define CAST 388
#define PPTR 389
#define NEG 390
#define ADDRESSOF 391




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 136 "parser.y"
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
#line 333 "parser.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE parser_lval;



