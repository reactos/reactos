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
     tARRAYS = 270,
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
     tDEFAULTVALUE = 294,
     tDISPINTERFACE = 295,
     tDLLNAME = 296,
     tDOUBLE = 297,
     tDUAL = 298,
     tENDPOINT = 299,
     tENTRY = 300,
     tENUM = 301,
     tERRORSTATUST = 302,
     tEXPLICITHANDLE = 303,
     tEXTERN = 304,
     tFLOAT = 305,
     tHANDLE = 306,
     tHANDLET = 307,
     tHELPCONTEXT = 308,
     tHELPFILE = 309,
     tHELPSTRING = 310,
     tHELPSTRINGCONTEXT = 311,
     tHELPSTRINGDLL = 312,
     tHIDDEN = 313,
     tHYPER = 314,
     tID = 315,
     tIDEMPOTENT = 316,
     tIIDIS = 317,
     tIMPLICITHANDLE = 318,
     tIMPORT = 319,
     tIMPORTLIB = 320,
     tIN = 321,
     tINCLUDE = 322,
     tINLINE = 323,
     tINPUTSYNC = 324,
     tINT = 325,
     tINT64 = 326,
     tINTERFACE = 327,
     tLENGTHIS = 328,
     tLIBRARY = 329,
     tLOCAL = 330,
     tLONG = 331,
     tMETHODS = 332,
     tMODULE = 333,
     tNONCREATABLE = 334,
     tOBJECT = 335,
     tODL = 336,
     tOLEAUTOMATION = 337,
     tOPTIONAL = 338,
     tOUT = 339,
     tPOINTERDEFAULT = 340,
     tPROPERTIES = 341,
     tPROPGET = 342,
     tPROPPUT = 343,
     tPROPPUTREF = 344,
     tPTR = 345,
     tPUBLIC = 346,
     tREADONLY = 347,
     tREF = 348,
     tRESTRICTED = 349,
     tRETVAL = 350,
     tSHORT = 351,
     tSIGNED = 352,
     tSINGLE = 353,
     tSIZEIS = 354,
     tSIZEOF = 355,
     tSMALL = 356,
     tSOURCE = 357,
     tSTDCALL = 358,
     tSTRING = 359,
     tSTRUCT = 360,
     tSWITCH = 361,
     tSWITCHIS = 362,
     tSWITCHTYPE = 363,
     tTRANSMITAS = 364,
     tTYPEDEF = 365,
     tUNION = 366,
     tUNIQUE = 367,
     tUNSIGNED = 368,
     tUUID = 369,
     tV1ENUM = 370,
     tVARARG = 371,
     tVERSION = 372,
     tVOID = 373,
     tWCHAR = 374,
     tWIREMARSHAL = 375,
     tPOINTERTYPE = 376,
     COND = 377,
     CAST = 378,
     PPTR = 379,
     NEG = 380
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
#define tARRAYS 270
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
#define tDEFAULTVALUE 294
#define tDISPINTERFACE 295
#define tDLLNAME 296
#define tDOUBLE 297
#define tDUAL 298
#define tENDPOINT 299
#define tENTRY 300
#define tENUM 301
#define tERRORSTATUST 302
#define tEXPLICITHANDLE 303
#define tEXTERN 304
#define tFLOAT 305
#define tHANDLE 306
#define tHANDLET 307
#define tHELPCONTEXT 308
#define tHELPFILE 309
#define tHELPSTRING 310
#define tHELPSTRINGCONTEXT 311
#define tHELPSTRINGDLL 312
#define tHIDDEN 313
#define tHYPER 314
#define tID 315
#define tIDEMPOTENT 316
#define tIIDIS 317
#define tIMPLICITHANDLE 318
#define tIMPORT 319
#define tIMPORTLIB 320
#define tIN 321
#define tINCLUDE 322
#define tINLINE 323
#define tINPUTSYNC 324
#define tINT 325
#define tINT64 326
#define tINTERFACE 327
#define tLENGTHIS 328
#define tLIBRARY 329
#define tLOCAL 330
#define tLONG 331
#define tMETHODS 332
#define tMODULE 333
#define tNONCREATABLE 334
#define tOBJECT 335
#define tODL 336
#define tOLEAUTOMATION 337
#define tOPTIONAL 338
#define tOUT 339
#define tPOINTERDEFAULT 340
#define tPROPERTIES 341
#define tPROPGET 342
#define tPROPPUT 343
#define tPROPPUTREF 344
#define tPTR 345
#define tPUBLIC 346
#define tREADONLY 347
#define tREF 348
#define tRESTRICTED 349
#define tRETVAL 350
#define tSHORT 351
#define tSIGNED 352
#define tSINGLE 353
#define tSIZEIS 354
#define tSIZEOF 355
#define tSMALL 356
#define tSOURCE 357
#define tSTDCALL 358
#define tSTRING 359
#define tSTRUCT 360
#define tSWITCH 361
#define tSWITCHIS 362
#define tSWITCHTYPE 363
#define tTRANSMITAS 364
#define tTYPEDEF 365
#define tUNION 366
#define tUNIQUE 367
#define tUNSIGNED 368
#define tUUID 369
#define tV1ENUM 370
#define tVARARG 371
#define tVERSION 372
#define tVOID 373
#define tWCHAR 374
#define tWIREMARSHAL 375
#define tPOINTERTYPE 376
#define COND 377
#define CAST 378
#define PPTR 379
#define NEG 380




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 106 "parser.y"
typedef union YYSTYPE {
	attr_t *attr;
	expr_t *expr;
	type_t *type;
	typeref_t *tref;
	var_t *var;
	func_t *func;
	ifref_t *ifref;
	class_t *clas;
	char *str;
	UUID *uuid;
	unsigned int num;
} YYSTYPE;
/* Line 1248 of yacc.c.  */
#line 300 "y.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



