/*
 * Print CC properties.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/micros/ccprops.c,v $
 * $Id: ccprops.c,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

#include <stdio.h>

/*
 * Static functions.
 */

static void
sizes ()
{
  printf ("\n* The sizes of different types\n\n");
  printf ("\ttype\tsize\n\t--------------------\n");
  printf ("\
\tchar\t%d\n\
\tshort\t%d\n\
\tint\t%d\n\
\tlong\t%d\n\
\tfloat\t%d\n\
\tdouble\t%d\n\
\tvoid *\t%d\n",
	  sizeof (char),
	  sizeof (short),
	  sizeof (int),
	  sizeof (long),
	  sizeof (float),
	  sizeof (double),
	  sizeof (void *));
}

static void
preprocessor ()
{
  printf ("\n* Preprocessor\n\n");
#ifdef __STDC__
  printf ("\t__STDC__=%d\n", __STDC__);
#else /* no __STDC__ */
  printf ("\tno __STDC__\n");
#endif /* no __STDC__ */
}

/*
 * Global functions.
 */

int
main (int argc, char *argv[])
{
  sizes ();
  preprocessor ();
}
