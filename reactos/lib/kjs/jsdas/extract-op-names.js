#!../src/js
/*
 * Extract byte-code operand names from the operands.def file.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsdas/extract-op-names.js,v $
 * $Id: extract-op-names.js,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

source = "../src/operands.def";
target = "operands.js";

op_re = new RegExp ("\
^operand[ \t]+([a-zA-Z_][a-zA-Z0-9_]*)[ \t]+([^ \t]+)[ \t]+{(.*)");

function main ()
{
  var ifp = new File (source);
  var ofp = new File (target);
  var count = 0;

  if (ifp.open ("r"))
    {
      if (ofp.open ("w"))
	{
	  header (ofp);

	  while (!ifp.eof ())
	    {
	      var line = ifp.readln ();
	      if (op_re.test (line))
		operand (ofp, RegExp.$1, RegExp.$2, RegExp.$3, count++);
	    }

	  ofp.close ();
	}
      else
	{
	  ifp.close ();
	  System.error ("Couldn't create target file `", target, "':",
			System.strerror (System.errno), "\n");
	  System.exit (1);
	}

      ifp.close ();
    }
  else
    {
      System.error ("Couldn't open source file `", source, "':",
		    System.strerror (System.errno), "\n");
      System.exit (1);
    }
}

function header (fp)
{
  fp.write ("\
/*                                                              -*- c -*-
 * Operand definitions for the JavaScript byte-code.
 *
 * This file is automatically create from the operands.def file.
 * Editing is strongly discouraged.  You should edit the file
 * `extract-op-names.js' instead.
 */

DASM$op_names = new Array ();
DASM$op_data = new Array ();
DASM$op_flags = new Array ();
");
}


function operand (ofp, name, data, flags, count)
{
  var f = 0;

  if (/symbol/.test (flags))
    f |= 0x01;
  if (/jump/.test (flags))
    f |= 0x02;

  ofp.write ("\
DASM$op_names[" + count.toString () + "]\t= \"" + name + "\";
DASM$op_data[" + count.toString () + "] \t= " + data + ";
DASM$op_flags[" + count.toString () + "] \t= 0x" + f.toString (16) + ";
");
}

main ();


/*
Local variables:
mode: c
End:
*/
