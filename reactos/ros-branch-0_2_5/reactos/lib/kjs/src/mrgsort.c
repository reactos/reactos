/*
 * Re-entrant mergesort.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/mrgsort.c,v $
 * $Id: mrgsort.c,v 1.1 2004/01/10 20:38:18 arty Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "mrgsort.h"

/*
 * Types and definitions.
 */

#define COPY(buf1, idx1, buf2, idx2)	\
  memcpy ((buf1) + (idx1) * size, (buf2) + (idx2) * size, size);

/*
 * Static functions.
 */

static void
do_mergesort (unsigned char *base, unsigned int size, unsigned char *tmp,
	      unsigned int l, unsigned int r,
	      MergesortCompFunc func, void *func_ctx)
{
  unsigned int i, j, k, m;

  if (r <= l)
    return;

  m = (r + l) / 2;
  do_mergesort (base, size, tmp, l, m, func, func_ctx);
  do_mergesort (base, size, tmp, m + 1, r, func, func_ctx);

  memcpy (tmp + l * size, base + l * size, (r - l + 1) * size);

  i = l;
  j = m + 1;
  k = l;

  /* Merge em. */
  while (i <= m && j <= r)
    {
      if ((*func) (tmp + i * size, tmp + j * size, func_ctx) <= 0)
	{
	  COPY (base, k, tmp, i);
	  i++;
	}
      else
	{
	  COPY (base, k, tmp, j);
	  j++;
	}
      k++;
    }

  /* Copy left-overs.  Only one of the following will be executed. */
  for (; i <= m; i++)
    {
      COPY (base, k, tmp, i);
      k++;
    }
  for (; j <= r; j++)
    {
      COPY (base, k, tmp, j);
      k++;
    }
}


/*
 * Global functions.
 */

void
mergesort_r (void *base, unsigned int number_of_elements,
	     unsigned int size, MergesortCompFunc comparison_func,
	     void *comparison_func_context)
{
  void *tmp;

  if (number_of_elements == 0)
    return;

  /* Allocate tmp buffer. */
  tmp = malloc (number_of_elements * size);
  assert (tmp != NULL);

  do_mergesort (base, size, tmp, 0, number_of_elements - 1, comparison_func,
		comparison_func_context);

  free (tmp);
}
