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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsc/entry.js,v $
 * $Id: entry.js,v 1.1 2004/01/10 20:38:17 arty Exp $
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
