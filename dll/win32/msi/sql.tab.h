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
     TK_ALTER = 258,
     TK_AND = 259,
     TK_BY = 260,
     TK_CHAR = 261,
     TK_COMMA = 262,
     TK_CREATE = 263,
     TK_DELETE = 264,
     TK_DROP = 265,
     TK_DISTINCT = 266,
     TK_DOT = 267,
     TK_EQ = 268,
     TK_FREE = 269,
     TK_FROM = 270,
     TK_GE = 271,
     TK_GT = 272,
     TK_HOLD = 273,
     TK_ADD = 274,
     TK_ID = 275,
     TK_ILLEGAL = 276,
     TK_INSERT = 277,
     TK_INT = 278,
     TK_INTEGER = 279,
     TK_INTO = 280,
     TK_IS = 281,
     TK_KEY = 282,
     TK_LE = 283,
     TK_LONG = 284,
     TK_LONGCHAR = 285,
     TK_LP = 286,
     TK_LT = 287,
     TK_LOCALIZABLE = 288,
     TK_MINUS = 289,
     TK_NE = 290,
     TK_NOT = 291,
     TK_NULL = 292,
     TK_OBJECT = 293,
     TK_OR = 294,
     TK_ORDER = 295,
     TK_PRIMARY = 296,
     TK_RP = 297,
     TK_SELECT = 298,
     TK_SET = 299,
     TK_SHORT = 300,
     TK_SPACE = 301,
     TK_STAR = 302,
     TK_STRING = 303,
     TK_TABLE = 304,
     TK_TEMPORARY = 305,
     TK_UPDATE = 306,
     TK_VALUES = 307,
     TK_WHERE = 308,
     TK_WILDCARD = 309,
     COLUMN = 311,
     FUNCTION = 312,
     COMMENT = 313,
     UNCLOSED_STRING = 314,
     SPACE = 315,
     ILLEGAL = 316,
     END_OF_FILE = 317,
     TK_LIKE = 318,
     TK_NEGATION = 319
   };
#endif
/* Tokens.  */
#define TK_ALTER 258
#define TK_AND 259
#define TK_BY 260
#define TK_CHAR 261
#define TK_COMMA 262
#define TK_CREATE 263
#define TK_DELETE 264
#define TK_DROP 265
#define TK_DISTINCT 266
#define TK_DOT 267
#define TK_EQ 268
#define TK_FREE 269
#define TK_FROM 270
#define TK_GE 271
#define TK_GT 272
#define TK_HOLD 273
#define TK_ADD 274
#define TK_ID 275
#define TK_ILLEGAL 276
#define TK_INSERT 277
#define TK_INT 278
#define TK_INTEGER 279
#define TK_INTO 280
#define TK_IS 281
#define TK_KEY 282
#define TK_LE 283
#define TK_LONG 284
#define TK_LONGCHAR 285
#define TK_LP 286
#define TK_LT 287
#define TK_LOCALIZABLE 288
#define TK_MINUS 289
#define TK_NE 290
#define TK_NOT 291
#define TK_NULL 292
#define TK_OBJECT 293
#define TK_OR 294
#define TK_ORDER 295
#define TK_PRIMARY 296
#define TK_RP 297
#define TK_SELECT 298
#define TK_SET 299
#define TK_SHORT 300
#define TK_SPACE 301
#define TK_STAR 302
#define TK_STRING 303
#define TK_TABLE 304
#define TK_TEMPORARY 305
#define TK_UPDATE 306
#define TK_VALUES 307
#define TK_WHERE 308
#define TK_WILDCARD 309
#define COLUMN 311
#define FUNCTION 312
#define COMMENT 313
#define UNCLOSED_STRING 314
#define SPACE 315
#define ILLEGAL 316
#define END_OF_FILE 317
#define TK_LIKE 318
#define TK_NEGATION 319




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 74 "sql.y"
typedef union YYSTYPE {
    struct sql_str str;
    LPWSTR string;
    column_info *column_list;
    MSIVIEW *query;
    struct expr *expr;
    USHORT column_type;
    int integer;
} YYSTYPE;
/* Line 1447 of yacc.c.  */
#line 174 "sql.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif





