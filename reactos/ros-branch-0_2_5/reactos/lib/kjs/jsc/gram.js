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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsc/gram.js,v $
 * $Id: gram.js,v 1.1 2004/01/10 20:38:17 arty Exp $
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
