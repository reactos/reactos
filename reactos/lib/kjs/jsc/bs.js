/*
 * Bootstrap file for the JavaScript compiler.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/jsc/bs.js,v $
 * $Id: bs.js,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

/*
 * To run the compiler from vm, just call the entry point with fixed
 * arguments.
 */
VM.verbose = 0;

function compile ()
{
  var file = "a.js";
  var verbose = JSC$FLAG_VERBOSE;

  if (ARGS.length == 2)
    {
      file = ARGS[1];
      verbose = 0;
    }

  JSC$compile_file (file,
		    verbose
		    | JSC$FLAG_ANNOTATE_ASSEMBLER
		    | JSC$FLAG_GENERATE_DEBUG_INFO
		    | JSC$FLAG_OPTIMIZE_MASK
		    | JSC$FLAG_WARN_MASK & ~JSC$FLAG_WARN_MISSING_SEMICOLON,
		    "./a.jas",
		    "./a.jsc");
}

compile ();



/*
Local variables:
mode: c
End:
*/
