// Boris - this file is, um, rather incomplete. Please remove from distribution.

/***********************************************************************************
  test_string.cpp

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
#include "Prefix.h"
#if defined( EH_VALARRAY_IMPLEMENTED )
#include "Tests.h"
#include <valarray>
#include "TestClass.h"
#include "LeakCheck.h"
#include "test_construct.h"
#include "test_assign_op.h"
#include "test_push_back.h"
#include "test_insert.h"
#include "test_push_front.h"

typedef __valarray__<TestClass, eh_allocator(TestClass) > TestValarray;

inline sequence_container_tag
container_category(const TestValarray&)
{
  return sequence_container_tag();
}

void test_rope()
{
    TestValarray testValarray, testValarray2;
    size_t ropeSize = random_number(random_base);

    while ( testValarray.size() < ropeSize )
    {
        TestValarray::value_type x = random_number(random_base) ;  // initialize before use
        testValarray.push_back( x );
        testValarray2.push_back( TestValarray::value_type() );
    }
    WeakCheck( testValarray, test_insert_one<TestValarray>(testValarray) );
    WeakCheck( testValarray, test_insert_one<TestValarray>(testValarray, 0) );
    WeakCheck( testValarray, test_insert_one<TestValarray>(testValarray, testValarray.size()) );

    WeakCheck( testValarray, test_insert_n<TestValarray>(testValarray, random_number(random_base) ) );
    WeakCheck( testValarray, test_insert_n<TestValarray>(testValarray, random_number(random_base), 0 ) );
    WeakCheck( testValarray, test_insert_n<TestValarray>(testValarray, random_number(random_base), testValarray.size() ) );

    size_t insCnt = random_number(random_base);
    TestValarray::value_type *insFirst = new TestValarray::value_type[1+insCnt];

    WeakCheck( testValarray, insert_range_tester(testValarray, insFirst, insFirst+insCnt) );
    WeakCheck( testValarray, insert_range_at_begin_tester(testValarray, insFirst, insFirst+insCnt) );
    WeakCheck( testValarray, insert_range_at_end_tester(testValarray, insFirst, insFirst+insCnt) );

    ConstCheck( 0, test_construct_pointer_range<TestValarray>(insFirst, insFirst+insCnt) );
    delete[] insFirst;

    WeakCheck( testValarray, insert_range_tester(testValarray, testValarray2.begin(), testValarray2.end() ) );

    WeakCheck( testValarray, test_push_front<TestValarray>(testValarray) );
    WeakCheck( testValarray, test_push_back<TestValarray>(testValarray) );

    ConstCheck( 0, test_default_construct<TestValarray>() );
    ConstCheck( 0, test_construct_n<TestValarray>( random_number(random_base) ) );
    ConstCheck( 0, test_construct_n_instance<TestValarray>( random_number(random_base) ) );
    ConstCheck( 0, test_construct_iter_range<TestValarray>( testValarray2 ) );
    ConstCheck( testValarray, test_copy_construct<TestValarray>() );

    WeakCheck( testValarray, test_assign_op<TestValarray>( testValarray2 ) );
}

#endif // EH_ROPE_IMPLEMENTED
