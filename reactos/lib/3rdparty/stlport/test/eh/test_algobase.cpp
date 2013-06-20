/***********************************************************************************
  test_algobase.cpp

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

# include "Prefix.h"
# if defined (EH_NEW_HEADERS)
# ifdef __SUNPRO_CC
# include <stdio.h>
# endif
#include <algorithm>
# else
#include <algo.h>
# endif
#include "Tests.h"
#include "LeakCheck.h"
#include "TestClass.h"

// EH_USE_STD

enum { kBufferSize = 100 };

struct test_uninitialized_copy
{
    test_uninitialized_copy()
        : stuff( new TestClass[kBufferSize] ), end_of_stuff(stuff + kBufferSize) {
        gTestController.SetCurrentTestName("uninitialized_copy()");
        }

    ~test_uninitialized_copy() { delete[] stuff; }

    void operator()( TestClass* buffer ) const
    {
        EH_STD::uninitialized_copy((TestClass*)stuff, (TestClass*)end_of_stuff, buffer );
        EH_ASSERT( EH_STD::equal( (TestClass*)stuff, (TestClass*)end_of_stuff, buffer ) );
        stl_destroy( buffer, buffer+kBufferSize );
    }

private:
    TestClass * stuff;
    TestClass * end_of_stuff;
};

struct test_uninitialized_fill
{
    test_uninitialized_fill() {
        gTestController.SetCurrentTestName("uninitialized_fill()");
    }

    void operator()( TestClass* buffer ) const
    {
        TestClass* buf_end = buffer + kBufferSize;
        EH_STD::uninitialized_fill( buffer, buf_end, testValue );
        for ( EH_CSTD::size_t i = 0; i < kBufferSize; i++ )
            EH_ASSERT( buffer[i] == testValue );
        stl_destroy( buffer, buf_end );
    }
private:
    TestClass testValue;
};

struct test_uninitialized_fill_n
{
    test_uninitialized_fill_n() {
        gTestController.SetCurrentTestName("uninitialized_fill_n()");
    }
    void operator()( TestClass* buffer ) const
    {
        TestClass* end = buffer + kBufferSize;
        EH_STD::uninitialized_fill_n( buffer, (EH_CSTD::size_t)kBufferSize, testValue );
        for ( EH_CSTD::size_t i = 0; i < kBufferSize; i++ )
            EH_ASSERT( buffer[i] == testValue );
        stl_destroy( buffer, end );
    }
private:
    TestClass testValue;
};

void test_algobase()
{
  // force alignment
  double arr[ sizeof(TestClass) * kBufferSize ];
  TestClass* c = (TestClass*)arr;
  WeakCheck( c, test_uninitialized_copy() );
  WeakCheck( c, test_uninitialized_fill() );
  WeakCheck( c, test_uninitialized_fill_n() );
}
