/*
 * Process assembler file.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsas/jsas.js,v $
 * $Id: jsas.js,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

function process_file (name)
{
  var fp = new File (name);
  var labels = new Object ();
  var label_waits = new Object ();
  var op;

  if (fp.open ("r"))
    {
      var linenum = 0;

      while (!fp.eof ())
	{
	  var line = fp.readln ();
	  linenum++;

	  /* Remove comments. */
	  line = line.replace (/;.*$/, "");

	  /* Extract the labels and symbols. */
	  if (/^([^ \t\n]+):(.*)$/.exec (line))
	    {
	      var label = RegExp.$1;
	      line = RegExp.$2;

	      if (/^\.[^ ]+$/.test (label) && label != ".global")
		{
		  /* It is a label. */
		  if (labels[label])
		    {
		      System.stderr.writeln (name + ":" + linenum.toString ()
					     + ": label `" + label +
					     "' is already defined");
		      System.stderr.writeln (name + ":"
					     + (labels[label].linenum
						.toString ())
					     + ": this is the place of the "
					     + "previous definition");
		      System.exit (1);
		    }
		  labels[label] = new JSC$ASM_label ();
		  labels[label].linenum = linenum;
		  labels[label].link ();
		}
	      else
		{
		  /* It is a symbol. */
		  new JSC$ASM_symbol (linenum, label).link ();
		}
	    }

	  /* Skip empty lines. */
	  if (/^[ \n\t]*$/.test (line))
	    continue;

	  if (!/^[ \n\t]*([^ \n\t]+)[ \n\t]*(.*)[ \n\t]*$/.exec (line))
	    {
	      System.stderr.writeln (name + ":" + linenum.toString ()
				     + ": syntax error");
	      System.exit (1);
	    }
	  var operand = RegExp.$1;
	  var argument = RegExp.$2;

	  /* Check that this is a known operand. */
	  if (typeof argument_sizes[operand] == "undefined")
	    {
	      System.stderr.writeln (name + ":" + linenum.toString ()
				     + ": unknown operand `" + operand
				     + "'");
	      System.exit (1);
	    }

	  if ((operand_flags[operand] & 0x02) != 0)
	    {
	      /* The branch operands. */
	      if (argument.length == 0)
		{
		  System.stderr.writeln (name + ":" + linenum.toString ()
					 + ": branch operand requires a "
					 + " label argument");
		  System.exit (1);
		}
	      if (!label_waits[argument])
		label_waits[argument] = new Array ();

	      op = give_operand (operand, linenum);
	      label_waits[argument].push (op);
	      op.link ();
	    }
	  else if ((operand_flags[operand] & 0x01) != 0)
	    {
	      /* The symbol operands. */
	      if (!/^[A-Za-z_$][A-Za-z_$0-9]*$/.test (argument))
		{
		  System.stderr.writeln (name + ":" + linenum.toString ()
					 + ": operand `" + operand
					 + "' requires a symbol argument");
		  System.exit (1);
		}
	      op = give_operand (operand, linenum);
	      op.link ();
	      op.value = argument;
	    }
	  else if (operand == "const")
	    {
	      var value;

	      if (/^\"(.*)\"$/.test (argument))
		value = const_string (RegExp.$1);
	      else if (/^[0-9]+$/.test (argument))
		value = parseInt(argument);
	      else
		{
		  System.stderr.writeln (name + ":" + linenum.toString ()
					 + ": malformed argument `"
					 + argument + "' for operand `"
					 + operand + "'");
		  System.exit (1);
		}

	      op = give_operand (operand, linenum);
	      op.link ();
	      op.value = value;
	    }
	  else
	    {
	      if (argument_sizes[operand] == 0)
		{
		  /* Simple operands without arugments. */
		  if (argument.length != 0)
		    {
		      System.stderr.writeln (name + ":" + linenum.toString ()
					     + ": operand `" + operand
					     + "' doesn't take argument");
		      System.exit (1);
		    }
		  give_operand (operand, linenum).link ();
		}
	      else
		{
		  /* Simple operands with integer arguments. */
		  if (!/^[-+]?[0-9]+$/.test (argument))
		    {
		      System.stderr.writeln (name + ":" + linenum.toString ()
					     + ": operand `" + operand
					     + "' requires an integer "
					     + "argument");
		      System.exit (1);
		    }
		  op = give_operand (operand, linenum);
		  op.link ();
		  op.value = parseInt (argument);
		}
	    }
	}
      fp.close ();

      /* Patch labels to branch operands. */
      for (var label in label_waits)
	for (op in label_waits[label])
	  {
	    if (!labels[label])
	      {
		System.stderr.writeln (name + ":" + op.linenum.toString ()
				       + ": undefined label `" + label + "'");
		System.exit (1);
	      }
	    op.value = labels[label];
	  }
    }
  else
    {
      System.stderr.writeln (program + ": couldn't open input file `"
			     + name + "': " + System.strerror (System.errno));
      System.exit (1);
    }
}


function const_string (source)
{
  var str = new String ("");
  var i;

  for (i = 0; i < source.length; i++)
    {
      switch (source[i])
	{
	case #'\\':
	  if (i + 1 > source.length)
	    str.append ("\\");
	  else
	    {
	      i++;
	      var ch;

	      switch (source[i])
		{
		case #'n':
		  ch = "\n";
		  break;

		case #'t':
		  ch = "\t";
		  break;

		case #'v':
		  ch = "\v";
		  break;

		case #'b':
		  ch = "\b";
		  break;

		case #'r':
		  ch = "\r";
		  break;

		case #'f':
		  ch = "\f";
		  break;

		case #'a':
		  ch = "\a";
		  break;

		case #'\\':
		  ch = "\\";
		  break;

		case #'?':
		  ch = "?";
		  break;

		case #'\'':
		  ch = "'";
		  break;

		case #'"':
		  ch = "\"";
		  break;

		default:
		  ch = File.byteToString (source[i]);
		  break;
		}

	      str.append (ch);
	    }
	  break;

	default:
	  str.append (File.byteToString (source[i]));
	  break;
	}
    }

  return str;
}


/*
Local variables:
mode: c
End:
*/
/*                                                              -*- c -*-
 * Operand definitions for the JavaScript byte-code.
 *
 * This file is automatically create from the operands.def file.
 * Editing is strongly discouraged.  You should edit the file
 * `extract-operands.js' instead.
 */

argument_sizes = new Object ();
operand_flags = new Object ();

function give_operand (name, ln)
{
  switch (name)
    {
    case "halt":
      return new JSC$ASM_halt (ln);
      break;

    case "done":
      return new JSC$ASM_done (ln);
      break;

    case "nop":
      return new JSC$ASM_nop (ln);
      break;

    case "dup":
      return new JSC$ASM_dup (ln);
      break;

    case "pop":
      return new JSC$ASM_pop (ln);
      break;

    case "pop_n":
      return new JSC$ASM_pop_n (ln);
      break;

    case "apop":
      return new JSC$ASM_apop (ln);
      break;

    case "swap":
      return new JSC$ASM_swap (ln);
      break;

    case "roll":
      return new JSC$ASM_roll (ln);
      break;

    case "const":
      return new JSC$ASM_const (ln);
      break;

    case "const_null":
      return new JSC$ASM_const_null (ln);
      break;

    case "const_true":
      return new JSC$ASM_const_true (ln);
      break;

    case "const_false":
      return new JSC$ASM_const_false (ln);
      break;

    case "const_undefined":
      return new JSC$ASM_const_undefined (ln);
      break;

    case "const_i0":
      return new JSC$ASM_const_i0 (ln);
      break;

    case "const_i1":
      return new JSC$ASM_const_i1 (ln);
      break;

    case "const_i2":
      return new JSC$ASM_const_i2 (ln);
      break;

    case "const_i3":
      return new JSC$ASM_const_i3 (ln);
      break;

    case "const_i":
      return new JSC$ASM_const_i (ln);
      break;

    case "load_global":
      return new JSC$ASM_load_global (ln);
      break;

    case "store_global":
      return new JSC$ASM_store_global (ln);
      break;

    case "load_arg":
      return new JSC$ASM_load_arg (ln);
      break;

    case "store_arg":
      return new JSC$ASM_store_arg (ln);
      break;

    case "load_local":
      return new JSC$ASM_load_local (ln);
      break;

    case "store_local":
      return new JSC$ASM_store_local (ln);
      break;

    case "load_property":
      return new JSC$ASM_load_property (ln);
      break;

    case "store_property":
      return new JSC$ASM_store_property (ln);
      break;

    case "load_array":
      return new JSC$ASM_load_array (ln);
      break;

    case "store_array":
      return new JSC$ASM_store_array (ln);
      break;

    case "nth":
      return new JSC$ASM_nth (ln);
      break;

    case "cmp_eq":
      return new JSC$ASM_cmp_eq (ln);
      break;

    case "cmp_ne":
      return new JSC$ASM_cmp_ne (ln);
      break;

    case "cmp_lt":
      return new JSC$ASM_cmp_lt (ln);
      break;

    case "cmp_gt":
      return new JSC$ASM_cmp_gt (ln);
      break;

    case "cmp_le":
      return new JSC$ASM_cmp_le (ln);
      break;

    case "cmp_ge":
      return new JSC$ASM_cmp_ge (ln);
      break;

    case "cmp_seq":
      return new JSC$ASM_cmp_seq (ln);
      break;

    case "cmp_sne":
      return new JSC$ASM_cmp_sne (ln);
      break;

    case "sub":
      return new JSC$ASM_sub (ln);
      break;

    case "add":
      return new JSC$ASM_add (ln);
      break;

    case "mul":
      return new JSC$ASM_mul (ln);
      break;

    case "div":
      return new JSC$ASM_div (ln);
      break;

    case "mod":
      return new JSC$ASM_mod (ln);
      break;

    case "neg":
      return new JSC$ASM_neg (ln);
      break;

    case "and":
      return new JSC$ASM_and (ln);
      break;

    case "not":
      return new JSC$ASM_not (ln);
      break;

    case "or":
      return new JSC$ASM_or (ln);
      break;

    case "xor":
      return new JSC$ASM_xor (ln);
      break;

    case "shift_left":
      return new JSC$ASM_shift_left (ln);
      break;

    case "shift_right":
      return new JSC$ASM_shift_right (ln);
      break;

    case "shift_rright":
      return new JSC$ASM_shift_rright (ln);
      break;

    case "iffalse":
      return new JSC$ASM_iffalse (ln);
      break;

    case "iftrue":
      return new JSC$ASM_iftrue (ln);
      break;

    case "call_method":
      return new JSC$ASM_call_method (ln);
      break;

    case "jmp":
      return new JSC$ASM_jmp (ln);
      break;

    case "jsr":
      return new JSC$ASM_jsr (ln);
      break;

    case "return":
      return new JSC$ASM_return (ln);
      break;

    case "typeof":
      return new JSC$ASM_typeof (ln);
      break;

    case "new":
      return new JSC$ASM_new (ln);
      break;

    case "delete_property":
      return new JSC$ASM_delete_property (ln);
      break;

    case "delete_array":
      return new JSC$ASM_delete_array (ln);
      break;

    case "locals":
      return new JSC$ASM_locals (ln);
      break;

    case "min_args":
      return new JSC$ASM_min_args (ln);
      break;

    case "load_nth_arg":
      return new JSC$ASM_load_nth_arg (ln);
      break;

    case "with_push":
      return new JSC$ASM_with_push (ln);
      break;

    case "with_pop":
      return new JSC$ASM_with_pop (ln);
      break;

    case "try_push":
      return new JSC$ASM_try_push (ln);
      break;

    case "try_pop":
      return new JSC$ASM_try_pop (ln);
      break;

    case "throw":
      return new JSC$ASM_throw (ln);
      break;

    case "iffalse_b":
      return new JSC$ASM_iffalse_b (ln);
      break;

    case "iftrue_b":
      return new JSC$ASM_iftrue_b (ln);
      break;

    case "add_1_i":
      return new JSC$ASM_add_1_i (ln);
      break;

    case "add_2_i":
      return new JSC$ASM_add_2_i (ln);
      break;

    case "load_global_w":
      return new JSC$ASM_load_global_w (ln);
      break;

    case "jsr_w":
      return new JSC$ASM_jsr_w (ln);
      break;

    default:
      System.stderr.writeln (program + ":" + ln.toString ()
			     + ": unknown operand `" + name + "'");
      break;
    }
}

argument_sizes["halt"] = 0;
operand_flags["halt"] = 0x0;
argument_sizes["done"] = 0;
operand_flags["done"] = 0x0;
argument_sizes["nop"] = 0;
operand_flags["nop"] = 0x0;
argument_sizes["dup"] = 0;
operand_flags["dup"] = 0x0;
argument_sizes["pop"] = 0;
operand_flags["pop"] = 0x0;
argument_sizes["pop_n"] = 1;
operand_flags["pop_n"] = 0x0;
argument_sizes["apop"] = 1;
operand_flags["apop"] = 0x0;
argument_sizes["swap"] = 0;
operand_flags["swap"] = 0x0;
argument_sizes["roll"] = 1;
operand_flags["roll"] = 0x0;
argument_sizes["const"] = 4;
operand_flags["const"] = 0x0;
argument_sizes["const_null"] = 0;
operand_flags["const_null"] = 0x0;
argument_sizes["const_true"] = 0;
operand_flags["const_true"] = 0x0;
argument_sizes["const_false"] = 0;
operand_flags["const_false"] = 0x0;
argument_sizes["const_undefined"] = 0;
operand_flags["const_undefined"] = 0x0;
argument_sizes["const_i0"] = 0;
operand_flags["const_i0"] = 0x0;
argument_sizes["const_i1"] = 0;
operand_flags["const_i1"] = 0x0;
argument_sizes["const_i2"] = 0;
operand_flags["const_i2"] = 0x0;
argument_sizes["const_i3"] = 0;
operand_flags["const_i3"] = 0x0;
argument_sizes["const_i"] = 4;
operand_flags["const_i"] = 0x0;
argument_sizes["load_global"] = 4;
operand_flags["load_global"] = 0x1;
argument_sizes["store_global"] = 4;
operand_flags["store_global"] = 0x1;
argument_sizes["load_arg"] = 1;
operand_flags["load_arg"] = 0x0;
argument_sizes["store_arg"] = 1;
operand_flags["store_arg"] = 0x0;
argument_sizes["load_local"] = 2;
operand_flags["load_local"] = 0x0;
argument_sizes["store_local"] = 2;
operand_flags["store_local"] = 0x0;
argument_sizes["load_property"] = 4;
operand_flags["load_property"] = 0x1;
argument_sizes["store_property"] = 4;
operand_flags["store_property"] = 0x1;
argument_sizes["load_array"] = 0;
operand_flags["load_array"] = 0x0;
argument_sizes["store_array"] = 0;
operand_flags["store_array"] = 0x0;
argument_sizes["nth"] = 0;
operand_flags["nth"] = 0x0;
argument_sizes["cmp_eq"] = 0;
operand_flags["cmp_eq"] = 0x0;
argument_sizes["cmp_ne"] = 0;
operand_flags["cmp_ne"] = 0x0;
argument_sizes["cmp_lt"] = 0;
operand_flags["cmp_lt"] = 0x0;
argument_sizes["cmp_gt"] = 0;
operand_flags["cmp_gt"] = 0x0;
argument_sizes["cmp_le"] = 0;
operand_flags["cmp_le"] = 0x0;
argument_sizes["cmp_ge"] = 0;
operand_flags["cmp_ge"] = 0x0;
argument_sizes["cmp_seq"] = 0;
operand_flags["cmp_seq"] = 0x0;
argument_sizes["cmp_sne"] = 0;
operand_flags["cmp_sne"] = 0x0;
argument_sizes["sub"] = 0;
operand_flags["sub"] = 0x0;
argument_sizes["add"] = 0;
operand_flags["add"] = 0x0;
argument_sizes["mul"] = 0;
operand_flags["mul"] = 0x0;
argument_sizes["div"] = 0;
operand_flags["div"] = 0x0;
argument_sizes["mod"] = 0;
operand_flags["mod"] = 0x0;
argument_sizes["neg"] = 0;
operand_flags["neg"] = 0x0;
argument_sizes["and"] = 0;
operand_flags["and"] = 0x0;
argument_sizes["not"] = 0;
operand_flags["not"] = 0x0;
argument_sizes["or"] = 0;
operand_flags["or"] = 0x0;
argument_sizes["xor"] = 0;
operand_flags["xor"] = 0x0;
argument_sizes["shift_left"] = 0;
operand_flags["shift_left"] = 0x0;
argument_sizes["shift_right"] = 0;
operand_flags["shift_right"] = 0x0;
argument_sizes["shift_rright"] = 0;
operand_flags["shift_rright"] = 0x0;
argument_sizes["iffalse"] = 4;
operand_flags["iffalse"] = 0x2;
argument_sizes["iftrue"] = 4;
operand_flags["iftrue"] = 0x2;
argument_sizes["call_method"] = 4;
operand_flags["call_method"] = 0x1;
argument_sizes["jmp"] = 4;
operand_flags["jmp"] = 0x2;
argument_sizes["jsr"] = 0;
operand_flags["jsr"] = 0x0;
argument_sizes["return"] = 0;
operand_flags["return"] = 0x0;
argument_sizes["typeof"] = 0;
operand_flags["typeof"] = 0x0;
argument_sizes["new"] = 0;
operand_flags["new"] = 0x0;
argument_sizes["delete_property"] = 4;
operand_flags["delete_property"] = 0x1;
argument_sizes["delete_array"] = 0;
operand_flags["delete_array"] = 0x0;
argument_sizes["locals"] = 2;
operand_flags["locals"] = 0x0;
argument_sizes["min_args"] = 1;
operand_flags["min_args"] = 0x0;
argument_sizes["load_nth_arg"] = 0;
operand_flags["load_nth_arg"] = 0x0;
argument_sizes["with_push"] = 0;
operand_flags["with_push"] = 0x0;
argument_sizes["with_pop"] = 1;
operand_flags["with_pop"] = 0x0;
argument_sizes["try_push"] = 4;
operand_flags["try_push"] = 0x2;
argument_sizes["try_pop"] = 1;
operand_flags["try_pop"] = 0x0;
argument_sizes["throw"] = 0;
operand_flags["throw"] = 0x0;
argument_sizes["iffalse_b"] = 4;
operand_flags["iffalse_b"] = 0x2;
argument_sizes["iftrue_b"] = 4;
operand_flags["iftrue_b"] = 0x2;
argument_sizes["add_1_i"] = 0;
operand_flags["add_1_i"] = 0x0;
argument_sizes["add_2_i"] = 0;
operand_flags["add_2_i"] = 0x0;
argument_sizes["load_global_w"] = 4;
operand_flags["load_global_w"] = 0x1;
argument_sizes["jsr_w"] = 4;
operand_flags["jsr_w"] = 0x1;
/*
 * Assembler for JavaScript assembler.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsas/jsas.js,v $
 * $Id: jsas.js,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

/*
 * Variables and definitions.
 */

version_number = "0.0.1";

/*
 * Options.
 */

/*
 * -g, --debug
 *
 * Generate debugging information.
 */

opt_debug = false;

/*
 * -h, --help
 *
 * Print short help and exit successfully.
 */

/*
 * -O, --optimize
 *
 * Optimize the assembler.
 */

opt_optimize = false;

/*
 * -v, --verbose
 *
 * Tell what we are doing.
 */

opt_verbose = false;

/*
 * -V, --version
 *
 * Print version information and exit successfully.
 */

/*
 * Functions.
 */

function main ()
{
  var idx = ARGS[0].lastIndexOf ("/");
  if (idx >= 0)
    program = ARGS[0].substr (idx + 1);
  else
    program = ARGS[0];

  /* Handle arguments. */
  var i;
  for (i = 1; i < ARGS.length; i++)
    {
      if (ARGS[i][0] == #'-')
	{
	  if (ARGS[i] == "-g" || ARGS[i] == "--debug")
	    opt_debug = true;
	  else if (ARGS[i] == "-h" || ARGS[i] == "--help")
	    {
	      usage ();
	      System.exit (1);
	    }
	  else if (ARGS[i] == "-O" || ARGS[i] == "--optimize")
	    opt_optimize = true;
	  else if (ARGS[i] == "-v" || ARGS[i] == "--verbose")
	    opt_verbose = true;
	  else if (ARGS[i] == "-V" || ARGS[i] == "--version")
	    {
	      version ();
	      System.exit (0);
	    }
	  else
	    {
	      /* Unrecognized option. */
	      System.error (program, ": unrecognized option `",
			    ARGS[i], "'\n");
	      System.error ("Try `", program,
			    " --help' for more information.\n");
	      System.exit (1);
	    }
	}
      else
	{
	  /* End of arguments. */
	  break;
	}
    }

  if (i >= ARGS.length)
    {
      System.error (program, ": no files specified\n");
      System.exit (1);
    }

  /* Options. */

  JSC$verbose = opt_verbose;
  JSC$generate_debug_info = opt_debug;

  /* Process files. */
  for (; i < ARGS.length; i++)
    {
      /* Reset the JSC's assembler package. */
      JSC$asm_reset ();

      /* Process the input file. */
      JSC$filename = ARGS[i];
      process_file (JSC$filename);

      if (opt_optimize)
	JSC$asm_optimize (JSC$FLAG_OPTIMIZE_MASK);

      JSC$asm_finalize ();

      var result = JSC$asm_bytecode ();

      /* Create the output file name. */
      var oname = ARGS[i].replace (/\.jas$/, ".jsc");
      if (oname == ARGS[i])
	  oname += ".jsc";

      var ostream = new File (oname);
      if (ostream.open ("w"))
	{
	  ostream.write (result);
	  ostream.close ();
	}
      else
	{
	  System.stderr.writeln ("jsas: couldn't create bc file `"
				 + oname + "': "
				 + System.strerror (System.errno));
	  System.exit (1);
	}
    }
}

function usage ()
{
  System.print ("\
Usage: ", program, " [OPTION]... FILE...\n\
Mandatory arguments to long options are mandatory for short options too.\n");

  System.print ("\
  -g, --debug                generate debugging information\n\
  -h, --help                 print this help and exit\n\
  -O, --optimize             optimize the assembler code\n\
  -v, --verbose              tell what the assembler is doing\n\
  -V, --version              print version number\n\
");

  System.print ("\nReport bugs to mtr@ngs.fi.\n");
}

function version ()
{
  System.print ("NGS JavaScript assembler ", version_number, "\n");
  System.print ("\
Copyright (C) 1998 New Generation Software (NGS) Oy.\n\
NGS JavaScript Interpreter comes with NO WARRANTY, to the extent\n\
permitted by law.  You may redistribute copies of NGS JavaScript\n\
Interpreter under the terms of the GNU Library General Public License.\n\
For more information about these matters, see the files named COPYING.\n\
");
}

main ();


/*
Local variables:
mode: c
End:
*/
