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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsas/main.js,v $
 * $Id: main.js,v 1.1 2004/01/10 20:38:17 arty Exp $
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
