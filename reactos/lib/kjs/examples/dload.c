/*
 * An example of a shared library extension for js.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/examples/dload.c,v $
 * $Id: dload.c,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

#include <js.h>
#include <stdio.h>

/*
 * Static functions.
 */

/* The `hello' command. */
JSMethodResult
dlhello_proc (void *context, JSInterpPtr interp, int argc, JSType *argv,
	      JSType *result_return, char *error_return)
{
  char *msg = context;

  printf ("%s\n", msg);

  return JS_OK;
}



/*
 * Global functions.
 */

void
dload (JSInterpPtr interp)
{
  js_create_global_method (interp, "dlhello", dlhello_proc,
			   "dlhello: Hello, world!", NULL);
}
