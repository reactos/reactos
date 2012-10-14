/***********************************************************************************
  test_list.cpp

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
#include <list>
#else
#include <list.h>
#endif
#include "test_construct.h"
#include "test_assign_op.h"
#include "test_push_back.h"
#include "test_insert.h"
#include "test_push_front.h"
#include "nc_alloc.h"

typedef EH_STD::__list__<TestClass, eh_allocator(TestClass) > TestList;

inline sequence_container_tag
container_category(const TestList&)
{
  return sequence_container_tag();
}

//
//  list sort() member test operation. Does not verify stability.
//
struct test_list_sort
{
    test_list_sort()
    {
        gTestController.SetCurrentTestName("list::sort()");
    }

    void operator()( TestList& list ) const
    {
        list.sort();

        gTestController.CancelFailureCountdown();

        for ( TestList::iterator p = list.begin(); p != list.end(); p++ )
            if ( p != list.begin() ) {
                TestList::iterator tmp=p;
                --tmp;
                EH_ASSERT( *p >= *tmp );
            }
    }
};

void test_list()
{
    TestList testList, testList2;
    size_t listSize = random_number(random_base);

    while ( testList.size() < listSize )
    {
        TestClass x;
        testList.push_back( x );
        testList2.push_back( TestClass() );
    }

    StrongCheck( testList, test_insert_one<TestList>(testList) );
    StrongCheck( testList, test_insert_one<TestList>(testList, 0) );
    StrongCheck( testList, test_insert_one<TestList>(testList, (int)testList.size()) );

    WeakCheck( testList, test_insert_n<TestList>(testList, random_number(random_base) ) );
    WeakCheck( testList, test_insert_n<TestList>(testList, random_number(random_base), 0 ) );
    WeakCheck( testList, test_insert_n<TestList>(testList, random_number(random_base), (int)testList.size() ) );

    size_t insCnt = random_number(random_base);
    TestClass *insFirst = new TestList::value_type[1+insCnt];

    WeakCheck( testList, insert_range_tester(testList, insFirst, insFirst+insCnt) );
    WeakCheck( testList, insert_range_at_begin_tester(testList, insFirst, insFirst+insCnt) );
    WeakCheck( testList, insert_range_at_end_tester(testList, insFirst, insFirst+insCnt) );

    ConstCheck( 0, test_construct_pointer_range<TestList>(insFirst, insFirst+insCnt) );
    delete[] insFirst;

    WeakCheck( testList, insert_range_tester(testList, testList2.begin(), testList2.end() ) );

    StrongCheck( testList, test_push_front<TestList>(testList) );
    StrongCheck( testList, test_push_back<TestList>(testList) );

    StrongCheck( testList, test_list_sort() );  // Simply to verify strength.

    ConstCheck( 0, test_default_construct<TestList>() );
    ConstCheck( 0, test_construct_n<TestList>( random_number(random_base) ) );
    ConstCheck( 0, test_construct_n_instance<TestList>( random_number(random_base) ) );
    ConstCheck( 0, test_construct_iter_range<TestList>( testList2 ) );
    ConstCheck( testList, test_copy_construct<TestList>() );

    WeakCheck( testList, test_assign_op<TestList>( testList2 ) );
}
