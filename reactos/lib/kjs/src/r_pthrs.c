/*
 * Re-entrant functions from the Posix thread library.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/r_pthrs.c,v $
 * $Id: r_pthrs.c,v 1.1 2004/01/10 20:38:18 arty Exp $
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
  localtime_r (clock, result);
}


void
js_gmtime (const time_t *clock, struct tm *result)
{
  gmtime_r (clock, result);
}


void
js_asctime (const struct tm *tm, char *buffer, int buffer_length)
{
  asctime_r (tm, buffer
#if ASCTIME_R_WITH_THREE_ARGS
	     , buffer_length
#endif /* ASCTIME_R_WITH_THREE_ARGS */
	     );
}


/* Drand48. */

#if DRAND48_R_WITH_DRAND48D

void *
js_drand48_create (JSVirtualMachine *vm)
{
  return js_malloc (vm, sizeof (DRAND48D));
}


void
js_drand48_destroy (void *drand48_context)
{
  js_free (drand48_context);
}


void
js_srand48 (void *drand48_context, long seed)
{
  DRAND48D *ctx = drand48_context;
  srand48_r (seed, ctx);
}

void
js_drand48 (void *drand48_context, double *random_return)
{
  DRAND48D *ctx = drand48_context;
  drand48_r (ctx, random_return);
}

#else /* not DRAND48_R_WITH_DRAND48D */

/* We trust the drand48() to be thread-safe. */

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
  srand48 (seed);
}

void
js_drand48 (void *drand48_context, double *random_return)
{
  *random_return = drand48 ();
}

#endif /* not DRAND48_R_WITH_DRAND48D */
