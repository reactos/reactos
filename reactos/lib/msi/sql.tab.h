#ifndef BISON_SQL_TAB_H
# define BISON_SQL_TAB_H

#ifndef YYSTYPE
typedef union
{
    struct sql_str str;
    LPWSTR string;
    string_list *column_list;
    value_list *val_list;
    MSIVIEW *query;
    struct expr *expr;
    USHORT column_type;
    create_col_info *column_info;
    column_assignment update_col_info;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	TK_ABORT	257
# define	TK_AFTER	258
# define	TK_AGG_FUNCTION	259
# define	TK_ALL	260
# define	TK_AND	261
# define	TK_AS	262
# define	TK_ASC	263
# define	TK_BEFORE	264
# define	TK_BEGIN	265
# define	TK_BETWEEN	266
# define	TK_BITAND	267
# define	TK_BITNOT	268
# define	TK_BITOR	269
# define	TK_BY	270
# define	TK_CASCADE	271
# define	TK_CASE	272
# define	TK_CHAR	273
# define	TK_CHECK	274
# define	TK_CLUSTER	275
# define	TK_COLLATE	276
# define	TK_COLUMN	277
# define	TK_COMMA	278
# define	TK_COMMENT	279
# define	TK_COMMIT	280
# define	TK_CONCAT	281
# define	TK_CONFLICT	282
# define	TK_CONSTRAINT	283
# define	TK_COPY	284
# define	TK_CREATE	285
# define	TK_DEFAULT	286
# define	TK_DEFERRABLE	287
# define	TK_DEFERRED	288
# define	TK_DELETE	289
# define	TK_DELIMITERS	290
# define	TK_DESC	291
# define	TK_DISTINCT	292
# define	TK_DOT	293
# define	TK_DROP	294
# define	TK_EACH	295
# define	TK_ELSE	296
# define	TK_END	297
# define	TK_END_OF_FILE	298
# define	TK_EQ	299
# define	TK_EXCEPT	300
# define	TK_EXPLAIN	301
# define	TK_FAIL	302
# define	TK_FLOAT	303
# define	TK_FOR	304
# define	TK_FOREIGN	305
# define	TK_FROM	306
# define	TK_FUNCTION	307
# define	TK_GE	308
# define	TK_GLOB	309
# define	TK_GROUP	310
# define	TK_GT	311
# define	TK_HAVING	312
# define	TK_HOLD	313
# define	TK_IGNORE	314
# define	TK_ILLEGAL	315
# define	TK_IMMEDIATE	316
# define	TK_IN	317
# define	TK_INDEX	318
# define	TK_INITIALLY	319
# define	TK_ID	320
# define	TK_INSERT	321
# define	TK_INSTEAD	322
# define	TK_INT	323
# define	TK_INTEGER	324
# define	TK_INTERSECT	325
# define	TK_INTO	326
# define	TK_IS	327
# define	TK_ISNULL	328
# define	TK_JOIN	329
# define	TK_JOIN_KW	330
# define	TK_KEY	331
# define	TK_LE	332
# define	TK_LIKE	333
# define	TK_LIMIT	334
# define	TK_LONG	335
# define	TK_LONGCHAR	336
# define	TK_LP	337
# define	TK_LSHIFT	338
# define	TK_LT	339
# define	TK_LOCALIZABLE	340
# define	TK_MATCH	341
# define	TK_MINUS	342
# define	TK_NE	343
# define	TK_NOT	344
# define	TK_NOTNULL	345
# define	TK_NULL	346
# define	TK_OBJECT	347
# define	TK_OF	348
# define	TK_OFFSET	349
# define	TK_ON	350
# define	TK_OR	351
# define	TK_ORACLE_OUTER_JOIN	352
# define	TK_ORDER	353
# define	TK_PLUS	354
# define	TK_PRAGMA	355
# define	TK_PRIMARY	356
# define	TK_RAISE	357
# define	TK_REFERENCES	358
# define	TK_REM	359
# define	TK_REPLACE	360
# define	TK_RESTRICT	361
# define	TK_ROLLBACK	362
# define	TK_ROW	363
# define	TK_RP	364
# define	TK_RSHIFT	365
# define	TK_SELECT	366
# define	TK_SEMI	367
# define	TK_SET	368
# define	TK_SHORT	369
# define	TK_SLASH	370
# define	TK_SPACE	371
# define	TK_STAR	372
# define	TK_STATEMENT	373
# define	TK_STRING	374
# define	TK_TABLE	375
# define	TK_TEMP	376
# define	TK_THEN	377
# define	TK_TRANSACTION	378
# define	TK_TRIGGER	379
# define	TK_UMINUS	380
# define	TK_UNCLOSED_STRING	381
# define	TK_UNION	382
# define	TK_UNIQUE	383
# define	TK_UPDATE	384
# define	TK_UPLUS	385
# define	TK_USING	386
# define	TK_VACUUM	387
# define	TK_VALUES	388
# define	TK_VIEW	389
# define	TK_WHEN	390
# define	TK_WHERE	391
# define	TK_WILDCARD	392
# define	END_OF_FILE	393
# define	ILLEGAL	394
# define	SPACE	395
# define	UNCLOSED_STRING	396
# define	COMMENT	397
# define	FUNCTION	398
# define	COLUMN	399


#endif /* not BISON_SQL_TAB_H */
