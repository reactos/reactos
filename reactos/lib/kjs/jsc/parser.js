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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsc/parser.js,v $
 * $Id: parser.js,v 1.1 2004/01/10 20:38:17 arty Exp $
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
