typedef union {
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
#define	aIDENTIFIER	257
#define	aKNOWNTYPE	258
#define	aNUM	259
#define	aHEXNUM	260
#define	aSTRING	261
#define	aUUID	262
#define	aEOF	263
#define	SHL	264
#define	SHR	265
#define	tAGGREGATABLE	266
#define	tALLOCATE	267
#define	tAPPOBJECT	268
#define	tARRAYS	269
#define	tASYNC	270
#define	tASYNCUUID	271
#define	tAUTOHANDLE	272
#define	tBINDABLE	273
#define	tBOOLEAN	274
#define	tBROADCAST	275
#define	tBYTE	276
#define	tBYTECOUNT	277
#define	tCALLAS	278
#define	tCALLBACK	279
#define	tCASE	280
#define	tCDECL	281
#define	tCHAR	282
#define	tCOCLASS	283
#define	tCODE	284
#define	tCOMMSTATUS	285
#define	tCONST	286
#define	tCONTEXTHANDLE	287
#define	tCONTEXTHANDLENOSERIALIZE	288
#define	tCONTEXTHANDLESERIALIZE	289
#define	tCONTROL	290
#define	tCPPQUOTE	291
#define	tDEFAULT	292
#define	tDEFAULTVALUE	293
#define	tDISPINTERFACE	294
#define	tDLLNAME	295
#define	tDOUBLE	296
#define	tDUAL	297
#define	tENDPOINT	298
#define	tENTRY	299
#define	tENUM	300
#define	tERRORSTATUST	301
#define	tEXPLICITHANDLE	302
#define	tEXTERN	303
#define	tFLOAT	304
#define	tHANDLE	305
#define	tHANDLET	306
#define	tHELPCONTEXT	307
#define	tHELPFILE	308
#define	tHELPSTRING	309
#define	tHELPSTRINGCONTEXT	310
#define	tHELPSTRINGDLL	311
#define	tHIDDEN	312
#define	tHYPER	313
#define	tID	314
#define	tIDEMPOTENT	315
#define	tIIDIS	316
#define	tIMPLICITHANDLE	317
#define	tIMPORT	318
#define	tIMPORTLIB	319
#define	tIN	320
#define	tINCLUDE	321
#define	tINLINE	322
#define	tINPUTSYNC	323
#define	tINT	324
#define	tINT64	325
#define	tINTERFACE	326
#define	tLENGTHIS	327
#define	tLIBRARY	328
#define	tLOCAL	329
#define	tLONG	330
#define	tMETHODS	331
#define	tMODULE	332
#define	tNONCREATABLE	333
#define	tOBJECT	334
#define	tODL	335
#define	tOLEAUTOMATION	336
#define	tOPTIONAL	337
#define	tOUT	338
#define	tPOINTERDEFAULT	339
#define	tPROPERTIES	340
#define	tPROPGET	341
#define	tPROPPUT	342
#define	tPROPPUTREF	343
#define	tPTR	344
#define	tPUBLIC	345
#define	tREADONLY	346
#define	tREF	347
#define	tRESTRICTED	348
#define	tRETVAL	349
#define	tSHORT	350
#define	tSIGNED	351
#define	tSIZEIS	352
#define	tSIZEOF	353
#define	tSMALL	354
#define	tSOURCE	355
#define	tSTDCALL	356
#define	tSTRING	357
#define	tSTRUCT	358
#define	tSWITCH	359
#define	tSWITCHIS	360
#define	tSWITCHTYPE	361
#define	tTRANSMITAS	362
#define	tTYPEDEF	363
#define	tUNION	364
#define	tUNIQUE	365
#define	tUNSIGNED	366
#define	tUUID	367
#define	tV1ENUM	368
#define	tVARARG	369
#define	tVERSION	370
#define	tVOID	371
#define	tWCHAR	372
#define	tWIREMARSHAL	373
#define	tPOINTERTYPE	374
#define	COND	375
#define	CAST	376
#define	PPTR	377
#define	NEG	378


extern YYSTYPE yylval;
