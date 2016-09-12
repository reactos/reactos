/***********************************************************************************
  random_number.cpp

 * Copyright (c) 1997
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
#include "random_number.h"
#include "Prefix.h"
#if defined (EH_NEW_HEADERS)
# include <functional>
# include <cstdlib>
#else
# include <function.h>
# include <stdlib.h>
#endif

unsigned random_number( size_t range )
{
#if !defined( __SGI_STL )
  if (range == 0) return 0;
  return (unsigned)(EH_STD::rand() + EH_STD::rand()) % range;
#else
  static EH_STD::subtractive_rng rnd;
        if (range==0) return 0;
        return rnd(range);
#endif
}

// default base for random container sizes
unsigned random_base = 1000;
