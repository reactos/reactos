/*
 * Make C-string from a binary data file.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/make-data.c,v $
 * $Id: make-data.c,v 1.1 2004/01/10 20:38:18 arty Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

int
main (int argc, char *argv[])
{
  FILE *ifp, *ofp;
  unsigned int pos;
  int ch, i;

  if (argc != 4)
    {
      fprintf (stderr, "Usage: %s DATANAME INPUT OUTPUT\n", argv[0]);
      exit (1);
    }

  ifp = fopen (argv[2], "r");
  if (ifp == NULL)
    {
      fprintf (stderr, "%s: couldn't open input file \"%s\": %s\n",
	       argv[0], argv[2], strerror (errno));
      exit (1);
    }

  ofp = fopen (argv[3], "w");
  if (ofp == NULL)
    {
      fprintf (stderr, "%s: couldn't create output file \"%s\": %s\n",
	       argv[0], argv[3], strerror (errno));
      exit (1);
    }

  fprintf (ofp, "unsigned char %s[] = {", argv[1]);

  pos = 0;

  while ((ch = getc (ifp)) != EOF)
    {
      if ((pos % 8) == 0)
	fprintf (ofp, "\n ");
      fprintf (ofp, " 0x%02x,", ch);
      pos++;
    }

  fprintf (ofp, "\n};\n");
  fprintf (ofp, "unsigned int %s_len = %u;\n", argv[1], pos);

  for (i = 0; argv[1][i]; i++)
    if (islower (argv[1][i]))
      argv[1][i] = toupper (argv[1][i]);

  fprintf (ofp, "#define %s_LEN %u\n", argv[1], pos);

  fclose (ifp);
  fclose (ofp);

  return 0;
}
