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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/mrgsort.h,v $
 * $Id: mrgsort.h,v 1.1 2004/01/10 20:38:18 arty Exp $
 */

#ifndef MERGESORT_H
#define MERGESORT_H

/*
 * Types and definitions.
 */

typedef int (*MergesortCompFunc) (const void *a, const void *b,
				  void *context);

/*
 * Prototypes for global functions.
 */

void mergesort_r (void *base, unsigned int number_of_elements,
		  unsigned int size, MergesortCompFunc comparison_func,
		  void *comparison_func_context);


#endif /* not MERGESORT_H */
