/*
 * JS C wrapper.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jswrap/main.js,v $
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
 * Generate debugging information to the generated JS byte-code.
 */

opt_debug = false;

/*
 * -h FILE, --header FILE
 *
 * Generate the C header to file FILE.
 */

header_file = null;

/*
 * -n, --no-error-handler
 *
 * Do not create the default error handler to the C files.
 */
opt_no_error_handler = false;

/*
 * -o FILE, --output FILE
 *
 * Generate the C output to file FILE.
 */

output_file = null;

/*
 * -r, --reentrant
 *
 * Generate re-entrant C stubs.
 */

opt_reentrant = false;

/*
 * -V, --version
 *
 * Print version information and exit successfully.
 */

/*
 * --help
 *
 * Print short help and exit successfully.
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
	  else if (ARGS[i] == "-h" || ARGS[i] == "--header")
	    {
	      if (i + 1 >= ARGS.length)
		{
		  System.error (program,
				": no argument for option --header\n");
		  System.exit (1);
		}

	      header_file = ARGS[++i];
	    }
	  else if (ARGS[i] == "-n" || ARGS[i] == "--no-error-handler")
	    opt_no_error_handler = true;
	  else if (ARGS[i] == "-o" || ARGS[i] == "--output")
	    {
	      if (i + 1 >= ARGS.length)
		{
		  System.error (program,
				": no argument for option --output\n");
		  System.exit (1);
		}

	      output_file = ARGS[++i];
	    }
	  else if (ARGS[i] == "-r" || ARGS[i] == "--reentrant")
	    opt_reentrant = true;
	  else if (ARGS[i] == "-V" || ARGS[i] == "--version")
	    {
	      version ();
	      System.exit (0);
	    }
	  else if (ARGS[i] == "--help")
	    {
	      usage ();
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
      System.error  (program, ": no input file specified\n");
      System.exit (1);
    }
  if (i + 1 < ARGS.length)
    {
      System.error (program, ": extraneous arguments after input file\n");
      System.exit (1);
    }

  input_file = ARGS[i];

  /* Set the defaults. */

  var re = new RegExp ("^(.*)\\.jsw$");
  if (!header_file)
    {
      if (re.test (input_file))
	header_file = RegExp.$1 + ".h";
      else
	header_file = input_file + ".h";
    }
  if (!output_file)
    {
      if (re.test (input_file))
	output_file = RegExp.$1 + ".c";
      else
	output_file = input_file + ".c";
    }
  if (false)
    {
      if (re.test (input_file))
	js_file = RegExp.$1 + ".js";
      else
	js_file = input_file + ".js";
    }

  process ();
}


function usage ()
{
  System.print ("\
Usage: ", program, " [OPTION]... FILE\n\
Mandatory arguments to long options are mandatory for short options too.\n");

  System.print ("\
  -g, --debug                generate debugging information to the generated\n\
                             JS byte-code\n\
  -h, --header FILE          generate the C header to file FILE\n\
  -n, --no-error-handler     do not generate the default error handler\n\
  -o, --output FILE          generate the C output to file FILE\n\
  -r, --reentrant            generate re-entrant C stubs\n\
  -V, --version              print version number\n\
  --help                     print this help and exit\n\
");

  System.print ("\nReport bugs to mtr@ngs.fi.\n");
}


function version ()
{
  System.print ("NGS JavaScript C wrapper generator ", version_number, "\n");
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
