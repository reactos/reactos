/*
 * Internal definitions.
 * Copyright (c) 1998 New Generation Software (NGS) Oy
 *
 * Author: Markku Rossi <mtr@ngs.fi>
 */

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA
 */

/*
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsc/defs.js,v $
 * $Id: defs.js,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

/*
 * Constants.
 */

/* Tokens. */

JSC$tEOF 	= 128;
JSC$tINTEGER	= 129;
JSC$tFLOAT 	= 130;
JSC$tSTRING	= 131;
JSC$tIDENTIFIER	= 132;

JSC$tBREAK 	= 133;
JSC$tCONTINUE 	= 134;
JSC$tDELETE 	= 135;
JSC$tELSE 	= 136;
JSC$tFOR 	= 137;
JSC$tFUNCTION	= 138;
JSC$tIF 	= 139;
JSC$tIN 	= 140;
JSC$tNEW 	= 141;
JSC$tRETURN 	= 142;
JSC$tTHIS 	= 143;
JSC$tTYPEOF 	= 144;
JSC$tVAR 	= 145;
JSC$tVOID 	= 146;
JSC$tWHILE	= 147;
JSC$tWITH 	= 148;

JSC$tCASE	= 149;
JSC$tCATCH	= 150;
JSC$tCLASS	= 151;
JSC$tCONST	= 152;
JSC$tDEBUGGER	= 153;
JSC$tDEFAULT	= 154;
JSC$tDO		= 155;
JSC$tENUM	= 156;
JSC$tEXPORT	= 157;
JSC$tEXTENDS	= 158;
JSC$tFINALLY	= 159;
JSC$tIMPORT	= 160;
JSC$tSUPER	= 161;
JSC$tSWITCH	= 162;
JSC$tTHROW	= 163;
JSC$tTRY	= 164;

JSC$tNULL 	= 165;
JSC$tTRUE 	= 166;
JSC$tFALSE 	= 167;

JSC$tEQUAL 	= 168;
JSC$tNEQUAL 	= 169;
JSC$tLE 	= 170;
JSC$tGE 	= 171;
JSC$tAND 	= 172;
JSC$tOR 	= 173;
JSC$tPLUSPLUS 	= 174;
JSC$tMINUSMINUS	= 175;
JSC$tMULA	= 176;
JSC$tDIVA	= 177;
JSC$tMODA	= 178;
JSC$tADDA	= 179;
JSC$tSUBA	= 180;
JSC$tANDA	= 181;
JSC$tXORA	= 182;
JSC$tORA	= 183;
JSC$tLSIA	= 184;
JSC$tLSHIFT	= 185;
JSC$tRSHIFT	= 186;
JSC$tRRSHIFT	= 187;
JSC$tRSIA	= 188;
JSC$tRRSA	= 189;
JSC$tSEQUAL	= 190;
JSC$tSNEQUAL	= 191;


/* Expressions. */

JSC$EXPR_COMMA			= 0;
JSC$EXPR_ASSIGNMENT		= 1;
JSC$EXPR_QUEST_COLON		= 2;
JSC$EXPR_LOGICAL		= 3;
JSC$EXPR_BITWISE		= 4;
JSC$EXPR_EQUALITY		= 5;
JSC$EXPR_RELATIONAL		= 6;
JSC$EXPR_SHIFT			= 7;
JSC$EXPR_MULTIPLICATIVE		= 8;
JSC$EXPR_ADDITIVE		= 9;
JSC$EXPR_THIS			= 10;
JSC$EXPR_NULL			= 11;
JSC$EXPR_TRUE			= 12;
JSC$EXPR_FALSE			= 13;
JSC$EXPR_IDENTIFIER		= 14;
JSC$EXPR_FLOAT			= 15;
JSC$EXPR_INTEGER		= 16;
JSC$EXPR_STRING			= 17;
JSC$EXPR_CALL			= 18;
JSC$EXPR_OBJECT_PROPERTY	= 19;
JSC$EXPR_OBJECT_ARRAY		= 20;
JSC$EXPR_NEW			= 21;
JSC$EXPR_DELETE			= 22;
JSC$EXPR_VOID			= 23;
JSC$EXPR_TYPEOF			= 24;
JSC$EXPR_PREFIX			= 25;
JSC$EXPR_POSTFIX		= 26;
JSC$EXPR_UNARY			= 27;
JSC$EXPR_REGEXP			= 28;
JSC$EXPR_ARRAY_INITIALIZER	= 29;
JSC$EXPR_OBJECT_INITIALIZER	= 30;

/* Statements */

JSC$STMT_BLOCK			= 0;
JSC$STMT_FUNCTION_DECLARATION	= 1;
JSC$STMT_VARIABLE		= 2;
JSC$STMT_EMPTY			= 3;
JSC$STMT_EXPR			= 4;
JSC$STMT_IF			= 5;
JSC$STMT_WHILE			= 6;
JSC$STMT_FOR			= 7;
JSC$STMT_FOR_IN			= 8;
JSC$STMT_CONTINUE		= 9;
JSC$STMT_BREAK			= 10;
JSC$STMT_RETURN			= 11;
JSC$STMT_WITH			= 12;
JSC$STMT_TRY			= 13;
JSC$STMT_THROW			= 14;
JSC$STMT_DO_WHILE		= 15;
JSC$STMT_SWITCH			= 16;
JSC$STMT_LABELED_STMT		= 17;

/* JavaScript types. */

JSC$JS_UNDEFINED	= 0;
JSC$JS_NULL		= 1;
JSC$JS_BOOLEAN		= 2;
JSC$JS_INTEGER		= 3;
JSC$JS_STRING		= 4;
JSC$JS_FLOAT		= 5;
JSC$JS_ARRAY		= 6;
JSC$JS_OBJECT		= 7;
JSC$JS_BUILTIN		= 11;


/*
Local variables:
mode: c
End:
*/
