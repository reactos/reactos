#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

#ifndef YYSTYPE
typedef union{
	int		sint;
	unsigned int	uint;
	long		slong;
	unsigned long	ulong;
	wrc_sll_t	sll;
	wrc_ull_t	ull;
	int		*iptr;
	char		*cptr;
	cval_t		cval;
	marg_t		*marg;
	mtext_t		*mtext;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	tRCINCLUDE	257
# define	tIF	258
# define	tIFDEF	259
# define	tIFNDEF	260
# define	tELSE	261
# define	tELIF	262
# define	tENDIF	263
# define	tDEFINED	264
# define	tNL	265
# define	tINCLUDE	266
# define	tLINE	267
# define	tGCCLINE	268
# define	tERROR	269
# define	tWARNING	270
# define	tPRAGMA	271
# define	tPPIDENT	272
# define	tUNDEF	273
# define	tMACROEND	274
# define	tCONCAT	275
# define	tELIPSIS	276
# define	tSTRINGIZE	277
# define	tIDENT	278
# define	tLITERAL	279
# define	tMACRO	280
# define	tDEFINE	281
# define	tDQSTRING	282
# define	tSQSTRING	283
# define	tIQSTRING	284
# define	tUINT	285
# define	tSINT	286
# define	tULONG	287
# define	tSLONG	288
# define	tULONGLONG	289
# define	tSLONGLONG	290
# define	tRCINCLUDEPATH	291
# define	tLOGOR	292
# define	tLOGAND	293
# define	tEQ	294
# define	tNE	295
# define	tLTE	296
# define	tGTE	297
# define	tLSHIFT	298
# define	tRSHIFT	299


extern YYSTYPE pplval;

#endif /* not BISON_Y_TAB_H */
