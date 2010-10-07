/***********************************************************************************
  test_set.cpp

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
#include "TestClass.h"
#include "LeakCheck.h"
#if defined (EH_NEW_HEADERS)
#  include <set>
#else
#  include <multiset.h>
#  include <set.h>
#endif
#include "test_construct.h"
#include "test_assign_op.h"
#include "test_push_back.h"
#include "test_insert.h"
#include "test_push_front.h"
#include "ThrowCompare.h"

void test_multiset();

typedef EH_STD::__multiset__<TestClass, ThrowCompare, eh_allocator(TestClass) > TestMultiSet;

inline multiset_tag
container_category(const TestMultiSet&) {
  return multiset_tag();
}

void test_multiset() {
  TestMultiSet testMultiSet, testMultiSet2;

  const size_t setSize = random_number(random_base);

  while (testMultiSet.size() < setSize) {
    TestMultiSet::value_type x;
    testMultiSet.insert( x );
    testMultiSet2.insert( TestMultiSet::value_type() );
  }

  StrongCheck( testMultiSet, test_insert_value<TestMultiSet>(testMultiSet) );

  size_t insCnt = random_number(random_base);
  TestMultiSet::value_type *insFirst = new TestMultiSet::value_type[1+insCnt];
  WeakCheck( testMultiSet, insert_range_tester(testMultiSet, insFirst, insFirst+insCnt) );
  ConstCheck( 0, test_construct_pointer_range<TestMultiSet>(insFirst, insFirst+insCnt) );
  delete[] insFirst;
  WeakCheck( testMultiSet, insert_range_tester(testMultiSet, testMultiSet2.begin(), testMultiSet2.end() ) );

  ConstCheck( 0, test_default_construct<TestMultiSet>() );
  ConstCheck( 0, test_construct_iter_range<TestMultiSet>( testMultiSet2 ) );
  ConstCheck( testMultiSet, test_copy_construct<TestMultiSet>() );

  WeakCheck( testMultiSet, test_assign_op<TestMultiSet>( testMultiSet2 ) );
}

typedef EH_STD::__set__<TestClass, ThrowCompare, eh_allocator(TestClass) > TestSet;

inline set_tag
container_category(const TestSet&) {
  return set_tag();
}

void test_set() {
  TestSet testSet, testSet2;

  const size_t setSize = random_number(random_base);

  while ( testSet.size() < setSize ) {
    TestSet::value_type x;
    testSet.insert( x );
    testSet2.insert( TestSet::value_type() );
  }
  StrongCheck( testSet, test_insert_value<TestSet>(testSet) );

  size_t insCnt = random_number(random_base);
  TestSet::value_type *insFirst = new TestSet::value_type[1+insCnt];

  WeakCheck( testSet, insert_range_tester(testSet, insFirst, insFirst+insCnt) );

  ConstCheck( 0, test_construct_pointer_range<TestSet>(insFirst, insFirst+insCnt) );
  delete[] insFirst;
  WeakCheck( testSet, insert_range_tester(testSet, testSet2.begin(), testSet2.end() ) );

  ConstCheck( 0, test_default_construct<TestSet>() );
  ConstCheck( 0, test_construct_iter_range<TestSet>( testSet2 ) );
  ConstCheck( testSet, test_copy_construct<TestSet>() );
  WeakCheck( testSet, test_assign_op<TestSet>( testSet2 ) );
}
