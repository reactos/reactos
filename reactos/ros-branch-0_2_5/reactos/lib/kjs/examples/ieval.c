/*
 * Show how the evaluation can be done in phases.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/examples/ieval.c,v $
 * $Id: ieval.c,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <js.h>

/*
 * Global functions.
 */

int
main (int argc, char *argv[])
{
  char buf[1024];
  JSInterpPtr interp;

  /* Create one interpreter. */
  interp = js_create_interp (NULL);

  /* Enter the query loop. */
  while (1)
    {
      unsigned char *bc;
      unsigned char *bc_copy;
      unsigned int bc_len;
      char *cp;

      printf ("What file should I evaluate? ");

      if (fgets (buf, sizeof (buf), stdin) == NULL)
	break;

      cp = strchr (buf, '\n');
      if (cp)
	*cp = '\0';

      if (js_compile_to_byte_code (interp, buf, &bc, &bc_len) == 0)
	{
	  printf ("Compilation to the byte-code failed: %s\n",
		  js_error_message (interp));
	  continue;
	}

      printf ("Compilation returned %d bytes of byte-code data.\n",
	      bc_len);

      bc_copy = malloc (bc_len);
      assert (bc_copy != NULL);
      memcpy (bc_copy, bc, bc_len);

      while (1)
	{
	  printf ("Hit ENTER to execute the code.  ^D exits the loop. ");
	  if (getchar () == EOF)
	    break;

	  if (js_execute_byte_code (interp, bc_copy, bc_len) == 0)
	    printf ("Execution failed: %s\n", js_error_message (interp));
	}

      free (bc_copy);
      printf ("\n");
    }

  printf ("\n");

  js_destroy_interp (interp);

  return 1;
}
