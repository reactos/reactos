/*
 * Definitions for functions that must be re-implement to guarantee
 * the re-entrancy of the interpreter.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/rentrant.h,v $
 * $Id: rentrant.h,v 1.1 2004/01/10 20:38:18 arty Exp $
 */

#ifndef RENTRANT_H
#define RENTRANT_H

/* Time. */

void js_localtime (const time_t *clock, struct tm *result);

void js_gmtime (const time_t *clock, struct tm *result);

void js_asctime (const struct tm *tm, char *buffer, int buffer_length);

/* Drand48. */

void *js_drand48_create (JSVirtualMachine *vm);

void js_drand48_destroy (void *drand48_context);

void js_srand48 (void *drand48_context, long seed);

void js_drand48 (void *drand48_context, double *random_return);

#endif /* not RENTRANT_H */
