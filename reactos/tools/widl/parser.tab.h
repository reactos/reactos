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
     aIDENTIFIER = 258,
     aKNOWNTYPE = 259,
     aNUM = 260,
     aHEXNUM = 261,
     aSTRING = 262,
     aUUID = 263,
     aEOF = 264,
     SHL = 265,
     SHR = 266,
     tAGGREGATABLE = 267,
     tALLOCATE = 268,
     tAPPOBJECT = 269,
     tASYNC = 270,
     tASYNCUUID = 271,
     tAUTOHANDLE = 272,
     tBINDABLE = 273,
     tBOOLEAN = 274,
     tBROADCAST = 275,
     tBYTE = 276,
     tBYTECOUNT = 277,
     tCALLAS = 278,
     tCALLBACK = 279,
     tCASE = 280,
     tCDECL = 281,
     tCHAR = 282,
     tCOCLASS = 283,
     tCODE = 284,
     tCOMMSTATUS = 285,
     tCONST = 286,
     tCONTEXTHANDLE = 287,
     tCONTEXTHANDLENOSERIALIZE = 288,
     tCONTEXTHANDLESERIALIZE = 289,
     tCONTROL = 290,
     tCPPQUOTE = 291,
     tDEFAULT = 292,
     tDEFAULTCOLLELEM = 293,
     tDEFAULTVALUE = 294,
     tDEFAULTVTABLE = 295,
     tDISPLAYBIND = 296,
     tDISPINTERFACE = 297,
     tDLLNAME = 298,
     tDOUBLE = 299,
     tDUAL = 300,
     tENDPOINT = 301,
     tENTRY = 302,
     tENUM = 303,
     tERRORSTATUST = 304,
     tEXPLICITHANDLE = 305,
     tEXTERN = 306,
     tFALSE = 307,
     tFLOAT = 308,
     tHANDLE = 309,
     tHANDLET = 310,
     tHELPCONTEXT = 311,
     tHELPFILE = 312,
     tHELPSTRING = 313,
     tHELPSTRINGCONTEXT = 314,
     tHELPSTRINGDLL = 315,
     tHIDDEN = 316,
     tHYPER = 317,
     tID = 318,
     tIDEMPOTENT = 319,
     tIIDIS = 320,
     tIMMEDIATEBIND = 321,
     tIMPLICITHANDLE = 322,
     tIMPORT = 323,
     tIMPORTLIB = 324,
     tIN = 325,
     tINLINE = 326,
     tINPUTSYNC = 327,
     tINT = 328,
     tINT64 = 329,
     tINTERFACE = 330,
     tLCID = 331,
     tLENGTHIS = 332,
     tLIBRARY = 333,
     tLOCAL = 334,
     tLONG = 335,
     tMETHODS = 336,
     tMODULE = 337,
     tNONBROWSABLE = 338,
     tNONCREATABLE = 339,
     tNONEXTENSIBLE = 340,
     tOBJECT = 341,
     tODL = 342,
     tOLEAUTOMATION = 343,
     tOPTIONAL = 344,
     tOUT = 345,
     tPOINTERDEFAULT = 346,
     tPROPERTIES = 347,
     tPROPGET = 348,
     tPROPPUT = 349,
     tPROPPUTREF = 350,
     tPTR = 351,
     tPUBLIC = 352,
     tRANGE = 353,
     tREADONLY = 354,
     tREF = 355,
     tREQUESTEDIT = 356,
     tRESTRICTED = 357,
     tRETVAL = 358,
     tSAFEARRAY = 359,
     tSHORT = 360,
     tSIGNED = 361,
     tSINGLE = 362,
     tSIZEIS = 363,
     tSIZEOF = 364,
     tSMALL = 365,
     tSOURCE = 366,
     tSTDCALL = 367,
     tSTRING = 368,
     tSTRUCT = 369,
     tSWITCH = 370,
     tSWITCHIS = 371,
     tSWITCHTYPE = 372,
     tTRANSMITAS = 373,
     tTRUE = 374,
     tTYPEDEF = 375,
     tUNION = 376,
     tUNIQUE = 377,
     tUNSIGNED = 378,
     tUUID = 379,
     tV1ENUM = 380,
     tVARARG = 381,
     tVERSION = 382,
     tVOID = 383,
     tWCHAR = 384,
     tWIREMARSHAL = 385,
     CAST = 386,
     PPTR = 387,
     NEG = 388
   };
#endif
#define aIDENTIFIER 258
#define aKNOWNTYPE 259
#define aNUM 260
#define aHEXNUM 261
#define aSTRING 262
#define aUUID 263
#define aEOF 264
#define SHL 265
#define SHR 266
#define tAGGREGATABLE 267
#define tALLOCATE 268
#define tAPPOBJECT 269
#define tASYNC 270
#define tASYNCUUID 271
#define tAUTOHANDLE 272
#define tBINDABLE 273
#define tBOOLEAN 274
#define tBROADCAST 275
#define tBYTE 276
#define tBYTECOUNT 277
#define tCALLAS 278
#define tCALLBACK 279
#define tCASE 280
#define tCDECL 281
#define tCHAR 282
#define tCOCLASS 283
#define tCODE 284
#define tCOMMSTATUS 285
#define tCONST 286
#define tCONTEXTHANDLE 287
#define tCONTEXTHANDLENOSERIALIZE 288
#define tCONTEXTHANDLESERIALIZE 289
#define tCONTROL 290
#define tCPPQUOTE 291
#define tDEFAULT 292
#define tDEFAULTCOLLELEM 293
#define tDEFAULTVALUE 294
#define tDEFAULTVTABLE 295
#define tDISPLAYBIND 296
#define tDISPINTERFACE 297
#define tDLLNAME 298
#define tDOUBLE 299
#define tDUAL 300
#define tENDPOINT 301
#define tENTRY 302
#define tENUM 303
#define tERRORSTATUST 304
#define tEXPLICITHANDLE 305
#define tEXTERN 306
#define tFALSE 307
#define tFLOAT 308
#define tHANDLE 309
#define tHANDLET 310
#define tHELPCONTEXT 311
#define tHELPFILE 312
#define tHELPSTRING 313
#define tHELPSTRINGCONTEXT 314
#define tHELPSTRINGDLL 315
#define tHIDDEN 316
#define tHYPER 317
#define tID 318
#define tIDEMPOTENT 319
#define tIIDIS 320
#define tIMMEDIATEBIND 321
#define tIMPLICITHANDLE 322
#define tIMPORT 323
#define tIMPORTLIB 324
#define tIN 325
#define tINLINE 326
#define tINPUTSYNC 327
#define tINT 328
#define tINT64 329
#define tINTERFACE 330
#define tLCID 331
#define tLENGTHIS 332
#define tLIBRARY 333
#define tLOCAL 334
#define tLONG 335
#define tMETHODS 336
#define tMODULE 337
#define tNONBROWSABLE 338
#define tNONCREATABLE 339
#define tNONEXTENSIBLE 340
#define tOBJECT 341
#define tODL 342
#define tOLEAUTOMATION 343
#define tOPTIONAL 344
#define tOUT 345
#define tPOINTERDEFAULT 346
#define tPROPERTIES 347
#define tPROPGET 348
#define tPROPPUT 349
#define tPROPPUTREF 350
#define tPTR 351
#define tPUBLIC 352
#define tRANGE 353
#define tREADONLY 354
#define tREF 355
#define tREQUESTEDIT 356
#define tRESTRICTED 357
#define tRETVAL 358
#define tSAFEARRAY 359
#define tSHORT 360
#define tSIGNED 361
#define tSINGLE 362
#define tSIZEIS 363
#define tSIZEOF 364
#define tSMALL 365
#define tSOURCE 366
#define tSTDCALL 367
#define tSTRING 368
#define tSTRUCT 369
#define tSWITCH 370
#define tSWITCHIS 371
#define tSWITCHTYPE 372
#define tTRANSMITAS 373
#define tTRUE 374
#define tTYPEDEF 375
#define tUNION 376
#define tUNIQUE 377
#define tUNSIGNED 378
#define tUUID 379
#define tV1ENUM 380
#define tVARARG 381
#define tVERSION 382
#define tVOID 383
#define tWCHAR 384
#define tWIREMARSHAL 385
#define CAST 386
#define PPTR 387
#define NEG 388




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 109 "parser.y"
typedef union YYSTYPE {
	attr_t *attr;
	expr_t *expr;
	type_t *type;
	typeref_t *tref;
	var_t *var;
	func_t *func;
	ifref_t *ifref;
	char *str;
	UUID *uuid;
	unsigned int num;
} YYSTYPE;
/* Line 1248 of yacc.c.  */
#line 315 "parser.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



