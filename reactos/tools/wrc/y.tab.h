#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

#ifndef YYSTYPE
typedef union{
	string_t	*str;
	int		num;
	int		*iptr;
	char		*cptr;
	resource_t	*res;
	accelerator_t	*acc;
	bitmap_t	*bmp;
	dialog_t	*dlg;
	dialogex_t	*dlgex;
	font_t		*fnt;
	fontdir_t	*fnd;
	menu_t		*men;
	menuex_t	*menex;
	rcdata_t	*rdt;
	stringtable_t	*stt;
	stt_entry_t	*stte;
	user_t		*usr;
	messagetable_t	*msg;
	versioninfo_t	*veri;
	control_t	*ctl;
	name_id_t	*nid;
	font_id_t	*fntid;
	language_t	*lan;
	version_t	*ver;
	characts_t	*chars;
	event_t		*event;
	menu_item_t	*menitm;
	menuex_item_t	*menexitm;
	itemex_opt_t	*exopt;
	raw_data_t	*raw;
	lvc_t		*lvc;
	ver_value_t	*val;
	ver_block_t	*blk;
	ver_words_t	*verw;
	toolbar_t	*tlbar;
	toolbar_item_t	*tlbarItems;
	dlginit_t       *dginit;
	style_pair_t	*styles;
	style_t		*style;
	ani_any_t	*ani;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	tNL	257
# define	tNUMBER	258
# define	tLNUMBER	259
# define	tSTRING	260
# define	tIDENT	261
# define	tFILENAME	262
# define	tRAWDATA	263
# define	tACCELERATORS	264
# define	tBITMAP	265
# define	tCURSOR	266
# define	tDIALOG	267
# define	tDIALOGEX	268
# define	tMENU	269
# define	tMENUEX	270
# define	tMESSAGETABLE	271
# define	tRCDATA	272
# define	tVERSIONINFO	273
# define	tSTRINGTABLE	274
# define	tFONT	275
# define	tFONTDIR	276
# define	tICON	277
# define	tAUTO3STATE	278
# define	tAUTOCHECKBOX	279
# define	tAUTORADIOBUTTON	280
# define	tCHECKBOX	281
# define	tDEFPUSHBUTTON	282
# define	tPUSHBUTTON	283
# define	tRADIOBUTTON	284
# define	tSTATE3	285
# define	tGROUPBOX	286
# define	tCOMBOBOX	287
# define	tLISTBOX	288
# define	tSCROLLBAR	289
# define	tCONTROL	290
# define	tEDITTEXT	291
# define	tRTEXT	292
# define	tCTEXT	293
# define	tLTEXT	294
# define	tBLOCK	295
# define	tVALUE	296
# define	tSHIFT	297
# define	tALT	298
# define	tASCII	299
# define	tVIRTKEY	300
# define	tGRAYED	301
# define	tCHECKED	302
# define	tINACTIVE	303
# define	tNOINVERT	304
# define	tPURE	305
# define	tIMPURE	306
# define	tDISCARDABLE	307
# define	tLOADONCALL	308
# define	tPRELOAD	309
# define	tFIXED	310
# define	tMOVEABLE	311
# define	tCLASS	312
# define	tCAPTION	313
# define	tCHARACTERISTICS	314
# define	tEXSTYLE	315
# define	tSTYLE	316
# define	tVERSION	317
# define	tLANGUAGE	318
# define	tFILEVERSION	319
# define	tPRODUCTVERSION	320
# define	tFILEFLAGSMASK	321
# define	tFILEOS	322
# define	tFILETYPE	323
# define	tFILEFLAGS	324
# define	tFILESUBTYPE	325
# define	tMENUBARBREAK	326
# define	tMENUBREAK	327
# define	tMENUITEM	328
# define	tPOPUP	329
# define	tSEPARATOR	330
# define	tHELP	331
# define	tTOOLBAR	332
# define	tBUTTON	333
# define	tBEGIN	334
# define	tEND	335
# define	tDLGINIT	336
# define	tNOT	337
# define	pUPM	338


extern YYSTYPE yylval;

#endif /* not BISON_Y_TAB_H */
