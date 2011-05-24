/***********************************************************************************
  test_hash_resize.h

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
#ifndef test_hash_resize__
#define test_hash_resize__

#include "random_number.h"
#include "nc_alloc.h"

template <class T>
struct test_hash_resize
{
    test_hash_resize()
    {
      gTestController.SetCurrentTestName("hash resize");
    }

  void operator()( T& t ) const
  {
    t.resize( 1+random_number(random_base) + t.bucket_count() );
  }
};

template <class T>
struct test_construct_iter_range_n
{
  test_construct_iter_range_n( const T& src )
    : fItems( src )
  {
        gTestController.SetCurrentTestName("iterator range n-size constructor");
    }

  void operator()( int ) const
  {
    T t( fItems.begin(), fItems.end(), fItems.size() );
    // prevent simulated failures during verification
    gTestController.CancelFailureCountdown();
    CheckInvariant(t);
  }

  const T& fItems;
};

#endif //__test_hash_resize__
