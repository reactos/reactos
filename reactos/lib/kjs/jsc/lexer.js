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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsc/lexer.js,v $
 * $Id: lexer.js,v 1.1 2004/01/10 20:38:17 arty Exp $
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
