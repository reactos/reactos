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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsc/compiler.js,v $
 * $Id: compiler.js,v 1.1 2004/01/10 20:38:17 arty Exp $
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
/*
 * Lexer.
 * Copyright (c) 1998-1999 New Generation Software (NGS) Oy
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsc/compiler.js,v $
 * $Id: compiler.js,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

/*
 * Global functions.
 */

function JSC$lexer (stream)
{
  var ch, ch2;

  JSC$token_value = null;

  while ((ch = stream.readByte ()) != -1)
    {
      if (ch == #'\n')
	{
	  JSC$linenum++;
	  continue;
	}

      if (JSC$lexer_is_white_space (ch))
	continue;

      JSC$token_linenum = JSC$linenum;

      if (ch == #'/' && JSC$lexer_peek_char (stream) == #'*')
	{
	  /* Multi line comment. */
	  stream.readByte ();
	  while ((ch = stream.readByte ()) != -1
		 && (ch != #'*' || JSC$lexer_peek_char (stream) != #'/'))
	    if (ch == #'\n')
	      JSC$linenum++;

	  /* Consume the peeked #'/' character. */
	  stream.readByte ();
	}
      else if ((ch == #'/' && JSC$lexer_peek_char (stream) == #'/')
	       || (ch == #'#' && JSC$lexer_peek_char (stream) == #'!'))
	{
	  /* Single line comment. */
	  while ((ch = stream.readByte ()) != -1 && ch != #'\n')
	    ;
	  if (ch == #'\n')
	    JSC$linenum++;
	}
      else if (ch == #'"' || ch == #'\'')
	{
	  /* String constant. */
	  JSC$token_value = JSC$lexer_read_string (stream, "string", ch);
	  return JSC$tSTRING;
	}

      /* Literals. */
      else if (ch == #'=' && JSC$lexer_peek_char (stream) == #'=')
	{
	  stream.readByte ();
	  if (JSC$lexer_peek_char (stream) == #'=')
	    {
	      stream.readByte ();
	      return JSC$tSEQUAL;
	    }
	  return JSC$tEQUAL;
	}
      else if (ch == #'!' && JSC$lexer_peek_char (stream) == #'=')
	{
	  stream.readByte ();
	  if (JSC$lexer_peek_char (stream) == #'=')
	    {
	      stream.readByte ();
	      return JSC$tSNEQUAL;
	    }
	  return JSC$tNEQUAL;
	}
      else if (ch == #'<' && JSC$lexer_peek_char (stream) == #'=')
	{
	  stream.readByte ();
	  return JSC$tLE;
	}
      else if (ch == #'>' && JSC$lexer_peek_char (stream) == #'=')
	{
	  stream.readByte ();
	  return JSC$tGE;
	}
      else if (ch == #'&' && JSC$lexer_peek_char (stream) == #'&')
	{
	  stream.readByte ();
	  return JSC$tAND;
	}
      else if (ch == #'|' && JSC$lexer_peek_char (stream) == #'|')
	{
	  stream.readByte ();
	  return JSC$tOR;
	}
      else if (ch == #'+' && JSC$lexer_peek_char (stream) == #'+')
	{
	  stream.readByte ();
	  return JSC$tPLUSPLUS;
	}
      else if (ch == #'-' && JSC$lexer_peek_char (stream) == #'-')
	{
	  stream.readByte ();
	  return JSC$tMINUSMINUS;
	}
      else if (ch == #'*' && JSC$lexer_peek_char (stream) == #'=')
	{
	  stream.readByte ();
	  return JSC$tMULA;
	}
      else if (ch == #'/' && JSC$lexer_peek_char (stream) == #'=')
	{
	  stream.readByte ();
	  return JSC$tDIVA;
	}
      else if (ch == #'%' && JSC$lexer_peek_char (stream) == #'=')
	{
	  stream.readByte ();
	  return JSC$tMODA;
	}
      else if (ch == #'+' && JSC$lexer_peek_char (stream) == #'=')
	{
	  stream.readByte ();
	  return JSC$tADDA;
	}
      else if (ch == #'-' && JSC$lexer_peek_char (stream) == #'=')
	{
	  stream.readByte ();
	  return JSC$tSUBA;
	}
      else if (ch == #'&' && JSC$lexer_peek_char (stream) == #'=')
	{
	  stream.readByte ();
	  return JSC$tANDA;
	}
      else if (ch == #'^' && JSC$lexer_peek_char (stream) == #'=')
	{
	  stream.readByte ();
	  return JSC$tXORA;
	}
      else if (ch == #'|' && JSC$lexer_peek_char (stream) == #'=')
	{
	  stream.readByte ();
	  return JSC$tORA;
	}
      else if (ch == #'<' && JSC$lexer_peek_char (stream) == #'<')
	{
	  stream.readByte ();
	  if (JSC$lexer_peek_char (stream) == #'=')
	    {
	      stream.readByte ();
	      return JSC$tLSIA;
	    }
	  else
	    return JSC$tLSHIFT;
	}
      else if (ch == #'>' && JSC$lexer_peek_char (stream) == #'>')
	{
	  stream.readByte ();
	  ch2 = JSC$lexer_peek_char (stream);
	  if (ch2 == #'=')
	    {
	      stream.readByte ();
	      return JSC$tRSIA;
	    }
	  else if (ch2 == #'>')
	    {
	      stream.readByte ();
	      if (JSC$lexer_peek_char (stream) == #'=')
		{
		  stream.readByte ();
		  return JSC$tRRSA;
		}
	      else
		return JSC$tRRSHIFT;
	    }
	  else
	    return JSC$tRSHIFT;
	}

      /* Identifiers and keywords. */
      else if (JSC$lexer_is_identifier_letter (ch))
	{
	  /* An identifier. */
	  var id = String.fromCharCode (ch);

	  while ((ch = stream.readByte ()) != -1
		 && (JSC$lexer_is_identifier_letter (ch)
		     || JSC$lexer_is_decimal_digit (ch)))
	    id.append (File.byteToString (ch));
	  stream.ungetByte (ch);

	  /* Keywords. */
	  if (id == "break")
	    return JSC$tBREAK;
	  else if (id == "continue")
	    return JSC$tCONTINUE;
	  else if (id == "delete")
	    return JSC$tDELETE;
	  else if (id == "else")
	    return JSC$tELSE;
	  else if (id == "for")
	    return JSC$tFOR;
	  else if (id == "function")
	    return JSC$tFUNCTION;
	  else if (id == "if")
	    return JSC$tIF;
	  else if (id == "in")
	    return JSC$tIN;
	  else if (id == "new")
	    return JSC$tNEW;
	  else if (id == "return")
	    return JSC$tRETURN;
	  else if (id == "this")
	    return JSC$tTHIS;
	  else if (id == "typeof")
	    return JSC$tTYPEOF;
	  else if (id == "var")
	    return JSC$tVAR;
	  else if (id == "void")
	    return JSC$tVOID;
	  else if (id == "while")
	    return JSC$tWHILE;
	  else if (id == "with")
	    return JSC$tWITH;

	  /*
	   * Future reserved keywords (some of these is already in use
	   * in this implementation).
	   */
	  else if (id == "case")
	    return JSC$tCASE;
	  else if (id == "catch")
	    return JSC$tCATCH;
	  else if (id == "class")
	    return JSC$tCLASS;
	  else if (id == "const")
	    return JSC$tCONST;
	  else if (id == "debugger")
	    return JSC$tDEBUGGER;
	  else if (id == "default")
	    return JSC$tDEFAULT;
	  else if (id == "do")
	    return JSC$tDO;
	  else if (id == "enum")
	    return JSC$tENUM;
	  else if (id == "export")
	    return JSC$tEXPORT;
	  else if (id == "extends")
	    return JSC$tEXTENDS;
	  else if (id == "finally")
	    return JSC$tFINALLY;
	  else if (id == "import")
	    return JSC$tIMPORT;
	  else if (id == "super")
	    return JSC$tSUPER;
	  else if (id == "switch")
	    return JSC$tSWITCH;
	  else if (id == "throw")
	    return JSC$tTHROW;
	  else if (id == "try")
	    return JSC$tTRY;

	  /* Null and boolean literals. */
	  else if (id == "null")
	    return JSC$tNULL;
	  else if (id == "true")
	    return JSC$tTRUE;
	  else if (id == "false")
	    return JSC$tFALSE;
	  else
	    {
	      /* It really is an identifier. */
	      JSC$token_value = id;
	      return JSC$tIDENTIFIER;
	    }
	}

      /* Character constants. */
      else if (ch == #'#' && JSC$lexer_peek_char (stream) == #'\'')
	{
	  /* Skip the starting #'\'' and read more. */
	  stream.readByte ();

	  ch = stream.readByte ();
	  if (ch == #'\\')
	    {
	      JSC$token_value
		= JSC$lexer_read_backslash_escape (stream, 0, "character");

	      if (stream.readByte () != #'\'')
		error (JSC$filename + ":" + JSC$linenum.toString ()
		       + ": malformed character constant");
	    }
	  else if (JSC$lexer_peek_char (stream) == #'\'')
	    {
	      stream.readByte ();
	      JSC$token_value = ch;
	    }
	  else
	    error (JSC$filename + ":" + JSC$linenum.toString ()
		   + ": malformed character constant");

	  return JSC$tINTEGER;
	}

      /* Octal and hex numbers. */
      else if (ch == #'0'
	       && JSC$lexer_peek_char (stream) != #'.'
	       && JSC$lexer_peek_char (stream) != #'e'
	       && JSC$lexer_peek_char (stream) != #'E')
	{
	  JSC$token_value = 0;
	  ch = stream.readByte ();
	  if (ch == #'x' || ch == #'X')
	    {
	      ch = stream.readByte ();
	      while (JSC$lexer_is_hex_digit (ch))
		{
		  JSC$token_value *= 16;
		  JSC$token_value += JSC$lexer_hex_to_dec (ch);
		  ch = stream.readByte ();
		}
	      stream.ungetByte (ch);
	    }
	  else
	    {
	      while (JSC$lexer_is_octal_digit (ch))
		{
		  JSC$token_value *= 8;
		  JSC$token_value += ch - #'0';
		  ch = stream.readByte ();
		}
	      stream.ungetByte (ch);
	    }

	  return JSC$tINTEGER;
	}

      /* Decimal numbers. */
      else if (JSC$lexer_is_decimal_digit (ch)
	       || (ch == #'.'
		   && JSC$lexer_is_decimal_digit (
					JSC$lexer_peek_char (stream))))
	{
	  var is_float = false;
	  var buf = new String (File.byteToString (ch));
	  var accept_dot = true;

	  if (ch == #'.')
	    {
	      /*
	       * We started with #'.' and we know that the next character
	       * is a decimal digit (we peeked it).
	       */
	      is_float = true;

	      ch = stream.readByte ();
	      while (JSC$lexer_is_decimal_digit (ch))
		{
		  buf.append (File.byteToString (ch));
		  ch = stream.readByte ();
		}
	      accept_dot = false;
	    }
	  else
	    {
	      /* We did start with a decimal digit. */
	      ch = stream.readByte ();
	      while (JSC$lexer_is_decimal_digit (ch))
		{
		  buf.append (File.byteToString (ch));
		  ch = stream.readByte ();
		}
	    }

	  if ((accept_dot && ch == #'.')
	      || ch == #'e' || ch == #'E')
	    {
	      is_float = true;

	      if (ch == #'.')
		{
		  buf.append (File.byteToString (ch));
		  ch = stream.readByte ();
		  while (JSC$lexer_is_decimal_digit (ch))
		    {
		      buf.append (File.byteToString (ch));
		      ch = stream.readByte ();
		    }
		}

	      if (ch == #'e' || ch == #'E')
		{
		  buf.append (File.byteToString (ch));
		  ch = stream.readByte ();
		  if (ch == #'+' || ch == #'-')
		    {
		      buf.append (File.byteToString (ch));
		      ch = stream.readByte ();
		    }
		  if (!JSC$lexer_is_decimal_digit (ch))
		    error (JSC$filename + ":" + JSC$linenum.toString ()
			   + ": malformed exponent part in a decimal literal");

		  while (JSC$lexer_is_decimal_digit (ch))
		    {
		      buf.append (File.byteToString (ch));
		      ch = stream.readByte ();
		    }
		}
	    }

	  /* Finally, we put the last character pack to the stream. */
	  stream.ungetByte (ch);

	  if (is_float)
	    {
	      JSC$token_value = parseFloat (buf);
	      return JSC$tFLOAT;
	    }

	  JSC$token_value = parseInt (buf);
	  return JSC$tINTEGER;
	}

      /* Just return the character as-is. */
      else
	return ch;
    }

  /* EOF reached. */
  return JSC$tEOF;
}


/*
 * Help functions.
 */

function JSC$lexer_peek_char (stream)
{
  var ch2 = stream.readByte ();
  stream.ungetByte (ch2);

  return ch2;
}


function JSC$lexer_is_identifier_letter (ch)
{
  return ((#'a' <= ch && ch <= #'z') || (#'A' <= ch && ch <= #'Z')
	  || ch == #'$' || ch == #'_');
}


function JSC$lexer_is_octal_digit (ch)
{
  return (#'0' <= ch && ch <= #'7');
}


function JSC$lexer_is_decimal_digit (ch)
{
  return #'0' <= ch && ch <= #'9';
}


function JSC$lexer_is_hex_digit (ch)
{
  return ((#'0' <= ch && ch <= #'9')
	  || (#'a' <= ch && ch <= #'f')
	  || (#'A' <= ch && ch <= #'F'));
}


function JSC$lexer_is_white_space (ch)
{
  return (ch == #' ' || ch == #'\t' || ch == #'\v' || ch == #'\r'
	  || ch == #'\f' || ch == #'\n');
}


function JSC$lexer_hex_to_dec (ch)
{
  return ((#'0' <= ch && ch <= #'9')
	  ? ch - #'0'
	  : ((#'a' <= ch && ch <= #'f')
	     ? 10 + ch - #'a'
	     : 10 + ch - #'A'));
}


function JSC$lexer_read_backslash_escape (stream, possible_start, name)
{
  var ch = stream.readByte ();

  if (ch == #'n')
    ch = #'\n';
  else if (ch == #'t')
    ch = #'\t';
  else if (ch == #'v')
    ch = #'\v';
  else if (ch == #'b')
    ch = #'\b';
  else if (ch == #'r')
    ch = #'\r';
  else if (ch == #'f')
    ch = #'\f';
  else if (ch == #'a')
    ch = #'\a';
  else if (ch == #'\\')
    ch = #'\\';
  else if (ch == #'?')
    ch = #'?';
  else if (ch == #'\'')
    ch = #'\'';
  else if (ch == #'"')
    ch = #'"';
  else if (ch == #'x')
    {
      /* HexEscapeSequence. */
      var c1, c2;

      c1 = stream.readByte ();
      c2 = stream.readByte ();

      if (c1 == -1 || c2 == -1)
	JSC$lexer_eof_in_constant (possible_start, name);

      if (!JSC$lexer_is_hex_digit (c1) || !JSC$lexer_is_hex_digit (c2))
	error (JSC$filename + ":" + JSC$linenum.toString ()
	       + ": \\x used with no following hex digits");

      ch = (JSC$lexer_hex_to_dec (c1) << 4) + JSC$lexer_hex_to_dec (c2);
    }
  else if (ch == #'u')
    {
      /* UnicodeEscapeSequence. */
      var c1, c2, c3, c4;

      c1 = stream.readByte ();
      c2 = stream.readByte ();
      c3 = stream.readByte ();
      c4 = stream.readByte ();

      if (c1 == -1 || c2 == -1 || c3 == -1 || c4 == -1)
	JSC$lexer_eof_in_constant (possible_start, name);

      if (!JSC$lexer_is_hex_digit (c1) || !JSC$lexer_is_hex_digit (c2)
	  || !JSC$lexer_is_hex_digit (c3) || !JSC$lexer_is_hex_digit (c4))
	error (JSC$filename + ":" + JSC$linenum.toString ()
	       + ": \\u used with no following hex digits");

      ch = ((JSC$lexer_hex_to_dec (c1) << 12)
	    + (JSC$lexer_hex_to_dec (c2) << 8)
	    + (JSC$lexer_hex_to_dec (c3) << 4)
	    + JSC$lexer_hex_to_dec (c4));
    }
  else if (JSC$lexer_is_octal_digit (ch))
    {
      var result = ch - #'0';
      var i = 1;

      if (ch == #'0')
	/* Allow three octal digits after '0'. */
	i = 0;

      ch = stream.readByte ();
      while (i < 3 && JSC$lexer_is_octal_digit (ch))
	{
	  result *= 8;
	  result += ch - #'0';
	  ch = stream.readByte ();
	  i++;
	}
      stream.ungetByte (ch);
      ch = result;
    }
  else
    {
      if (ch == -1)
	error (JSC$filename + ":" + JSC$linenum.toString ()
	       + ": unterminated " + name);

      JSC$warning (JSC$filename + ":" + JSC$linenum.toString ()
		   + ": warning: unknown escape sequence `\\"
		   + File.byteToString (ch) + "'");
    }

  return ch;
}


function JSC$lexer_read_string (stream, name, ender)
{
  var str = new String ("");
  var done = false, ch;
  var possible_start_ln = JSC$linenum;
  var warned_line_terminator = false;

  while (!done)
    {
      ch = stream.readByte ();
      if (ch == #'\n')
	{
	  if (JSC$warn_strict_ecma && !warned_line_terminator)
	    {
	      JSC$warning (JSC$filename + ":" + JSC$linenum.toString ()
			   + ": warning: ECMAScript don't allow line terminators in "
			   + name + " constants");
	      warned_line_terminator = true;
	    }
	  JSC$linenum++;
	}

      if (ch == -1)
	JSC$lexer_eof_in_constant (possible_start_ln, name);

      else if (ch == ender)
	done = true;
      else
	{
	  if (ch == #'\\')
	    {
	      if (JSC$lexer_peek_char (stream) == #'\n')
		{
		  /*
		   * Backslash followed by a newline character.  Ignore
		   * them both.
		   */
		  stream.readByte ();
		  JSC$linenum++;
		  continue;
		}
	      ch = JSC$lexer_read_backslash_escape (stream, possible_start_ln,
						    name);
	    }
	  str.append (ch);
	}
    }

  return str;
}


function JSC$lexer_read_regexp_constant (stream)
{
  /* Regexp literal. */
  var source = JSC$lexer_read_regexp_source (stream);

  /* Check the possible flags. */
  var flags = new String ("");
  while ((ch = JSC$lexer_peek_char (stream)) == #'g' || ch == #'i')
    {
      stream.readByte ();
      flags.append (File.byteToString (ch));
    }

  /* Try to compile it. */
  var msg = false;
  var result;
  try
    {
      result = new RegExp (source, flags);
    }
  catch (msg)
    {
      var start = msg.lastIndexOf (":");
      msg = (JSC$filename + ":" + JSC$token_linenum.toString ()
	     + ": malformed regular expression constant:"
	     + msg.substr (start + 1));
    }
  if (msg)
    error (msg);

  /* Success. */

  return result;
}


function JSC$lexer_read_regexp_source (stream)
{
  var str = new String ("");
  var done = false, ch;
  var possible_start_ln = JSC$linenum;
  var warned_line_terminator = false;
  var name = "regular expression";

  while (!done)
    {
      ch = stream.readByte ();
      if (ch == #'\n')
	{
	  if (JSC$warn_strict_ecma && !warned_line_terminator)
	    {
	      JSC$warning (JSC$filename + ":" + JSC$linenum.toString ()
			   + ": warning: ECMAScript don't allow line "
			   + "terminators in " + name + " constants");
	      warned_line_terminator = true;
	    }
	  JSC$linenum++;
	}

      if (ch == -1)
	JSC$lexer_eof_in_constant (possible_start_ln, name);

      else if (ch == #'/')
	done = true;
      else
	{
	  if (ch == #'\\')
	    {
	      ch = stream.readByte ();
	      if (ch == #'\n')
		{
		  /*
		   * Backslash followed by a newline character.  Ignore
		   * them both.
		   */
		  JSC$linenum++;
		  continue;
		}
	      if (ch == -1)
		JSC$lexer_eof_in_constant (possible_start_ln, name);

	      /* Handle the backslash escapes. */
	      if (ch == #'f')
		ch = #'\f';
	      else if (ch == #'n')
		ch = #'\n';
	      else if (ch == #'r')
		ch = #'\r';
	      else if (ch == #'t')
		ch = #'\t';
	      else if (ch == #'v')
		ch == #'\v';
	      else if (ch == #'c')
		{
		  /* SourceCharacter. */
		  ch = stream.readByte ();
		  if (ch == -1)
		    JSC$lexer_eof_in_constant (possible_start_ln, name);

		  if (ch == #'\n' && JSC$warn_strict_ecma)
		    JSC$warning (JSC$filename + ":" + JSC$linenum.toString ()
				 + ": warning: ECMAScript don't allow line termiantor after \\c in regular expression constants");

		  /*
		   * Append the source-character escape start.  The ch
		   * will be appended later.
		   */
		  str.append ("\\c");
		}
	      else if (ch == #'u' || ch == #'x' || ch == #'0')
		{
		  /* These can be handled with the read_backslash_escape(). */
		  stream.ungetByte (ch);
		  ch = JSC$lexer_read_backslash_escape (stream);
		}
	      else
		{
		  /*
		   * Nothing special.  Leave it to the result as-is.
		   * The regular expression backage will handle it.
		   */
		  stream.ungetByte (ch);
		  ch = #'\\';
		}
	    }
	  str.append (File.byteToString (ch));
	}
    }

  return str;
}


function JSC$lexer_eof_in_constant (possible_start, name)
{
  var msg = (JSC$filename + ":" + JSC$linenum.toString ()
	     + ": unterminated " + name + " constant");

  if (possible_start > 0)
    msg += (System.lineBreakSequence
	    + JSC$filename + ":" + possible_start.toString ()
	    + ": possible real start of unterminated " + name + " constant");

  error (msg);
}


/*
Local variables:
mode: c
End:
*/
/*
 * Parser.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsc/compiler.js,v $
 * $Id: compiler.js,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

/*
 * Global functions.
 */

function JSC$parser_reset ()
{
  JSC$function = null;
  JSC$global_stmts = null;
  JSC$nested_function_declarations = null;
}


function JSC$parser_parse (stream)
{
  JSC$linenum = 1;
  JSC$filename = stream.name;
  JSC$functions = new Array ();
  JSC$global_stmts = new Array ();
  JSC$nested_function_declarations = new Array ();
  JSC$anonymous_function_count = 0;
  JSC$parser_peek_token_valid = false;
  JSC$num_tokens = 0;
  JSC$num_arguments_identifiers = 0;
  JSC$num_missing_semicolons = 0;

  if (JSC$verbose)
    JSC$message ("jsc: parsing");

  while (JSC$parser_peek_token (stream) != JSC$tEOF)
    if (!JSC$parser_parse_source_element (stream))
      JSC$parser_syntax_error ();

  if (JSC$verbose)
    {
      var msg = ("jsc: input stream had " + (JSC$linenum - 1).toString ()
		 + " lines, " + JSC$num_tokens.toString () + " tokens");

      if (JSC$num_missing_semicolons > 0)
	msg += (", " + JSC$num_missing_semicolons.toString ()
		+ " missing semicolons");

      JSC$message (msg);
    }
}


/*
 * General help functions.
 */

function JSC$parser_syntax_error ()
{
  error (JSC$filename + ":" + JSC$linenum.toString () + ": syntax error");
}

/* All warnings are reported through this function. */
function JSC$warning (line)
{
  System.stderr.writeln (line);
}

/* All messages are reported throught this function. */
function JSC$message (line)
{
  System.stderr.writeln (line);
}


function JSC$parser_get_token (stream)
{
  JSC$num_tokens++;

  var token;
  if (JSC$parser_peek_token_valid)
    {
      JSC$parser_peek_token_valid = false;
      JSC$parser_token_value = JSC$parser_peek_token_value;
      JSC$parser_token_linenum = JSC$parser_peek_token_linenum;
      token = JSC$parser_peek_token_token;
    }
  else
    {
      token = JSC$lexer (stream);
      JSC$parser_token_value = JSC$token_value;
      JSC$parser_token_linenum = JSC$token_linenum;
    }

  if (token == JSC$tIDENTIFIER && JSC$parser_token_value == "arguments")
    JSC$num_arguments_identifiers++;

  return token;
}


function JSC$parser_peek_token (stream)
{
  if (JSC$parser_peek_token_valid)
    return JSC$parser_peek_token_token;
  else
    {
      JSC$parser_peek_token_token = JSC$lexer (stream);
      JSC$parser_peek_token_value = JSC$token_value;
      JSC$parser_peek_token_linenum = JSC$token_linenum;
      JSC$parser_peek_token_valid = true;
      return JSC$parser_peek_token_token;
    }
}


function JSC$parser_get_semicolon_asci (stream)
{
  var token = JSC$parser_peek_token (stream);

  if (token == #';')
    {
      /* Everything ok.  It was there. */
      return JSC$parser_get_token (stream);
    }

  /* No semicolon.  Let's see if we can insert it there. */
  if (token == #'}'
      || JSC$parser_token_linenum < JSC$parser_peek_token_linenum
      || token == JSC$tEOF)
    {
      /* Ok, do the automatic semicolon insertion. */
      if (JSC$warn_missing_semicolon)
	JSC$warning (JSC$filename + ":" + JSC$parser_token_linenum.toString ()
		     + ": warning: missing semicolon");
      JSC$num_missing_semicolons++;
      return #';';
    }

  /* Sorry, no can do. */
  JSC$parser_syntax_error ();
}


function JSC$parser_expr_is_left_hand_side (expr)
{
  return (expr.etype == JSC$EXPR_CALL
	  || expr.etype == JSC$EXPR_OBJECT_PROPERTY
	  || expr.etype == JSC$EXPR_OBJECT_ARRAY
	  || expr.etype == JSC$EXPR_NEW
	  || expr.etype == JSC$EXPR_THIS
	  || expr.etype == JSC$EXPR_IDENTIFIER
	  || expr.etype == JSC$EXPR_FLOAT
	  || expr.etype == JSC$EXPR_INTEGER
	  || expr.etype == JSC$EXPR_STRING
	  || expr.etype == JSC$EXPR_REGEXP
	  || expr.etype == JSC$EXPR_ARRAY_INITIALIZER
	  || expr.etype == JSC$EXPR_NULL
	  || expr.etype == JSC$EXPR_TRUE
	  || expr.etype == JSC$EXPR_FALSE);
}


function JSC$parser_parse_source_element (stream)
{
  if (JSC$parser_parse_function_declaration (stream))
    return true;

  var stmt = JSC$parser_parse_stmt (stream);
  if (!stmt)
    return false;

  if (stmt.stype == JSC$STMT_VARIABLE)
    /*
     * This is a variable declaration at the global level.  These
     * are actually global variables.
     */
    stmt.global_level = true;

  JSC$global_stmts.push (stmt);

  return true;
}


function JSC$parser_parse_function_declaration (stream)
{
  var id, args, block;

  if (JSC$parser_peek_token (stream) != JSC$tFUNCTION)
    return false;

  /* Record how many `arguments' identifiers have been seen so far. */
  var num_arguments_identifiers = JSC$num_arguments_identifiers;

  JSC$parser_get_token (stream);
  if (JSC$parser_get_token (stream) != JSC$tIDENTIFIER)
    JSC$parser_syntax_error ();

  id = JSC$parser_token_value;
  var ln = JSC$parser_token_linenum;
  var id_given = id;

  if (JSC$nested_function_declarations.length > 0)
    {
      /* This is a nested function declaration. */
      id = ".F:" + (JSC$anonymous_function_count++).toString ();
    }
  JSC$nested_function_declarations.push (id);

  if (JSC$parser_get_token (stream) != #'(')
    JSC$parser_syntax_error ();

  /* Formal parameter list opt. */
  args = new Array ();
  while (JSC$parser_peek_token (stream) != #')')
    {
      if (JSC$parser_get_token (stream) != JSC$tIDENTIFIER)
	JSC$parser_syntax_error ();
      args.push (JSC$parser_token_value);

      var token = JSC$parser_peek_token (stream);
      if (token == #',')
	{
	  JSC$parser_get_token (stream);
	  if (JSC$parser_peek_token (stream) != JSC$tIDENTIFIER)
	    JSC$parser_syntax_error ();
	}
      else if (token != #')')
	JSC$parser_syntax_error ();
    }

  if (JSC$parser_get_token (stream) != #')')
    JSC$parser_syntax_error ();

  JSC$parser_peek_token (stream);
  var lbrace_ln = JSC$parser_peek_token_linenum;

  block = JSC$parser_parse_block (stream);
  if (typeof block == "boolean")
    JSC$parser_syntax_error ();

  /* Did the function use the `arguments' identifier? */
  var use_arguments = false;
  if (JSC$num_arguments_identifiers > num_arguments_identifiers)
    {
      use_arguments = true;
      if (JSC$warn_deprecated)
	JSC$warning (JSC$filename + ":" + ln.toString ()
		     + ": warning: the `arguments' property of Function "
		     + "instance is deprecated");
    }

  JSC$functions.push (new JSC$function_declaration (ln, lbrace_ln, id,
						    id_given, args,
						    block, use_arguments));

  JSC$nested_function_declarations.pop ();

  return true;
}


function JSC$parser_parse_block (stream)
{
  var block;

  if (JSC$parser_peek_token (stream) != #'{')
    return false;

  JSC$parser_get_token (stream) != #'{';
  var ln = JSC$parser_peek_token_linenum;

  /* Do we have a statement list? */
  if (JSC$parser_peek_token (stream) != #'}')
    /* Yes we have. */
    block = JSC$parser_parse_stmt_list (stream);
  else
    /* Do we don't */
    block = new Array ();

  if (JSC$parser_get_token (stream) != #'}')
    JSC$parser_syntax_error ();

  block.linenum = ln;

  return block;
}


function JSC$parser_parse_stmt_list (stream)
{
  var list, done, item;

  list = new Array ();
  done = false;

  while (!done)
    {
      item = JSC$parser_parse_stmt (stream);
      if (typeof item == "boolean")
	{
	  /* Can't parse more statements.  We'r done. */
	  done = true;
	}
      else
	list.push (item);
    }

  return list;
}


function JSC$parser_parse_stmt (stream)
{
  var item, token;

  if (typeof (item = JSC$parser_parse_block (stream)) != "boolean")
    return new JSC$stmt_block (item.linenum, item);
  else if (JSC$parser_parse_function_declaration (stream))
    {
      /* XXX The function declaration as statement might be incomplete. */

      if (JSC$nested_function_declarations.length == 0)
	/* Function declaration at top-level statements. */
	return new JSC$stmt_empty (JSC$parser_token_linenum);

      /* Function declaration inside another function. */

      var container_id = JSC$nested_function_declarations.pop ();
      JSC$nested_function_declarations.push (container_id);

      var f = JSC$functions[JSC$functions.length - 1];
      var function_id = f.name;
      var given_id = f.name_given;

      return new JSC$stmt_function_declaration (JSC$parser_token_linenum,
						container_id, function_id,
						given_id);
    }
  else if (typeof (item = JSC$parser_parse_variable_stmt (stream))
	   != "boolean")
    return item;
  else if (typeof (item = JSC$parser_parse_if_stmt (stream))
	   != "boolean")
    return item;
  else if (typeof (item = JSC$parser_parse_iteration_stmt (stream))
	   != "boolean")
    return item;
  else if (typeof (item = JSC$parser_parse_expr (stream))
	   != "boolean")
    {
      if (item.etype == JSC$EXPR_IDENTIFIER)
	{
	  /* Possible `Labeled Statement'. */
	  token = JSC$parser_peek_token (stream);
	  if (token == #':' && item.linenum == JSC$parser_peek_token_linenum)
	    {
	      /* Yes it is. */
	      JSC$parser_get_token (stream);
	      var stmt = JSC$parser_parse_stmt (stream);
	      if (!stmt)
		JSC$parser_syntax_error;

	      return new JSC$stmt_labeled_stmt (item.linenum, item.value,
						stmt);
	    }
	  /* FALLTHROUGH */
	}

      JSC$parser_get_semicolon_asci (stream);
      return new JSC$stmt_expr (item);
    }
  else
    {
      token = JSC$parser_peek_token (stream);
      if (token == #';')
	{
	  JSC$parser_get_token (stream);
	  return new JSC$stmt_empty (JSC$parser_token_linenum);
	}
      else if (token == JSC$tCONTINUE)
	{
	  JSC$parser_get_token (stream);

	  /* Check the possible label. */
	  var label = null;
	  token = JSC$parser_peek_token (stream);
	  if (token == JSC$tIDENTIFIER
	      && JSC$parser_token_linenum == JSC$parser_peek_token_linenum)
	    {
	      JSC$parser_get_token (stream);
	      label = JSC$parser_token_value;
	    }

	  item = new JSC$stmt_continue (JSC$parser_token_linenum, label);

	  JSC$parser_get_semicolon_asci (stream);

	  return item;
	}
      else if (token == JSC$tBREAK)
	{
	  JSC$parser_get_token (stream);

	  /* Check the possible label. */
	  var label = null;
	  token = JSC$parser_peek_token (stream);
	  if (token == JSC$tIDENTIFIER
	      && JSC$parser_token_linenum == JSC$parser_peek_token_linenum)
	    {
	      JSC$parser_get_token (stream);
	      label = JSC$parser_token_value;
	    }

	  item = new JSC$stmt_break (JSC$parser_token_linenum, label);

	  JSC$parser_get_semicolon_asci (stream);

	  return item;
	}
      else if (token == JSC$tRETURN)
	{
	  JSC$parser_get_token (stream);
	  var linenum = JSC$parser_token_linenum;

	  if (JSC$parser_peek_token (stream) == #';')
	    {
	      /* Consume the semicolon. */
	      JSC$parser_get_token (stream);
	      item = null;
	    }
	  else
	    {
	      if (JSC$parser_peek_token_linenum > linenum)
		{
		  /*
		   * A line terminator between tRETURN and the next
		   * token that is not a semicolon.  ASCI here.
		   */
		  if (JSC$warn_missing_semicolon)
		    JSC$warning (JSC$filename + ":" + linenum.toString ()
				 + ": warning: missing semicolon");

		  JSC$num_missing_semicolons++;
		  item = null;
		}
	      else
		{
		  item = JSC$parser_parse_expr (stream);
		  if (typeof item == "boolean")
		    JSC$parser_syntax_error ();

		  JSC$parser_get_semicolon_asci (stream);
		}
	    }

	  return new JSC$stmt_return (linenum, item);
	}
      else if (token == JSC$tSWITCH)
	{
	  JSC$parser_get_token (stream);
	  return JSC$parser_parse_switch (stream);
	}
      else if (token == JSC$tWITH)
	{
	  JSC$parser_get_token (stream);
	  var linenum = JSC$parser_token_linenum;

	  if (JSC$parser_get_token (stream) != #'(')
	    JSC$parser_syntax_error ();

	  var expr = JSC$parser_parse_expr (stream);
	  if (typeof expr == "boolean")
	    JSC$parser_syntax_error ();

	  if (JSC$parser_get_token (stream) != #')')
	    JSC$parser_syntax_error ();

	  var stmt = JSC$parser_parse_stmt (stream);
	  if (typeof stmt == "boolean")
	    JSC$parser_syntax_error ();

	  return new JSC$stmt_with (linenum, expr, stmt);
	}
      else if (token == JSC$tTRY)
	{
	  JSC$parser_get_token (stream);
	  return JSC$parser_parse_try (stream);
	}
      else if (token == JSC$tTHROW)
	{
	  JSC$parser_get_token (stream);
	  var linenum = JSC$parser_token_linenum;

	  /*
	   * Get the next token's linenum.  We need it for strict_ecma
	   * warning.
	   */
	  JSC$parser_peek_token (stream);
	  var peek_linenum = JSC$parser_peek_token_linenum;

	  /* The expression to throw. */
	  var expr = JSC$parser_parse_expr (stream);
	  if (typeof expr == "boolean")
	    JSC$parser_syntax_error ();

	  if (JSC$warn_strict_ecma && peek_linenum > linenum)
	    JSC$warning (JSC$filename + ":" + JSC$linenum.toString ()
			 + ": warning: ECMAScript don't allow line terminators"
			 + " between `throw' and expression");

	  JSC$parser_get_semicolon_asci (stream);

	  return new JSC$stmt_throw (linenum, expr);
	}
      else
	/* Can't parse more.  We'r done. */
	return false;
    }
}


function JSC$parser_parse_switch (stream)
{
  var linenum = JSC$parser_token_linenum;

  if (JSC$parser_get_token (stream) != #'(')
    JSC$parser_syntax_error ();

  var expr = JSC$parser_parse_expr (stream);
  if (!expr)
    JSC$parser_syntax_error ();

  if (JSC$parser_get_token (stream) != #')')
    JSC$parser_syntax_error ();

  if (JSC$parser_get_token (stream) != #'{')
    JSC$parser_syntax_error ();

  /* Parse case clauses. */
  var clauses = new Array ();
  while (true)
    {
      var token = JSC$parser_get_token (stream);

      if (token == #'}')
	break;
      else if (token == JSC$tCASE || token == JSC$tDEFAULT)
	{
	  var stmts = new Array ();
	  stmts.expr = null;

	  if (token == JSC$tCASE)
	    {
	      stmts.expr = JSC$parser_parse_expr (stream);
	      if (!stmts.expr)
		JSC$parser_syntax_error ();
	    }
	  if (JSC$parser_get_token (stream) != #':')
	    JSC$parser_syntax_error ();

	  stmts.linenum = JSC$parser_token_linenum;

	  /* Read the statement list. */
	  while (true)
	    {
	      token = JSC$parser_peek_token (stream);
	      if (token == #'}' || token == JSC$tCASE || token == JSC$tDEFAULT)
		/* Done with this branch. */
		break;

	      var stmt = JSC$parser_parse_stmt (stream);
	      if (!stmt)
		JSC$parser_syntax_error ();

	      stmts.push (stmt);
	    }

	  stmts.last_linenum = JSC$parser_token_linenum;

	  /* One clause parsed. */
	  clauses.push (stmts);
	}
      else
	JSC$parser_syntax_error ();
    }

  return new JSC$stmt_switch (linenum, JSC$parser_token_linenum, expr,
			      clauses);
}


function JSC$parser_parse_try (stream)
{
  var linenum = JSC$parser_token_linenum;

  var block = JSC$parser_parse_stmt (stream);
  if (!block)
    JSC$parser_syntax_error ();

  var try_block_last_linenum = JSC$parser_token_linenum;

  /* Now we must see `catch' or `finally'. */
  var token = JSC$parser_peek_token (stream);
  if (token != JSC$tCATCH && token != JSC$tFINALLY)
    JSC$parser_syntax_error ();

  var catch_list = false;
  if (token == JSC$tCATCH)
    {
      /* Parse catch list. */

      catch_list = new Array ();
      catch_list.linenum = JSC$parser_peek_token_linenum;

      while (token == JSC$tCATCH)
	{
	  JSC$parser_get_token (stream);
	  var c = new Object ();
	  c.linenum = JSC$parser_token_linenum;

	  if (JSC$parser_get_token (stream) != #'(')
	    JSC$parser_syntax_error ();

	  if (JSC$parser_get_token (stream) != JSC$tIDENTIFIER)
	    JSC$parser_syntax_error ();
	  c.id = JSC$parser_token_value;

	  c.guard = false;
	  if (JSC$parser_peek_token (stream) == JSC$tIF)
	    {
	      JSC$parser_get_token (stream);
	      c.guard = JSC$parser_parse_expr (stream);
	      if (!c.guard)
		JSC$parser_syntax_error ();
	    }

	  if (JSC$parser_get_token (stream) != #')')
	    JSC$parser_syntax_error ();

	  c.stmt = JSC$parser_parse_stmt (stream);
	  if (!c.stmt)
	    JSC$parser_syntax_error ();

	  catch_list.push (c);

	  token = JSC$parser_peek_token (stream);
	}

      catch_list.last_linenum = JSC$parser_token_linenum;
    }

  var fin = false;
  if (token == JSC$tFINALLY)
    {
      /* Parse the finally. */
      JSC$parser_get_token (stream);

      fin = JSC$parser_parse_stmt (stream);
      if (!fin)
	JSC$parser_syntax_error ();
    }

  return new JSC$stmt_try (linenum, try_block_last_linenum,
			   JSC$parser_token_linenum, block, catch_list,
			   fin);
}


function JSC$parser_parse_variable_stmt (stream)
{
  var list, id, expr, token;

  if (JSC$parser_peek_token (stream) != JSC$tVAR)
    return false;
  JSC$parser_get_token (stream);
  var ln = JSC$parser_token_linenum;

  list = new Array ();

  while (true)
    {
      token = JSC$parser_peek_token (stream);
      if (token == JSC$tIDENTIFIER)
	{
	  JSC$parser_get_token ();
	  id = JSC$parser_token_value;

	  if (JSC$parser_peek_token (stream) == #'=')
	    {
	      JSC$parser_get_token (stream);
	      expr = JSC$parser_parse_assignment_expr (stream);
	      if (typeof expr == "boolean")
		JSC$parser_syntax_error ();
	    }
	  else
	    expr = null;

	  list.push (new JSC$var_declaration (id, expr));

	  /* Check if we have more input. */
	  if (JSC$parser_peek_token (stream) == #',')
	    {
	      /* Yes we have.  */
	      JSC$parser_get_token (stream);

	      /* The next token must be tIDENTIFIER. */
	      if (JSC$parser_peek_token (stream) != JSC$tIDENTIFIER)
		JSC$parser_syntax_error ();
	    }
	  else
	    {
	      /* No, we don't. */
	      JSC$parser_get_semicolon_asci (stream);
	      break;
	    }
	}
      else
	{
	  /* We'r done. */
	  JSC$parser_get_semicolon_asci (stream);
	  break;
	}
    }

  /* There must be at least one variable declaration. */
  if (list.length == 0)
    JSC$parser_syntax_error ();

  return new JSC$stmt_variable (ln, list);
}


function JSC$parser_parse_if_stmt (stream)
{
  var expr, stmt, stmt2;

  if (JSC$parser_peek_token (stream) != JSC$tIF)
    return false;
  JSC$parser_get_token (stream);
  var ln = JSC$parser_token_linenum;

  if (JSC$parser_get_token (stream) != #'(')
    JSC$parser_syntax_error ();

  expr = JSC$parser_parse_expr (stream);
  if (typeof expr == "boolean")
    JSC$parser_syntax_error ();

  if (JSC$parser_get_token (stream) != #')')
    JSC$parser_syntax_error ();

  stmt = JSC$parser_parse_stmt (stream);
  if (typeof stmt == "boolean")
    JSC$parser_syntax_error ();

  if (JSC$parser_peek_token (stream) == JSC$tELSE)
    {
      JSC$parser_get_token (stream);
      stmt2 = JSC$parser_parse_stmt (stream);
      if (typeof stmt2 == "boolean")
	JSC$parser_syntax_error ();
    }
  else
    stmt2 = null;

  return new JSC$stmt_if (ln, expr, stmt, stmt2);
}


function JSC$parser_parse_iteration_stmt (stream)
{
  var token, expr1, expr2, expr3, stmt;

  token = JSC$parser_peek_token (stream);
  if (token == JSC$tDO)
    {
      /* do Statement while (Expression); */
      JSC$parser_get_token (stream);
      var ln = JSC$parser_token_linenum;

      stmt = JSC$parser_parse_stmt (stream);
      if (typeof stmt == "boolean")
	JSC$parser_syntax_error ();

      if (JSC$parser_get_token (stream) != JSC$tWHILE)
	JSC$parser_syntax_error ();

      if (JSC$parser_get_token (stream) != #'(')
	JSC$parser_syntax_error ();

      expr1 = JSC$parser_parse_expr (stream);
      if (typeof expr1 == "boolean")
	JSC$parser_syntax_error ();

      if (JSC$parser_get_token (stream) != #')')
	JSC$parser_syntax_error ();

      JSC$parser_get_semicolon_asci (stream);

      return new JSC$stmt_do_while (ln, expr1, stmt);
    }
  else if (token == JSC$tWHILE)
    {
      /* while (Expression) Statement */
      JSC$parser_get_token (stream);
      var ln = JSC$parser_token_linenum;

      if (JSC$parser_get_token (stream) != #'(')
	JSC$parser_syntax_error ();

      expr1 = JSC$parser_parse_expr (stream);
      if (typeof expr1 == "boolean")
	JSC$parser_syntax_error ();

      if (JSC$parser_get_token (stream) != #')')
	JSC$parser_syntax_error ();

      stmt = JSC$parser_parse_stmt (stream);
      if (typeof stmt == "boolean")
	JSC$parser_syntax_error ();

      return new JSC$stmt_while (ln, expr1, stmt);
    }
  else if (token == JSC$tFOR)
    {
      JSC$parser_get_token (stream);
      var ln = JSC$parser_token_linenum;

      if (JSC$parser_get_token (stream) != #'(')
	JSC$parser_syntax_error ();

      /* Init */

      var vars = null;

      token = JSC$parser_peek_token (stream);
      if (token == JSC$tVAR)
	{
	  JSC$parser_get_token (stream);
	  vars = new Array ();

	  while (true)
	    {
	      /* The identifier. */
	      token = JSC$parser_peek_token (stream);
	      if (token != JSC$tIDENTIFIER)
		break;

	      JSC$parser_get_token (stream);
	      var id = JSC$parser_token_value;

	      /* Possible initializer. */
	      var expr = null;
	      if (JSC$parser_peek_token (stream) == #'=')
		{
		  JSC$parser_get_token (stream);
		  expr = JSC$parser_parse_assignment_expr (stream);
		  if (!expr)
		    JSC$parser_syntax_error ();
		}

	      vars.push (new JSC$var_declaration (id, expr));

	      /* Check if we have more input. */
	      if (JSC$parser_peek_token (stream) == #',')
		{
		  /* Yes we have. */
		  JSC$parser_get_token (stream);

		  /* The next token must be tIDENTIFIER. */
		  if (JSC$parser_peek_token (stream) != JSC$tIDENTIFIER)
		    JSC$parser_syntax_error ();
		}
	      else
		/* No more input. */
		break;
	    }

	  /* Must have at least one variable declaration. */
	  if (vars.length == 0)
	    JSC$parser_syntax_error ();
	}
      else if (token != #';')
	{
	  expr1 = JSC$parser_parse_expr (stream);
	  if (typeof expr1 == "boolean")
	    JSC$parser_syntax_error ();
	}
      else
	expr1 = null;

      token = JSC$parser_get_token (stream);
      var for_in = false;

      if (token == #';')
	{
	  /* Normal for-statement. */

	  /* Check */
	  if (JSC$parser_peek_token (stream) != #';')
	    {
	      expr2 = JSC$parser_parse_expr (stream);
	      if (typeof expr2 == "boolean")
		JSC$parser_syntax_error ();
	    }
	  else
	    expr2 = null;

	  if (JSC$parser_get_token (stream) != #';')
	    JSC$parser_syntax_error ();

	  /* Increment */
	  if (JSC$parser_peek_token (stream) != #')')
	    {
	      expr3 = JSC$parser_parse_expr (stream);
	      if (typeof expr3 == "boolean")
		JSC$parser_syntax_error ();
	    }
	  else
	    expr3 = null;
	}
      else if (token == JSC$tIN)
	{
	  /* The `for (VAR in EXPR)'-statement. */

	  for_in = true;

	  if (expr1)
	    {
	      /* The first expression must be an identifier. */
	      if (expr1.etype != JSC$EXPR_IDENTIFIER)
		JSC$parser_syntax_error ();
	    }
	  else
	    {
	      /* We must have only one variable declaration. */
	      if (vars.length != 1)
		JSC$parser_syntax_error ();
	    }

	  /* The second expressions. */
	  expr2 = JSC$parser_parse_expr (stream);
	  if (typeof expr2 == "boolean")
	    JSC$parser_syntax_error ();
	}
      else
	JSC$parser_syntax_error ();

      if (JSC$parser_get_token (stream) != #')')
	JSC$parser_syntax_error ();

      /* Stmt. */
      stmt = JSC$parser_parse_stmt (stream);
      if (typeof stmt == "boolean")
	JSC$parser_syntax_error ();

      if (for_in)
	return new JSC$stmt_for_in (ln, vars, expr1, expr2, stmt);

      return new JSC$stmt_for (ln, vars, expr1, expr2, expr3, stmt);
    }
  return false;
}


function JSC$parser_parse_expr (stream)
{
  var expr, expr2;

  if (typeof (expr = JSC$parser_parse_assignment_expr (stream))
      == "boolean")
    return false;

  /* Check for the comma expression. */
  while (JSC$parser_peek_token (stream) == #',')
    {
      JSC$parser_get_token (stream);
      var ln = JSC$parser_token_linenum;

      if (typeof (expr2 = JSC$parser_parse_assignment_expr (stream))
	  == "boolean")
	JSC$parser_syntax_error ();
      expr = new JSC$expr_comma (ln, expr, expr2);
    }

  return expr;
}


function JSC$parser_parse_assignment_expr (stream)
{
  var expr, expr2, token;

  if (typeof (expr = JSC$parser_parse_conditional_expr (stream))
      == "boolean")
    return false;

  if (JSC$parser_expr_is_left_hand_side (expr))
    {
      token = JSC$parser_peek_token (stream);
      if (token == #'=' || token == JSC$tMULA
	  || token == JSC$tDIVA || token == JSC$tMODA
	  || token == JSC$tADDA || token == JSC$tSUBA
	  || token == JSC$tLSIA || token == JSC$tRSIA
	  || token == JSC$tRRSA || token == JSC$tANDA
	  || token == JSC$tXORA || token == JSC$tORA)
	{
	  JSC$parser_get_token (stream);
	  var ln = JSC$parser_token_linenum;

	  expr2 = JSC$parser_parse_assignment_expr (stream);
	  if (typeof expr2 == "boolean")
	    JSC$parser_syntax_error ();

	  expr = new JSC$expr_assignment (ln, token, expr, expr2);
	}
    }

  if (JSC$optimize_constant_folding && expr.constant_folding)
    return expr.constant_folding ();

  return expr;
}


function JSC$parser_parse_conditional_expr (stream)
{
  var expr, expr2, expr3, token;

  if (typeof (expr = JSC$parser_parse_logical_or_expr (stream))
      == "boolean")
    return false;

  token = JSC$parser_peek_token (stream);
  if (token == #'?')
    {
      JSC$parser_get_token (stream);
      var ln = JSC$parser_token_linenum;

      expr2 = JSC$parser_parse_assignment_expr (stream);
      if (typeof expr2 == "boolean")
	JSC$parser_syntax_error ();

      if (JSC$parser_get_token (stream) != #':')
	JSC$parser_syntax_error ();
      expr3 = JSC$parser_parse_assignment_expr (stream);
      if (typeof expr3 == "boolean")
	JSC$parser_syntax_error ();

      expr = new JSC$expr_quest_colon (ln, expr, expr2, expr3);
    }

  return expr;
}


function JSC$parser_parse_logical_or_expr (stream)
{
  var expr, expr2;

  if (typeof (expr = JSC$parser_parse_logical_and_expr (stream))
      == "boolean")
    return false;

  while (JSC$parser_peek_token (stream) == JSC$tOR)
    {
      JSC$parser_get_token (stream);
      var ln = JSC$parser_token_linenum;

      expr2 = JSC$parser_parse_logical_and_expr (stream);
      if (typeof expr2 == "boolean")
	JSC$parser_syntax_error ();

      expr = new JSC$expr_logical_or (ln, expr, expr2);
    }

  return expr;
}


function JSC$parser_parse_logical_and_expr (stream)
{
  var expr, expr2;

  if (typeof (expr = JSC$parser_parse_bitwise_or_expr (stream))
      == "boolean")
    return false;

  while (JSC$parser_peek_token (stream) == JSC$tAND)
    {
      JSC$parser_get_token (stream);
      var ln = JSC$parser_token_linenum;

      expr2 = JSC$parser_parse_bitwise_or_expr (stream);
      if (typeof expr2 == "boolean")
	JSC$parser_syntax_error ();

      expr = new JSC$expr_logical_and (ln, expr, expr2);
    }

  return expr;
}


function JSC$parser_parse_bitwise_or_expr (stream)
{
  var expr, expr2;

  if (typeof (expr = JSC$parser_parse_bitwise_xor_expr (stream))
      == "boolean")
    return false;

  while (JSC$parser_peek_token (stream) == #'|')
    {
      JSC$parser_get_token (stream);
      var ln = JSC$parser_token_linenum;

      expr2 = JSC$parser_parse_bitwise_xor_expr (stream);
      if (typeof expr2 == "boolean")
	JSC$parser_syntax_error ();

      expr = new JSC$expr_bitwise_or (ln, expr, expr2);
    }

  return expr;
}


function JSC$parser_parse_bitwise_xor_expr (stream)
{
  var expr, expr2;

  if (typeof (expr = JSC$parser_parse_bitwise_and_expr (stream))
      == "boolean")
    return false;

  while (JSC$parser_peek_token (stream) == #'^')
    {
      JSC$parser_get_token (stream);
      var ln = JSC$parser_token_linenum;

      expr2 = JSC$parser_parse_bitwise_and_expr (stream);
      if (typeof expr2 == "boolean")
	JSC$parser_syntax_error ();

      expr = new JSC$expr_bitwise_xor (ln, expr, expr2);
    }

  return expr;
}


function JSC$parser_parse_bitwise_and_expr (stream)
{
  var expr, expr2;

  if (typeof (expr = JSC$parser_parse_equality_expr (stream))
      == "boolean")
    return false;

  while (JSC$parser_peek_token (stream) == #'&')
    {
      JSC$parser_get_token (stream);
      var ln = JSC$parser_token_linenum;

      expr2 = JSC$parser_parse_equality_expr (stream);
      if (typeof expr2 == "boolean")
	JSC$parser_syntax_error ();

      expr = new JSC$expr_bitwise_and (ln, expr, expr2);
    }

  return expr;
}


function JSC$parser_parse_equality_expr (stream)
{
  var expr, expr2, token;

  if (typeof (expr = JSC$parser_parse_relational_expr (stream))
      == "boolean")
    return false;

  token = JSC$parser_peek_token (stream);
  while (token == JSC$tEQUAL || token == JSC$tNEQUAL
	 || token == JSC$tSEQUAL || token == JSC$tSNEQUAL)
    {
      JSC$parser_get_token (stream);
      var ln = JSC$parser_token_linenum;

      expr2 = JSC$parser_parse_relational_expr (stream);
      if (typeof expr2 == "boolean")
	JSC$parser_syntax_error ();

      expr = new JSC$expr_equality (ln, token, expr, expr2);
      token = JSC$parser_peek_token (stream);
    }

  return expr;
}


function JSC$parser_parse_relational_expr (stream)
{
  var expr, expr2, token;

  if (typeof (expr = JSC$parser_parse_shift_expr (stream))
      == "boolean")
    return false;

  token = JSC$parser_peek_token (stream);
  while (token == #'<' || token == #'>' || token == JSC$tLE
	 || token == JSC$tGE)
    {
      JSC$parser_get_token (stream);
      var ln = JSC$parser_token_linenum;

      expr2 = JSC$parser_parse_shift_expr (stream);
      if (typeof expr2 == "boolean")
	JSC$parser_syntax_error ();

      expr = new JSC$expr_relational (ln, token, expr, expr2);
      token = JSC$parser_peek_token (stream);
    }

  return expr;
}


function JSC$parser_parse_shift_expr (stream)
{
  var expr, expr2, token;

  if (typeof (expr = JSC$parser_parse_additive_expr (stream))
      == "boolean")
    return false;

  token = JSC$parser_peek_token (stream);
  while (token == JSC$tLSHIFT || token == JSC$tRSHIFT || token == JSC$tRRSHIFT)
    {
      JSC$parser_get_token (stream);
      var ln = JSC$parser_token_linenum;

      expr2 = JSC$parser_parse_additive_expr (stream);

      if (typeof expr2 == "boolean")
	JSC$parser_syntax_error ();

      expr = new JSC$expr_shift (ln, token, expr, expr2);
      token = JSC$parser_peek_token (stream);
    }

  return expr;
}


function JSC$parser_parse_additive_expr (stream)
{
  var expr, expr2, token;

  if (typeof (expr = JSC$parser_parse_multiplicative_expr (stream))
      == "boolean")
    return false;

  token = JSC$parser_peek_token (stream);
  while (token == #'+' || token == #'-')
    {
      JSC$parser_get_token (stream);
      var ln = JSC$parser_token_linenum;

      expr2 = JSC$parser_parse_multiplicative_expr (stream);
      if (typeof expr2 == "boolean")
	JSC$parser_syntax_error ();

      expr = new JSC$expr_additive (ln, token, expr, expr2);
      token = JSC$parser_peek_token (stream);
    }

  return expr;
}


function JSC$parser_parse_multiplicative_expr (stream)
{
  var expr, expr2, token;

  if (typeof (expr = JSC$parser_parse_unary_expr (stream)) == "boolean")
    return false;

  token = JSC$parser_peek_token (stream);
  while (token == #'*' || token == #'/' || token == #'%')
    {
      JSC$parser_get_token (stream);
      var ln = JSC$parser_token_linenum;

      expr2 = JSC$parser_parse_unary_expr (stream);
      if (typeof expr2 == "boolean")
	JSC$parser_syntax_error ();

      expr = new JSC$expr_multiplicative (ln, token, expr, expr2);
      token = JSC$parser_peek_token (stream);
    }

  return expr;
}


function JSC$parser_parse_unary_expr (stream)
{
  var expr, token;

  token = JSC$parser_peek_token (stream);
  if (token == JSC$tDELETE
      || token == JSC$tVOID
      || token == JSC$tTYPEOF
      || token == JSC$tPLUSPLUS
      || token == JSC$tMINUSMINUS
      || token == #'+'
      || token == #'-'
      || token == #'~'
      || token == #'!')
    {
      JSC$parser_get_token (stream);
      var ln = JSC$parser_token_linenum;

      expr = JSC$parser_parse_unary_expr (stream);
      if (typeof expr == "boolean")
	JSC$parser_syntax_error ();

      return new JSC$expr_unary (ln, token, expr);
    }

  return JSC$parser_parse_postfix_expr (stream);
}


function JSC$parser_parse_postfix_expr (stream)
{
  var expr, token;

  if (typeof (expr = JSC$parser_parse_left_hand_side_expr (stream))
      == "boolean")
    return false;

  token = JSC$parser_peek_token (stream);
  if (token == JSC$tPLUSPLUS || token == JSC$tMINUSMINUS)
    {
      if (JSC$parser_peek_token_linenum > JSC$parser_token_linenum)
	{
	  if (JSC$warn_missing_semicolon)
	    JSC$warning (JSC$filename + ":"
			 + JSC$parser_token_linenum.toString ()
			 + ": warning: automatic semicolon insertion cuts the expression before ++ or --");
	}
      else
	{
	  JSC$parser_get_token (stream);
	  var ln = JSC$parser_token_linenum;

	  return new JSC$expr_postfix (ln, token, expr);
	}
    }

  return expr;
}


function JSC$parser_parse_left_hand_side_expr (stream)
{
  var expr, args, token, expr2;

  if (typeof (expr = JSC$parser_parse_member_expr (stream))
      == "boolean")
    return false;

  /* Parse the possible first pair of arguments. */
  if (JSC$parser_peek_token (stream) == #'(')
    {
      var ln = JSC$parser_peek_token_linenum;

      args = JSC$parser_parse_arguments (stream);
      if (typeof args == "boolean")
	JSC$parser_syntax_error ();

      expr = new JSC$expr_call (ln, expr, args);
    }
  else
    return expr;

  /* Parse to possibly following arguments and selectors. */
  while ((token = JSC$parser_peek_token (stream)) == #'('
	 || token == #'[' || token == #'.')
    {
      var ln = JSC$parser_peek_token_linenum;

      if (token == #'(')
	{
	  args = JSC$parser_parse_arguments (stream);
	  expr = new JSC$expr_call (ln, expr, args);
	}
      else if (token == #'[')
	{
	  JSC$parser_get_token (stream);

	  expr2 = JSC$parser_parse_expr (stream);
	  if (typeof expr2 == "boolean")
	    JSC$parser_syntax_error ();

	  if (JSC$parser_get_token (stream) != #']')
	    JSC$parser_syntax_error ();

	  expr = new JSC$expr_object_array (ln, expr, expr2);
	}
      else
	{
	  JSC$parser_get_token (stream);
	  if (JSC$parser_get_token (stream) != JSC$tIDENTIFIER)
	    JSC$parser_syntax_error ();

	  expr = new JSC$expr_object_property (ln, expr,
					       JSC$parser_token_value);
	}
    }

  return expr;
}


function JSC$parser_parse_member_expr (stream)
{
  var expr, args, token, expr2;

  if (typeof (expr = JSC$parser_parse_primary_expr (stream))
      == "boolean")
    {
      token = JSC$parser_peek_token (stream);

      if (token == JSC$tNEW)
	{
	  JSC$parser_get_token (stream);
	  var ln = JSC$parser_token_linenum;

	  expr = JSC$parser_parse_member_expr (stream);
	  if (typeof expr == "boolean")
	    JSC$parser_syntax_error ();

	  if (JSC$parser_peek_token (stream) == #'(')
	    {
	      args = JSC$parser_parse_arguments (stream);
	      if (typeof args == "boolean")
		JSC$parser_syntax_error ();
	    }
	  else
	    return new JSC$expr_new (ln, expr, null);

	  expr = new JSC$expr_new (ln, expr, args);
	}
      else
	return false;
    }

  /* Ok, now we have valid starter. */
  token = JSC$parser_peek_token (stream);
  while (token == #'[' || token == #'.')
    {
      JSC$parser_get_token (stream);
      var ln = JSC$parser_token_linenum;

      if (token == #'[')
	{
	  expr2 = JSC$parser_parse_expr (stream);
	  if (typeof expr2 == "boolean")
	    JSC$parser_syntax_error ();

	  if (JSC$parser_get_token (stream) != #']')
	    JSC$parser_syntax_error ();

	  expr = new JSC$expr_object_array (ln, expr, expr2);
	}
      else
	{
	  if (JSC$parser_get_token (stream) != JSC$tIDENTIFIER)
	    JSC$parser_syntax_error ();

	  expr = new JSC$expr_object_property (ln, expr,
					       JSC$parser_token_value);
	}

      token = JSC$parser_peek_token (stream);
    }

  return expr;
}


function JSC$parser_parse_primary_expr (stream)
{
  var token, val;

  token = JSC$parser_peek_token (stream);
  var ln = JSC$parser_peek_token_linenum;

  if (token == JSC$tTHIS)
    val = new JSC$expr_this (ln);
  else if (token == JSC$tIDENTIFIER)
    val = new JSC$expr_identifier (ln, JSC$parser_peek_token_value);
  else if (token == JSC$tFLOAT)
    val = new JSC$expr_float (ln, JSC$parser_peek_token_value);
  else if (token == JSC$tINTEGER)
    val = new JSC$expr_integer (ln, JSC$parser_peek_token_value);
  else if (token == JSC$tSTRING)
    val = new JSC$expr_string (ln, JSC$parser_peek_token_value);
  else if (token == #'/')
    {
      /*
       * Kludge alert!  The regular expression constants (/.../) and
       * div operands are impossible to distinguish, based only on the
       * lexical analysis.  Therefore, we need some syntactical
       * knowledge when the regular expression constants are possible
       * at all.  This is the place where they can appear.  In all
       * other places, the character `/' is interpreted as a div
       * operator.
       */
      JSC$parser_get_token (stream);

      return new JSC$expr_regexp (ln, JSC$lexer_read_regexp_constant (stream));
    }
  else if (token == JSC$tNULL)
    val = new JSC$expr_null (ln);
  else if (token == JSC$tTRUE)
    val = new JSC$expr_true (ln);
  else if (token == JSC$tFALSE)
    val = new JSC$expr_false (ln);
  else if (token == #'[')
    {
      /* Array initializer. */
      /* TODO: SharpVarDefinition_{opt} */

      JSC$parser_get_token (stream);

      var items = new Array ();
      var pos = 0;

      while ((token = JSC$parser_peek_token (stream)) != #']')
	{
	  if (token == #',')
	    {
	      JSC$parser_get_token (stream);
	      items[++pos] = false;
	      continue;
	    }

	  var expr = JSC$parser_parse_assignment_expr (stream);
	  if (!expr)
	    JSC$parser_syntax_error ();

	  items[pos] = expr;

	  /* Got one expression.  It must be followed by ',' or ']'. */
	  token = JSC$parser_peek_token (stream);
	  if (token != #',' && token != #']')
	    JSC$parser_syntax_error ();
	}

      val = new JSC$expr_array_initializer (ln, items);
    }
  else if (token == #'{')
    {
      /* Object literal. */
      /* TODO: SharpVarDefinition_{opt} */

      JSC$parser_get_token (stream);

      var items = new Array ();

      while ((token = JSC$parser_peek_token (stream)) != #'}')
	{
	  var pair = new Object ();

	  token = JSC$parser_get_token (stream);

	  pair.linenum = JSC$linenum;
	  pair.id_type = token;
	  pair.id = JSC$parser_token_value;

	  if (token != JSC$tIDENTIFIER && token != JSC$tSTRING
	      && token != JSC$tINTEGER)
	    JSC$parser_syntax_error ();

	  if (JSC$parser_get_token (stream) != #':')
	    JSC$parser_syntax_error ();

	  pair.expr = JSC$parser_parse_assignment_expr (stream);
	  if (!pair.expr)
	    JSC$parser_syntax_error ();

	  items.push (pair);

	  /*
	   * Got one property, initializer pair.  It must be followed
	   * by ',' or '}'.
	   */
	  token = JSC$parser_peek_token (stream);
	  if (token == #',')
	    {
	      /* Ok, we have more items. */
	      JSC$parser_get_token (stream);

	      token = JSC$parser_peek_token (stream);
	      if (token != JSC$tIDENTIFIER && token != JSC$tSTRING
		  && token != JSC$tINTEGER)
		JSC$parser_syntax_error ();
	    }
	  else if (token != #'}' && token)
	    JSC$parser_syntax_error ();
	}

      val = new JSC$expr_object_initializer (ln, items);
    }
  else if (token == #'(')
    {
      JSC$parser_get_token (stream);

      val = JSC$parser_parse_expr (stream);
      if (typeof val == "boolean"
	  || JSC$parser_peek_token (stream) != #')')
	JSC$parser_syntax_error ();
    }
  else
    return false;

  JSC$parser_get_token (stream);
  return val;
}


function JSC$parser_parse_arguments (stream)
{
  var args, item;

  if (JSC$parser_peek_token (stream) != #'(')
    return false;

  args = new Array ();

  JSC$parser_get_token (stream);
  while (JSC$parser_peek_token (stream) != #')')
    {
      item = JSC$parser_parse_assignment_expr (stream);
      if (typeof item == "boolean")
	JSC$parser_syntax_error ();
      args.push (item);

      var token = JSC$parser_peek_token (stream);
      if (token == #',')
	JSC$parser_get_token (stream);
      else if (token != #')')
	JSC$parser_syntax_error ();
    }
  JSC$parser_get_token (stream);

  return args;
}


/*
Local variables:
mode: c
End:
*/
/*
 * Grammar components.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsc/compiler.js,v $
 * $Id: compiler.js,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

/* General helpers. */

function JSC$gram_reset ()
{
  JSC$label_count = 1;
  JSC$cont_break = new JSC$ContBreak ();
}


function JSC$alloc_label (num_labels)
{
  JSC$label_count += num_labels;

  return JSC$label_count - num_labels;
}


function JSC$format_label (num)
{
  return ".L" + num.toString ();
}


function JSC$count_locals_from_stmt_list (list)
{
  var i;

  /* First, count how many variables we need at the toplevel. */
  var lcount = 0;
  for (i = 0; i < list.length; i++)
    lcount += list[i].count_locals (false);

  /* Second, count the maximum amount needed by the nested blocks. */
  var rmax = 0;
  for (i = 0; i < list.length; i++)
    {
      var rc = list[i].count_locals (true);
      if (rc > rmax)
	rmax = rc;
    }

  return lcount + rmax;
}

/*
 * The handling of the `continue' and `break' labels for looping
 * constructs.  The variable `JSC$cont_break' holds an instance of
 * JSC$ContBreak class.  The instance contains a valid chain of
 * looping constructs and the currently active with and try testing
 * levels.  The actual `continue', `break', and `return' statements
 * investigate the chain and generate appropriate `with_pop' and
 * `try_pop' operands.
 *
 * If the instance variable `inswitch' is true, the continue statement
 * is inside a switch statement.  In this case, the continue statement
 * must pop one item from the stack.  That item is the value of the
 * case expression.
 */

function JSC$ContBreakFrame (loop_break, loop_continue, inswitch, label, next)
{
  this.loop_break = loop_break;
  this.loop_continue = loop_continue;
  this.inswitch = inswitch;
  this.label = label;
  this.next = next;

  this.with_nesting = 0;
  this.try_nesting = 0;
}


function JSC$ContBreak ()
{
  this.top = new JSC$ContBreakFrame (null, null, false, null);
}

new JSC$ContBreak ();

function JSC$ContBreak$push (loop_break, loop_continue, inswitch, label)
{
  this.top = new JSC$ContBreakFrame (loop_break, loop_continue, inswitch,
				     label, this.top);
}
JSC$ContBreak.prototype.push = JSC$ContBreak$push;

function JSC$ContBreak$pop ()
{
  if (this.top == null)
    error ("jsc: internal error: continue-break stack underflow");

  this.top = this.top.next;
}
JSC$ContBreak.prototype.pop = JSC$ContBreak$pop;

/*
 * Count the currently active `try' nesting that should be removed on
 * `return' statement.
 */
function JSC$ContBreak$try_return_nesting ()
{
  var f;
  var count = 0;

  for (f = this.top; f; f = f.next)
    count += f.try_nesting;

  return count;
}
JSC$ContBreak.prototype.try_return_nesting = JSC$ContBreak$try_return_nesting;

/*
 * Count currently active `with' nesting that should be removed on
 * `continue' or `break' statement.
 */
function JSC$ContBreak$count_with_nesting (label)
{
  var f;
  var count = 0;

  for (f = this.top; f; f = f.next)
    {
      count += f.with_nesting;
      if (label)
	{
	  if (f.label == label)
	    break;
	}
      else
	if (f.loop_continue)
	  break;
    }
  return count;
}
JSC$ContBreak.prototype.count_with_nesting = JSC$ContBreak$count_with_nesting;

/*
 * Count the currently active `try' nesting that should be removed on
 * `continue' or `break' statement.
 */
function JSC$ContBreak$count_try_nesting (label)
{
  var f;
  var count = 0;

  for (f = this.top; f; f = f.next)
    {
      count += f.try_nesting;
      if (label)
	{
	  if (f.label == label)
	    break;
	}
      else
	if (f.loop_continue)
	  break;
    }
  return count;
}
JSC$ContBreak.prototype.count_try_nesting = JSC$ContBreak$count_try_nesting;

function JSC$ContBreak$count_switch_nesting (label)
{
  var f;
  var count = 0;

  for (f = this.top; f; f = f.next)
    {
      if (f.inswitch)
	count++;
      if (label)
	{
	  if (f.label == label)
	    break;
	  }
      else
	if (f.loop_continue)
	  break;
    }
  return count;
}
JSC$ContBreak.prototype.count_switch_nesting
  = JSC$ContBreak$count_switch_nesting;

function JSC$ContBreak$get_continue (label)
{
  var f;

  for (f = this.top; f; f = f.next)
    if (label)
      {
	if (f.label == label)
	  return f.loop_continue;
      }
    else
      if (f.loop_continue)
	return f.loop_continue;

  return null;
}
JSC$ContBreak.prototype.get_continue = JSC$ContBreak$get_continue;

function JSC$ContBreak$get_break (label)
{
  var f;

  for (f = this.top; f; f = f.next)
    if (label)
      {
	if (f.label == label)
	  return f.loop_break;
      }
    else
      if (f.loop_break)
	return f.loop_break;

  return null;
}
JSC$ContBreak.prototype.get_break = JSC$ContBreak$get_break;

function JSC$ContBreak$is_unique_label (label)
{
  var f;

  for (f = this.top; f; f = f.next)
    if (f.label == label)
      return false;

  return true;
}
JSC$ContBreak.prototype.is_unique_label = JSC$ContBreak$is_unique_label;

JSC$cont_break = null;


/* Function declaration. */

function JSC$function_declaration (ln, lbrace_ln, name, name_given, args,
				   block, use_arguments_prop)
{
  this.linenum = ln;
  this.lbrace_linenum = lbrace_ln;
  this.name = name;
  this.name_given = name_given;
  this.args = args;
  this.block = block;
  this.use_arguments_prop = use_arguments_prop;
  this.asm = JSC$function_declaration_asm;
}

function JSC$function_declaration_asm ()
{
  var i, a;

  /* Define arguments. */
  JSC$ns.push_frame ();

  for (i = 0, a = 2; i < this.args.length; i++, a++)
    JSC$ns.define_symbol (this.args[i], JSC$SCOPE_ARG, a, this.linenum);

  /* Define the function name to be a global symbol. */
  new JSC$ASM_symbol (this.linenum, this.name).link ();

  /* Check that function gets the required amount of arguments. */
  new JSC$ASM_load_arg (this.lbrace_linenum, 1).link ();
  new JSC$ASM_add_2_i (this.lbrace_linenum).link ();
  new JSC$ASM_min_args (this.lbrace_linenum, this.args.length + 2).link ();

  /* Count how many local variables we need. */
  var num_locals = JSC$count_locals_from_stmt_list (this.block);

  /* Is the `arguments' property of function instance used? */
  if (this.use_arguments_prop)
    num_locals++;

  if (num_locals > 0)
    {
      new JSC$ASM_locals (this.lbrace_linenum, num_locals).link ();
      if (this.use_arguments_prop)
	{
	  /*
	   * Create an array for the arguments array and store it to
	   * the first local variable.
	   */
	  var ln = this.lbrace_linenum;
	  var locn = JSC$ns.alloc_local ();
	  JSC$ns.define_symbol ("arguments", JSC$SCOPE_LOCAL, locn, ln);

	  new JSC$ASM_const_i0 (ln).link ();
	  new JSC$ASM_load_global (ln, "Array").link ();
	  new JSC$ASM_new (ln).link ();
	  new JSC$ASM_swap (ln).link ();
	  new JSC$ASM_apop (ln, 2).link ();
	  new JSC$ASM_store_local (ln, locn).link ();

	  /* Push individual argumens to the array. */

	  /* Init the loop counter. */
	  new JSC$ASM_const_i0 (ln).link ();

	  var l_loop = new JSC$ASM_label ();
	  var l_out = new JSC$ASM_label ();

	  l_loop.link ();

	  /* Check if we'r done. */
	  new JSC$ASM_dup (ln).link ();
	  new JSC$ASM_load_arg (ln, 1).link ();
	  new JSC$ASM_cmp_ge (ln).link ();
	  new JSC$ASM_iftrue_b (ln, l_out).link ();

	  /* Load the nth argument to the top of the stack. */
	  new JSC$ASM_dup (ln).link ();
	  new JSC$ASM_add_2_i (ln).link ();
	  new JSC$ASM_load_nth_arg (ln).link ();

	  /* Push it to the array. */
	  new JSC$ASM_const_i1 (ln).link ();
	  new JSC$ASM_load_local (ln, locn).link ();
	  new JSC$ASM_call_method (ln, "push").link ();
	  new JSC$ASM_pop_n (ln, 4).link ();

	  /* Increment loop counter and continue. */
	  new JSC$ASM_add_1_i (ln).link ();
	  new JSC$ASM_jmp (ln, l_loop).link ();

	  /* We'r done. */
	  l_out.link ();

	  /* Pop the loop counter. */
	  new JSC$ASM_pop (ln).link ();
	}
    }

  /* Assembler for our body. */
  for (i = 0; i < this.block.length; i++)
    this.block[i].asm ();

  /*
   * Every function must return something.  We could check if all
   * control flows in this function ends to a return, but that would
   * bee too hard...  Just append a return const_undefined.  The optimizer
   * will remove it if it is not needed.
   */
  var ln;
  if (this.block.length > 0)
    ln = this.block[this.block.length - 1].linenum;
  else
    ln = this.linenum;

  new JSC$ASM_const_undefined (ln).link ();
  new JSC$ASM_return (ln).link ();

  /* Pop our namespace. */
  JSC$ns.pop_frame ();
}


function JSC$zero_function ()
{
  return 0;
}


/*
 * Statements.
 */

/* Block. */

function JSC$stmt_block (ln, list)
{
  this.stype = JSC$STMT_BLOCK;
  this.linenum = ln;
  this.stmts = list;
  this.asm = JSC$stmt_block_asm;
  this.count_locals = JSC$stmt_block_count_locals;
}

function JSC$stmt_block_asm ()
{
  JSC$ns.push_frame ();

  /* Assembler for our stmts. */
  var i;
  for (i = 0; i < this.stmts.length; i++)
    this.stmts[i].asm ();

  JSC$ns.pop_frame ();
}


function JSC$stmt_block_count_locals (recursive)
{
  if (!recursive)
    return 0;

  return JSC$count_locals_from_stmt_list (this.stmts);
}

/* Function declaration. */

function JSC$stmt_function_declaration (ln, container_id, function_id,
					given_id)
{
  this.stype = JSC$STMT_FUNCTION_DECLARATION;
  this.linenum = ln;
  this.container_id = container_id;
  this.function_id = function_id;
  this.given_id = given_id;
  this.asm = JSC$stmt_function_declaration_asm;
  this.count_locals = JSC$zero_function;
}

function JSC$stmt_function_declaration_asm ()
{
  new JSC$ASM_load_global (this.linenum, this.function_id).link ();
  new JSC$ASM_load_global (this.linenum, this.container_id).link ();
  new JSC$ASM_store_property (this.linenum, this.given_id).link ();
}

/* Empty */

function JSC$stmt_empty (ln)
{
  this.stype = JSC$STMT_EMPTY;
  this.linenum = ln;
  this.asm = JSC$stmt_empty_asm;
  this.count_locals = JSC$zero_function;
}

function JSC$stmt_empty_asm ()
{
  /* Nothing here. */
}


/* Continue. */

function JSC$stmt_continue (ln, label)
{
  this.stype = JSC$STMT_CONTINUE;
  this.linenum = ln;
  this.label = label;
  this.asm = JSC$stmt_continue_asm;
  this.count_locals = JSC$zero_function;
}

function JSC$stmt_continue_asm ()
{
  var l_cont = JSC$cont_break.get_continue (this.label);

  if (l_cont == null)
    {
      if (this.label)
	error (JSC$filename + ":" + this.linenum.toString ()
	       + ": label `" + this.label
	       + "' not found for continue statement");
      else
	error (JSC$filename + ":" + this.linenum.toString ()
	       + ": continue statement not within a loop");
    }

  var nesting = JSC$cont_break.count_with_nesting (this.label);
  if (nesting > 0)
    new JSC$ASM_with_pop (this.linenum, nesting).link ();

  nesting = JSC$cont_break.count_try_nesting (this.label);
  if (nesting > 0)
    new JSC$ASM_try_pop (this.linenum, nesting).link ();

  nesting = JSC$cont_break.count_switch_nesting (this.label);
  if (nesting > 0)
    {
      /* Pop the value of the switch expression. */
      if (nesting == 1)
	new JSC$ASM_pop (this.linenum).link ();
      else
	new JSC$ASM_pop_n (this.linenum, nesting).link ();
    }

  new JSC$ASM_jmp (this.linenum, l_cont).link ();
}


/* Break. */

function JSC$stmt_break (ln, label)
{
  this.stype = JSC$STMT_BREAK;
  this.linenum = ln;
  this.label = label;
  this.asm = JSC$stmt_break_asm;
  this.count_locals = JSC$zero_function;
}

function JSC$stmt_break_asm ()
{
  var l_break = JSC$cont_break.get_break (this.label);

  if (l_break == null)
    {
      if (this.label)
	error (JSC$filename + ":" + this.linenum.toString ()
	       + ": label `" + this.label
	       + "' not found for break statement");
      else
	error (JSC$filename + ":" + this.linenum.toString()
	       + ": break statement not within a loop or switch");
    }

  var nesting = JSC$cont_break.count_with_nesting (this.label);
  if (nesting > 0)
    new JSC$ASM_with_pop (this.linenum, nesting).link ();

  nesting = JSC$cont_break.count_try_nesting (this.label);
  if (nesting > 0)
    new JSC$ASM_try_pop (this.linenum, nesting).link ();

  /*
   * For non-labeled breaks, the switch nesting is handled in the
   * stmt_switch().  The code after the label, returned by the
   * get_break(), will handle the switch nesting in these cases.
   * For the labeled breaks, we must pop the switch nesting here.
   */
  if (this.label)
    {
      nesting = JSC$cont_break.count_switch_nesting (this.label);
      if (nesting > 0)
	{
	  if (nesting == 1)
	    new JSC$ASM_pop (this.linenum).link ();
	  else
	    new JSC$ASM_pop_n (this.linenum, nesting).link ();
	}
    }

  new JSC$ASM_jmp (this.linenum, l_break).link ();
}


/* Return. */

function JSC$stmt_return (ln, expr)
{
  this.stype = JSC$STMT_RETURN;
  this.linenum = ln;
  this.expr = expr;
  this.asm = JSC$stmt_return_asm;
  this.count_locals = JSC$zero_function;
}

function JSC$stmt_return_asm ()
{
  var nesting = JSC$cont_break.try_return_nesting ();
  if (nesting > 0)
    new JSC$ASM_try_pop (this.linenum, nesting).link ();

  if (this.expr != null)
    this.expr.asm ();
  else
    new JSC$ASM_const_undefined (this.linenum).link ();

  new JSC$ASM_return (this.linenum).link ();
}


/* Switch. */

function JSC$stmt_switch (ln, last_ln, expr, clauses)
{
  this.stype = JSC$STMT_SWITCH;
  this.linenum = ln;
  this.last_linenum = last_ln;
  this.expr = expr;
  this.clauses = clauses;
  this.asm = JSC$stmt_switch_asm;
  this.count_locals = JSC$stmt_switch_count_locals;
}

function JSC$stmt_switch_asm ()
{
  /* Evaluate the switch expression to the top of the stack. */
  this.expr.asm ();

  /* The switch statement define a break label. */
  var l_break = new JSC$ASM_label ();
  JSC$cont_break.push (l_break, null, true, null);

  /* For each clause (except the first), insert check and body labels. */
  var i;
  for (i = 1; i < this.clauses.length; i++)
    {
      this.clauses[i].l_check = new JSC$ASM_label ();
      this.clauses[i].l_body = new JSC$ASM_label ();
    }

  /* Generate code for each clause. */
  for (i = 0; i < this.clauses.length; i++)
    {
      /* Is this the last clause? */
      var last = i + 1 >= this.clauses.length;
      var c = this.clauses[i];

      var next_check, next_body;
      if (last)
	next_check = next_body = l_break;
      else
	{
	  next_check = this.clauses[i + 1].l_check;
	  next_body = this.clauses[i + 1].l_body;
	}

      if (c.expr)
	{
	  /*
	   * Must check if this clause matches the expression.  If c.expr
	   * is null, this is the default clause that matches always.
	   */

	  if (i > 0)
	    c.l_check.link ();

	  new JSC$ASM_dup (c.linenum).link ();
	  c.expr.asm ();
	  new JSC$ASM_cmp_eq (c.linenum).link ();
	  new JSC$ASM_iffalse_b (c.linenum, next_check).link ();
	}
      else
	{
	  if (i > 0)
	    /* The check label for the default case. */
	    c.l_check.link ();
	}

      /* Generate assembler for the body. */
      if (i > 0)
	c.l_body.link ();

      var j;
      for (j = 0; j < c.length; j++)
	c[j].asm ();

      /* And finally, jump to the next body. (this is the fallthrough case). */
      new JSC$ASM_jmp (c.last_linenum, next_body).link ();
    }

  JSC$cont_break.pop ();

  /* The break label. */
  l_break.link ();

  /* Pop the value of the switch expression. */
  new JSC$ASM_pop (this.last_linenum).link ();
}

function JSC$stmt_switch_count_locals (recursive)
{
  var locals = 0;
  var i, j;

  if (recursive)
    {
      /* For the recursive cases, we need the maximum of our clause stmts. */
      for (i = 0; i < this.clauses.length; i++)
	{
	  var c = this.clauses[i];
	  for (j = 0; j < c.length; j++)
	    {
	      var l = c[j].count_locals (true);
	      if (l > locals)
		locals = l;
	    }
	}
    }
  else
    {
      /*
       * The case clauses are not blocks.  Therefore, we need the amount,
       * needed by the clauses at the top-level.
       */

      for (i = 0; i < this.clauses.length; i++)
	{
	  var c = this.clauses[i];
	  for (j = 0; j < c.length; j++)
	    locals += c[j].count_locals (false);
	}
    }

  return locals;
}


/* With. */

function JSC$stmt_with (ln, expr, stmt)
{
  this.stype = JSC$STMT_WITH;
  this.linenum = ln;
  this.expr = expr;
  this.stmt = stmt;
  this.asm = JSC$stmt_with_asm;
  this.count_locals = JSC$stmt_with_count_locals;
}

function JSC$stmt_with_asm ()
{
  this.expr.asm ();

  new JSC$ASM_with_push (this.linenum).link ();
  JSC$cont_break.top.with_nesting++;

  this.stmt.asm ();

  JSC$cont_break.top.with_nesting--;
  new JSC$ASM_with_pop (this.linenum, 1).link ();
}

function JSC$stmt_with_count_locals (recursive)
{
  if (!recursive)
    {
      if (this.stmt.stype == JSC$STMT_VARIABLE)
	return this.stmt.list.length;

      return 0;
    }
  else
    return this.stmt.count_locals (true);
}


/* Try. */

function JSC$stmt_try (ln, try_block_last_ln, try_last_ln, block, catch_list,
		       fin)
{
  this.stype = JSC$STMT_TRY;
  this.linenum = ln;
  this.try_block_last_linenum = try_block_last_ln;
  this.try_last_linenum = try_last_ln;
  this.block = block;
  this.catch_list = catch_list;
  this.fin = fin;
  this.asm = JSC$stmt_try_asm;
  this.count_locals = JSC$stmt_try_count_locals;
}

function JSC$stmt_try_asm ()
{
  var l_finally = new JSC$ASM_label ();

  /* Protect and execute the try-block. */

  var l_try_error = new JSC$ASM_label ();
  new JSC$ASM_try_push (this.linenum, l_try_error).link ();
  JSC$cont_break.top.try_nesting++;

  this.block.asm ();

  JSC$cont_break.top.try_nesting--;
  new JSC$ASM_try_pop (this.try_block_last_linenum, 1).link ();

  /*
   * All ok so far.  Push a `false' to indicate no error and jump to
   * the finally block (or out if we have no finally block).
   */
  new JSC$ASM_const_false (this.try_block_last_linenum).link ();
  new JSC$ASM_jmp (this.try_block_last_linenum, l_finally).link ();

  /*
   * Handle try block failures.  The thrown value is on the top of the
   * stack.
   */

  l_try_error.link ();

  if (this.catch_list)
    {
      /*
       * We keep one boolean variable below the thrown value.  Its default
       * value is false.  When one of our catch blocks are entered, it is
       * set to true to indicate that we shouldn't throw the error
       * anymore.
       */
      new JSC$ASM_const_false (this.catch_list.linenum).link ();
      new JSC$ASM_swap (this.catch_list.linenum).link ();

      /* Protect and execute the catch list. */

      var l_catch_list_error = new JSC$ASM_label ();
      new JSC$ASM_try_push (this.catch_list.linenum,
			    l_catch_list_error).link ();
      JSC$cont_break.top.try_nesting++;

      /* Insert start and body labels for each catch list item. */
      var i;
      for (i = 0; i < this.catch_list.length; i++)
	{
	  this.catch_list[i].l_start = new JSC$ASM_label ();
	  this.catch_list[i].l_body = new JSC$ASM_label ();
	}

      /* A label for the catch list end. */
      var l_catch_list_end = new JSC$ASM_label ();

      /* Process the individual catches. */
      for (i = 0; i < this.catch_list.length; i++)
	{
	  var c = this.catch_list[i];

	  /* This is the starting point of this catch frame. */
	  c.l_start.link ();

	  /*
	   * Create a new namespace frame and bind the catch's
	   * identifier to the thrown exception.
	   */

	  JSC$ns.push_frame ();
	  JSC$ns.define_symbol (c.id, JSC$SCOPE_LOCAL, JSC$ns.alloc_local (),
				c.linenum);

	  new JSC$ASM_dup (c.linenum).link ();
	  new JSC$ASM_store_local (c.linenum,
				   JSC$ns.lookup_symbol (c.id).value).link ();

	  /* Check the possible guard.  We must protect its calculation. */
	  if (c.guard)
	    {
	      var l_guard_error = new JSC$ASM_label ();
	      new JSC$ASM_try_push (c.linenum, l_guard_error).link ();
	      JSC$cont_break.top.try_nesting++;

	      /* Calculate the guard. */
	      c.guard.asm ();

	      JSC$cont_break.top.try_nesting--;
	      new JSC$ASM_try_pop (c.linenum, 1).link ();

	      /*
	       * Wow!  We managed to do it.  Now, let's check if we
	       * accept this catch case.
	       */

	      var next;
	      if (i + 1 >= this.catch_list.length)
		next = l_catch_list_end;
	      else
		next = this.catch_list[i + 1].l_start;

	      if (c.guard.lang_type == JSC$JS_BOOLEAN)
		new JSC$ASM_iffalse_b (c.linenum, next).link ();
	      else
		new JSC$ASM_iffalse (c.linenum, next).link ();

	      /* Yes, we do accept it.  Just jump to do the stuffs. */
	      new JSC$ASM_jmp (c.linenum, c.l_body).link ();

	      /*
	       * The evaluation of the guard failed.  Do the cleanup
	       * and jump to the next case.
	       */

	      l_guard_error.link ();

	      /* Pop the exception. */
	      new JSC$ASM_pop (c.linenum).link ();

	      /* Check the next case. */
	      new JSC$ASM_jmp (c.linenum, next).link ();
	    }

	  /*
	   * We did enter the catch body.  Let's update our boolean
	   * status variable to reflect this fact.
	   */
	  c.l_body.link ();

	  new JSC$ASM_swap (c.linenum).link ();
	  new JSC$ASM_pop (c.linenum).link ();
	  new JSC$ASM_const_true (c.linenum).link ();
	  new JSC$ASM_swap (c.linenum).link ();

	  /* Code for the catch body. */
	  c.stmt.asm ();

	  /* We'r done with the namespace frame. */
	  JSC$ns.pop_frame ();

	  /*
	   * The next catch tag, or the l_catch_list_end follows us,
	   * so we don't need a jumps here.
	   */
	}

      /*
       * The catch list was evaluated without errors.
       */

      l_catch_list_end.link ();
      JSC$cont_break.top.try_nesting--;
      new JSC$ASM_try_pop (this.catch_list.last_linenum, 1).link ();

      /* Did we enter any of our catch lists? */

      var l_we_did_enter = new JSC$ASM_label ();
      new JSC$ASM_swap (this.catch_list.last_linenum).link ();
      new JSC$ASM_iftrue_b (this.catch_list.last_linenum,
			    l_we_did_enter).link ();

      /* No we didn't. */

      /*
       * Push `true' to indicate an exception and jump to the finally
       * block.  The exception is now on the top of the stack.
       */
      new JSC$ASM_const_true (this.catch_list.last_linenum).link ();
      new JSC$ASM_jmp (this.catch_list.last_linenum, l_finally).link ();

      /* Yes, we did enter one (or many) of our catch lists. */

      l_we_did_enter.link ();

      /* Pop the try-block's exception */
      new JSC$ASM_pop (this.catch_list.last_linenum).link ();

      /*
       * Push a `false' to indicate "no errors" and jump to the
       * finally block.
       */
      new JSC$ASM_const_false (this.catch_list.last_linenum).link ();
      new JSC$ASM_jmp (this.catch_list.last_linenum, l_finally).link ();


      /*
       * Handle catch list failures.  The thrown value is on the top of the
       * stack.
       */

      l_catch_list_error.link ();

      /*
       * Pop the try-block's exception and our boolean `did we enter a
       * catch block' variable.  They are below our new exception.
       */
      new JSC$ASM_apop (this.catch_list.last_linenum, 2).link ();

      /*
       * Push `true' to indicate an exception.  We will fallthrough to
       * the finally part, so no jump is needed here.
       */
      new JSC$ASM_const_true (this.catch_list.last_linenum).link ();
    }
  else
    {
      /* No catch list. */
      new JSC$ASM_const_true (this.try_block_last_linenum).link ();
    }

  /* The possible finally block. */

  l_finally.link ();

  if (this.fin)
    /* Execute it without protection. */
    this.fin.asm ();

  /* We'r almost there.  Let's see if we have to raise a new exception. */

  var l_out = new JSC$ASM_label ();
  new JSC$ASM_iffalse_b (this.try_last_linenum, l_out).link ();

  /* Do raise it. */
  new JSC$ASM_throw (this.try_last_linenum).link ();

  /* The possible exception is handled.  Please, continue. */
  l_out.link ();
}

function JSC$stmt_try_count_locals (recursive)
{
  var count = 0;
  var c;

  if (recursive)
    {
      c = this.block.count_locals (true);
      if (c > count)
	count = c;

      if (this.catch_list)
	{
	  var i;
	  for (i = 0; i < this.catch_list.length; i++)
	    {
	      c = this.catch_list[i].stmt.count_locals (true);
	      if (c > count)
		count = c;
	    }
	}
      if (this.fin)
	{
	  c = this.fin.count_locals (true);
	  if (c > count)
	    count = c;
	}
    }
  else
    {
      if (this.block.stype == JSC$STMT_VARIABLE)
	count += this.block.list.length;

      if (this.catch_list)
	{
	  /* One for the call variable. */
	  count++;

	  var i;
	  for (i = 0; i < this.catch_list.length; i++)
	    if (this.catch_list[i].stmt.stype == JSC$STMT_VARIABLE)
	      count += this.catch_list[i].stmt.list.length;
	}

      if (this.fin)
	if (this.fin.stype == JSC$STMT_VARIABLE)
	  count += this.fin.list.length;
    }

  return count;
}


/* Throw. */

function JSC$stmt_throw (ln, expr)
{
  this.stype = JSC$STMT_THROW;
  this.linenum = ln;
  this.expr = expr;
  this.asm = JSC$stmt_throw_asm;
  this.count_locals = JSC$zero_function;
}

function JSC$stmt_throw_asm ()
{
  this.expr.asm ();
  new JSC$ASM_throw (this.linenum).link ();
}


/* Labeled statement. */
function JSC$stmt_labeled_stmt (ln, id, stmt)
{
  this.stype = JSC$STMT_LABELED_STMT;
  this.linenum = ln;
  this.id = id;
  this.stmt = stmt;
  this.asm = JSC$stmt_labeled_stmt_asm;
  this.count_locals = JSC$stmt_labeled_stmt_count_locals;
}

function JSC$stmt_labeled_stmt_asm ()
{
  var l_continue = new JSC$ASM_label ();
  var l_break = new JSC$ASM_label ();

  /*
   * It is an error if we already have a labeled statement with the
   * same id.
   */
  if (!JSC$cont_break.is_unique_label (this.id))
    error (JSC$filename + ":" + this.linenum.toString ()
	   + ": labeled statement is enclosed by another labeled statement "
	   + "with the same label");

  /* Push the break and continue labels. */
  JSC$cont_break.push (l_break, l_continue, false, this.id);

  /* Dump the assembler. */
  l_continue.link ();
  this.stmt.asm ();
  l_break.link ();

  /* And we'r done with out label scope. */
  JSC$cont_break.pop ();
}

function JSC$stmt_labeled_stmt_count_locals (recursive)
{
  return this.stmt.count_locals (recursive);
}


/* Expression. */

function JSC$stmt_expr (expr)
{
  this.stype = JSC$STMT_EXPR;
  this.linenum = expr.linenum;
  this.expr = expr;
  this.asm = JSC$stmt_expr_asm;
  this.count_locals = JSC$zero_function;
}

function JSC$stmt_expr_asm ()
{
  this.expr.asm ();
  new JSC$ASM_pop (this.linenum).link ();
}


/* If. */

function JSC$stmt_if (ln, expr, stmt1, stmt2)
{
  this.stype = JSC$STMT_IF;
  this.linenum = ln;
  this.expr = expr;
  this.stmt1 = stmt1;
  this.stmt2 = stmt2;
  this.asm = JSC$stmt_if_asm;
  this.count_locals = JSC$stmt_if_count_locals;
}

function JSC$stmt_if_asm ()
{
  this.expr.asm ();

  var l1 = new JSC$ASM_label ();
  var l2 = new JSC$ASM_label ();

  if (JSC$optimize_type && this.expr.lang_type
      && this.expr.lang_type == JSC$JS_BOOLEAN)
    new JSC$ASM_iffalse_b (this.linenum, l1).link ();
  else
    new JSC$ASM_iffalse (this.linenum, l1).link ();

  /* Code for the then branch. */
  this.stmt1.asm ();
  new JSC$ASM_jmp (this.linenum, l2).link ();

  /* Code for the else branch. */
  l1.link ();
  if (this.stmt2 != null)
    this.stmt2.asm ();

  /* Done label. */
  l2.link ();
}


function JSC$stmt_if_count_locals (recursive)
{
  var lcount;

  if (!recursive)
    {
      lcount = 0;
      if (this.stmt1.stype == JSC$STMT_VARIABLE)
	lcount += this.stmt1.list.length;

      if (this.stmt2 != null && this.stmt2.stype == JSC$STMT_VARIABLE)
	lcount += this.stmt2.list.length;
    }
  else
    {
      lcount = this.stmt1.count_locals (true);

      if (this.stmt2)
	{
	  var c = this.stmt2.count_locals (true);
	  if (c > lcount)
	    lcount = c;
	}
    }

  return lcount;
}


/* Do...While. */

function JSC$stmt_do_while (ln, expr, stmt)
{
  this.stype = JSC$STMT_DO_WHILE;
  this.linenum = ln;
  this.expr = expr;
  this.stmt = stmt;
  this.asm = JSC$stmt_do_while_asm;
  this.count_locals = JSC$stmt_do_while_count_locals;
}

function JSC$stmt_do_while_asm ()
{
  var l1 = new JSC$ASM_label ();
  var l2 = new JSC$ASM_label ();
  var l3 = new JSC$ASM_label ();

  /* Loop label. */
  l1.link ();

  /* Body. */
  JSC$cont_break.push (l3, l2, false, null);
  this.stmt.asm ();
  JSC$cont_break.pop ();

  /* Condition & continue. */
  l2.link ();
  this.expr.asm ();
  if (JSC$optimize_type && this.expr.lang_type
      && this.expr.lang_type == JSC$JS_BOOLEAN)
    new JSC$ASM_iftrue_b (this.linenum, l1).link ();
  else
    new JSC$ASM_iftrue (this.linenum, l1).link ();

  /* Break label. */
  l3.link ();
}

function JSC$stmt_do_while_count_locals (recursive)
{
  if (!recursive)
    {
      if (this.stmt.stype == JSC$STMT_VARIABLE)
	return this.stmt.list.length;

      return 0;
    }
  else
    return this.stmt.count_locals (true);
}

/* While. */

function JSC$stmt_while (ln, expr, stmt)
{
  this.stype = JSC$STMT_WHILE;
  this.linenum = ln;
  this.expr = expr;
  this.stmt = stmt;
  this.asm = JSC$stmt_while_asm;
  this.count_locals = JSC$stmt_while_count_locals;
}

function JSC$stmt_while_asm ()
{
  var l1 = new JSC$ASM_label ();
  var l2 = new JSC$ASM_label ();

  /* Loop label. */
  l1.link ();

  /* Condition. */
  this.expr.asm ();
  if (JSC$optimize_type && this.expr.lang_type
      && this.expr.lang_type == JSC$JS_BOOLEAN)
    new JSC$ASM_iffalse_b (this.linenum, l2).link ();
  else
    new JSC$ASM_iffalse (this.linenum, l2).link ();

  /* Body. */
  JSC$cont_break.push (l2, l1, false, null);
  this.stmt.asm ();
  JSC$cont_break.pop ();

  /* Goto loop. */
  new JSC$ASM_jmp (this.linenum, l1).link ();

  /* Break label. */
  l2.link ();
}


function JSC$stmt_while_count_locals (recursive)
{
  if (!recursive)
    {
      if (this.stmt.stype == JSC$STMT_VARIABLE)
	return this.stmt.list.length;

      return 0;
    }
  else
    return this.stmt.count_locals (true);
}


/* For. */

function JSC$stmt_for (ln, vars, e1, e2, e3, stmt)
{
  this.stype = JSC$STMT_FOR;
  this.linenum = ln;
  this.vars = vars;
  this.expr1 = e1;
  this.expr2 = e2;
  this.expr3 = e3;
  this.stmt = stmt;
  this.asm = JSC$stmt_for_asm;
  this.count_locals = JSC$stmt_for_count_locals;
}

function JSC$stmt_for_asm ()
{
  /* Code for the init. */
  if (this.vars)
    {
      /* We have our own variable scope. */
      JSC$ns.push_frame ();

      var i;
      for (i = 0; i < this.vars.length; i++)
	{
	  var decl = this.vars[i];

	  JSC$ns.define_symbol (decl.id, JSC$SCOPE_LOCAL,
				JSC$ns.alloc_local (), this.linenum);

	  /* Possible init. */
	  if (decl.expr)
	    {
	      decl.expr.asm ();

	      var r = JSC$ns.lookup_symbol (decl.id);
	      if (r == null || r.scope != JSC$SCOPE_LOCAL)
		error (JSC$filename + ":" + this.liennum.toString ()
		       + ": internal compiler error in local variable "
		       + "declaration in for statement");

	      new JSC$ASM_store_local (this.linenum, r.value).link ();
	    }
	}
    }
  else if (this.expr1 != null)
    {
      this.expr1.asm ();
      new JSC$ASM_pop (this.linenum).link ();
    }

  var l1 = new JSC$ASM_label ();
  var l2 = new JSC$ASM_label ();
  var l3 = new JSC$ASM_label ();

  /* Loop label. */
  l1.link ();

  /* Condition. */
  var type_op = false;
  if (this.expr2 != null)
    {
      this.expr2.asm ();
      if (JSC$optimize_type && this.expr2.lang_type
	  && this.expr2.lang_type == JSC$JS_BOOLEAN)
	type_op = true;
    }
  else
    {
      new JSC$ASM_const_true (this.linenum).link ();
      type_op = JSC$optimize_type;
    }
  if (type_op)
    new JSC$ASM_iffalse_b (this.linenum, l3).link ();
  else
    new JSC$ASM_iffalse (this.linenum, l3).link ();

  JSC$cont_break.push (l3, l2, false, null);
  /* Body. */
  this.stmt.asm ();
  JSC$cont_break.pop ();

  /* Continue label. */
  l2.link ();

  /* Increment. */
  if (this.expr3 != null)
    {
      this.expr3.asm ();
      new JSC$ASM_pop (this.linenum).link ();
    }

  /* Goto loop. */
  new JSC$ASM_jmp (this.linenum, l1).link ();

  /* Break label. */
  l3.link ();

  if (this.vars)
    /* Pop the local variable scope. */
    JSC$ns.pop_frame ();
}


function JSC$stmt_for_count_locals (recursive)
{
  var count = 0;

  if (recursive)
    {
      if (this.vars)
	count += this.vars.length;

      count += this.stmt.count_locals (true);
    }
  else
    {
      if (this.stmt.stype == JSC$STMT_VARIABLE)
	count += this.stmt.list.length;
    }

  return count;
}


/* For...in. */

function JSC$stmt_for_in (ln, vars, e1, e2, stmt)
{
  this.stype = JSC$STMT_FOR_IN;
  this.linenum = ln;
  this.vars = vars;
  this.expr1 = e1;
  this.expr2 = e2;
  this.stmt = stmt;
  this.asm = JSC$stmt_for_in_asm;
  this.count_locals = JSC$stmt_for_in_count_locals;
}

function JSC$stmt_for_in_asm ()
{
  var local_id;

  if (this.vars)
    {
      /* We need our own variable scope here. */
      JSC$ns.push_frame ();

      var decl = this.vars[0];
      local_id = JSC$ns.alloc_local ();
      JSC$ns.define_symbol (decl.id, JSC$SCOPE_LOCAL, local_id, this.linenum);

      /* Possible init. */
      if (decl.expr)
	{
	  decl.expr.asm ();
	  new JSC$ASM_store_local (this.linenum, local_id).link ();
	}
    }

  /* Init the world. */
  this.expr2.asm ();
  new JSC$ASM_dup (this.linenum).link ();
  new JSC$ASM_const_i0 (this.linenum).link ();
  new JSC$ASM_swap (this.linenum).link ();
  new JSC$ASM_const_i0 (this.linenum).link ();

  var l_loop = new JSC$ASM_label ();
  var l_cont = new JSC$ASM_label ();
  var l_iffalse_b = new JSC$ASM_label ();
  var l_break = new JSC$ASM_label ();

  /* Loop label. */
  l_loop.link ();

  /* Fetch nth. */
  new JSC$ASM_nth (this.linenum).link ();
  new JSC$ASM_iffalse_b (this.linenum, l_iffalse_b).link ();

  /* Store value to variable. */
  if (this.vars)
    new JSC$ASM_store_local (this.linenum, local_id).link ();
  else
    JSC$asm_expr_lvalue_store_asm (this.expr1);

  /* Body. */
  JSC$cont_break.push (l_break, l_cont, false, null);
  this.stmt.asm ();
  JSC$cont_break.pop ();

  /* Continue label. */
  l_cont.link ();

  /* Increment. */
  new JSC$ASM_const_i1 (this.linenum).link ();
  new JSC$ASM_add (this.linenum).link ();
  new JSC$ASM_dup (this.linenum).link ();
  new JSC$ASM_roll (this.linenum, -3).link ();
  new JSC$ASM_dup (this.linenum).link ();
  new JSC$ASM_roll (this.linenum, 4).link ();
  new JSC$ASM_swap (this.linenum).link ();

  /* Goto loop. */
  new JSC$ASM_jmp (this.linenum, l_loop).link ();

  /* Out label. */
  l_iffalse_b.link ();

  new JSC$ASM_pop (this.linenum).link ();

  /* Break label. */
  l_break.link ();
  new JSC$ASM_pop_n (this.linenum, 2).link ();

  if (this.vars)
    /* Pop the variable scope. */
    JSC$ns.pop_frame ();
}

function JSC$stmt_for_in_count_locals (recursive)
{
  var count = 0;

  if (recursive)
    {
      if (this.vars)
	count++;

      count += this.stmt.count_locals (true);
    }
  else
    {
      if (this.stmt.stype == JSC$STMT_VARIABLE)
	count += this.stmt.list.length;

    }

  return count;
}


/* Variable. */

function JSC$stmt_variable (ln, list)
{
  this.stype = JSC$STMT_VARIABLE;
  this.linenum = ln;
  this.global_level = false;
  this.list = list;
  this.asm = JSC$stmt_variable_asm;
  this.count_locals = JSC$stmt_variable_count_locals;
}

function JSC$stmt_variable_asm ()
{
  var j, r;

  /* Define all local variables to our namespace. */
  for (j = 0; j < this.list.length; j++)
    {
      var i = this.list[j];

      if (!this.global_level)
	JSC$ns.define_symbol (i.id, JSC$SCOPE_LOCAL,
			      JSC$ns.alloc_local (), this.linenum);
      if (i.expr)
	{
	  i.expr.asm ();

	  if (this.global_level)
	    new JSC$ASM_store_global (this.linenum, i.id).link ();
	  else
	    {
	      r = JSC$ns.lookup_symbol (i.id);
	      if (r == null || r.scope != JSC$SCOPE_LOCAL)
		error (JSC$filename + ":" + this.linenum.toString()
		       + ": internal compiler error in local variable declaration");

	      new JSC$ASM_store_local (this.linenum, r.value).link ();
	    }
	}
    }
}

function JSC$stmt_variable_count_locals (recursive)
{
  if (!recursive)
    {
      if (this.global_level)
	/* We define these as global variables. */
	return 0;

      return this.list.length;
    }

  return 0;
}

function JSC$var_declaration (id, expr)
{
  this.id = id;
  this.expr = expr;
}


/*
 * Expressions.
 */

/* This. */

function JSC$expr_this (ln)
{
  this.etype = JSC$EXPR_THIS;
  this.linenum = ln;
  this.asm = JSC$expr_this_asm;
}

function JSC$expr_this_asm ()
{
  new JSC$ASM_load_arg (this.linenum, 0).link ();
}

/* Identifier. */

function JSC$expr_identifier (ln, value)
{
  this.etype = JSC$EXPR_IDENTIFIER;
  this.linenum = ln;
  this.value = value;
  this.asm = JSC$expr_identifier_asm;
}

function JSC$expr_identifier_asm ()
{
  JSC$asm_expr_lvalue_load_asm (this);
}

/* Float. */

function JSC$expr_float (ln, value)
{
  this.etype = JSC$EXPR_FLOAT;
  this.lang_type = JSC$JS_FLOAT;
  this.linenum = ln;
  this.value = value;
  this.asm = JSC$expr_float_asm;
}

function JSC$expr_float_asm ()
{
  new JSC$ASM_const (this.linenum, this.value).link ();
}

/* Integer. */

function JSC$expr_integer (ln, value)
{
  this.etype = JSC$EXPR_INTEGER;
  this.lang_type = JSC$JS_INTEGER;
  this.linenum = ln;
  this.value = value;
  this.asm = JSC$expr_integer_asm;
}

function JSC$expr_integer_asm ()
{
  if (this.value == 0)
    new JSC$ASM_const_i0 (this.linenum).link ();
  else if (this.value == 1)
    new JSC$ASM_const_i1 (this.linenum).link ();
  else if (this.value == 2)
    new JSC$ASM_const_i2 (this.linenum).link ();
  else if (this.value == 3)
    new JSC$ASM_const_i3 (this.linenum).link ();
  else
    new JSC$ASM_const_i (this.linenum, this.value).link ();
}

/* String. */

function JSC$expr_string (ln, value)
{
  this.etype = JSC$EXPR_STRING;
  this.lang_type = JSC$JS_STRING;
  this.linenum = ln;
  this.value = value;
  this.asm = JSC$expr_string_asm;
}

function JSC$expr_string_asm ()
{
  new JSC$ASM_const (this.linenum, this.value).link ();
}

/* Regexp. */

function JSC$expr_regexp (ln, value)
{
  this.etype = JSC$EXPR_REGEXP;
  this.lang_type = JSC$JS_BUILTIN;
  this.linenum = ln;
  this.value = value;
  this.asm = JSC$expr_regexp_asm;
}

function JSC$expr_regexp_asm ()
{
  new JSC$ASM_const (this.linenum, this.value).link ();
}

/* Array initializer. */

function JSC$expr_array_initializer (ln, items)
{
  this.etype = JSC$EXPR_ARRAY_INITIALIZER;
  this.lang_type = JSC$JS_ARRAY;
  this.linenum = ln;
  this.items = items;
  this.asm = JSC$expr_array_initializer_asm;
}

function JSC$expr_array_initializer_asm ()
{
  /* Generate assembler for the individual items. */

  var i;
  for (i = this.items.length - 1; i >= 0; i--)
    {
      if (this.items[i])
	this.items[i].asm ();
      else
	new JSC$ASM_const_undefined (this.linenum).link ();
    }

  /*
   * The number of items as a negative integer.  The Array object's
   * constructor knows that if the number of arguments is negative, it
   * is called from the array initializer.  Why?  Because the code:
   *
   *   new Array (5);
   *
   * creates an array of length of 5, but code:
   *
   *   [5]
   *
   * creates an array with one item: integer number five.  These cases
   * must be separatable from the code and that's why the argument
   * counts for the array initializers are negative.
   */
  new JSC$ASM_const (this.linenum, -this.items.length).link ();

  /* Call the constructor. */
  new JSC$ASM_load_global (this.linenum, "Array").link ();
  new JSC$ASM_new (this.linenum).link ();
  new JSC$ASM_swap (this.linenum).link ();
  new JSC$ASM_apop (this.linenum, this.items.length + 2).link ();
}

/* Object initializer. */

function JSC$expr_object_initializer (ln, items)
{
  this.etype = JSC$EXPR_OBJECT_INITIALIZER;
  this.lang_type = JSC$JS_OBJECT;
  this.linenum = ln;
  this.items = items;
  this.asm = JSC$expr_object_initializer_asm;
}

function JSC$expr_object_initializer_asm ()
{
  /* Create a new object. */
  new JSC$ASM_const_i0 (this.linenum).link ();
  new JSC$ASM_load_global (this.linenum, "Object").link ();
  new JSC$ASM_new (this.linenum).link ();
  new JSC$ASM_swap (this.linenum).link ();
  new JSC$ASM_apop (this.linenum, 2).link ();

  /* Insert the items. */
  for (var i = 0; i < this.items.length; i++)
    {
      var item = this.items[i];

      new JSC$ASM_dup (item.linenum).link ();
      item.expr.asm ();
      new JSC$ASM_swap (item.linenum).link ();

      switch (item.id_type)
	{
	case JSC$tIDENTIFIER:
	  new JSC$ASM_store_property (item.linenum, item.id).link ();
	  break;

	case JSC$tSTRING:
	  new JSC$ASM_const (item.linenum, item.id).link ();
	  new JSC$ASM_store_array (item.linenum).link ();
	  break;

	case JSC$tINTEGER:
	  switch (item.id)
	    {
	    case 0:
	      new JSC$ASM_const_i0 (item.linenum).link ();
	      break;

	    case 1:
	      new JSC$ASM_const_i1 (item.linenum).link ();
	      break;

	    case 2:
	      new JSC$ASM_const_i2 (item.linenum).link ();
	      break;

	    case 3:
	      new JSC$ASM_const_i3 (item.linenum).link ();
	      break;

	    default:
	      new JSC$ASM_const_i (item.linenum, item.id).link ();
	      break;
	    }
	  new JSC$ASM_store_array (item.linenum).link ();
	  break;
	}
    }
}


/* Null. */

function JSC$expr_null (ln)
{
  this.etype = JSC$EXPR_NULL;
  this.lang_type = JSC$JS_NULL;
  this.linenum = ln;
  this.asm = JSC$expr_null_asm;
}

function JSC$expr_null_asm ()
{
  new JSC$ASM_const_null (this.linenum).link ();
}

/* True. */

function JSC$expr_true (ln)
{
  this.etype = JSC$EXPR_TRUE;
  this.lang_type = JSC$JS_BOOLEAN;
  this.linenum = ln;
  this.asm = JSC$expr_true_asm;
}

function JSC$expr_true_asm ()
{
  new JSC$ASM_const_true (this.linenum).link ();
}

/* False. */

function JSC$expr_false (ln)
{
  this.etype = JSC$EXPR_FALSE;
  this.lang_type = JSC$JS_BOOLEAN;
  this.linenum = ln;
  this.asm = JSC$expr_false_asm;
}

function JSC$expr_false_asm ()
{
  new JSC$ASM_const_false (this.linenum).link ();
}


/* Multiplicative expr. */

function JSC$expr_multiplicative (ln, type, e1, e2)
{
  this.etype = JSC$EXPR_MULTIPLICATIVE;
  this.linenum = ln;
  this.type = type;
  this.e1 = e1;
  this.e2 = e2;
  this.asm = JSC$expr_multiplicative_asm;
}

function JSC$expr_multiplicative_asm ()
{
  this.e1.asm ();
  this.e2.asm ();
  if (this.type == #'*')
    new JSC$ASM_mul (this.linenum).link ();
  else if (this.type == #'/')
    new JSC$ASM_div (this.linenum).link ();
  else
    new JSC$ASM_mod (this.linenum).link ();
}


/* Additive expr. */

function JSC$expr_additive (ln, type, e1, e2)
{
  this.etype = JSC$EXPR_ADDITIVE;
  this.linenum = ln;
  this.type = type;
  this.e1 = e1;
  this.e2 = e2;
  this.asm = JSC$expr_additive_asm;
  this.constant_folding = JSC$expr_additive_constant_folding;
}

function JSC$expr_additive_asm ()
{
  this.e1.asm ();
  this.e2.asm ();
  if (this.type == #'+')
    new JSC$ASM_add (this.linenum).link ();
  else
    new JSC$ASM_sub (this.linenum).link ();
}

function JSC$expr_additive_constant_folding ()
{
  if (this.e1.constant_folding)
    this.e1 = this.e1.constant_folding ();
  if (this.e2.constant_folding)
    this.e2 = this.e2.constant_folding ();

  /* This could be smarter. */
  if (this.e1.lang_type && this.e2.lang_type
      && this.e1.lang_type == this.e2.lang_type)
    {
      switch (this.e1.lang_type)
	{
	case JSC$JS_INTEGER:
	  return new JSC$expr_integer (this.linenum,
				       this.type == #'+'
				       ? this.e1.value + this.e2.value
				       : this.e1.value - this.e2.value);
	  break;

	case JSC$JS_FLOAT:
	  return new JSC$expr_float (this.linenum,
				     this.type == #'+'
				     ? this.e1.value + this.e2.value
				     : this.e1.value - this.e2.value);
	  break;

	case JSC$JS_STRING:
	  if (this.type == #'+')
	    /* Only the addition is available for the strings. */
	    return new JSC$expr_string (this.linenum,
					this.e1.value + this.e2.value);
	  break;

	default:
	  /* FALLTHROUGH */
	  break;
	}
    }

  return this;
}

/* Shift expr. */

function JSC$expr_shift (ln, type, e1, e2)
{
  this.etype = JSC$EXPR_SHIFT;
  this.linenum = ln;
  this.type = type;
  this.e1 = e1;
  this.e2 = e2;
  this.asm = JSC$expr_shift_asm;
}

function JSC$expr_shift_asm ()
{
  this.e1.asm ();
  this.e2.asm ();

  if (this.type == JSC$tLSHIFT)
    new JSC$ASM_shift_left (this.linenum).link ();
  else if (this.type == JSC$tRSHIFT)
    new JSC$ASM_shift_right (this.linenum).link ();
  else
    new JSC$ASM_shift_rright (this.linenum).link ();
}


/* Relational expr. */

function JSC$expr_relational (ln, type, e1, e2)
{
  this.etype = JSC$EXPR_RELATIONAL;
  this.lang_type = JSC$JS_BOOLEAN;
  this.linenum = ln;
  this.type = type;
  this.e1 = e1;
  this.e2 = e2;
  this.asm = JSC$expr_relational_asm;
}

function JSC$expr_relational_asm ()
{
  this.e1.asm ();
  this.e2.asm ();

  if (this.type == #'<')
    new JSC$ASM_cmp_lt (this.linenum).link ();
  else if (this.type == #'>')
    new JSC$ASM_cmp_gt (this.linenum).link ();
  else if (this.type == JSC$tLE)
    new JSC$ASM_cmp_le (this.linenum).link ();
  else
    new JSC$ASM_cmp_ge (this.linenum).link ();
}


/* Equality expr. */

function JSC$expr_equality (ln, type, e1, e2)
{
  this.etype = JSC$EXPR_EQUALITY;
  this.lang_type = JSC$JS_BOOLEAN;
  this.linenum = ln;
  this.type = type;
  this.e1 = e1;
  this.e2 = e2;
  this.asm = JSC$expr_equality_asm;
}

function JSC$expr_equality_asm ()
{
  this.e1.asm ();
  this.e2.asm ();

  switch (this.type)
    {
    case JSC$tEQUAL:
      new JSC$ASM_cmp_eq (this.linenum).link ();
      break;

    case JSC$tNEQUAL:
      new JSC$ASM_cmp_ne (this.linenum).link ();
      break;

    case JSC$tSEQUAL:
      new JSC$ASM_cmp_seq (this.linenum).link ();
      break;

    case JSC$tSNEQUAL:
      new JSC$ASM_cmp_sne (this.linenum).link ();
      break;

    default:
      error ("jsc: expr_equality: internal compiler error");
      break;
    }
}


/* Bitwise and expr. */

function JSC$expr_bitwise_and (ln, e1, e2)
{
  this.etype = JSC$EXPR_BITWISE;
  this.linenum = ln;
  this.e1 = e1;
  this.e2 = e2;
  this.asm = JSC$expr_bitwise_and_asm;
}

function JSC$expr_bitwise_and_asm ()
{
  this.e1.asm ();
  this.e2.asm ();

  new JSC$ASM_and (this.linenum).link ();
}


/* Bitwise or expr. */

function JSC$expr_bitwise_or (ln, e1, e2)
{
  this.etype = JSC$EXPR_BITWISE;
  this.linenum = ln;
  this.e1 = e1;
  this.e2 = e2;
  this.asm = JSC$expr_bitwise_or_asm;
}

function JSC$expr_bitwise_or_asm ()
{
  this.e1.asm ();
  this.e2.asm ();

  new JSC$ASM_or (this.linenum).link ();
}


/* Bitwise xor expr.  */

function JSC$expr_bitwise_xor (ln, e1, e2)
{
  this.etype = JSC$EXPR_BITWISE;
  this.linenum = ln;
  this.e1 = e1;
  this.e2 = e2;
  this.asm = JSC$expr_bitwise_xor_asm;
}

function JSC$expr_bitwise_xor_asm ()
{
  this.e1.asm ();
  this.e2.asm ();

  new JSC$ASM_xor (this.linenum).link ();
}


/* Logical and expr.  */

function JSC$expr_logical_and (ln, e1, e2)
{
  this.etype = JSC$EXPR_LOGICAL;

  if (e1.lang_type && e2.lang_type
      && e1.lang_type == JSC$JS_BOOLEAN && e2.lang_type == JSC$JS_BOOLEAN)
    this.lang_type = JSC$JS_BOOLEAN;

  this.linenum = ln;
  this.e1 = e1;
  this.e2 = e2;
  this.asm = JSC$expr_logical_and_asm;
}

function JSC$expr_logical_and_asm ()
{
  this.e1.asm ();

  var l = new JSC$ASM_label ();
  new JSC$ASM_dup (this.linenum).link ();

  if (JSC$optimize_type && this.e1.lang_type
      && this.e1.lang_type == JSC$JS_BOOLEAN)
    new JSC$ASM_iffalse_b (this.linenum, l).link ();
  else
    new JSC$ASM_iffalse (this.linenum, l).link ();

  new JSC$ASM_pop (this.linenum).link ();

  this.e2.asm ();

  /* Done label. */
  l.link ();
}


/* Logical or expr.  */

function JSC$expr_logical_or (ln, e1, e2)
{
  this.etype = JSC$EXPR_LOGICAL;

  if (e1.lang_type && e2.lang_type
      && e1.lang_type == JSC$JS_BOOLEAN && e2.lang_type == JSC$JS_BOOLEAN)
    this.lang_type = JSC$JS_BOOLEAN;

  this.linenum = ln;
  this.e1 = e1;
  this.e2 = e2;
  this.asm = JSC$expr_logical_or_asm;
}

function JSC$expr_logical_or_asm ()
{
  this.e1.asm ();

  var l = new JSC$ASM_label ();
  new JSC$ASM_dup (this.linenum).link ();

  if (JSC$optimize_type && this.e1.lang_type
      && this.e1.lang_type == JSC$JS_BOOLEAN)
    new JSC$ASM_iftrue_b (this.linenum, l).link ();
  else
    new JSC$ASM_iftrue (this.linenum, l).link ();

  new JSC$ASM_pop (this.linenum).link ();

  this.e2.asm ();

  /* Done label. */
  l.link ();
}


/* New expr. */

function JSC$expr_new (ln, expr, args)
{
  this.etype = JSC$EXPR_NEW;
  this.linenum = ln;
  this.expr = expr;
  this.args = args;
  this.asm = JSC$expr_new_asm;
}

function JSC$expr_new_asm ()
{
  var i;

  if (this.args)
    {
      /* Code for the arguments. */
      for (i = this.args.length - 1; i >= 0; i--)
	this.args[i].asm ();

      if (this.args.length == 0)
	new JSC$ASM_const_i0 (this.linenum).link ();
      else if (this.args.length == 1)
	new JSC$ASM_const_i1 (this.linenum).link ();
      else if (this.args.length == 2)
	new JSC$ASM_const_i2 (this.linenum).link ();
      else if (this.args.length == 3)
	new JSC$ASM_const_i3 (this.linenum).link ();
      else
	new JSC$ASM_const (this.linenum, this.args.length).link ();
    }
  else
    {
      /* A `new Foo' call.  This is identical to `new Foo ()'. */
      new JSC$ASM_const_i0 (this.linenum).link ();
    }

  /* Object. */
  this.expr.asm ();

  /* Call new. */
  new JSC$ASM_new (this.linenum).link ();

  /* Replace the constructor's return value with the object. */
  new JSC$ASM_swap (this.linenum).link ();

  /* Remove the arguments and return the new object. */
  new JSC$ASM_apop (this.linenum,
		    (this.args ? this.args.length : 0) + 2).link ();
}


/* Object property expr. */

function JSC$expr_object_property (ln, expr, id)
{
  this.etype = JSC$EXPR_OBJECT_PROPERTY;
  this.linenum = ln;
  this.expr = expr;
  this.id = id;
  this.asm = JSC$expr_object_property_asm;
}

function JSC$expr_object_property_asm ()
{
  JSC$asm_expr_lvalue_load_asm (this);
}


/* Object array expr. */

function JSC$expr_object_array (ln, expr1, expr2)
{
  this.etype = JSC$EXPR_OBJECT_ARRAY;
  this.linenum = ln;
  this.expr1 = expr1;
  this.expr2 = expr2;
  this.asm = JSC$expr_object_array_asm;
}

function JSC$expr_object_array_asm ()
{
  JSC$asm_expr_lvalue_load_asm (this);
}


/* Call. */

function JSC$expr_call (ln, expr, args)
{
  this.etype = JSC$EXPR_CALL;
  this.linenum = ln;
  this.expr = expr;
  this.args = args;
  this.asm = JSC$expr_call_asm;
}

function JSC$expr_call_asm ()
{
  var i;

  /* Code for the arguments. */
  for (i = this.args.length - 1; i >= 0; i--)
    this.args[i].asm ();

  if (this.args.length == 0)
    new JSC$ASM_const_i0 (this.linenum).link ();
  else if (this.args.length == 1)
    new JSC$ASM_const_i1 (this.linenum).link ();
  else if (this.args.length == 2)
    new JSC$ASM_const_i2 (this.linenum).link ();
  else if (this.args.length == 3)
    new JSC$ASM_const_i3 (this.linenum).link ();
  else
    new JSC$ASM_const (this.linenum, this.args.length).link ();

  /* Check the function type. */
  if (this.expr.etype == JSC$EXPR_IDENTIFIER)
    {
      /* The simple subroutine or global object method invocation. */

      var saved_with_nesting = JSC$cont_break.top.with_nesting;
      JSC$cont_break.top.with_nesting = 0;

      JSC$asm_expr_lvalue_load_asm (this.expr);

      JSC$cont_break.top.with_nesting = saved_with_nesting;

      if (JSC$cont_break.top.with_nesting > 0)
	new JSC$ASM_jsr_w (this.linenum, this.expr.value).link ();
      else
	new JSC$ASM_jsr (this.linenum).link ();

      new JSC$ASM_apop (this.linenum, this.args.length + 2).link ();
    }
  else if (this.expr.etype == JSC$EXPR_OBJECT_PROPERTY)
    {
      /* Method invocation. */
      this.expr.expr.asm ();
      new JSC$ASM_call_method (this.linenum, this.expr.id).link ();
      new JSC$ASM_apop (this.linenum, this.args.length + 2).link ();
    }
  else
    {
      /* Something like a function pointer invocation. */
      JSC$asm_expr_lvalue_load_asm (this.expr);
      new JSC$ASM_jsr (this.linenum).link ();
      new JSC$ASM_apop (this.linenum, this.args.length + 2).link ();
    }
}


/* Assignment. */

function JSC$expr_assignment (ln, type, expr1, expr2)
{
  this.etype = JSC$EXPR_ASSIGNMENT;
  this.linenum = ln;
  this.type = type;
  this.expr1 = expr1;
  this.expr2 = expr2;
  this.asm = JSC$expr_assignment_asm;
}

function JSC$expr_assignment_asm ()
{
  if (this.type != #'=')
    JSC$asm_expr_lvalue_load_asm (this.expr1);

  /* Count the rvalue. */
  this.expr2.asm ();

  if (this.type == #'=')
    /* Nothing here. */
    ;
  else if (this.type == JSC$tMULA)
    new JSC$ASM_mul (this.linenum).link ();
  else if (this.type == JSC$tDIVA)
    new JSC$ASM_div (this.linenum).link ();
  else if (this.type == JSC$tMODA)
    new JSC$ASM_mod (this.linenum).link ();
  else if (this.type == JSC$tADDA)
    new JSC$ASM_add (this.linenum).link ();
  else if (this.type == JSC$tSUBA)
    new JSC$ASM_sub (this.linenum).link ();
  else if (this.type == JSC$tLSIA)
    new JSC$ASM_shift_left (this.linenum).link ();
  else if (this.type == JSC$tRSIA)
    new JSC$ASM_shift_right (this.linenum).link ();
  else if (this.type == JSC$tRRSA)
    new JSC$ASM_shift_rright (this.linenum).link ();
  else if (this.type == JSC$tANDA)
    new JSC$ASM_and (this.linenum).link ();
  else if (this.type == JSC$tXORA)
    new JSC$ASM_xor (this.linenum).link ();
  else if (this.type == JSC$tORA)
    new JSC$ASM_or (this.linenum).link ();
  else
    error (JSC$filename + ":" + this.linenum.toString ()
	   + ": internal compiler error in assignment expression");

  /* Duplicate the value. */
  new JSC$ASM_dup (this.linenum).link ();

  /* Store it to the lvalue. */
  JSC$asm_expr_lvalue_store_asm (this.expr1);
}

function JSC$asm_expr_lvalue_load_asm (expr)
{
  var i;

  if (expr.etype == JSC$EXPR_IDENTIFIER)
    {
      /* Must check global / local / argument. */
      i = JSC$ns.lookup_symbol (expr.value);
      if (i == null)
	{
	  if (JSC$cont_break.top.with_nesting > 0)
	    new JSC$ASM_load_global_w (expr.linenum, expr.value).link ();
	  else
	    new JSC$ASM_load_global (expr.linenum, expr.value).link ();
	}
      else if (i.scope == JSC$SCOPE_ARG)
	{
	  if (JSC$cont_break.top.with_nesting > 0 && JSC$warn_with_clobber)
	    JSC$warning (JSC$filename + ":" + expr.linenum.toString ()
			 + ": warning: the with-lookup of symbol `" + i.symbol
			 + "' is clobbered by the argument definition");

	  new JSC$ASM_load_arg (expr.linenum, i.value).link ();
	}
      else
	{
	  if (JSC$cont_break.top.with_nesting > 0 && JSC$warn_with_clobber)
	    JSC$warning (JSC$filename + ":" + expr.linenum.toString ()
			 + ": warning: the with-lookup of symbol `" + i.symbol
			 + "' is clobbered by the local variable definition");

	  new JSC$ASM_load_local (expr.linenum, i.value).link ();
	}
    }
  else if (expr.etype == JSC$EXPR_OBJECT_PROPERTY)
    {
      expr.expr.asm ();
      new JSC$ASM_load_property (expr.linenum, expr.id).link ();
    }
  else if (expr.etype == JSC$EXPR_OBJECT_ARRAY)
    {
      expr.expr1.asm ();
      expr.expr2.asm ();
      new JSC$ASM_load_array (expr.linenum).link ();
    }
  else
    error (JSC$filename + ":" + expr.linenum.toString () + ": syntax error");
}

function JSC$asm_expr_lvalue_store_asm (expr)
{
  var i;

  if (expr.etype == JSC$EXPR_IDENTIFIER)
    {
      i = JSC$ns.lookup_symbol (expr.value);
      if (i == null)
	new JSC$ASM_store_global (expr.linenum, expr.value).link ();
      else if (i.scope == JSC$SCOPE_ARG)
	new JSC$ASM_store_arg (expr.linenum, i.value).link ();
      else
	new JSC$ASM_store_local (expr.linenum, i.value).link ();
    }
  else if (expr.etype == JSC$EXPR_OBJECT_PROPERTY)
    {
      expr.expr.asm ();
      new JSC$ASM_store_property (expr.linenum, expr.id).link ();
    }
  else if (expr.etype == JSC$EXPR_OBJECT_ARRAY)
    {
      expr.expr1.asm ();
      expr.expr2.asm ();
      new JSC$ASM_store_array (expr.linenum).link ();
    }
  else
    error (JSC$filename + ":" + expr.linenum.toString () + ": syntax error");
}


/* Quest colon. */

function JSC$expr_quest_colon (ln, e1, e2, e3)
{
  this.etype = JSC$EXPR_QUEST_COLON;
  this.linenum = ln;
  this.e1 = e1;
  this.e2 = e2;
  this.e3 = e3;
  this.asm = JSC$expr_quest_colon_asm;
}

function JSC$expr_quest_colon_asm()
{
  /* Code for the condition. */
  this.e1.asm ();

  var l1 = new JSC$ASM_label ();
  var l2 = new JSC$ASM_label ();

  if (JSC$optimize_type && this.e1.lang_type
      && this.e1.lang_type == JSC$JS_BOOLEAN)
    new JSC$ASM_iffalse_b (this.linenum, l1).link ();
  else
    new JSC$ASM_iffalse (this.linenum, l1).link ();

  /* Code for the true branch. */
  this.e2.asm ();
  new JSC$ASM_jmp (this.linenum, l2).link ();

  /* Code for the false branch. */
  l1.link ();
  this.e3.asm ();

  /* Done label. */
  l2.link ();
}


/* Unary. */

function JSC$expr_unary (ln, type, expr)
{
  this.etype = JSC$EXPR_UNARY;
  this.linenum = ln;
  this.type = type;
  this.expr = expr;
  this.asm = JSC$expr_unary_asm;
}

function JSC$expr_unary_asm ()
{
  if (this.type == #'!')
    {
      this.expr.asm ();
      new JSC$ASM_not (this.linenum).link ();
    }
  else if (this.type == #'+')
    {
      this.expr.asm ();
      /* Nothing here. */
    }
  else if (this.type == #'~')
    {
      this.expr.asm ();
      new JSC$ASM_const (this.linenum, -1).link ();
      new JSC$ASM_xor (this.linenum).link ();
    }
  else if (this.type == #'-')
    {
      this.expr.asm ();
      new JSC$ASM_neg (this.linenum).link ();
    }
  else if (this.type == JSC$tDELETE)
    {
      if (this.expr.etype == JSC$EXPR_OBJECT_PROPERTY)
	{
	  this.expr.expr.asm ();
	  new JSC$ASM_delete_property (this.linenum, this.expr.id).link ();
	}
      else if (this.expr.etype == JSC$EXPR_OBJECT_ARRAY)
	{
	  this.expr.expr1.asm ();
	  this.expr.expr2.asm ();
	  new JSC$ASM_delete_array (this.linenum).link ();
	}
      else if (this.expr.etype == JSC$EXPR_IDENTIFIER)
	{
	  if (JSC$cont_break.top.with_nesting == 0)
	    error (JSC$filename + ":" + this.linenum.toString ()
		   + ": `delete property' called outside of a with-block");

	  new JSC$ASM_const_null (this.linenum).link ();
	  new JSC$ASM_delete_property (this.linenum, this.expr.value).link ();
	}
      else
	error (JSC$filename + ":" + this.linenum.toString ()
	       + ": illegal target for the delete operand");
    }
  else if (this.type == JSC$tVOID)
    {
      this.expr.asm ();
      new JSC$ASM_pop (this.linenum).link ();
      new JSC$ASM_const_undefined (this.linenum).link ();
    }
  else if (this.type == JSC$tTYPEOF)
    {
      this.expr.asm ();
      new JSC$ASM_typeof (this.linenum).link ();
    }
  else if (this.type == JSC$tPLUSPLUS
	   || this.type == JSC$tMINUSMINUS)
    {
      /* Fetch the old value. */
      JSC$asm_expr_lvalue_load_asm (this.expr);

      /* Do the operation. */
      new JSC$ASM_const_i1 (this.linenum).link ();
      if (this.type == JSC$tPLUSPLUS)
	new JSC$ASM_add (this.linenum).link ();
      else
	new JSC$ASM_sub (this.linenum).link ();

      /* Duplicate the value and store one copy pack to lvalue. */
      new JSC$ASM_dup (this.linenum).link ();
      JSC$asm_expr_lvalue_store_asm (this.expr);
    }
  else
    {
      error ("jsc: internal error: unary expr's type is "
	     + this.type.toString ());
    }
}


/* Postfix. */

function JSC$expr_postfix (ln, type, expr)
{
  this.etype = JSC$EXPR_POSTFIX;
  this.linenum = ln;
  this.type = type;
  this.expr = expr;
  this.asm = JSC$expr_postfix_asm;
}

function JSC$expr_postfix_asm ()
{
  /* Fetch the old value. */
  JSC$asm_expr_lvalue_load_asm (this.expr);

  /* Duplicate the value since it is the expression's value. */
  new JSC$ASM_dup (this.linenum).link ();

  /* Do the operation. */
  new JSC$ASM_const_i1 (this.linenum).link ();
  if (this.type == JSC$tPLUSPLUS)
    new JSC$ASM_add (this.linenum).link ();
  else
    new JSC$ASM_sub (this.linenum).link ();

  /* And finally, store it back. */
  JSC$asm_expr_lvalue_store_asm (this.expr);
}


/* Postfix. */

function JSC$expr_comma (ln, expr1, expr2)
{
  this.etype = JSC$EXPR_COMMA;
  this.linenum = ln;
  this.expr1 = expr1;
  this.expr2 = expr2;
  this.asm = JSC$expr_comma_asm;
}

function JSC$expr_comma_asm ()
{
  this.expr1.asm ();
  new JSC$ASM_pop (this.linenum).link ();
  this.expr2.asm ();
}


/*
Local variables:
mode: c
End:
*/
/*
 * Namespace handling.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsc/compiler.js,v $
 * $Id: compiler.js,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

/*
 * Global functions.
 */

JSC$SCOPE_ARG = 1;
JSC$SCOPE_LOCAL = 2;

function JSC$NameSpace ()
{
  this.frame = new JSC$NameSpaceFrame ();
  this.push_frame = JSC$NameSpace_push_frame;
  this.pop_frame = JSC$NameSpace_pop_frame;
  this.alloc_local = JSC$NameSpace_alloc_local;
  this.define_symbol = JSC$NameSpace_define_symbol;
  this.lookup_symbol = JSC$NameSpace_lookup_symbol;
}


function JSC$NameSpace_push_frame ()
{
  var f = new JSC$NameSpaceFrame ();

  f.num_locals = this.frame.num_locals;

  f.next = this.frame;
  this.frame = f;
}


function JSC$NameSpace_pop_frame ()
{
  var i;

  for (i = this.frame.defs; i != null; i = i.next)
    if (i.usecount == 0)
      {
	if (i.scope == JSC$SCOPE_ARG)
	  {
	    if (JSC$warn_unused_argument)
	      JSC$warning (JSC$filename + ":" + i.linenum.toString ()
			   + ": warning: unused argument `" + i.symbol + "'");
	  }
	else
	  {
	    if (JSC$warn_unused_variable)
	      JSC$warning (JSC$filename + ":" + i.linenum.toString ()
			   + ": warning: unused variable `" + i.symbol + "'");
	  }
      }

  this.frame = this.frame.next;
}


function JSC$NameSpace_alloc_local ()
{
  return this.frame.num_locals++;
}


function JSC$NameSpace_define_symbol (symbol, scope, value, linenum)
{
  var i;

  for (i = this.frame.defs; i != null; i = i.next)
    if (i.symbol == symbol)
      {
	if (i.scope == scope)
	  error (JSC$filename + ":" + i.linenum.toString()
		 + ": redeclaration of `" + i.symbol + "'");
	if (i.scope == JSC$SCOPE_ARG && JSC$warn_shadow)
	  JSC$warning (JSC$filename + ":" + linenum.toString ()
		       + ": warning: declaration of `" + symbol
		       + "' shadows a parameter");

	i.scope = scope;
	i.value = value;
	i.linenum = linenum;

	return;
      }

  /* Create a new definition. */
  i = new JSC$SymbolDefinition (symbol, scope, value, linenum);
  i.next = this.frame.defs;
  this.frame.defs = i;
}


function JSC$NameSpace_lookup_symbol (symbol)
{
  var f, i;

  for (f = this.frame; f != null; f = f.next)
    for (i = f.defs; i != null; i = i.next)
      if (i.symbol == symbol)
	{
	  i.usecount++;
	  return i;
	}

  return null;
}

/*
 * Static helpers.
 */

function JSC$NameSpaceFrame ()
{
  this.next = null;
  this.defs = null;
  this.num_locals = 0;
}


function JSC$SymbolDefinition (symbol, scope, value, linenum)
{
  this.next = null;
  this.symbol = symbol;
  this.scope = scope;
  this.value = value;
  this.linenum = linenum;
  this.usecount = 0;
}


/*
Local variables:
mode: c
End:
*/
/*
 * Input stream definitions.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsc/compiler.js,v $
 * $Id: compiler.js,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

/*
 * File stream.
 */

function JSC$StreamFile (name)
{
  this.name = name;
  this.stream = new File (name);
  this.error = "";

  this.open		= JSC$StreamFile_open;
  this.close		= JSC$StreamFile_close;
  this.rewind		= JSC$StreamFile_rewind;
  this.readByte		= JSC$StreamFile_read_byte;
  this.ungetByte	= JSC$StreamFile_unget_byte;
  this.readln		= JSC$StreamFile_readln;
}


function JSC$StreamFile_open ()
{
  if (!this.stream.open ("r"))
    {
      this.error = System.strerror (System.errno);
      return false;
    }

  return true;
}


function JSC$StreamFile_close ()
{
  return this.stream.close ();
}


function JSC$StreamFile_rewind ()
{
  return this.stream.setPosition (0);
}


function JSC$StreamFile_read_byte ()
{
  return this.stream.readByte ();
}


function JSC$StreamFile_unget_byte (byte)
{
  this.stream.ungetByte (byte);
}


function JSC$StreamFile_readln ()
{
  return this.stream.readln ();
}


/*
 * String stream.
 */

function JSC$StreamString (str)
{
  this.name = "StringStream";
  this.string = str;
  this.pos = 0;
  this.unget_byte = -1;
  this.error = "";

  this.open		= JSC$StreamString_open;
  this.close		= JSC$StreamString_close;
  this.rewind 		= JSC$StreamString_rewind;
  this.readByte		= JSC$StreamString_read_byte;
  this.ungetByte	= JSC$StreamString_unget_byte;
  this.readln 		= JSC$StreamString_readln;
}


function JSC$StreamString_open ()
{
  return true;
}


function JSC$StreamString_close ()
{
  return true;
}


function JSC$StreamString_rewind ()
{
  this.pos = 0;
  this.unget_byte = -1;
  this.error = "";
  return true;
}


function JSC$StreamString_read_byte ()
{
  var ch;

  if (this.unget_byte >= 0)
    {
      ch = this.unget_byte;
      this.unget_byte = -1;
      return ch;
    }

  if (this.pos >= this.string.length)
    return -1;

  return this.string.charCodeAt (this.pos++);
}


function JSC$StreamString_unget_byte (byte)
{
  this.unget_byte = byte;
}


function JSC$StreamString_readln ()
{
  var line = new String ("");
  var ch;

  while ((ch = this.readByte ()) != -1 && ch != #'\n')
    line.append (String.pack ("C", ch));

  return line;
}


/*
Local variables:
mode: c
End:
*/
/*
 * JavaScript Assembler.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsc/compiler.js,v $
 * $Id: compiler.js,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

/* Byte-code file definitions. */

JSC$BC_MAGIC 		= 0xc0014a53;

JSC$BC_SECT_CODE 	= 0;
JSC$BC_SECT_CONSTANTS	= 1;
JSC$BC_SECT_SYMTAB 	= 2;
JSC$BC_SECT_DEBUG	= 3;

JSC$CONST_INT		= 3;
JSC$CONST_STRING	= 4;
JSC$CONST_FLOAT		= 5;
JSC$CONST_SYMBOL	= 10;
JSC$CONST_REGEXP	= 11;
JSC$CONST_NAN		= 13;

JSC$CONST_REGEXP_FLAG_G	= 0x01;
JSC$CONST_REGEXP_FLAG_I	= 0x02;

JSC$DEBUG_FILENAME	= 1;
JSC$DEBUG_LINENUMBER	= 2;

/* Opcode definitions. */

JSC$OP_HALT		= 0;
JSC$OP_DONE		= 1;
JSC$OP_NOP		= 2;
JSC$OP_DUP		= 3;
JSC$OP_POP		= 4;
JSC$OP_POP_N		= 5;
JSC$OP_APOP		= 6;
JSC$OP_SWAP		= 7;
JSC$OP_ROLL		= 8;
JSC$OP_CONST		= 9;
JSC$OP_CONST_NULL	= 10;
JSC$OP_CONST_TRUE	= 11;
JSC$OP_CONST_FALSE	= 12;
JSC$OP_CONST_UNDEFINED	= 13;
JSC$OP_CONST_I0		= 14;
JSC$OP_CONST_I1		= 15;
JSC$OP_CONST_I2		= 16;
JSC$OP_CONST_I3		= 17;
JSC$OP_CONST_I		= 18;
JSC$OP_LOAD_GLOBAL	= 19;
JSC$OP_STORE_GLOBAL	= 20;
JSC$OP_LOAD_ARG		= 21;
JSC$OP_STORE_ARG	= 22;
JSC$OP_LOAD_LOCAL	= 23;
JSC$OP_STORE_LOCAL	= 24;
JSC$OP_LOAD_PROPERTY	= 25;
JSC$OP_STORE_PROPERTY	= 26;
JSC$OP_LOAD_ARRAY	= 27;
JSC$OP_STORE_ARRAY	= 28;
JSC$OP_NTH		= 29;
JSC$OP_CMP_EQ		= 30;
JSC$OP_CMP_NE		= 31;
JSC$OP_CMP_LT		= 32;
JSC$OP_CMP_GT		= 33;
JSC$OP_CMP_LE		= 34;
JSC$OP_CMP_GE		= 35;
JSC$OP_CMP_SEQ		= 36;
JSC$OP_CMP_SNE		= 37;
JSC$OP_SUB		= 38;
JSC$OP_ADD		= 39;
JSC$OP_MUL		= 40;
JSC$OP_DIV		= 41;
JSC$OP_MOD		= 42;
JSC$OP_NEG		= 43;
JSC$OP_AND		= 44;
JSC$OP_NOT		= 45;
JSC$OP_OR		= 46;
JSC$OP_XOR		= 47;
JSC$OP_SHIFT_LEFT	= 48;
JSC$OP_SHIFT_RIGHT	= 49;
JSC$OP_SHIFT_RRIGHT	= 50;
JSC$OP_IFFALSE		= 51;
JSC$OP_IFTRUE		= 52;
JSC$OP_CALL_METHOD	= 53;
JSC$OP_JMP		= 54;
JSC$OP_JSR		= 55;
JSC$OP_RETURN		= 56;
JSC$OP_TYPEOF		= 57;
JSC$OP_NEW		= 58;
JSC$OP_DELETE_PROPERTY	= 59;
JSC$OP_DELETE_ARRAY	= 60;
JSC$OP_LOCALS		= 61;
JSC$OP_MIN_ARGS		= 62;
JSC$OP_LOAD_NTH_ARG	= 63;
JSC$OP_WITH_PUSH	= 64;
JSC$OP_WITH_POP		= 65;
JSC$OP_TRY_PUSH		= 66;
JSC$OP_TRY_POP		= 67;
JSC$OP_THROW		= 68;

/* Type aware operands. */
JSC$OP_IFFALSE_B	= 69;
JSC$OP_IFTRUE_B		= 70;
JSC$OP_ADD_1_I		= 71;
JSC$OP_ADD_2_I		= 72;
JSC$OP_LOAD_GLOBAL_W	= 73;
JSC$OP_JSR_W		= 74;

/* Internal values. */
JSC$ASM_SYMBOL		= 1000;
JSC$ASM_LABEL		= 1001;

/*
 * General helpers.
 */

/* Generate byte-code for operands with Int8 value. */
function JSC$ASM_bytecode_int8 ()
{
  return String.pack ("C", this.value);
}

/* Generate byte-code for operands with Int16 value. */
function JSC$ASM_bytecode_int16 ()
{
  return String.pack ("n", this.value);
}

/* Generate byte-code for operands with Int32 value. */
function JSC$ASM_bytecode_int32 ()
{
  return String.pack ("N", this.value);
}

/* Generate byte-code for operands with Symbol value. */
function JSC$ASM_bytecode_symbol ()
{
  var cid = JSC$asm_genconstant (String.pack ("C", JSC$CONST_SYMBOL)
				 + this.value + String.pack ("C", 0));
  return String.pack ("N", cid);
}

/* Generate byte-code for local jump operands. */
function JSC$ASM_bytecode_local_jump ()
{
  var delta = this.value.offset - (this.offset + this.size);
  return String.pack ("N", delta);
}


/*
 * Assembler operands.
 */

/* Symbol. */

function JSC$ASM_symbol (ln, value)
{
  this.type = JSC$ASM_SYMBOL;
  this.linenum = ln;
  this.value = value;
  this.size = 0;
  this.print = JSC$ASM_symbol_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_symbol_print (stream)
{
  stream.write ("\n" + this.value + ":\n");
}

/* Label */

function JSC$ASM_label ()
{
  this.type = JSC$ASM_LABEL;
  this.linenum = 0;
  this.size = 0;
  this.value = JSC$asm_label_count++;
  this.referenced = false;
  this.next = null;
  this.print = JSC$ASM_label_print;
  this.link = JSC$asm_link;
  this.format = JSC$ASM_label_format;
}

function JSC$ASM_label_print (stream)
{
  stream.write (this.format () + ":\n");
}

function JSC$ASM_label_format ()
{
  return ".L" + this.value.toString ();
}

/* halt */

function JSC$ASM_halt (ln)
{
  this.type = JSC$OP_HALT;
  this.linenum = ln;
  this.size = 1;
  this.print = JSC$ASM_halt_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_halt_print (stream)
{
  stream.write ("\thalt\n");
}

/* done */

function JSC$ASM_done (ln)
{
  this.type = JSC$OP_DONE;
  this.linenum = ln;
  this.size = 1;
  this.print = JSC$ASM_done_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_done_print (stream)
{
  stream.write ("\tdone\n");
}

/* nop */

function JSC$ASM_nop (ln)
{
  this.type = JSC$OP_NOP;
  this.linenum = ln;
  this.size = 1;
  this.print = JSC$ASM_nop_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_nop_print (stream)
{
  stream.write ("\tnop\n");
}

/* dup */

function JSC$ASM_dup (ln)
{
  this.type = JSC$OP_DUP;
  this.linenum = ln;
  this.stack_delta = 1;
  this.size = 1;
  this.print = JSC$ASM_dup_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_dup_print (stream)
{
  stream.write ("\tdup\n");
}

/* pop */

function JSC$ASM_pop (ln)
{
  this.type = JSC$OP_POP;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_pop_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_pop_print (stream)
{
  stream.write ("\tpop\n");
}

/* pop_n */

function JSC$ASM_pop_n (ln, value)
{
  this.type = JSC$OP_POP_N;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = -value;
  this.size = 2;
  this.print = JSC$ASM_pop_n_print;
  this.link = JSC$asm_link;
  this.bytecode = JSC$ASM_bytecode_int8;
}

function JSC$ASM_pop_n_print (stream)
{
  stream.write ("\tpop_n\t\t" + this.value.toString () + "\n");
}

/* apop */

function JSC$ASM_apop (ln, value)
{
  this.type = JSC$OP_APOP;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = -value;
  this.size = 2;
  this.print = JSC$ASM_apop_print;
  this.link = JSC$asm_link;
  this.bytecode = JSC$ASM_bytecode_int8;
}

function JSC$ASM_apop_print (stream)
{
  stream.write ("\tapop\t\t" + this.value.toString () + "\n");
}

/* swap */

function JSC$ASM_swap (ln)
{
  this.type = JSC$OP_SWAP;
  this.linenum = ln;
  this.size = 1;
  this.print = JSC$ASM_swap_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_swap_print (stream)
{
  stream.write ("\tswap\n");
}

/* roll */
function JSC$ASM_roll (ln, value)
{
  this.type = JSC$OP_ROLL;
  this.linenum = ln;
  this.value = value;
  this.size = 2;
  this.print = JSC$ASM_roll_print;
  this.link = JSC$asm_link;
  this.bytecode = JSC$ASM_bytecode_int8;
}

function JSC$ASM_roll_print (stream)
{
  stream.write ("\troll\t\t" + this.value.toString () + "\n");
}

/* const */

function JSC$ASM_const (ln, value)
{
  this.type = JSC$OP_CONST;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = 1;
  this.size = 5;
  this.print = JSC$ASM_const_print;
  this.link = JSC$asm_link;
  this.bytecode = JSC$ASM_const_bytecode;
}

function JSC$ASM_const_print (stream)
{
  if (typeof this.value == "number")
    stream.write ("\tconst\t\t" + this.value.toString () + "\n");
  else if (typeof this.value == "string"
	   || typeof this.value == "#builtin")
    {
      var i, c;
      var ender, src;
      var stringp = (typeof this.value == "string");

      if (stringp)
	{
	  ender = "\"";
	  src = this.value;
	}
      else
	{
	  ender = "/";
	  src = this.value.source;
	}

      stream.write ("\tconst\t\t" + ender);
      for (i = 0; i < src.length; i++)
	{
	  c = src.charCodeAt (i);
	  if (c == ender[0] || c == #'\\')
	    stream.write ("\\" + src.charAt (i));
	  else if (c == #'\n')
	    stream.write ("\\n");
	  else if (c == #'\r')
	    stream.write ("\\r");
	  else if (c == #'\t')
	    stream.write ("\\t");
	  else if (c == #'\f')
	    stream.write ("\\f");
	  else
	    stream.write (src.charAt (i));
	}
      stream.write (ender);

      if (!stringp)
	{
	  if (this.value.global)
	    stream.write ("g");
	  if (this.value.ignoreCase)
	    stream.write ("i");
	}

      stream.write ("\n");
    }
}

function JSC$ASM_const_bytecode ()
{
  var cid;

  if (typeof this.value == "number")
    {
      if (isInt (this.value))
	cid = JSC$asm_genconstant (String.pack ("CN", JSC$CONST_INT,
						this.value));
      else if (isFloat (this.value))
	cid = JSC$asm_genconstant (String.pack ("Cd", JSC$CONST_FLOAT,
						this.value));
      else
	cid = JSC$asm_genconstant (String.pack ("C", JSC$CONST_NAN));
    }
  else if (typeof this.value == "string")
    cid = JSC$asm_genconstant (String.pack ("CN", JSC$CONST_STRING,
					    this.value.length)
			       + this.value);
  else if (typeof this.value == "#builtin")
    {
      /* Regular expression. */
      var flags = 0;

      if (this.value.global)
	flags |= JSC$CONST_REGEXP_FLAG_G;
      if (this.value.ignoreCase)
	flags |= JSC$CONST_REGEXP_FLAG_I;

      cid = JSC$asm_genconstant (String.pack ("CCN", JSC$CONST_REGEXP, flags,
					      this.value.source.length)
				 + this.value.source);
    }
  else
    error ("ASM_const_bytecode(): unknown type: " + typeof this.value);

  return String.pack ("N", cid);
}

/* const_null */

function JSC$ASM_const_null (ln)
{
  this.type = JSC$OP_CONST_NULL;
  this.linenum = ln;
  this.stack_delta = 1;
  this.size = 1;
  this.print = JSC$ASM_const_null_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_const_null_print (stream)
{
  stream.write ("\tconst_null\n");
}

/* const_true */

function JSC$ASM_const_true (ln)
{
  this.type = JSC$OP_CONST_TRUE;
  this.linenum = ln;
  this.stack_delta = 1;
  this.size = 1;
  this.print = JSC$ASM_const_true_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_const_true_print (stream)
{
  stream.write ("\tconst_true\n");
}

/* const_false */

function JSC$ASM_const_false (ln)
{
  this.type = JSC$OP_CONST_FALSE;
  this.linenum = ln;
  this.stack_delta = 1;
  this.size = 1;
  this.print = JSC$ASM_const_false_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_const_false_print (stream)
{
  stream.write ("\tconst_false\n");
}

/* const_undefined */

function JSC$ASM_const_undefined (ln)
{
  this.type = JSC$OP_CONST_UNDEFINED;
  this.linenum = ln;
  this.stack_delta = 1;
  this.size = 1;
  this.print = JSC$ASM_const_undefined_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_const_undefined_print (stream)
{
  stream.write ("\tconst_undefined\n");
}

/* const_i0 */

function JSC$ASM_const_i0 (ln)
{
  this.type = JSC$OP_CONST_I0;
  this.linenum = ln;
  this.stack_delta = 1;
  this.size = 1;
  this.print = JSC$ASM_const_i0_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_const_i0_print (stream)
{
  stream.write ("\tconst_i0\n");
}

/* const_i1 */

function JSC$ASM_const_i1 (ln)
{
  this.type = JSC$OP_CONST_I1;
  this.linenum = ln;
  this.stack_delta = 1;
  this.size = 1;
  this.print = JSC$ASM_const_i1_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_const_i1_print (stream)
{
  stream.write ("\tconst_i1\n");
}

/* const_i2 */

function JSC$ASM_const_i2 (ln)
{
  this.type = JSC$OP_CONST_I2;
  this.linenum = ln;
  this.stack_delta = 1;
  this.size = 1;
  this.print = JSC$ASM_const_i2_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_const_i2_print (stream)
{
  stream.write ("\tconst_i2\n");
}

/* const_i3 */

function JSC$ASM_const_i3 (ln)
{
  this.type = JSC$OP_CONST_I3;
  this.linenum = ln;
  this.stack_delta = 1;
  this.size = 1;
  this.print = JSC$ASM_const_i3_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_const_i3_print (stream)
{
  stream.write ("\tconst_i3\n");
}

/* const_i */

function JSC$ASM_const_i (ln, value)
{
  this.type = JSC$OP_CONST_I;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = 1;
  this.size = 5;
  this.print = JSC$ASM_const_i_print;
  this.bytecode = JSC$ASM_bytecode_int32;
  this.link = JSC$asm_link;
}

function JSC$ASM_const_i_print (stream)
{
  stream.write ("\tconst_i\t\t" + this.value.toString () + "\n");
}

/* load_global */

function JSC$ASM_load_global (ln, value)
{
  this.type = JSC$OP_LOAD_GLOBAL;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = 1;
  this.size = 5;
  this.print = JSC$ASM_load_global_print;
  this.bytecode = JSC$ASM_bytecode_symbol;
  this.link = JSC$asm_link;
}

function JSC$ASM_load_global_print (stream)
{
  stream.write ("\tload_global\t" + this.value + "\n");
}

/* store_global */

function JSC$ASM_store_global (ln, value)
{
  this.type = JSC$OP_STORE_GLOBAL;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = -1;
  this.size = 5;
  this.print = JSC$ASM_store_global_print;
  this.bytecode = JSC$ASM_bytecode_symbol;
  this.link = JSC$asm_link;
}

function JSC$ASM_store_global_print (stream)
{
  stream.write ("\tstore_global\t" + this.value + "\n");
}

/* load_arg */

function JSC$ASM_load_arg (ln, value)
{
  this.type = JSC$OP_LOAD_ARG;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = 1;
  this.size = 2;
  this.print = JSC$ASM_load_arg_print;
  this.bytecode = JSC$ASM_bytecode_int8;
  this.link = JSC$asm_link;
}

function JSC$ASM_load_arg_print (stream)
{
  stream.write ("\tload_arg\t" + this.value.toString () + "\n");
}

/* store_arg */

function JSC$ASM_store_arg (ln, value)
{
  this.type = JSC$OP_STORE_ARG;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = -1;
  this.size = 2;
  this.print = JSC$ASM_store_arg_print;
  this.bytecode = JSC$ASM_bytecode_int8;
  this.link = JSC$asm_link;
}

function JSC$ASM_store_arg_print (stream)
{
  stream.write ("\tstore_arg\t" + this.value.toString () + "\n");
}

/* load_local */

function JSC$ASM_load_local (ln, value)
{
  this.type = JSC$OP_LOAD_LOCAL;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = 1;
  this.size = 3;
  this.print = JSC$ASM_load_local_print;
  this.bytecode = JSC$ASM_bytecode_int16;
  this.link = JSC$asm_link;
}

function JSC$ASM_load_local_print (stream)
{
  stream.write ("\tload_local\t" + this.value.toString () + "\n");
}

/* store_local */

function JSC$ASM_store_local (ln, value)
{
  this.type = JSC$OP_STORE_LOCAL;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = -1;
  this.size = 3;
  this.print = JSC$ASM_store_local_print;
  this.bytecode = JSC$ASM_bytecode_int16;
  this.link = JSC$asm_link;
}

function JSC$ASM_store_local_print (stream)
{
  stream.write ("\tstore_local\t" + this.value.toString () + "\n");
}

/* load_property */

function JSC$ASM_load_property (ln, value)
{
  this.type = JSC$OP_LOAD_PROPERTY;
  this.linenum = ln;
  this.value = value;
  this.size = 5;
  this.print = JSC$ASM_load_property_print;
  this.bytecode = JSC$ASM_bytecode_symbol;
  this.link = JSC$asm_link;
}

function JSC$ASM_load_property_print (stream)
{
  stream.write ("\tload_property\t" + this.value + "\n");
}

/* store_property */

function JSC$ASM_store_property (ln, value)
{
  this.type = JSC$OP_STORE_PROPERTY;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = -2;
  this.size = 5;
  this.print = JSC$ASM_store_property_print;
  this.bytecode = JSC$ASM_bytecode_symbol;
  this.link = JSC$asm_link;
}

function JSC$ASM_store_property_print (stream)
{
  stream.write ("\tstore_property\t" + this.value + "\n");
}

/* load_array */

function JSC$ASM_load_array (ln)
{
  this.type = JSC$OP_LOAD_ARRAY;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_load_array_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_load_array_print (stream)
{
  stream.write ("\tload_array\n");
}

/* store_array */

function JSC$ASM_store_array (ln)
{
  this.type = JSC$OP_STORE_ARRAY;
  this.linenum = ln;
  this.stack_delta = -3;
  this.size = 1;
  this.print = JSC$ASM_store_array_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_store_array_print (stream)
{
  stream.write ("\tstore_array\n");
}

/* nth */

function JSC$ASM_nth (ln)
{
  this.type = JSC$OP_NTH;
  this.linenum = ln;
  this.size = 1;
  this.print = JSC$ASM_nth_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_nth_print (stream)
{
  stream.write ("\tnth\n");
}

/* cmp_eq */

function JSC$ASM_cmp_eq (ln)
{
  this.type = JSC$OP_CMP_EQ;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_cmp_eq_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_cmp_eq_print (stream)
{
  stream.write ("\tcmp_eq\n");
}

/* cmp_ne */

function JSC$ASM_cmp_ne (ln)
{
  this.type = JSC$OP_CMP_NE;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_cmp_ne_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_cmp_ne_print (stream)
{
  stream.write ("\tcmp_ne\n");
}

/* cmp_lt */

function JSC$ASM_cmp_lt (ln)
{
  this.type = JSC$OP_CMP_LT;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_cmp_lt_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_cmp_lt_print (stream)
{
  stream.write ("\tcmp_lt\n");
}

/* cmp_gt */

function JSC$ASM_cmp_gt (ln)
{
  this.type = JSC$OP_CMP_GT;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_cmp_gt_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_cmp_gt_print (stream)
{
  stream.write ("\tcmp_gt\n");
}

/* cmp_le */

function JSC$ASM_cmp_le (ln)
{
  this.type = JSC$OP_CMP_LE;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_cmp_le_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_cmp_le_print (stream)
{
  stream.write ("\tcmp_le\n");
}

/* cmp_ge */

function JSC$ASM_cmp_ge (ln)
{
  this.type = JSC$OP_CMP_GE;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_cmp_ge_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_cmp_ge_print (stream)
{
  stream.write ("\tcmp_ge\n");
}

/* cmp_seq */

function JSC$ASM_cmp_seq (ln)
{
  this.type = JSC$OP_CMP_SEQ;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_cmp_seq_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_cmp_seq_print (stream)
{
  stream.write ("\tcmp_seq\n");
}

/* cmp_sne */

function JSC$ASM_cmp_sne (ln)
{
  this.type = JSC$OP_CMP_SNE;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_cmp_sne_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_cmp_sne_print (stream)
{
  stream.write ("\tcmp_sne\n");
}

/* sub */

function JSC$ASM_sub (ln)
{
  this.type = JSC$OP_SUB;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_sub_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_sub_print (stream)
{
  stream.write ("\tsub\n");
}

/* add */

function JSC$ASM_add (ln)
{
  this.type = JSC$OP_ADD;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_add_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_add_print (stream)
{
  stream.write ("\tadd\n");
}

/* mul */

function JSC$ASM_mul (ln)
{
  this.type = JSC$OP_MUL;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_mul_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_mul_print (stream)
{
  stream.write ("\tmul\n");
}

/* div */

function JSC$ASM_div (ln)
{
  this.type = JSC$OP_DIV;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_div_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_div_print (stream)
{
  stream.write ("\tdiv\n");
}

/* mod */

function JSC$ASM_mod (ln)
{
  this.type = JSC$OP_MOD;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_mod_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_mod_print (stream)
{
  stream.write ("\tmod\n");
}

/* neg */

function JSC$ASM_neg (ln)
{
  this.type = JSC$OP_NEG;
  this.linenum = ln;
  this.size = 1;
  this.print = JSC$ASM_neg_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_neg_print (stream)
{
  stream.write ("\tneg\n");
}

/* and */

function JSC$ASM_and (ln)
{
  this.type = JSC$OP_AND;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_and_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_and_print (stream)
{
  stream.write ("\tand\n");
}

/* not */

function JSC$ASM_not (ln)
{
  this.type = JSC$OP_NOT;
  this.linenum = ln;
  this.size = 1;
  this.print = JSC$ASM_not_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_not_print (stream)
{
  stream.write ("\tnot\n");
}

/* or */

function JSC$ASM_or (ln)
{
  this.type = JSC$OP_OR;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_or_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_or_print (stream)
{
  stream.write ("\tor\n");
}

/* xor */

function JSC$ASM_xor (ln)
{
  this.type = JSC$OP_XOR;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_xor_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_xor_print (stream)
{
  stream.write ("\txor\n");
}

/* shift_left */

function JSC$ASM_shift_left (ln)
{
  this.type = JSC$OP_SHIFT_LEFT;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_shift_left_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_shift_left_print (stream)
{
  stream.write ("\tshift_left\n");
}

/* shift_right */

function JSC$ASM_shift_right (ln)
{
  this.type = JSC$OP_SHIFT_RIGHT;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_shift_right_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_shift_right_print (stream)
{
  stream.write ("\tshift_right\n");
}

/* shift_rright */

function JSC$ASM_shift_rright (ln)
{
  this.type = JSC$OP_SHIFT_RRIGHT;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_shift_rright_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_shift_rright_print (stream)
{
  stream.write ("\tshift_rright\n");
}

/* iffalse */

function JSC$ASM_iffalse (ln, value)
{
  this.type = JSC$OP_IFFALSE;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = -1;
  this.size = 5;
  this.print = JSC$ASM_iffalse_print;
  this.bytecode = JSC$ASM_bytecode_local_jump;
  this.link = JSC$asm_link;
}

function JSC$ASM_iffalse_print (stream)
{
  stream.write ("\tiffalse\t\t" + this.value.format () + "\n");
}

/* iftrue */

function JSC$ASM_iftrue (ln, value)
{
  this.type = JSC$OP_IFTRUE;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = -1;
  this.size = 5;
  this.print = JSC$ASM_iftrue_print;
  this.bytecode = JSC$ASM_bytecode_local_jump;
  this.link = JSC$asm_link;
}

function JSC$ASM_iftrue_print (stream)
{
  stream.write ("\tiftrue\t\t" + this.value.format () + "\n");
}

/* call_method */

function JSC$ASM_call_method (ln, value)
{
  this.type = JSC$OP_CALL_METHOD;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = 1;
  this.size = 5;
  this.print = JSC$ASM_call_method_print;
  this.bytecode = JSC$ASM_bytecode_symbol;
  this.link = JSC$asm_link;
}

function JSC$ASM_call_method_print (stream)
{
  stream.write ("\tcall_method\t" + this.value + "\n");
}

/* jmp */

function JSC$ASM_jmp (ln, value)
{
  this.type = JSC$OP_JMP;
  this.linenum = ln;
  this.value = value;
  this.size = 5;
  this.print = JSC$ASM_jmp_print;
  this.bytecode = JSC$ASM_bytecode_local_jump;
  this.link = JSC$asm_link;
}

function JSC$ASM_jmp_print (stream)
{
  stream.write ("\tjmp\t\t" + this.value.format () + "\n");
}

/* jsr */

function JSC$ASM_jsr (ln)
{
  this.type = JSC$OP_JSR;
  this.linenum = ln;
  this.stack_delta = 1;
  this.size = 1;
  this.print = JSC$ASM_jsr_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_jsr_print (stream)
{
  stream.write ("\tjsr\n");
}

/* return */

function JSC$ASM_return (ln)
{
  this.type = JSC$OP_RETURN;
  this.linenum = ln;
  this.size = 1;
  this.print = JSC$ASM_return_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_return_print (stream)
{
  stream.write ("\treturn\n");
}

/* typeof */

function JSC$ASM_typeof (ln)
{
  this.type = JSC$OP_TYPEOF;
  this.linenum = ln;
  this.size = 1;
  this.print = JSC$ASM_typeof_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_typeof_print (stream)
{
  stream.write ("\ttypeof\n");
}

/* new */

function JSC$ASM_new (ln)
{
  this.type = JSC$OP_NEW;
  this.linenum = ln;
  this.stack_delta = 1;
  this.size = 1;
  this.print = JSC$ASM_new_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_new_print (stream)
{
  stream.write ("\tnew\n");
}

/* delete_property */

function JSC$ASM_delete_property (ln, value)
{
  this.type = JSC$OP_DELETE_PROPERTY;
  this.linenum = ln;
  this.value = value;
  this.size = 5;
  this.print = JSC$ASM_delete_property_print;
  this.bytecode = JSC$ASM_bytecode_symbol;
  this.link = JSC$asm_link;
}

function JSC$ASM_delete_property_print (stream)
{
  stream.write ("\tdelete_property\t" + this.value + "\n");
}

/* delete_array */

function JSC$ASM_delete_array (ln)
{
  this.type = JSC$OP_DELETE_ARRAY;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_delete_array_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_delete_array_print (stream)
{
  stream.write ("\tdelete_array\n");
}

/* locals */

function JSC$ASM_locals (ln, value)
{
  this.type = JSC$OP_LOCALS;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = value;
  this.size = 3;
  this.print = JSC$ASM_locals_print;
  this.bytecode = JSC$ASM_bytecode_int16;
  this.link = JSC$asm_link;
}

function JSC$ASM_locals_print (stream)
{
  stream.write ("\tlocals\t\t" + this.value.toString () + "\n");
}

/* min_args */

function JSC$ASM_min_args (ln, value)
{
  this.type = JSC$OP_MIN_ARGS;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = -1;
  this.size = 2;
  this.print = JSC$ASM_min_args_print;
  this.bytecode = JSC$ASM_bytecode_int8;
  this.link = JSC$asm_link;
}

function JSC$ASM_min_args_print (stream)
{
  stream.write ("\tmin_args\t" + this.value.toString () + "\n");
}

/* load_nth_arg */

function JSC$ASM_load_nth_arg (ln)
{
  this.type = JSC$OP_LOAD_NTH_ARG;
  this.linenum = ln;
  this.size = 1;
  this.print = JSC$ASM_load_nth_arg_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_load_nth_arg_print (stream)
{
  stream.write ("\tload_nth_arg\n");
}

/* with_push */

function JSC$ASM_with_push (ln)
{
  this.type = JSC$OP_WITH_PUSH;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_with_push_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_with_push_print (stream)
{
  stream.write ("\twith_push\n");
}

/* with_pop */

function JSC$ASM_with_pop (ln, value)
{
  this.type = JSC$OP_WITH_POP;
  this.linenum = ln;
  this.value = value;
  this.size = 2;
  this.print = JSC$ASM_with_pop_print;
  this.bytecode = JSC$ASM_bytecode_int8;
  this.link = JSC$asm_link;
}

function JSC$ASM_with_pop_print (stream)
{
  stream.write ("\twith_pop\t" + this.value.toString () + "\n");
}

/* try_push */

function JSC$ASM_try_push (ln, value)
{
  this.type = JSC$OP_TRY_PUSH;
  this.linenum = ln;
  this.value = value;
  this.size = 5;
  this.print = JSC$ASM_try_push_print;
  this.bytecode = JSC$ASM_bytecode_local_jump;
  this.link = JSC$asm_link;
}

function JSC$ASM_try_push_print (stream)
{
  stream.write ("\ttry_push\t" + this.value.format () + "\n");
}

/* try_pop */

function JSC$ASM_try_pop (ln, value)
{
  this.type = JSC$OP_TRY_POP;
  this.linenum = ln;
  this.value = value;
  this.size = 2;
  this.print = JSC$ASM_try_pop_print;
  this.bytecode = JSC$ASM_bytecode_int8;
  this.link = JSC$asm_link;
}

function JSC$ASM_try_pop_print (stream)
{
  stream.write ("\ttry_pop\t\t" + this.value.toString () + "\n");
}

/* throw */

function JSC$ASM_throw (ln)
{
  this.type = JSC$OP_THROW;
  this.linenum = ln;
  this.stack_delta = -1;
  this.size = 1;
  this.print = JSC$ASM_throw_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_throw_print (stream)
{
  stream.write ("\tthrow\n");
}

/* iffalse_b */

function JSC$ASM_iffalse_b (ln, value)
{
  this.type = JSC$OP_IFFALSE_B;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = -1;
  this.size = 5;
  this.print = JSC$ASM_iffalse_b_print;
  this.bytecode = JSC$ASM_bytecode_local_jump;
  this.link = JSC$asm_link;
}

function JSC$ASM_iffalse_b_print (stream)
{
  stream.write ("\tiffalse_b\t" + this.value.format () + "\n");
}

/* iftrue */

function JSC$ASM_iftrue_b (ln, value)
{
  this.type = JSC$OP_IFTRUE_B;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = -1;
  this.size = 5;
  this.print = JSC$ASM_iftrue_b_print;
  this.bytecode = JSC$ASM_bytecode_local_jump;
  this.link = JSC$asm_link;
}

function JSC$ASM_iftrue_b_print (stream)
{
  stream.write ("\tiftrue_b\t" + this.value.format () + "\n");
}

/* add_1_i */

function JSC$ASM_add_1_i (ln)
{
  this.type = JSC$OP_ADD_1_I;
  this.linenum = ln;
  this.size = 1;
  this.print = JSC$ASM_add_1_i_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_add_1_i_print (stream)
{
  stream.write ("\tadd_1_i\n");
}

/* add_2_i */

function JSC$ASM_add_2_i (ln)
{
  this.type = JSC$OP_ADD_2_I;
  this.linenum = ln;
  this.size = 1;
  this.print = JSC$ASM_add_2_i_print;
  this.link = JSC$asm_link;
}

function JSC$ASM_add_2_i_print (stream)
{
  stream.write ("\tadd_2_i\n");
}

/* load_global_w */

function JSC$ASM_load_global_w (ln, value)
{
  this.type = JSC$OP_LOAD_GLOBAL_W;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = 1;
  this.size = 5;
  this.print = JSC$ASM_load_global_w_print;
  this.bytecode = JSC$ASM_bytecode_symbol;
  this.link = JSC$asm_link;
}

function JSC$ASM_load_global_w_print (stream)
{
  stream.write ("\tload_global_w\t" + this.value + "\n");
}

/* jsr_w */

function JSC$ASM_jsr_w (ln, value)
{
  this.type = JSC$OP_JSR_W;
  this.linenum = ln;
  this.value = value;
  this.stack_delta = 1;
  this.size = 5;
  this.print = JSC$ASM_jsr_w_print;
  this.bytecode = JSC$ASM_bytecode_symbol;
  this.link = JSC$asm_link;
}

function JSC$ASM_jsr_w_print (stream)
{
  stream.write ("\tjsr_w\t\t" + this.value + "\n");
}

/*
 * General helpers.
 */

function JSC$asm_link ()
{
  this.next = null;

  if (JSC$asm_tail != null)
    {
      JSC$asm_tail_prev = JSC$asm_tail;
      JSC$asm_tail.next = this;
    }
  else
    JSC$asm_head = this;

  JSC$asm_tail = this;
}


/*
 * The phases of the assembler.
 */

/* This is called from the compiler initialization code. */
function JSC$asm_reset ()
{
  JSC$asm_label_count = 1;
  JSC$asm_head = JSC$asm_tail = JSC$asm_tail_prev = null;
  JSC$asm_constcount = 0;
  JSC$asm_constants = null;
  JSC$asm_known_constants = null;
}


function JSC$asm_generate ()
{
  var i;

  if (JSC$verbose)
    JSC$message ("jsc: generating assembler");

  JSC$ns = new JSC$NameSpace ();

  /* Functions. */
  for (i = 0; i < JSC$functions.length; i++)
    JSC$functions[i].asm ();

  /* Global statements. */
  if (JSC$global_stmts.length > 0)
    {
      /* Define the `.global' symbol. */
      new JSC$ASM_symbol (JSC$global_stmts[0].linenum, ".global").link ();

      /* Handle local variables. */
      var num_locals = JSC$count_locals_from_stmt_list (JSC$global_stmts);
      if (num_locals > 0)
	new JSC$ASM_locals (JSC$global_stmts[0].linenum, num_locals).link ();

      /* Generate assembler. */
      for (i = 0; i < JSC$global_stmts.length; i++)
	JSC$global_stmts[i].asm ();

      /*
       * Fix things so that also the global statement returns something
       * (this is required when we use eval() in JavaScript).
       */
      if (JSC$asm_tail_prev == null)
	{
	  /* This is probably illegal, but we don't panic. */
	  new JSC$ASM_const_undefined (0).link ();
	}
      else
	{
	  /*
	   * If the latest op is `pop', remove it.  Otherwise, append
	   * a `const_undefined'.
	   */
	  if (JSC$asm_tail.type == JSC$OP_POP)
	    {
	      JSC$asm_tail = JSC$asm_tail_prev;
	      JSC$asm_tail.next = null;
	      JSC$asm_tail_prev = null;
	    }
	  else
	    new JSC$ASM_const_undefined (JSC$asm_tail.linenum).link ();
	}
    }

  JSC$ns = null;
}


function JSC$asm_print (src_stream, stream)
{
  var i;
  var last_ln;
  var annotate = src_stream ? true : false;

  if (annotate)
    {
      stream.write ("; -*- asm -*-\n");

      /* Set the prev properties. */
      var prev = null;
      for (i = JSC$asm_head; i != null; prev = i, i = i.next)
	i.prev = prev;

      /*
       * Fix the label line numbers to be the same that the next
       * assembler operand has.
       */
      last_ln = 0;
      for (i = JSC$asm_tail; i != null; i = i.prev)
	{
	  if (i.type == JSC$ASM_LABEL)
	    i.linenum = last_ln;
	  else if (typeof i.linenum != "undefined")
	    last_ln = i.linenum;
	}
    }

  last_ln = 0;
  for (i = JSC$asm_head; i != null; i = i.next)
    {
      if (typeof i.linenum == "undefined")
	{
	  if (annotate)
	    stream.write ("; undefined linenum\n");
	}
      else
	while (annotate && i.linenum > last_ln)
	  {
	    var line = src_stream.readln ();
	    stream.write ("; " + line + "\n");
	    last_ln++;
	  }

      i.print (stream);
    }
}


function JSC$asm_is_load_op (op)
{
  return (op.type == JSC$OP_LOAD_GLOBAL
	  || op.type == JSC$OP_LOAD_ARG
	  || op.type == JSC$OP_LOAD_LOCAL);
}


function JSC$asm_is_store_op (op)
{
  return (op.type == JSC$OP_STORE_GLOBAL
	  || op.type == JSC$OP_STORE_ARG
	  || op.type == JSC$OP_STORE_LOCAL);
}


function JSC$asm_is_local_jump (op)
{
  return (op.type == JSC$OP_JMP
	  || op.type == JSC$OP_IFFALSE
	  || op.type == JSC$OP_IFTRUE
	  || op.type == JSC$OP_IFFALSE_B
	  || op.type == JSC$OP_IFTRUE_B
	  || op.type == JSC$OP_TRY_PUSH);
}


function JSC$asm_is_const_op (op)
{
  return (JSC$OP_CONST <= op.type && op.type <= JSC$OP_CONST_I3);
}


function JSC$asm_lookup_next_op (item)
{
  while (item != null &&
	 (item.type == JSC$ASM_LABEL || item.type == JSC$ASM_SYMBOL))
    item = item.next;

  return item;
}


function JSC$asm_optimize (flags)
{
  var item;

  /* Simple peephole optimization. */
  if ((flags & JSC$FLAG_OPTIMIZE_PEEPHOLE) != 0)
    {
      if (JSC$verbose)
	JSC$message ("jsc: optimize: peephole");

      for (item = JSC$asm_head; item != null; item = item.next)
	{
	  /*
	   * Optimization for dup ... pop cases where pop removes the
	   * item duplicated by dup.
	   */
	  if (item.next != null && item.next.type == JSC$OP_DUP)
	    {
	      var balance = 2;
	      var found = false;
	      var i1;

	      for (i1 = item.next.next;
		   i1 != null && i1.next != null;
		   i1 = i1.next)
		{
		  var i2 = i1.next;

		  /*
		   * The lookup ends on branches, and on dup, throw,
		   * and try_pop operands.  We optimize on a basic
		   * block and we match the closest dup-pop pairs.
		   */
		  if (JSC$asm_is_local_jump (i1)
		      || i1.type == JSC$OP_JSR
		      || i1.type == JSC$OP_NEW
		      || i1.type == JSC$OP_CALL_METHOD
		      || i1.type == JSC$OP_RETURN
		      || i1.type == JSC$ASM_SYMBOL
		      || i1.type == JSC$ASM_LABEL
		      || i1.type == JSC$OP_DUP
		      || i1.type == JSC$OP_TRY_POP
		      || i1.type == JSC$OP_THROW)
		    break;

		  if (i1.stack_delta)
		    {
		      balance += i1.stack_delta;
		      if (balance <= 0)
			/* Going to negative.  Stop here. */
			break;
		    }

		  if (i2.type == JSC$OP_POP && balance == 1)
		    {
		      /* Found a matching pop. */
		      found = true;
		      i1.next = i2.next;
		      break;
		    }
		}

	      if (found)
		{
		  /* The dup can be removed. */
		  item.next = item.next.next;
		}
	    }

	  /* Two instruction optimization (starting from item.next). */
	  if (item.next != null && item.next.next != null)
	    {
	      var i1 = item.next;
	      var i2 = i1.next;

	      if (i1.type == JSC$OP_APOP
		  && i2.type == JSC$OP_POP)
		{
		  /*
		   * i1:	apop n
		   * i2:	pop		->	pop_n n + 1
		   */
		  var i = new JSC$ASM_pop_n (i1.linenum, i1.value + 1);
		  item.next = i;
		  i.next = i2.next;
		}
	    }
	  if (item.next != null && item.next.next != null)
	    {
	      var i1 = item.next;
	      var i2 = i1.next;

	      if (i1.type == JSC$OP_CONST_TRUE
		  && (i2.type == JSC$OP_IFFALSE
		      || i2.type == JSC$OP_IFFALSE_B))
		{
		  /*
		   * i1:	const_true
		   * i2:	iffalse{,_b}	.LX	=> ---
		   */
		  item.next = i2.next;
		}
	    }
	  if (item.next != null && item.next.next != null)
	    {
	      var i1 = item.next;
	      var i2 = i1.next;

	      if (i1.type == JSC$OP_CONST_FALSE
		  && (i2.type == JSC$OP_IFTRUE
		      || i2.type == JSC$OP_IFTRUE_B))
		{
		  /*
		   * i1:	const_false
		   * i2:	iftrue{,_b}	.LX	=> ---
		   */
		  item.next = i2.next;
		}
	    }
	  if (item.next != null && item.next.next != null)
	    {
	      var i1 = item.next;
	      var i2 = i1.next;

	      if ((i1.type == JSC$OP_CONST_FALSE
		   && (i2.type == JSC$OP_IFFALSE
		       || i2.type == JSC$OP_IFFALSE_B))
		  || (i1.type == JSC$OP_CONST_TRUE
		      && (i2.type == JSC$OP_IFTRUE
			  || i2.type == JSC$OP_IFTRUE_B)))
		{
		  /*
		   * i1:	const_false
		   * i2:	iffalse{,_b}	.LX	=> jmp .LX
		   *
		   * i1:	const_true
		   * i2:	iftrue{,_b}	.LX	=> jmp .LX
		   */
		  var i = new JSC$ASM_jmp (i1.linenum, i2.value);
		  item.next = i;
		  i.next = i2.next;
		}
	    }
	}
    }

  /* Jumps to jumps. */
  if ((flags & JSC$FLAG_OPTIMIZE_JUMPS) != 0)
    {
      if (JSC$verbose)
	JSC$message ("jsc: optimize: jumps to jumps");
      for (item = JSC$asm_head; item != null; item = item.next)
	if (JSC$asm_is_local_jump (item))
	  {
	    var i2;

	    /* Operand's value is a label */
	    i2 = JSC$asm_lookup_next_op (item.value);

	    if (i2 != null && i2.type == JSC$OP_JMP)
	      /* Ok, we can jump there directly. */
	      item.value = i2.value;
	  }
    }

  if ((flags & JSC$FLAG_OPTIMIZE_HEAVY) != 0)
    JSC$optimize_heavy ();

  /*
   * Optimizations for the size of the generated byte-code.  It isn't
   * probably worth of doing these optimization for interactive
   * scripts since these won't affect the speed of the execution.
   * However, these optimizations make the byte-code files smaller so
   * these are nice for batch-compiled files.
   */
  if ((flags & JSC$FLAG_OPTIMIZE_BC_SIZE) != 0)
    {
      var delta = true;

      while (delta)
	{
	  delta = false;

	  /* Remove un-referenced labels. */

	  if (JSC$verbose)
	    JSC$message ("jsc: optimize: removing un-referenced labels");

	  /* First, make all labels unreferenced. */
	  for (item = JSC$asm_head; item != null; item = item.next)
	    if (item.type == JSC$ASM_LABEL)
	      item.referenced = false;

	  /* Second, mark all referenced labels. */
	  for (item = JSC$asm_head; item != null; item = item.next)
	    if (JSC$asm_is_local_jump (item))
	      item.value.referenced = true;

	  /* Third, remove all un-referenced labels. */
	  for (item = JSC$asm_head; item != null; item = item.next)
	    while (item.next != null && item.next.type == JSC$ASM_LABEL
		   && !item.next.referenced
		   && item.next.next != null)
	      {
		delta = true;
		item.next = item.next.next;
	      }

	  /* Dead code elimination. */
	  if (JSC$verbose)
	    JSC$message ("jsc: optimize: dead code elimination");
	  for (item = JSC$asm_head; item != null; item = item.next)
	    if (item.type == JSC$OP_RETURN || item.type == JSC$OP_JMP)
	      while (item.next != null && item.next.type != JSC$ASM_SYMBOL
		     && item.next.type != JSC$ASM_LABEL)
		{
		  delta = true;
		  item.next = item.next.next;
		}


	  /* Simple peephole optimization. */
	  if (JSC$verbose)
	    JSC$message ("jsc: optimize: peephole");
	  for (item = JSC$asm_head; item != null; item = item.next)
	    {
	      /* Two instruction optimization (starting from item.next). */
	      if (item.next != null && item.next.next != null)
		{
		  var i1 = item.next;
		  var i2 = i1.next;

		  if (i1.type == JSC$OP_JMP
		      && i2.type == JSC$ASM_LABEL
		      && i1.value == i2)
		    {
		      /*
		       * i1:	jmp	.LX
		       * i2:	.LX		=> .LX
		       */
		      item.next = i2;
		      delta = true;
		    }
		}
	    }
	}
    }
}


function JSC$optimize_heavy ()
{
  if (JSC$verbose)
    JSC$message ("jsc: optimize: liveness analyzing");

  /* First, set the prev pointers and zero usage flags. */
  var item, prev = null;

  for (item = JSC$asm_head; item != null; prev = item, item = item.next)
    {
      item.prev = prev;
      item.live_args = 0;
      item.live_locals = 0;
      item.live_used = false;
    }

  /* For each function. */
  var ftail, fhead;
  for (ftail = JSC$asm_tail; ftail != null; ftail = fhead.prev)
    {
      var change = true;

      /* While there is change in the liveness. */
      while (change)
	{
	  change = false;

	  for (fhead = ftail;
	       fhead.type != JSC$ASM_SYMBOL;
	       fhead = fhead.prev)
	    {
	      var floc, farg;

	      if (fhead.next != null)
		{
		  floc = fhead.next.live_locals;
		  farg = fhead.next.live_args;
		}
	      else
		floc = farg = 0;

	      if (fhead.type == JSC$OP_LOAD_LOCAL && fhead.value < 32)
		floc |= (1 << fhead.value);

	      if (fhead.type == JSC$OP_STORE_LOCAL && fhead.value < 32)
		floc &= ~(1 << fhead.value);

	      if (fhead.type == JSC$OP_LOAD_ARG && fhead.value < 32)
		farg |= (1 << fhead.value);

	      if (fhead.type == JSC$OP_STORE_ARG && fhead.value < 32)
		farg &= ~(1 << fhead.value);

	      if (JSC$asm_is_local_jump (fhead))
		{
		  floc |= fhead.value.live_locals;
		  fhead.value.live_used = true;
		}

	      if (fhead.live_used && (fhead.live_locals != floc
				      || fhead.live_args != farg))
		change = true;

	      fhead.live_used = false;
	      fhead.live_locals = floc;
	      fhead.live_args = farg;
	    }
	}
    }

  /*
   * When we have the liveness analyzing performed, we can do some
   * fancy optimizations.
   */

  if (JSC$verbose)
    JSC$message ("jsc: optimize: peephole");

  for (item = JSC$asm_head; item != null; item = item.next)
    {
      /* Three instruction optimization. */
      if (item.next != null && item.next.next != null
	  && item.next.next.next != null)
	{
	  var i1 = item.next;
	  var i2 = i1.next;
	  var i3 = i2.next;

	  if (i1.type == JSC$OP_STORE_LOCAL
	      && i2.type == JSC$OP_LOAD_LOCAL
	      && i1.value == i2.value
	      && (i3.live_locals & (1 << i1.value)) == 0)
	    {
	      /*
	       * i1:	store_local 	n
	       * i2:	load_local	n
	       * i3:	nnn (n not live)	=> nnn
	       */

	      item.next = i3;
	    }
	}
    }
}


function JSC$asm_finalize ()
{
  var item;
  var offset = 0;

  for (item = JSC$asm_head; item != null; item = item.next)
    {
      item.offset = offset;
      offset += item.size;
    }
}


function JSC$ConstantReg ()
{
}

function JSC$asm_genconstant (val)
{
  if (JSC$asm_known_constants == null)
    JSC$asm_known_constants = new JSC$ConstantReg ();

  /* Lookup <val> from a list of known constants. */
  var id = JSC$asm_known_constants[val];
  if (typeof id == "number")
    return id;

  /* This is a new constant. */
  JSC$asm_constants.append (val);
  JSC$asm_known_constants[val] = JSC$asm_constcount;

  return JSC$asm_constcount++;
}

function JSC$asm_bytecode ()
{
  var item;
  var symtab = new String ("");
  var nsymtab_entries = 0;
  var code = new String ("");
  var debug = new String ("");
  var debug_last_linenum = 0;

  if (JSC$verbose)
    JSC$message ("jsc: generating byte-code");

  if (JSC$generate_debug_info)
    /* Source file name. */
    debug.append (String.pack ("CN", JSC$DEBUG_FILENAME, JSC$filename.length)
		  + JSC$filename);

  JSC$asm_constants = new String ("");

  for (item = JSC$asm_head; item != null; item = item.next)
    {
      if (item.type == JSC$ASM_SYMBOL)
	{
	  symtab.append (item.value + String.pack ("CN", 0, item.offset));
	  nsymtab_entries++;
	}
      else if (item.type == JSC$ASM_LABEL)
	;
      else
	{
	  /* Real assembler operands. */

	  if (JSC$generate_debug_info)
	    if (item.linenum != debug_last_linenum)
	      {
		debug.append (String.pack ("CNN", JSC$DEBUG_LINENUMBER,
					   item.offset + item.size,
					   item.linenum));
		debug_last_linenum = item.linenum;
	      }

	  if (item.size == 1)
	    /* We handle these. */
	    code.append (String.pack ("C", item.type));
	  else
	    {
	      /*
	       * All operands which take an argument, have a method to create
	       * the byte code entry for their argument.
	       */
	      code.append (String.pack ("C", item.type) + item.bytecode ());
	    }
	}
    }

  symtab = String.pack ("N", nsymtab_entries) + symtab;

  if (JSC$verbose)
    {
      var msg = ("jsc: code=" + code.length.toString ()
		 + ", constants=" + JSC$asm_constants.length.toString ()
		 + ", symtab=" + symtab.length.toString ());

      if (JSC$generate_debug_info)
	msg += ", debug=" + debug.length.toString ();

      msg += (", headers="
	      + (32 + (JSC$generate_debug_info ? 8 : 0)).toString ()
	      + ", total="
	      + (code.length + JSC$asm_constants.length + symtab.length
		 + debug.length + 32
		 + (JSC$generate_debug_info ? 8 : 0)).toString ());
      JSC$message (msg);
    }

  return (String.pack ("NN", JSC$BC_MAGIC,
		       3 + (JSC$generate_debug_info ? 1 : 0))
	  + String.pack ("NN", JSC$BC_SECT_CODE, code.length) + code

	  + String.pack ("NN", JSC$BC_SECT_CONSTANTS,
			 JSC$asm_constants.length)
	  + JSC$asm_constants

	  + String.pack ("NN", JSC$BC_SECT_SYMTAB, symtab.length) + symtab

	  + (JSC$generate_debug_info
	     ? String.pack ("NN", JSC$BC_SECT_DEBUG, debug.length) + debug
	     : ""));
}


/*
Local variables:
mode: c
End:
*/
/*
 * Public entry points to the JavaScript compiler.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsc/compiler.js,v $
 * $Id: compiler.js,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

/*
 * Definitions.
 */

JSC$FLAG_VERBOSE			= 0x00000001;
JSC$FLAG_ANNOTATE_ASSEMBLER		= 0x00000002;
JSC$FLAG_GENERATE_DEBUG_INFO		= 0x00000004;
JSC$FLAG_GENERATE_EXECUTABLE_BC_FILES	= 0x00000008;

JSC$FLAG_OPTIMIZE_PEEPHOLE		= 0x00000020;
JSC$FLAG_OPTIMIZE_JUMPS			= 0x00000040;
JSC$FLAG_OPTIMIZE_BC_SIZE		= 0x00000080;
JSC$FLAG_OPTIMIZE_HEAVY			= 0x00000100;

JSC$FLAG_OPTIMIZE_MASK			= 0x0000fff0;

JSC$FLAG_WARN_UNUSED_ARGUMENT		= 0x00010000;
JSC$FLAG_WARN_UNUSED_VARIABLE		= 0x00020000;
JSC$FLAG_WARN_SHADOW			= 0x00040000;
JSC$FLAG_WARN_WITH_CLOBBER		= 0x00080000;
JSC$FLAG_WARN_MISSING_SEMICOLON		= 0x00100000;
JSC$FLAG_WARN_STRICT_ECMA		= 0x00200000;
JSC$FLAG_WARN_DEPRECATED		= 0x00400000;

JSC$FLAG_WARN_MASK			= 0xffff0000;

/*
 * Global interfaces to the compiler.
 */

function JSC$compile_file (fname, flags, asm_file, bc_file)
{
  var stream = new JSC$StreamFile (fname);
  return JSC$compile_stream (stream, flags, asm_file, bc_file);
}


function JSC$compile_string (str, flags, asm_file, bc_file)
{
  var stream = new JSC$StreamString (str);
  return JSC$compile_stream (stream, flags, asm_file, bc_file);
}


function JSC$compiler_reset ()
{
  /* Reset compiler to a known initial state. */
  JSC$parser_reset ();
  JSC$gram_reset ();
  JSC$asm_reset ();
}


function JSC$compile_stream (stream, flags, asm_file, bc_file)
{
  var result = false;

  JSC$compiler_reset ();

  if (stream.open ())
    {
      try
	{
	  JSC$verbose = ((flags & JSC$FLAG_VERBOSE) != 0);
	  JSC$generate_debug_info
	    = ((flags & JSC$FLAG_GENERATE_DEBUG_INFO) != 0);

	  JSC$warn_unused_argument
	    = ((flags & JSC$FLAG_WARN_UNUSED_ARGUMENT) != 0);
	  JSC$warn_unused_variable
	    = ((flags & JSC$FLAG_WARN_UNUSED_VARIABLE) != 0);
	  JSC$warn_shadow
	    = ((flags & JSC$FLAG_WARN_SHADOW) != 0);
	  JSC$warn_with_clobber
	    = ((flags & JSC$FLAG_WARN_WITH_CLOBBER) != 0);
	  JSC$warn_missing_semicolon
	    = ((flags & JSC$FLAG_WARN_MISSING_SEMICOLON) != 0);
	  JSC$warn_strict_ecma
	    = ((flags & JSC$FLAG_WARN_STRICT_ECMA) != 0);
	  JSC$warn_deprecated
	    = ((flags & JSC$FLAG_WARN_DEPRECATED) != 0);

	  /* Compilation and assembler generation time optimizations. */
	  JSC$optimize_constant_folding = true;
	  JSC$optimize_type = true;

	  JSC$parser_parse (stream);

	  /* Assembler. */
	  JSC$asm_generate ();

	  /*
	   * We don't need the syntax tree anymore.  Free it and save
	   * some memory.
	   */
	  JSC$parser_reset ();

	  /* Optimize if requested. */
	  if ((flags & JSC$FLAG_OPTIMIZE_MASK) != 0)
	    JSC$asm_optimize (flags);

	  if (typeof asm_file == "string")
	    {
	      var asm_stream = new File (asm_file);
	      var src_stream = false;

	      if (asm_stream.open ("w"))
		{
		  if ((flags & JSC$FLAG_ANNOTATE_ASSEMBLER) != 0)
		    if (stream.rewind ())
		      src_stream = stream;

		  JSC$asm_print (src_stream, asm_stream);
		  asm_stream.close ();
		}
	      else
		JSC$message ("jsc: couldn't create asm output file \""
			     + asm_file + "\": "
			     + System.strerror (System.errno));
	    }

	  JSC$asm_finalize ();

	  result = JSC$asm_bytecode ();

	  /* Remove all intermediate results from the memory. */
	  JSC$compiler_reset ();

	  if (typeof bc_file == "string")
	    {
	      var ostream = new File (bc_file);
	      if (ostream.open ("w"))
		{
		  ostream.write (result);
		  ostream.close ();

		  if ((flags & JSC$FLAG_GENERATE_EXECUTABLE_BC_FILES) != 0)
		    {
		      /* Add execute permissions to the output file. */
		      var st = File.stat (bc_file);
		      if (st)
			{
			  if (!File.chmod (bc_file, st[2] | 0111))
			    JSC$message ("jsc: couldn't add execute "
					 + "permissions to bc file \""
					 + bc_file + "\": "
					 + System.strerror (System.errno));
			}
		      else
			JSC$message ("jsc: couldn't stat bc file \"" + bc_file
				     + "\": "
				     + System.strerror (System.errno));
		    }
		}
	      else
		JSC$message ("jsc: couldn't create bc file \"" + bc_file
			     + "\": " + System.strerror (System.errno));
	    }
	}
      finally
	{
	  stream.close ();
	}
    }
  else
    error ("jsc: couldn't open input stream \"" + stream.name + "\": "
	   + stream.error);

  return result;
}


/*
Local variables:
mode: c
End:
*/
