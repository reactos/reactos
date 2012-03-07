/***********************************************************************************
  test_map.cpp

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
# if defined (EH_NEW_HEADERS)
#include <map>
# else
#include <multimap.h>
#include <map.h>
# endif

#include "test_construct.h"
#include "test_assign_op.h"
#include "test_push_back.h"
#include "test_insert.h"
#include "test_push_front.h"
#include "ThrowCompare.h"
#include "test_insert.h"

template <class K, class V, class Comp, class A>
inline multimap_tag
container_category(const EH_STD::__multimap__<K,V,Comp, A>&)
{
    return multimap_tag();
}

template <class K, class V, class Comp, class A >
inline map_tag
container_category(const EH_STD::__map__<K,V,Comp, A>&)
{
    return map_tag();
}

typedef EH_STD::__multimap__<TestClass, TestClass, ThrowCompare, eh_allocator(TestClass) > TestMultiMap;

void test_multimap()
{
    TestMultiMap testMultiMap, testMultiMap2;

    const size_t mapSize = random_number(random_base);

    while ( testMultiMap.size() < mapSize )
    {
        TestMultiMap::value_type x;
        testMultiMap.insert( x );
        testMultiMap2.insert( TestMultiMap::value_type() );
    }

    StrongCheck( testMultiMap, test_insert_value<TestMultiMap>(testMultiMap) );

    size_t insCnt = 1 + random_number(random_base);
    TestMultiMap::value_type *insFirst = new TestMultiMap::value_type[insCnt];

    WeakCheck( testMultiMap, insert_range_tester(testMultiMap, insFirst, insFirst+insCnt) );

    ConstCheck( 0, test_construct_pointer_range<TestMultiMap>(insFirst, insFirst+insCnt) );
    delete[] insFirst;


    WeakCheck( testMultiMap, insert_range_tester(testMultiMap, testMultiMap2.begin(), testMultiMap2.end() ) );


    ConstCheck( 0, test_default_construct<TestMultiMap>() );

    ConstCheck( 0, test_construct_iter_range<TestMultiMap>( testMultiMap2 ) );

    ConstCheck( testMultiMap, test_copy_construct<TestMultiMap>() );

    WeakCheck( testMultiMap, test_assign_op<TestMultiMap>( testMultiMap2 ) );
}

typedef EH_STD::__map__<TestClass, TestClass, ThrowCompare, eh_allocator(TestClass) > TestMap;

void CheckInvariant( const TestMap& m );

void CheckInvariant( const TestMap& m )
{
//  assert( map.__rb_verify() );
    size_t total = 0;
    EH_DISTANCE( m.begin(), m.end(), total );
    assert( m.size() == total );
}

void test_map()
{
    TestMap testMap, testMap2;

    const size_t mapSize = random_number(random_base);

    while ( testMap.size() < mapSize )
    {
        TestMap::value_type x;
        testMap.insert( x );
        testMap2.insert( TestMap::value_type() );
    }

    StrongCheck( testMap, test_insert_value<TestMap>(testMap) );

    size_t insCnt = random_number(random_base);
    TestMap::value_type *insFirst = new TestMap::value_type[1+insCnt];

    WeakCheck( testMap, insert_range_tester(testMap, insFirst, insFirst+insCnt) );

    ConstCheck( 0, test_construct_pointer_range<TestMap>(insFirst, insFirst+insCnt) );
    delete[] insFirst;

    WeakCheck( testMap, insert_range_tester(testMap, testMap2.begin(), testMap2.end() ) );
    ConstCheck( 0, test_default_construct<TestMap>() );
    ConstCheck( 0, test_construct_iter_range<TestMap>( testMap2 ) );
    ConstCheck( testMap, test_copy_construct<TestMap>() );
    WeakCheck( testMap, test_assign_op<TestMap>( testMap2 ) );
}

