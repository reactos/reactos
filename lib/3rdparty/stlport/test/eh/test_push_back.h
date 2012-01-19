/***********************************************************************************
  test_push_back.h

    Interface for the test_push_back class

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
#ifndef test_push_back_H_
#define test_push_back_H_
# ifdef EH_NEW_HEADERS
#  include <cassert>
# else
#  include <assert.h>
# endif

# include "Prefix.h"
#include "nc_alloc.h"

template <class C>
struct test_push_back
{
  test_push_back( const C& orig ) : original( orig )
  {
        gTestController.SetCurrentTestName("push_back() method");
    }

  void operator()( C& c ) const
  {
      typedef typename C::value_type _value_type;
    c.push_back(_value_type() );
    // Prevent simulated failures during verification
        gTestController.CancelFailureCountdown();
    EH_ASSERT( c.size() == original.size() + 1 );
    EH_ASSERT( EH_STD::equal( original.begin(), original.end(), c.begin() ) );
  }
private:
  const C& original;
};

#endif // test_push_back_H_
