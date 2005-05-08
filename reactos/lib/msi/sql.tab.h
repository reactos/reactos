/* A Bison parser, made by GNU Bison 1.875b.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

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
     TK_ABORT = 258,
     TK_AFTER = 259,
     TK_AGG_FUNCTION = 260,
     TK_ALL = 261,
     TK_AND = 262,
     TK_AS = 263,
     TK_ASC = 264,
     TK_BEFORE = 265,
     TK_BEGIN = 266,
     TK_BETWEEN = 267,
     TK_BITAND = 268,
     TK_BITNOT = 269,
     TK_BITOR = 270,
     TK_BY = 271,
     TK_CASCADE = 272,
     TK_CASE = 273,
     TK_CHAR = 274,
     TK_CHECK = 275,
     TK_CLUSTER = 276,
     TK_COLLATE = 277,
     TK_COLUMN = 278,
     TK_COMMA = 279,
     TK_COMMENT = 280,
     TK_COMMIT = 281,
     TK_CONCAT = 282,
     TK_CONFLICT = 283,
     TK_CONSTRAINT = 284,
     TK_COPY = 285,
     TK_CREATE = 286,
     TK_DEFAULT = 287,
     TK_DEFERRABLE = 288,
     TK_DEFERRED = 289,
     TK_DELETE = 290,
     TK_DELIMITERS = 291,
     TK_DESC = 292,
     TK_DISTINCT = 293,
     TK_DOT = 294,
     TK_DROP = 295,
     TK_EACH = 296,
     TK_ELSE = 297,
     TK_END = 298,
     TK_END_OF_FILE = 299,
     TK_EQ = 300,
     TK_EXCEPT = 301,
     TK_EXPLAIN = 302,
     TK_FAIL = 303,
     TK_FLOAT = 304,
     TK_FOR = 305,
     TK_FOREIGN = 306,
     TK_FROM = 307,
     TK_FUNCTION = 308,
     TK_GE = 309,
     TK_GLOB = 310,
     TK_GROUP = 311,
     TK_GT = 312,
     TK_HAVING = 313,
     TK_HOLD = 314,
     TK_IGNORE = 315,
     TK_ILLEGAL = 316,
     TK_IMMEDIATE = 317,
     TK_IN = 318,
     TK_INDEX = 319,
     TK_INITIALLY = 320,
     TK_ID = 321,
     TK_INSERT = 322,
     TK_INSTEAD = 323,
     TK_INT = 324,
     TK_INTEGER = 325,
     TK_INTERSECT = 326,
     TK_INTO = 327,
     TK_IS = 328,
     TK_ISNULL = 329,
     TK_JOIN = 330,
     TK_JOIN_KW = 331,
     TK_KEY = 332,
     TK_LE = 333,
     TK_LIKE = 334,
     TK_LIMIT = 335,
     TK_LONG = 336,
     TK_LONGCHAR = 337,
     TK_LP = 338,
     TK_LSHIFT = 339,
     TK_LT = 340,
     TK_LOCALIZABLE = 341,
     TK_MATCH = 342,
     TK_MINUS = 343,
     TK_NE = 344,
     TK_NOT = 345,
     TK_NOTNULL = 346,
     TK_NULL = 347,
     TK_OBJECT = 348,
     TK_OF = 349,
     TK_OFFSET = 350,
     TK_ON = 351,
     TK_OR = 352,
     TK_ORACLE_OUTER_JOIN = 353,
     TK_ORDER = 354,
     TK_PLUS = 355,
     TK_PRAGMA = 356,
     TK_PRIMARY = 357,
     TK_RAISE = 358,
     TK_REFERENCES = 359,
     TK_REM = 360,
     TK_REPLACE = 361,
     TK_RESTRICT = 362,
     TK_ROLLBACK = 363,
     TK_ROW = 364,
     TK_RP = 365,
     TK_RSHIFT = 366,
     TK_SELECT = 367,
     TK_SEMI = 368,
     TK_SET = 369,
     TK_SHORT = 370,
     TK_SLASH = 371,
     TK_SPACE = 372,
     TK_STAR = 373,
     TK_STATEMENT = 374,
     TK_STRING = 375,
     TK_TABLE = 376,
     TK_TEMP = 377,
     TK_THEN = 378,
     TK_TRANSACTION = 379,
     TK_TRIGGER = 380,
     TK_UMINUS = 381,
     TK_UNCLOSED_STRING = 382,
     TK_UNION = 383,
     TK_UNIQUE = 384,
     TK_UPDATE = 385,
     TK_UPLUS = 386,
     TK_USING = 387,
     TK_VACUUM = 388,
     TK_VALUES = 389,
     TK_VIEW = 390,
     TK_WHEN = 391,
     TK_WHERE = 392,
     TK_WILDCARD = 393,
     COLUMN = 395,
     FUNCTION = 396,
     COMMENT = 397,
     UNCLOSED_STRING = 398,
     SPACE = 399,
     ILLEGAL = 400,
     END_OF_FILE = 401
   };
#endif
#define TK_ABORT 258
#define TK_AFTER 259
#define TK_AGG_FUNCTION 260
#define TK_ALL 261
#define TK_AND 262
#define TK_AS 263
#define TK_ASC 264
#define TK_BEFORE 265
#define TK_BEGIN 266
#define TK_BETWEEN 267
#define TK_BITAND 268
#define TK_BITNOT 269
#define TK_BITOR 270
#define TK_BY 271
#define TK_CASCADE 272
#define TK_CASE 273
#define TK_CHAR 274
#define TK_CHECK 275
#define TK_CLUSTER 276
#define TK_COLLATE 277
#define TK_COLUMN 278
#define TK_COMMA 279
#define TK_COMMENT 280
#define TK_COMMIT 281
#define TK_CONCAT 282
#define TK_CONFLICT 283
#define TK_CONSTRAINT 284
#define TK_COPY 285
#define TK_CREATE 286
#define TK_DEFAULT 287
#define TK_DEFERRABLE 288
#define TK_DEFERRED 289
#define TK_DELETE 290
#define TK_DELIMITERS 291
#define TK_DESC 292
#define TK_DISTINCT 293
#define TK_DOT 294
#define TK_DROP 295
#define TK_EACH 296
#define TK_ELSE 297
#define TK_END 298
#define TK_END_OF_FILE 299
#define TK_EQ 300
#define TK_EXCEPT 301
#define TK_EXPLAIN 302
#define TK_FAIL 303
#define TK_FLOAT 304
#define TK_FOR 305
#define TK_FOREIGN 306
#define TK_FROM 307
#define TK_FUNCTION 308
#define TK_GE 309
#define TK_GLOB 310
#define TK_GROUP 311
#define TK_GT 312
#define TK_HAVING 313
#define TK_HOLD 314
#define TK_IGNORE 315
#define TK_ILLEGAL 316
#define TK_IMMEDIATE 317
#define TK_IN 318
#define TK_INDEX 319
#define TK_INITIALLY 320
#define TK_ID 321
#define TK_INSERT 322
#define TK_INSTEAD 323
#define TK_INT 324
#define TK_INTEGER 325
#define TK_INTERSECT 326
#define TK_INTO 327
#define TK_IS 328
#define TK_ISNULL 329
#define TK_JOIN 330
#define TK_JOIN_KW 331
#define TK_KEY 332
#define TK_LE 333
#define TK_LIKE 334
#define TK_LIMIT 335
#define TK_LONG 336
#define TK_LONGCHAR 337
#define TK_LP 338
#define TK_LSHIFT 339
#define TK_LT 340
#define TK_LOCALIZABLE 341
#define TK_MATCH 342
#define TK_MINUS 343
#define TK_NE 344
#define TK_NOT 345
#define TK_NOTNULL 346
#define TK_NULL 347
#define TK_OBJECT 348
#define TK_OF 349
#define TK_OFFSET 350
#define TK_ON 351
#define TK_OR 352
#define TK_ORACLE_OUTER_JOIN 353
#define TK_ORDER 354
#define TK_PLUS 355
#define TK_PRAGMA 356
#define TK_PRIMARY 357
#define TK_RAISE 358
#define TK_REFERENCES 359
#define TK_REM 360
#define TK_REPLACE 361
#define TK_RESTRICT 362
#define TK_ROLLBACK 363
#define TK_ROW 364
#define TK_RP 365
#define TK_RSHIFT 366
#define TK_SELECT 367
#define TK_SEMI 368
#define TK_SET 369
#define TK_SHORT 370
#define TK_SLASH 371
#define TK_SPACE 372
#define TK_STAR 373
#define TK_STATEMENT 374
#define TK_STRING 375
#define TK_TABLE 376
#define TK_TEMP 377
#define TK_THEN 378
#define TK_TRANSACTION 379
#define TK_TRIGGER 380
#define TK_UMINUS 381
#define TK_UNCLOSED_STRING 382
#define TK_UNION 383
#define TK_UNIQUE 384
#define TK_UPDATE 385
#define TK_UPLUS 386
#define TK_USING 387
#define TK_VACUUM 388
#define TK_VALUES 389
#define TK_VIEW 390
#define TK_WHEN 391
#define TK_WHERE 392
#define TK_WILDCARD 393
#define COLUMN 395
#define FUNCTION 396
#define COMMENT 397
#define UNCLOSED_STRING 398
#define SPACE 399
#define ILLEGAL 400
#define END_OF_FILE 401




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 74 "./sql.y"
typedef union YYSTYPE {
    struct sql_str str;
    LPWSTR string;
    string_list *column_list;
    value_list *val_list;
    MSIVIEW *query;
    struct expr *expr;
    USHORT column_type;
    create_col_info *column_info;
    column_assignment update_col_info;
} YYSTYPE;
/* Line 1252 of yacc.c.  */
#line 339 "sql.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif





