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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsas/process.js,v $
 * $Id: process.js,v 1.1 2004/01/10 20:38:17 arty Exp $
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
