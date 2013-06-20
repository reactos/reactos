/***********************************************************************************
  random_number.h

 * Copyright (c) 1997-1998
 * Mark of the Unicorn, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Mark of the Unicorn makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.

***********************************************************************************/
#ifndef RANDOM_NUMBER_DWA120298_H_
#define RANDOM_NUMBER_DWA120298_H_

#include <stddef.h>

// Return a random number in the given range.
unsigned random_number( size_t range );

// default base for random container sizes
extern unsigned random_base;

#endif // #include guard
