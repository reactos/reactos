/***********************************************************************************
  test_hash_map.cpp

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
#include "TestClass.h"
#include "LeakCheck.h"

#  include <hash_map>

#include "test_construct.h"
#include "test_assign_op.h"
#include "test_push_back.h"
#include "test_insert.h"
#include "test_push_front.h"
#include "ThrowCompare.h"
#include "test_hash_resize.h"
/*
template struct pair<const TestClass, TestClass>;
template struct __hashtable_node<pair<const TestClass, TestClass> >;
template class hash_map<TestClass, TestClass, ThrowHash, ThrowEqual>;
template class hash_multimap<TestClass, TestClass, ThrowHash, ThrowEqual>;
*/

typedef EH_STD::__hash_multimap__<TestClass, TestClass, ThrowHash, ThrowEqual,
  eh_allocator(TestClass) > TestMultiMap;


inline multimap_tag
container_category(const TestMultiMap&) {
  return multimap_tag();
}

void test_hash_multimap() {
# if !(defined (_MSC_VER) && (_MSC_VER < 1100))
  TestMultiMap testMultiMap, testMultiMap2;

        const size_t hash_mapSize = random_number(random_base);

  while ( testMultiMap.size() < hash_mapSize )
  {
    TestMultiMap::value_type x;
    testMultiMap.insert( x );
    testMultiMap2.insert( TestMultiMap::value_type() );
  }

#  if defined( EH_HASH_CONTAINERS_SUPPORT_RESIZE )
  WeakCheck( testMultiMap, test_hash_resize<TestMultiMap>() );
  // TestMultiMap == TestMultiMap: no such operator! - ptr
  // StrongCheck( testMultiMap, test_insert_noresize<TestMultiMap>(testMultiMap) );
#  endif
  WeakCheck( testMultiMap, test_insert_value<TestMultiMap>(testMultiMap) );

  size_t insCnt = random_number(random_base);
  TestMultiMap::value_type *insFirst = new TestMultiMap::value_type[1+insCnt];
  WeakCheck( testMultiMap, insert_range_tester(testMultiMap, insFirst, insFirst+insCnt) );
  ConstCheck( 0, test_construct_pointer_range<TestMultiMap>(insFirst, insFirst+insCnt) );
  delete[] insFirst;

  WeakCheck( testMultiMap, insert_range_tester(testMultiMap, testMultiMap2.begin(), testMultiMap2.end() ) );

  ConstCheck( 0, test_default_construct<TestMultiMap>() );
#  if EH_HASH_CONTAINERS_SUPPORT_ITERATOR_CONSTRUCTION
  ConstCheck( 0, test_construct_iter_range_n<TestMultiMap>( testMultiMap2 ) );
#  endif
  ConstCheck( testMultiMap, test_copy_construct<TestMultiMap>() );

  WeakCheck( testMultiMap, test_assign_op<TestMultiMap>( testMultiMap2 ) );
# endif
}

typedef EH_STD::__hash_map__<TestClass, TestClass, ThrowHash,
  ThrowEqual, eh_allocator(TestClass) > TestMap;

inline map_tag
container_category(const TestMap&)
{
  return map_tag();
}

void test_hash_map()
{
# if !(defined (_MSC_VER) && (_MSC_VER < 1100))
  TestMap testMap, testMap2;

  const size_t hash_mapSize = random_number(random_base);

  while ( testMap.size() < hash_mapSize ) {
    TestMap::value_type x;
    testMap.insert( x );
    testMap2.insert( TestMap::value_type() );
  }

#if defined( EH_HASH_CONTAINERS_SUPPORT_RESIZE )
  WeakCheck( testMap, test_hash_resize<TestMap>() );
  // TestMultiMap == TestMultiMap: no such operator! - ptr
  // StrongCheck( testMap, test_insert_noresize<TestMap>(testMap) );
#endif
  WeakCheck( testMap, test_insert_value<TestMap>(testMap) );

  size_t insCnt = random_number(random_base);
  TestMap::value_type *insFirst = new TestMap::value_type[1+insCnt];
  WeakCheck( testMap, insert_range_tester(testMap, insFirst, insFirst+insCnt) );
  ConstCheck( 0, test_construct_pointer_range<TestMap>(insFirst, insFirst+insCnt) );
  delete[] insFirst;

  WeakCheck( testMap, insert_range_tester(testMap, testMap2.begin(), testMap2.end() ) );

  ConstCheck( 0, test_default_construct<TestMap>() );
#  if EH_HASH_CONTAINERS_SUPPORT_ITERATOR_CONSTRUCTION
  ConstCheck( 0, test_construct_iter_range_n<TestMap>( testMap2 ) );
#  endif
  ConstCheck( testMap, test_copy_construct<TestMap>() );

  WeakCheck( testMap, test_assign_op<TestMap>( testMap2 ) );
# endif
}

#endif  // EH_HASHED_CONTAINERS_IMPLEMENTED
