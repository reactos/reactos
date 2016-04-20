/***********************************************************************************
  test_slist.cpp

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
#if defined( EH_SLIST_IMPLEMENTED )
#  include "TestClass.h"
#  include "LeakCheck.h"
#  if defined (EH_NEW_HEADERS) && defined (EH_USE_SGI_STL)
#    include <slist>
#  else
#    include <slist.h>
#  endif
#include "test_construct.h"
#include "test_assign_op.h"
#include "test_push_back.h"
#include "test_insert.h"
#include "test_push_front.h"

#if defined (__GNUC__) && defined (__APPLE__)
typedef EH_STD::slist<TestClass, eh_allocator(TestClass) > TestSList;
#else
typedef EH_STD::__slist__<TestClass, eh_allocator(TestClass) > TestSList;
#endif

inline sequence_container_tag
container_category(const TestSList&) {
  return sequence_container_tag();
}

struct test_slist_sort {
  test_slist_sort() {
    gTestController.SetCurrentTestName("slist::sort()");
  }
  void operator()( TestSList& slist ) const {
    slist.sort();
    for ( TestSList::iterator p = slist.begin(), q; p != slist.end(); q = p, p++ )
      if ( p != slist.begin() ) {
        EH_ASSERT( *p >= *q );
      }
  }
};

void test_slist() {
  TestSList testSList, testSList2;
  size_t slistSize = random_number(random_base);

  while (testSList.size() < slistSize) {
    TestClass x;
    testSList.push_front( x );
    testSList2.push_front( TestClass() );
  }

  StrongCheck( testSList, test_insert_one<TestSList>(testSList) );
  StrongCheck( testSList, test_insert_one<TestSList>(testSList, 0) );
  StrongCheck( testSList, test_insert_one<TestSList>(testSList, (int)testSList.size()) );

  WeakCheck( testSList, test_insert_n<TestSList>(testSList, random_number(random_base) ) );
  WeakCheck( testSList, test_insert_n<TestSList>(testSList, random_number(random_base), 0 ) );
  WeakCheck( testSList, test_insert_n<TestSList>(testSList, random_number(random_base), (int)testSList.size() ) );

  size_t insCnt = random_number(random_base);
  TestClass *insFirst = new TestSList::value_type[1+insCnt];
  WeakCheck( testSList, insert_range_tester(testSList, insFirst, insFirst+insCnt) );

  ConstCheck( 0, test_construct_pointer_range<TestSList>(insFirst, insFirst+insCnt) );
  delete[] insFirst;
  WeakCheck( testSList, test_insert_range<TestSList,TestSList::iterator>(testSList, testSList2.begin(), testSList2.end() ) );
  StrongCheck( testSList, test_push_front<TestSList>(testSList) );
  StrongCheck( testSList, test_slist_sort() );  // Simply to verify strength.

  ConstCheck( 0, test_default_construct<TestSList>() );
  ConstCheck( 0, test_construct_n<TestSList>( random_number(random_base) ) );
  ConstCheck( 0, test_construct_n_instance<TestSList>( random_number(random_base) ) );
  ConstCheck( 0, test_construct_iter_range<TestSList>( testSList2 ) );
  ConstCheck( testSList, test_copy_construct<TestSList>() );
  WeakCheck( testSList, test_assign_op<TestSList>( testSList2 ) );
}

#endif // EH_SLIST_IMPLEMENTED
