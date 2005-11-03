typedef union {
	WCHAR		*str;
	unsigned	num;
	token_t		*tok;
	lanmsg_t	*lmp;
	msg_t		*msg;
	lan_cp_t	lcp;
} YYSTYPE;
#define	tSEVNAMES	258
#define	tFACNAMES	259
#define	tLANNAMES	260
#define	tBASE	261
#define	tCODEPAGE	262
#define	tTYPEDEF	263
#define	tNL	264
#define	tSYMNAME	265
#define	tMSGEND	266
#define	tSEVERITY	267
#define	tFACILITY	268
#define	tLANGUAGE	269
#define	tMSGID	270
#define	tIDENT	271
#define	tLINE	272
#define	tFILE	273
#define	tCOMMENT	274
#define	tNUMBER	275
#define	tTOKEN	276


extern YYSTYPE yylval;
