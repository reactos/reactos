/*
 * Disassembler for JavaScript byte-code files.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsdas/main.js,v $
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
 * -c, --code
 *
 * Print code section.
 */

opt_code = false;

/*
 * -C, --constants
 *
 * Print constants section.
 */

opt_constants = false;

/*
 * -d, --debug
 *
 * Print the debug section.
 */

opt_debug = false;

/*
 * -h, --help
 *
 * Print short help and exit successfully.
 */

/*
 * -i, --info
 *
 * Show general information about the file.
 */

opt_info = false;

/*
 * -l TYPE DATA, --link TYPE DATA
 *
 * Link a new section to the byte-code file.  The new section's type is
 * TYPE and its data is in file DATA.
 */

opt_link = false;
opt_link_type = 0;
opt_link_data = "";

/*
 * -r TYPE, --remove TYPE
 *
 * Remove section TYPE.
 */

opt_remove = false;
opt_remove_type = 0;

/*
 * -s, --symtab
 *
 * Print symtab section.
 */

opt_symtab = false;

/*
 * -S, --strip
 *
 * Remove the debug section from byte-code files.
 */

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
  var opt_default = true;

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
	  if (ARGS[i] == "-c" || ARGS[i] == "--code")
	    {
	      opt_code = true;
	      opt_default = false;
	    }
	  else if (ARGS[i] == "-C" || ARGS[i] == "--constants")
	    {
	      opt_constants = true;
	      opt_default = false;
	    }
	  else if (ARGS[i] == "-d" || ARGS[i] == "--debug")
	    {
	      opt_debug = true;
	      opt_default = false;
	    }
	  else if (ARGS[i] == "-i" || ARGS[i] == "--info")
	    {
	      opt_info = true;
	      opt_default = false;
	    }
	  else if (ARGS[i] == "-l" || ARGS[i] == "--link")
	    {
	      opt_link = true;
	      opt_default = false;

	      if (i + 2 >= ARGS.length)
		{
		  System.error (program, ": no arguments for option --link\n");
		  System.exit (1);
		}

	      opt_link_type = parseInt (ARGS[++i]);
	      if (isNaN (opt_link_type))
		{
		  System.error (program, ": illegal section type `",
				ARGS[i], "'\n");
		  System.exit (1);
		}
	      opt_link_data = (ARGS[++i]);
	    }
	  else if (ARGS[i] == "-r" || ARGS[i] == "--remove")
	    {
	      opt_remove = true;
	      opt_default = false;

	      if (i + 1 >= ARGS.length)
		{
		  System.error (program,
				": no arguments for option --remove\n");
		  System.exit (1);
		}

	      opt_remove_type = parseInt (ARGS[++i]);
	      if (isNaN (opt_remove_type))
		{
		  System.error (program, ": illegal section type `",
				ARGS[i], "'\n");
		  System.exit (1);
		}
	    }
	  else if (ARGS[i] == "-s" || ARGS[i] == "--symtab")
	    {
	      opt_symtab = true;
	      opt_default = false;
	    }
	  else if (ARGS[i] == "-S" || ARGS[i] == "--strip")
	    {
	      opt_remove = true;
	      opt_default = false;
	      opt_remove_type = JSC$BC_SECT_DEBUG;
	    }
	  else if (ARGS[i] == "-h" || ARGS[i] == "--help")
	    {
	      usage ();
	      System.exit (0);
	    }
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

  /* Process files. */
  var first = true;
  for (; i < ARGS.length; i++)
    {
      if (first)
	first = false;
      else
	System.print ("\n");

      System.print (ARGS[i], ":\n");
      var is = new File (ARGS[i]);
      if (is.open ("r"))
	{
	  var bc = new ByteCode ();
	  var result = bc.parse (is);
	  is.close ();

	  if (!result)
	    System.print (program, ": couldn't parse byte-code file `",
			  ARGS[i], "'\n");
	  else
	    {
	      var write_back = false;

	      if (opt_info)
		{
		  System.print ("\n* byte-code file information\n\n");
		  bc.printInfo ();
		}

	      if (opt_constants)
		{
		  System.print ("\n* section `Constants'\n\n");
		  bc.printConstants ();
		}

	      if (opt_default || opt_code)
		{
		  System.print ("\n* section `Code'\n\n");
		  bc.printCode ();
		}

	      if (opt_symtab)
		{
		  System.print ("\n* section `Symtab'\n\n");
		  bc.printSymtab ();
		}

	      if (opt_debug)
		{
		  System.print ("\n* section `Debug'\n\n");
		  bc.printDebug ();
		}

	      if (opt_link)
		{
		  var df = new File (opt_link_data);
		  if (df.open ("r"))
		    {
		      var len = df.getLength ();
		      var data = df.read (len);
		      df.close ();

		      if (data.length < len)
			System.error (program,
				      ": couldn't read data from file `",
				      opt_link_data, "': ",
				      System.strerror (System.errno), "\n");
		      else
			{
			  System.print (program, ": linking ", data.length,
					" bytes of data to section ",
					opt_link_type.toString (), "\n");
			  bc.linkSection (opt_link_type, data);
			  write_back = true;
			}
		    }
		  else
		    System.error (program, ": couldn't open data file `",
				  opt_link_data, "': ",
				  System.strerror (System.errno), "\n");
		}

	      if (opt_remove)
		{
		  System.print (program, ": removing section ",
				opt_remove_type, "\n");
		  if (bc.removeSection (opt_remove_type))
		    write_back = true;
		}

	      if (write_back)
		{
		  /* Write the byte-code file back to its original file. */
		  if (!bc.write (ARGS[i]))
		    {
		      System.error (program, ": write failed: ",
				    System.strerror (System.errno),
				    "\n");
		      System.exit (1);
		    }
		}
	    }
	}
      else
	System.error (program, ": couldn't open bc file `", ARGS[i], "': ",
		      System.strerror (System.errno), "\n");
    }
}


function usage ()
{
  System.print ("\
Usage: ", program, " [OPTION]... FILE...\n\
Mandatory arguments to long options are mandatory for short options too.\n");

  System.print ("\
  -c, --code                 print code section (default)\n\
  -C, --constants            print constants section\n\
  -d, --debug                print debug section\n\
  -h, --help                 print this help and exit\n\
  -i, --info                 print the byte-code file header and general\n\
                             information about the sections\n\
  -l, --link TYPE DATA       link data from file DATA to section TYPE\n\
  -r, --remove TYPE          remove section TYPE\n\
  -s, --symtab               print symtab section\n\
  -S, --strip                remove debug section\n\
  -V, --version              print version number\n\
");

  System.print ("\nReport bugs to mtr@ngs.fi.\n");
}


function version ()
{
  System.print ("NGS JavaScript disassembler ", version_number, "\n");
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
