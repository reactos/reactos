/***********************************************************************************
  test_deque.cpp

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

#include "Tests.h"
# if defined (EH_NEW_HEADERS)
#  ifdef __SUNPRO_CC
#   include <stdio.h>
#  else
#   include <cstdio>
#  endif
#  include <deque>
# else
#  include <stdio.h>
#  include <deque.h>
# endif
#include "TestClass.h"
#include "LeakCheck.h"
#include "test_construct.h"
#include "test_assign_op.h"
#include "test_push_back.h"
#include "test_insert.h"
#include "test_push_front.h"

typedef TestClass DQTestClass;

typedef EH_STD::deque<DQTestClass, eh_allocator(DQTestClass) > TestDeque;

inline sequence_container_tag
container_category(const TestDeque&)
{
  return sequence_container_tag();
}

void test_deque()
{
    size_t dequeSize = random_number(random_base);
    TestDeque emptyDeque;
    TestDeque testDeque, testDeque2;
    while ( testDeque.size() < dequeSize )
    {
        DQTestClass x;
        testDeque.push_back( x );
        testDeque2.push_back( DQTestClass() );
    }

    ConstCheck( testDeque, test_copy_construct<TestDeque>() );
    WeakCheck( testDeque, test_insert_one<TestDeque>(testDeque) );
    StrongCheck( testDeque, test_insert_one<TestDeque>(testDeque, 0) );
    StrongCheck( testDeque, test_insert_one<TestDeque>(testDeque, (int)testDeque.size()) );

    WeakCheck( testDeque, test_insert_n<TestDeque>(testDeque, random_number(random_base) ) );
    StrongCheck( testDeque, test_insert_n<TestDeque>(testDeque, random_number(random_base), 0 ) );
    StrongCheck( testDeque, test_insert_n<TestDeque>(testDeque, random_number(random_base), (int)testDeque.size() ) );

    size_t insCnt = random_number(random_base);
    DQTestClass *insFirst = new TestDeque::value_type[insCnt + 1];

    WeakCheck( testDeque, insert_range_tester(testDeque, insFirst, insFirst + insCnt) );
    StrongCheck( testDeque, insert_range_at_begin_tester(testDeque, insFirst, insFirst + insCnt) );
    StrongCheck( testDeque, insert_range_at_end_tester(testDeque, insFirst, insFirst + insCnt) );

    ConstCheck( 0, test_construct_pointer_range<TestDeque>(insFirst, insFirst + insCnt) );
    delete[] insFirst;

    WeakCheck( testDeque, insert_range_tester(testDeque, testDeque2.begin(), testDeque2.end() ) );

    StrongCheck( testDeque, test_push_back<TestDeque>(testDeque) );
    StrongCheck( emptyDeque, test_push_back<TestDeque>(emptyDeque) );
    StrongCheck( testDeque, test_push_front<TestDeque>(testDeque) );
    StrongCheck( emptyDeque, test_push_front<TestDeque>(emptyDeque) );


    ConstCheck( 0, test_default_construct<TestDeque>() );
    ConstCheck( 0, test_construct_n<TestDeque>( random_number(random_base) ) );
    ConstCheck( 0, test_construct_n_instance<TestDeque>( random_number(random_base) ) );
    ConstCheck( 0, test_construct_iter_range<TestDeque>( testDeque2 ) );

    testDeque2.resize( testDeque.size() * 3 / 2 );
    WeakCheck( testDeque, test_assign_op<TestDeque>( testDeque2 ) );
    testDeque2.resize( testDeque.size() * 2 / 3 );
    WeakCheck( testDeque, test_assign_op<TestDeque>( testDeque2 ) );
}
