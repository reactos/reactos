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
     TK_DISTINCT = 265,
     TK_DOT = 266,
     TK_EQ = 267,
     TK_FREE = 268,
     TK_FROM = 269,
     TK_GE = 270,
     TK_GT = 271,
     TK_HOLD = 272,
     TK_ID = 273,
     TK_ILLEGAL = 274,
     TK_INSERT = 275,
     TK_INT = 276,
     TK_INTEGER = 277,
     TK_INTO = 278,
     TK_IS = 279,
     TK_KEY = 280,
     TK_LE = 281,
     TK_LONG = 282,
     TK_LONGCHAR = 283,
     TK_LP = 284,
     TK_LT = 285,
     TK_LOCALIZABLE = 286,
     TK_MINUS = 287,
     TK_NE = 288,
     TK_NOT = 289,
     TK_NULL = 290,
     TK_OBJECT = 291,
     TK_OR = 292,
     TK_ORDER = 293,
     TK_PRIMARY = 294,
     TK_RP = 295,
     TK_SELECT = 296,
     TK_SET = 297,
     TK_SHORT = 298,
     TK_SPACE = 299,
     TK_STAR = 300,
     TK_STRING = 301,
     TK_TABLE = 302,
     TK_TEMPORARY = 303,
     TK_UPDATE = 304,
     TK_VALUES = 305,
     TK_WHERE = 306,
     TK_WILDCARD = 307,
     COLUMN = 309,
     FUNCTION = 310,
     COMMENT = 311,
     UNCLOSED_STRING = 312,
     SPACE = 313,
     ILLEGAL = 314,
     END_OF_FILE = 315,
     TK_LIKE = 316,
     TK_NEGATION = 317
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
#define TK_DISTINCT 265
#define TK_DOT 266
#define TK_EQ 267
#define TK_FREE 268
#define TK_FROM 269
#define TK_GE 270
#define TK_GT 271
#define TK_HOLD 272
#define TK_ID 273
#define TK_ILLEGAL 274
#define TK_INSERT 275
#define TK_INT 276
#define TK_INTEGER 277
#define TK_INTO 278
#define TK_IS 279
#define TK_KEY 280
#define TK_LE 281
#define TK_LONG 282
#define TK_LONGCHAR 283
#define TK_LP 284
#define TK_LT 285
#define TK_LOCALIZABLE 286
#define TK_MINUS 287
#define TK_NE 288
#define TK_NOT 289
#define TK_NULL 290
#define TK_OBJECT 291
#define TK_OR 292
#define TK_ORDER 293
#define TK_PRIMARY 294
#define TK_RP 295
#define TK_SELECT 296
#define TK_SET 297
#define TK_SHORT 298
#define TK_SPACE 299
#define TK_STAR 300
#define TK_STRING 301
#define TK_TABLE 302
#define TK_TEMPORARY 303
#define TK_UPDATE 304
#define TK_VALUES 305
#define TK_WHERE 306
#define TK_WILDCARD 307
#define COLUMN 309
#define FUNCTION 310
#define COMMENT 311
#define UNCLOSED_STRING 312
#define SPACE 313
#define ILLEGAL 314
#define END_OF_FILE 315
#define TK_LIKE 316
#define TK_NEGATION 317




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 73 "sql.y"
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
#line 170 "sql.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif





