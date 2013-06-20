/***********************************************************************************
  test_hash_set.cpp

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

#if defined( EH_HASHED_CONTAINERS_IMPLEMENTED )

#  include <hash_set>

#include "TestClass.h"
#include "LeakCheck.h"
#include "test_construct.h"
#include "test_assign_op.h"
#include "test_push_back.h"
#include "test_insert.h"
#include "test_push_front.h"
#include "ThrowCompare.h"
#include "test_hash_resize.h"

typedef EH_STD::__hash_multiset__<TestClass, ThrowHash, ThrowEqual,
  eh_allocator(TestClass) > TestMultiSet;

inline multiset_tag
container_category(const TestMultiSet&)
{
  return multiset_tag();
}

void test_hash_multiset()
{
# if !(defined (_MSC_VER) && (_MSC_VER < 1100))
  TestMultiSet testMultiSet, testMultiSet2;

        const size_t hash_setSize = random_number(random_base);

  while ( testMultiSet.size() < hash_setSize )
  {
    TestMultiSet::value_type x;
    testMultiSet.insert( x );
    testMultiSet2.insert( TestMultiSet::value_type() );
  }

#  if defined( EH_HASH_CONTAINERS_SUPPORT_RESIZE )
  WeakCheck( testMultiSet, test_hash_resize<TestMultiSet>() );
  // TestMultiSet == TestMultiSet: no such operator! - ptr
  // StrongCheck( testMultiSet, test_insert_noresize<TestMultiSet>(testMultiSet) );
#  endif
  WeakCheck( testMultiSet, test_insert_value<TestMultiSet>(testMultiSet) );

  size_t insCnt = random_number(random_base);
  TestMultiSet::value_type *insFirst = new TestMultiSet::value_type[1+insCnt];
  WeakCheck( testMultiSet, insert_range_tester(testMultiSet, insFirst, insFirst+insCnt) );
  ConstCheck( 0, test_construct_pointer_range<TestMultiSet>(insFirst, insFirst+insCnt) );
  delete[] insFirst;

  WeakCheck( testMultiSet, insert_range_tester(testMultiSet, testMultiSet2.begin(), testMultiSet2.end() ) );

  ConstCheck( 0, test_default_construct<TestMultiSet>() );
#  if EH_HASH_CONTAINERS_SUPPORT_ITERATOR_CONSTRUCTION
  ConstCheck( 0, test_construct_iter_range_n<TestMultiSet>( testMultiSet2 ) );
#  endif
  ConstCheck( testMultiSet, test_copy_construct<TestMultiSet>() );

  WeakCheck( testMultiSet, test_assign_op<TestMultiSet>( testMultiSet2 ) );
# endif
}

typedef EH_STD::__hash_set__<TestClass, ThrowHash, ThrowEqual, eh_allocator(TestClass) > TestSet;

inline set_tag
container_category(const TestSet&)
{
  return set_tag();
}

void test_hash_set()
{
# if !(defined (_MSC_VER) && (_MSC_VER < 1100))
  TestSet testSet, testSet2;

        const size_t hash_setSize = random_number(random_base);

  while ( testSet.size() < hash_setSize )
  {
    TestSet::value_type x;
    testSet.insert( x );
    testSet2.insert( TestSet::value_type() );
  }

#  if defined( EH_HASH_CONTAINERS_SUPPORT_RESIZE )
  WeakCheck( testSet, test_hash_resize<TestSet>() );
  // TestMultiSet == TestMultiSet: no such operator! - ptr
  // StrongCheck( testSet, test_insert_noresize<TestSet>(testSet) );
#  endif
  WeakCheck( testSet, test_insert_value<TestSet>(testSet) );

  size_t insCnt = random_number(random_base);
  TestSet::value_type *insFirst = new TestSet::value_type[1+insCnt];
  WeakCheck( testSet, insert_range_tester(testSet, insFirst, insFirst+insCnt) );
  ConstCheck( 0, test_construct_pointer_range<TestSet>(insFirst, insFirst+insCnt) );
  delete[] insFirst;

  WeakCheck( testSet, insert_range_tester(testSet, testSet2.begin(), testSet2.end() ) );

  ConstCheck( 0, test_default_construct<TestSet>() );
#  if EH_HASH_CONTAINERS_SUPPORT_ITERATOR_CONSTRUCTION
  ConstCheck( 0, test_construct_iter_range_n<TestSet>( testSet2 ) );
#  endif
  ConstCheck( testSet, test_copy_construct<TestSet>() );

  WeakCheck( testSet, test_assign_op<TestSet>( testSet2 ) );
# endif
}

#endif  // EH_HASHED_CONTAINERS_IMPLEMENTED
