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
#define	tEXTERN	302
#define	tFLOAT	303
#define	tHANDLE	304
#define	tHANDLET	305
#define	tHELPCONTEXT	306
#define	tHELPFILE	307
#define	tHELPSTRING	308
#define	tHELPSTRINGCONTEXT	309
#define	tHELPSTRINGDLL	310
#define	tHIDDEN	311
#define	tHYPER	312
#define	tID	313
#define	tIDEMPOTENT	314
#define	tIIDIS	315
#define	tIMPORT	316
#define	tIMPORTLIB	317
#define	tIN	318
#define	tINCLUDE	319
#define	tINLINE	320
#define	tINPUTSYNC	321
#define	tINT	322
#define	tINT64	323
#define	tINTERFACE	324
#define	tLENGTHIS	325
#define	tLIBRARY	326
#define	tLOCAL	327
#define	tLONG	328
#define	tMETHODS	329
#define	tMODULE	330
#define	tNONCREATABLE	331
#define	tOBJECT	332
#define	tODL	333
#define	tOLEAUTOMATION	334
#define	tOPTIONAL	335
#define	tOUT	336
#define	tPOINTERDEFAULT	337
#define	tPROPERTIES	338
#define	tPROPGET	339
#define	tPROPPUT	340
#define	tPROPPUTREF	341
#define	tPUBLIC	342
#define	tREADONLY	343
#define	tREF	344
#define	tRESTRICTED	345
#define	tRETVAL	346
#define	tSHORT	347
#define	tSIGNED	348
#define	tSIZEIS	349
#define	tSIZEOF	350
#define	tSOURCE	351
#define	tSTDCALL	352
#define	tSTRING	353
#define	tSTRUCT	354
#define	tSWITCH	355
#define	tSWITCHIS	356
#define	tSWITCHTYPE	357
#define	tTRANSMITAS	358
#define	tTYPEDEF	359
#define	tUNION	360
#define	tUNIQUE	361
#define	tUNSIGNED	362
#define	tUUID	363
#define	tV1ENUM	364
#define	tVARARG	365
#define	tVERSION	366
#define	tVOID	367
#define	tWCHAR	368
#define	tWIREMARSHAL	369
#define	tPOINTERTYPE	370
#define	COND	371
#define	CAST	372
#define	PPTR	373
#define	NEG	374


extern YYSTYPE yylval;
