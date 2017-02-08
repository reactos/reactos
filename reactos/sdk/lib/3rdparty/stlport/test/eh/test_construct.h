/***********************************************************************************
  test_construct.h

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
#ifndef test_construct_H_
#define test_construct_H_

#include "Prefix.h"
#if defined (EH_NEW_HEADERS)
#  include <algorithm>
#  include <cassert>
#  include <cstdlib>
#else
#  include <algo.h>
#  include <assert.h>
#  include <stdlib.h>
#endif

USING_CSTD_NAME(size_t)

template <class T>
struct test_copy_construct {
  test_copy_construct() {
    gTestController.SetCurrentTestName("copy constructor");
  }

  void operator()( const T& t ) const {
    T aCopy( t );
    // Prevent simulated failures during verification
    gTestController.CancelFailureCountdown();
    //EH_ASSERT( aCopy == t );
    CheckInvariant(t);
  }
};

template <class T>
struct test_default_construct {
  test_default_construct() {
    gTestController.SetCurrentTestName("default constructor");
  }

  void operator()( int ) const {
    T t;
    CheckInvariant(t);
  }
};

template <class T>
struct test_construct_n {
  test_construct_n( size_t _n ) : n(_n+1) {
    gTestController.SetCurrentTestName("n-size constructor");
  }

  void operator()( int ) const {
    T t(n);
    CheckInvariant(t);
  }

  size_t n;
};

template <class T>
struct test_construct_n_instance {
  test_construct_n_instance( size_t _n ) : n(_n+1) {
    gTestController.SetCurrentTestName("n-size with instance constructor");
  }

  void operator()( int ) const {
    typedef typename T::value_type Value_type;
    Value_type Val = 0;
    T t( n, Val );
    CheckInvariant(t);
  }

  size_t n;
};

template <class T>
struct test_construct_pointer_range {
  test_construct_pointer_range( const typename T::value_type *first,
                                const typename T::value_type* last )
    : fItems( first ), fEnd( last ) {
    gTestController.SetCurrentTestName("pointer range constructor");
  }

  void operator()( int ) const {
    T t( fItems, fEnd );
    // Prevent simulated failures during verification
    gTestController.CancelFailureCountdown();
    CheckInvariant(t);
  }

  const typename T::value_type* fItems, *fEnd;
};

template <class T>
struct test_construct_iter_range {

  test_construct_iter_range( const T& src ) : fItems( src ) {
    gTestController.SetCurrentTestName("iterator range constructor");
  }

  void operator()( int ) const {
    T t( fItems.begin(), fItems.end() );
    // Prevent simulated failures during verification
    gTestController.CancelFailureCountdown();
    EH_ASSERT( t == fItems );
    CheckInvariant(t);
  }

  const T& fItems;
};

#endif // test_construct_H_
