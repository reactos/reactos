/*
 * Standard non re-entrant versions of the re-entrant functions.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/r_std.c,v $
 * $Id: r_std.c,v 1.1 2004/01/10 20:38:18 arty Exp $
 */

#include "jsint.h"
#include "rentrant.h"

/*
 * Types and definitions.
 */

/*
 * Global functions.
 */

/* Time. */

void
js_localtime (const time_t *clock, struct tm *result)
{
  struct tm *tm = localtime (clock);
  memcpy (result, tm, sizeof (*tm));
}


void
js_gmtime (const time_t *clock, struct tm *result)
{
  struct tm *tm = gmtime (clock);
  memcpy (result, tm, sizeof (*tm));
}


void
js_asctime (const struct tm *tm, char *buffer, int buffer_length)
{
  char *cp = asctime (tm);
  strcpy (buffer, cp);
}


/* Drand48. */

void *
js_drand48_create (JSVirtualMachine *vm)
{
  return NULL;
}


void
js_drand48_destroy (void *drand48_context)
{
}


void
js_srand48 (void *drand48_context, long seed)
{
#if HAVE_DRAND48
  srand48 (seed);
#else /* not HAVE_DRAND48 */
  srand (seed);
#endif /* not HAVE_DRAND48 */
}

void
js_drand48 (void *drand48_context, double *random_return)
{
#if HAVE_DRAND48
  *random_return = drand48 ();
#else /* not HAVE_DRAND48 */
  *random_return = (double ) rand () / (double) INT_MAX;
#endif /* not HAVE_DRAND48 */
}
